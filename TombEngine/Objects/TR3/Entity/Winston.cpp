#include "framework.h"
#include "Objects/TR3/Entity/Winston.h"

#include "Game/collision/collide_room.h"
#include "Game/control/box.h"
#include "Game/control/control.h"
#include "Game/control/lot.h"
#include "Game/effects/effects.h"
#include "Game/itemdata/creature_info.h"
#include "Game/items.h"
#include "Game/Lara/lara.h"
#include "Game/Lara/lara_helpers.h"
#include "Game/misc.h"
#include "Game/Setup.h"
#include "Math/Math.h"
#include "Specific/clock.h"
#include "Specific/level.h"

using namespace TEN::Math;

// NOTES:
// ItemFlags[0]: defeat timer.
// ItemFlags[1]: type.
// ItemFlags[2]: static stuck recovery step timer.
// ItemFlags[3]: static stuck recovery turn direction.
// ItemFlags[4]: repeated static stuck hit counter.
// ItemFlags[5]: static stuck recovery cooldown timer.

namespace TEN::Entities::Creatures::TR3
{
	constexpr auto WINSTON_IDLE_RANGE = SQUARE(BLOCK(1.5f));
	constexpr auto WINSTON_SHUFFLE_SOUND_CHANCE = 1 / 128.0f;
	constexpr auto WINSTON_TURN_RATE_MAX = ANGLE(2.0f);

	constexpr auto WINSTON_RECOVER_HIT_POINTS = 16;
	constexpr auto WINSTON_DEFEAT_TIMER_MAX = 5 * FPS;

	constexpr auto WINSTON_STATIC_COLLISION_RADIUS = CLICK(1);
	constexpr auto WINSTON_STATIC_COLLISION_HEIGHT = CLICK(3);
	constexpr auto WINSTON_STUCK_STEP_TIME = FPS / 4;
	constexpr auto WINSTON_STUCK_COOLDOWN_TIME = FPS / 3;
	constexpr auto WINSTON_STUCK_SIDE_TURN = ANGLE(45.0f);
	constexpr auto WINSTON_STUCK_COUNTER_MAX = 8;

	enum WinstonState
	{
		// No state 0.
		WINSTON_STATE_IDLE = 1,
		WINSTON_STATE_WALK_FORWARD = 2,
		WINSTON_STATE_GUARD_MID = 3,
		WINSTON_STATE_GUARD_LOW = 4, // Unused.
		WINSTON_STATE_GUARD_HIGH = 5, // Unused.
		WINSTON_STATE_RECOIL_MID = 6,
		WINSTON_STATE_RECOIL_LOW = 7, // Unused.
		WINSTON_STATE_RECOIL_HIGH = 8, // Unused.
		WINSTON_STATE_DEFEAT_CONTINUE = 9,
		WINSTON_STATE_DEFEAT_START = 10,
		WINSTON_STATE_DEFEAT_TO_IDLE = 11,
		WINSTON_STATE_BRUSH_OFF = 12,
		WINSTON_STATE_DEFEAT_END = 13
	};

	enum WinstonAnim
	{
		WINSTON_ANIM_WALK_FORWARD = 0,
		WINSTON_ANIM_WALK_FORWARD_TO_IDLE = 1,
		WINSTON_ANIM_IDLE = 2,
		WINSTON_ANIM_IDLE_TO_WALK_FORWARD = 3,
		WINSTON_ANIM_IDLE_TO_GUARD_MID = 4,
		WINSTON_ANIM_GUARD_MID = 5,
		WINSTON_ANIM_RECOIL_MID = 6,
		WINSTON_ANIM_RECOIL_MID_TO_IDLE = 7,
		WINSTON_ANIM_GUARD_MID_TO_LOW = 8,
		WINSTON_ANIM_GUARD_LOW = 9,
		WINSTON_ANIM_RECOIL_LOW = 10,
		WINSTON_ANIM_GUARD_LOW_TO_MID = 11,
		WINSTON_ANIM_GUARD_MID_TO_HIGH = 12,
		WINSTON_ANIM_GUARD_HIGH = 13,
		WINSTON_ANIM_RECOIL_HIGH = 14,
		WINSTON_ANIM_GUARD_HIGH_TO_MID = 15,
		WINSTON_ANIM_DEFEAT_START = 16,
		WINSTON_ANIM_DEFEAT_CONT = 17,
		WINSTON_ANIM_DEFEAT_TO_IDLE = 18,
		WINSTON_ANIM_BRUSH_OFF = 19,
		WINSTON_ANIM_DEFEAT_END = 20
	};

	enum class WinstonType
	{
		Normal = 0,
		Army = 1
	};

	static bool TestWinstonStaticCollision(ItemInfo& item, const Pose& previousPose, short forwardAngle)
	{
		CollisionInfo coll = {};
		coll.Setup.Radius = WINSTON_STATIC_COLLISION_RADIUS;
		coll.Setup.Height = WINSTON_STATIC_COLLISION_HEIGHT;
		coll.Setup.ForwardAngle = forwardAngle;
		coll.Setup.LowerFloorBound = STEPUP_HEIGHT;
		coll.Setup.UpperFloorBound = -STEPUP_HEIGHT;
		coll.Setup.LowerCeilingBound = 0;
		coll.Setup.UpperCeilingBound = MAX_HEIGHT;
		coll.Setup.ForceSolidStatics = true;
		coll.Setup.PrevPosition = previousPose.Position;

		auto collisionTestItem = item;
		GetCollisionInfo(&coll, &collisionTestItem);

		if (!coll.HitStatic)
			return false;

		item.Pose.Position = previousPose.Position;
		item.Animation.Velocity.z = 0;

		return true;
	}

	static void ResetWinstonStuckRecovery(ItemInfo& item)
	{
		if (item.ItemFlags[2] > 0 || item.ItemFlags[5] > 0)
			return;

		item.ItemFlags[3] = 0;
		item.ItemFlags[4] = 0;
	}

	static void StartWinstonStuckRecovery(ItemInfo& item, const AI_INFO& ai)
	{
		if (item.ItemFlags[2] > 0 || item.ItemFlags[5] > 0)
			return;

		if (item.ItemFlags[4] < WINSTON_STUCK_COUNTER_MAX)
			item.ItemFlags[4]++;

		const auto preferredDirection = ai.angle >= 0 ? 1 : -1;
		const auto alternateDirection = (item.ItemFlags[4] & 1) ? preferredDirection : -preferredDirection;

		item.ItemFlags[2] = WINSTON_STUCK_STEP_TIME;
		item.ItemFlags[3] = alternateDirection;
		item.ItemFlags[5] = WINSTON_STUCK_COOLDOWN_TIME;
		item.Pose.Orientation.y += alternateDirection * WINSTON_STUCK_SIDE_TURN;
		item.Animation.TargetState = WINSTON_STATE_WALK_FORWARD;
	}

	void InitializeWinston(short itemNumber)
	{
		auto& item = g_Level.Items[itemNumber];

		InitializeCreature(itemNumber);

		if (!item.TriggerFlags)
		{
			item.HitPoints = NOT_TARGETABLE;
			item.ItemFlags[1] = (int)WinstonType::Normal;
		}
		else
		{
			item.ItemFlags[1] = (int)WinstonType::Army;
		}
	}

	void ControlWinston(short itemNumber)
	{
		if (!CreatureActive(itemNumber))
			return;

		auto& item = g_Level.Items[itemNumber];
		auto& creature = *GetCreatureInfo(&item);
		const auto& player = Lara;

		short& defeatTimer = item.ItemFlags[0];

		AI_INFO ai;
		CreatureAIInfo(&item, &ai);
		GetCreatureMood(&item, &ai, 1);
		CreatureMood(&item, &ai, 1);

		// Set proper HP value if changing OCB at runtime.
		if (item.ItemFlags[1] != item.TriggerFlags)
		{
			if (!item.TriggerFlags)
			{
				item.HitPoints = NOT_TARGETABLE;
				item.ItemFlags[1] = (int)WinstonType::Normal;
			}
			else
			{
				item.HitPoints = WINSTON_RECOVER_HIT_POINTS;
				item.ItemFlags[1] = (int)WinstonType::Army;
			}
		}

		short headingAngle = CreatureTurn(&item, creature.MaxTurn);

		if (item.ItemFlags[2] > 0)
		{
			item.ItemFlags[2]--;
			headingAngle = 0;
		}
		else if (item.ItemFlags[5] > 0)
		{
			item.ItemFlags[5]--;
		}

		if (item.HitPoints <= 0 && item.TriggerFlags)
		{
			creature.MaxTurn = 0;

			switch (item.Animation.ActiveState)
			{
			case WINSTON_STATE_DEFEAT_CONTINUE:
			case WINSTON_STATE_DEFEAT_START:
				if (item.HitStatus)
				{
					item.Animation.TargetState = WINSTON_STATE_DEFEAT_CONTINUE;
				}
				else
				{
					defeatTimer--;
					if (defeatTimer < 0)
						item.Animation.TargetState = WINSTON_STATE_DEFEAT_END;
				}

				break;

			case WINSTON_STATE_DEFEAT_TO_IDLE:
				item.HitPoints = WINSTON_RECOVER_HIT_POINTS;

				if (Random::TestProbability(1 / 2.0f))
					creature.Flags = 999;

				break;

			case WINSTON_STATE_DEFEAT_END:
				if (item.HitStatus)
				{
					item.Animation.TargetState = WINSTON_STATE_DEFEAT_CONTINUE;
				}
				else
				{
					defeatTimer--;
					if (defeatTimer < 0)
						item.Animation.TargetState = WINSTON_STATE_DEFEAT_TO_IDLE;
				}

				break;

			default:
				SetAnimation(item, WINSTON_ANIM_DEFEAT_START);
				defeatTimer = WINSTON_DEFEAT_TIMER_MAX;
				break;
			}
		}
		else
		{
			switch (item.Animation.ActiveState)
			{
			case WINSTON_STATE_IDLE:
				creature.MaxTurn = WINSTON_TURN_RATE_MAX;

				if (creature.Flags == 999)
				{
					item.Animation.TargetState = WINSTON_STATE_BRUSH_OFF;
				}
				else if (player.TargetEntity == &item && item.TriggerFlags)
				{
					item.Animation.TargetState = WINSTON_STATE_GUARD_MID;
				}
				else if ((ai.distance > WINSTON_IDLE_RANGE || !ai.ahead) && item.Animation.TargetState != WINSTON_STATE_WALK_FORWARD)
				{
					item.Animation.TargetState = WINSTON_STATE_WALK_FORWARD;
				}

				break;

			case WINSTON_STATE_WALK_FORWARD:
				creature.MaxTurn = WINSTON_TURN_RATE_MAX;

				if (player.TargetEntity == &item)
				{
					item.Animation.TargetState = WINSTON_STATE_IDLE;
				}
				else if (ai.distance < WINSTON_IDLE_RANGE)
				{
					if (ai.ahead)
					{
						item.Animation.TargetState = WINSTON_STATE_IDLE;

						if (creature.Flags & 1)
							creature.Flags--;
					}
					else if ((creature.Flags & 1) == 0)
					{
						creature.Flags |= 1;

						SoundEffect(SFX_TR3_WINSTON_SURPRISED, &item.Pose);
						SoundEffect(SFX_TR3_WINSTON_SHUFFLE, &item.Pose);
					}
				}

				break;

			case WINSTON_STATE_GUARD_MID:
				creature.MaxTurn = WINSTON_TURN_RATE_MAX;

				if (item.Animation.RequiredState != NO_VALUE)
					item.Animation.TargetState = item.Animation.RequiredState;

				if (!item.TriggerFlags)
					item.Animation.TargetState = WINSTON_STATE_IDLE;

				if (item.HitStatus)
				{
					item.Animation.TargetState = WINSTON_STATE_RECOIL_MID;
				}
				else if (player.TargetEntity != &item)
				{
					item.Animation.TargetState = WINSTON_STATE_IDLE;
				}

				break;

			case WINSTON_STATE_GUARD_LOW:
				creature.MaxTurn = WINSTON_TURN_RATE_MAX;

				if (item.Animation.RequiredState != NO_VALUE)
					item.Animation.TargetState = item.Animation.RequiredState;

				if (item.HitStatus)
					item.Animation.TargetState = WINSTON_STATE_RECOIL_LOW;

				break;

			case WINSTON_STATE_GUARD_HIGH:
				creature.MaxTurn = WINSTON_TURN_RATE_MAX;

				if (item.Animation.RequiredState != NO_VALUE)
					item.Animation.TargetState = item.Animation.RequiredState;

				if (item.HitStatus)
				{
					item.Animation.TargetState = WINSTON_STATE_RECOIL_HIGH;
				}
				else if (player.TargetEntity == &item)
				{
					item.Animation.TargetState = WINSTON_STATE_GUARD_MID;
				}

				break;

			case WINSTON_STATE_RECOIL_MID:
				item.Animation.RequiredState = Random::TestProbability(1 / 2.0f) ? WINSTON_STATE_GUARD_HIGH : WINSTON_STATE_GUARD_LOW;
				break;

			case WINSTON_STATE_RECOIL_LOW:
			case WINSTON_STATE_RECOIL_HIGH:
				item.Animation.RequiredState = WINSTON_STATE_GUARD_MID;
				break;

			case WINSTON_STATE_BRUSH_OFF:
				creature.MaxTurn = 0;
				creature.Flags = 0;
				break;

			case WINSTON_STATE_DEFEAT_END:
				if (!item.TriggerFlags)
				{
					item.Animation.TargetState = WINSTON_STATE_DEFEAT_TO_IDLE;
					break;
				}
			}
		}

		if (Random::TestProbability(WINSTON_SHUFFLE_SOUND_CHANCE))
			SoundEffect(SFX_TR3_WINSTON_SHUFFLE, &item.Pose);

		auto previousPose = item.Pose;
		CreatureAnimation(itemNumber, headingAngle, 0);

		if (TestWinstonStaticCollision(item, previousPose, item.Pose.Orientation.y))
		{
			StartWinstonStuckRecovery(item, ai);
			creature.Flags &= ~1;
		}
		else
		{
			ResetWinstonStuckRecovery(item);
		}
	}

	void HitWinston(ItemInfo& target, ItemInfo& source, std::optional<GameVector> pos, int damage, bool isExplosive, int jointIndex)
	{
		const auto& object = Objects[target.ObjectNumber];

		if (pos.has_value())
		{
			DoItemHit(&target, damage, isExplosive, false);

			if (object.hitEffect == HitEffect::Richochet)
			{
				TriggerRicochetSpark(*pos, source.Pose.Orientation.y, false);
				SoundEffect(SFX_TR3_WINSTON_CUPS, &target.Pose);
			}
		}
	}
}
