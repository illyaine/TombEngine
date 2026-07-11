#include "framework.h"
#include "Game/effects/weather.h"

#include "Game/camera.h"
#include "Game/collision/collide_room.h"
#include "Game/collision/Los.h"
#include "Game/collision/Point.h"
#include "Game/effects/effects.h"
#include "Game/effects/Ripple.h"
#include "Game/effects/tomb4fx.h"
#include "Game/savegame.h"
#include "Game/Setup.h"
#include "Game/StaticMesh.h"
#include "Math/Math.h"
#include "Objects/game_object_ids.h"
#include "Sound/sound.h"
#include "Scripting/Include/ScriptInterfaceLevel.h"
#include "Specific/level.h"

using namespace TEN::Collision::Los;
using namespace TEN::Collision::Point;
using namespace TEN::Effects::Ripple;
using namespace TEN::Math;

namespace TEN::Effects::Environment
{
	namespace
	{
		constexpr auto RAIN_COLLISION_CHECK_DELAY_MAX = 2.0f;
		constexpr auto WEATHER_SURFACE_OFFSET = 8.0f;
		constexpr auto RAIN_WIND_RESPONSE = 0.20f;
		constexpr auto SNOW_WIND_RESPONSE = 0.12f;
		constexpr auto RAIN_WIND_SCALE = 6.0f;
		constexpr auto SNOW_WIND_SCALE = 4.0f;
		constexpr auto RAIN_HORIZONTAL_VELOCITY_MAX = 64.0f;

		struct WeatherSurfaceHit
		{
			Vector3 Position = Vector3::Zero;
			Vector3 Normal = Vector3::Zero;
			int RoomNumber = NO_VALUE;
		};

		float ApproachVelocity(float current, float target, float response, float maxStep)
		{
			auto step = std::clamp((target - current) * response, -maxStep, maxStep);
			return current + step;
		}

		std::optional<WeatherSurfaceHit> GetWeatherSurfaceHit(const Vector3& origin, int roomNumber, const Vector3& target)
		{
			auto direction = target - origin;
			float distance = direction.Length();
			if (distance <= std::numeric_limits<float>::epsilon())
				return std::nullopt;

			direction /= distance;
			auto los = GetLosCollision(origin, roomNumber, direction, distance, false, false, true);

			float nearestDistance = distance + 1.0f;
			auto result = WeatherSurfaceHit{};
			bool intersected = false;

			if (los.Room.IsIntersected && los.Room.Distance <= distance)
			{
				nearestDistance = los.Room.Distance;
				result.Position = los.Room.Position;
				result.RoomNumber = los.Room.RoomNumber;
				if (los.Room.Triangle.has_value())
					result.Normal = los.Room.Triangle->Normal;
				intersected = true;
			}

			for (const auto& staticHit : los.Statics)
			{
				if (staticHit.Static == nullptr || staticHit.Distance > distance || staticHit.Distance >= nearestDistance)
					continue;

				const auto flags = staticHit.Static->Flags;
				if (!(flags & StaticMeshFlags::SM_SOLID) || !(flags & StaticMeshFlags::SM_COLLISION))
					continue;

				nearestDistance = staticHit.Distance;
				result.Position = staticHit.Position;
				result.RoomNumber = staticHit.RoomNumber;
				result.Normal = Vector3::Zero;
				intersected = true;
			}

			if (!intersected)
				return std::nullopt;

			// Keep the visual impact on the incoming side of thin geometry.
			result.Position -= direction * WEATHER_SURFACE_OFFSET;
			return result;
		}

		bool CanSkipExactRainSweep(
			const Vector3& origin,
			int roomNumber,
			const Vector3& target,
			PointCollisionData& targetCollision)
		{
			if (roomNumber < 0 || roomNumber >= g_Level.Rooms.size())
				return false;

			if (targetCollision.GetRoomNumber() != roomNumber)
				return false;

			// Static collision geometry is not represented by room floor/ceiling samples.
			if (!g_Level.Rooms[roomNumber].mesh.empty())
				return false;

			auto originCollision = GetPointCollision(origin, roomNumber);
			if (originCollision.GetRoomNumber() != roomNumber)
				return false;

			// Crossing a sector boundary can cross walls or split geometry even when endpoint heights match.
			if (&originCollision.GetSector() != &targetCollision.GetSector())
				return false;

			// Bridge items and diagonal sectors require the exact swept collision path.
			if (originCollision.GetFloorBridgeItemNumber() != NO_VALUE ||
				originCollision.GetCeilingBridgeItemNumber() != NO_VALUE ||
				targetCollision.GetFloorBridgeItemNumber() != NO_VALUE ||
				targetCollision.GetCeilingBridgeItemNumber() != NO_VALUE ||
				originCollision.IsDiagonalFloorSplit() ||
				originCollision.IsDiagonalCeilingSplit() ||
				targetCollision.IsDiagonalFloorSplit() ||
				targetCollision.IsDiagonalCeilingSplit())
			{
				return false;
			}

			const auto originFloor = originCollision.GetFloorHeight();
			const auto targetFloor = targetCollision.GetFloorHeight();
			const auto originCeiling = originCollision.GetCeilingHeight();
			const auto targetCeiling = targetCollision.GetCeilingHeight();

			// Only skip the exact sweep when both endpoints describe the same open sector interval.
			if (originFloor != targetFloor || originCeiling != targetCeiling)
				return false;

			const float segmentTop = std::min(origin.y, target.y);
			const float segmentBottom = std::max(origin.y, target.y);
			const float safeCeiling = originCeiling + WEATHER_SURFACE_OFFSET;
			const float safeFloor = originFloor - WEATHER_SURFACE_OFFSET;

			return safeCeiling < safeFloor && segmentTop > safeCeiling && segmentBottom < safeFloor;
		}

		void SpawnRainSurfaceImpact(const WeatherSurfaceHit& hit)
		{
			// The legacy spark is vertically oriented, so avoid using it on near-vertical walls.
			if (hit.Normal != Vector3::Zero && abs(hit.Normal.y) < 0.25f)
				return;

			AddWaterSparks((int)hit.Position.x, (int)hit.Position.y, (int)hit.Position.z, 4);
		}
	}

	EnvironmentController Weather;

	float WeatherParticle::Transparency() const
	{
		float result = WEATHER_PARTICLE_OPACITY;
		float fade   = WEATHER_PARTICLE_NEAR_DEATH_LIFE;

		if (Life <= fade)
			result *= Life / fade;

		if ((StartLife - Life) < fade)
			result *= (StartLife - Life) / fade;

		if (Type != WeatherType::Snow)
			result *= 0.35f;

		return result;
	}

	EnvironmentController::EnvironmentController()
	{
		WeatherParticles.reserve(WEATHER_PARTICLE_COUNT_MAX);
		DustParticles.reserve(DUST_PARTICLE_RESERVE);
	}

	void EnvironmentController::Update()
	{
		const auto& level = *g_GameFlow->GetLevel(CurrentLevel);

		UpdateSky(level);
		UpdateStorm(level);
		UpdateWind(level);
		UpdateFlash(level);
		UpdateWeather(level);
		UpdateStarfield(level);

		SpawnWeatherParticles(level);
		SpawnDustParticles(level);
		SpawnMeteorParticles(level);
	}

	void EnvironmentController::Clear()
	{
		// Clear storm variables.
		StormTimer     = 0;
		StormSkyColor  = 1;
		StormSkyColor2 = 1;

		// Clear wind variables.
		WindCurrent =
		WindX =
		WindZ = 0;
		WindAngle =
		WindDAngle = 2048;

		// Clear flash variables.
		FlashProgress = 0.0f;
		FlashColorBase = Vector3::Zero;

		// Clear weather and underwater dust independently.
		WeatherParticles.clear();
		DustParticles.clear();

		// Clear starfield.
		ResetStarField = true;
		Stars.clear();
		StarfieldRevision++;
		Meteors.clear();
	}

	void EnvironmentController::Flash(int r, int g, int b, float speed)
	{
		FlashProgress = 1.0f;
		FlashSpeed = std::clamp(speed, 0.005f, 1.0f);
		FlashColorBase = Vector3(std::clamp(r, 0, UCHAR_MAX) / (float)UCHAR_MAX,
								 std::clamp(g, 0, UCHAR_MAX) / (float)UCHAR_MAX,
								 std::clamp(b, 0, UCHAR_MAX) / (float)UCHAR_MAX);
	}

	void EnvironmentController::UpdateSky(const ScriptInterfaceLevel& level)
	{
		for (int i = 0; i < 2; i++)
		{
			if (!level.GetSkyLayerEnabled(i))
				continue;

			auto& skyPos = SkyCurrentPosition[i];

			skyPos += level.GetSkyLayerSpeed(i);
			if (skyPos <= SKY_SIZE)
			{
				if (skyPos < 0)
					skyPos += SKY_SIZE;
			}
			else
			{
				skyPos -= SKY_SIZE;
			}
		}
	}

	void EnvironmentController::UpdateStorm(const ScriptInterfaceLevel& level)
	{
		if (level.GetStormEnabled())
		{
			if (StormCount || StormRand)
			{
				UpdateLightning();
				if (StormTimer > -1)
					StormTimer--;
				if (!StormTimer)
					SoundEffect(SFX_TR4_THUNDER_RUMBLE, nullptr);
			}
			else if (!(rand() & 0x7F))
			{
				StormCount = (rand() & 0x1F) + 16;
				StormTimer = (rand() & 3) + 12;
			}
		}

		for (int i = 0; i < 2; i++)
		{
			auto color = Color(
				level.GetSkyLayerColor(i).GetR() / 255.0f,
				level.GetSkyLayerColor(i).GetG() / 255.0f,
				level.GetSkyLayerColor(i).GetB() / 255.0f,
				1.0f);

			if (level.GetStormEnabled())
			{
				auto flashBrightness = StormSkyColor / 255.0f;
				auto r = std::clamp(color.x + flashBrightness, 0.0f, 1.0f);
				auto g = std::clamp(color.y + flashBrightness, 0.0f, 1.0f);
				auto b = std::clamp(color.z + flashBrightness, 0.0f, 1.0f);

				SkyCurrentColor[i] = Vector4(r, g, b, color.w);
			}
			else
			{
				SkyCurrentColor[i] = color;
			}
		}
	}

	void EnvironmentController::UpdateLightning()
	{
		StormCount--;

		if (StormCount <= 0)
		{
			StormSkyColor = 0;
			StormRand = 0;
		}
		else if (StormCount < 5 && StormSkyColor < 50)
		{
			auto newColor = StormSkyColor - (StormCount * 2);
			if (newColor < 0)
				newColor = 0;
			StormSkyColor = newColor;
		}
		else if (StormCount)
		{
			StormRand = ((rand() & 0x1FF - StormRand) >> 1) + StormRand;
			StormSkyColor2 += StormRand * StormSkyColor2 >> 8;
			StormSkyColor = StormSkyColor2;
			if (StormSkyColor > UCHAR_MAX)
				StormSkyColor = UCHAR_MAX;
		}
	}

	void EnvironmentController::UpdateWind(const ScriptInterfaceLevel& level)
	{
		WindCurrent += (GetRandomControl() & 7) - 3;
		if (WindCurrent <= -2)
		{
			WindCurrent++;
		}
		else if (WindCurrent >= 9)
		{
			WindCurrent--;
		}

		WindDAngle = (WindDAngle + 2 * (GetRandomControl() & 63) - 64) & 0x1FFE;

		if (WindDAngle < 1024)
		{
			WindDAngle = 2048 - WindDAngle;
		}
		else if (WindDAngle > 3072)
		{
			WindDAngle += 6144 - 2 * WindDAngle;
		}

		WindAngle = (WindAngle + ((WindDAngle - WindAngle) >> 3)) & 0x1FFE;
		WindX = WindCurrent * phd_sin(WindAngle << 3);
		WindZ = WindCurrent * phd_cos(WindAngle << 3);
	}

	void EnvironmentController::UpdateFlash(const ScriptInterfaceLevel& level)
	{
		if (FlashProgress > 0.0f)
		{
			FlashProgress -= FlashSpeed;
			if (FlashProgress < 0.0f)
				FlashProgress = 0.0f;
		}

		if (FlashProgress == 0.0f)
			FlashColorBase = Vector3::Zero;
	}

	void EnvironmentController::UpdateStarfield(const ScriptInterfaceLevel& level)
	{
		int starCount = level.GetStarfieldStarCount();
		if (starCount == 0)
		{
			if (!Stars.empty())
			{
				Stars.clear();
				StarfieldRevision++;
			}

			return;
		}

		bool starfieldChanged = false;

		if (ResetStarField)
		{
			Stars.clear();
			ResetStarField = false;
			starfieldChanged = true;
		}

		if (starCount != Stars.size())
		{
			// If starCount increased, add new stars to existing list.
			if (starCount > Stars.size())
			{
				// Reserve space for new stars if necessary.
				Stars.reserve(starCount);

				for (int i = (int)Stars.size(); i < starCount; i++)
				{
					auto starDir = Random::GenerateDirectionInCone(-Vector3::UnitY, 70.0f);
					starDir.Normalize();

					auto star = StarParticle{};
					star.Direction = starDir;
					star.Color = Vector3(
						Random::GenerateFloat(0.6f, 1.0f),
						Random::GenerateFloat(0.6f, 1.0f),
						Random::GenerateFloat(0.6f, 1.0f));
					star.Scale = Random::GenerateFloat(0.5f, 1.5f);

					float cosine = Vector3::UnitY.Dot(starDir);
					float maxCosine = cos(DEG_TO_RAD(50.0f));
					float minCosine = cos(DEG_TO_RAD(70.0f));

					if (cosine >= minCosine && cosine <= maxCosine)
					{
						star.Extinction = (cosine - minCosine) / (maxCosine - minCosine);
					}
					else
					{
						star.Extinction = 1.0f;
					}

					Stars.push_back(star);
				}
			}
			// If starCount decreased, resize vector without reinitializing.
			else
			{
				Stars.resize(starCount);
			}

			starfieldChanged = true;
		}

		if (starfieldChanged)
			StarfieldRevision++;

		if (level.GetStarfieldMeteorCount() > 0)
		{
			for (auto& meteor : Meteors)
			{
				meteor.Life--;

				if (meteor.Life <= 0)
				{
					meteor.Active = false;
					continue;
				}

				meteor.StoreInterpolationData();

				if (meteor.Life <= METEOR_PARTICLE_FADE_TIME)
				{
					meteor.Fade = meteor.Life / METEOR_PARTICLE_FADE_TIME;
				}
				else if (meteor.Life >= METEOR_PARTICLE_LIFE_MAX - METEOR_PARTICLE_FADE_TIME)
				{
					meteor.Fade = (METEOR_PARTICLE_LIFE_MAX - meteor.Life) / METEOR_PARTICLE_FADE_TIME;
				}
				else
				{
					meteor.Fade = 1.0f;
				}

				meteor.Position += meteor.Direction * level.GetStarfieldMeteorVelocity();
			}
		}
	}

	void EnvironmentController::UpdateWeather(const ScriptInterfaceLevel& level)
	{
		// Underwater dust has no collision work and is updated independently from outdoor weather.
		for (auto& part : DustParticles)
		{
			part.StoreInterpolationData();
			part.Life -= 2;

			if (part.Life <= 0)
			{
				part.Enabled = false;
				continue;
			}

			if (abs(Camera.pos.x - part.Position.x) > COLLISION_CHECK_DISTANCE ||
				abs(Camera.pos.z - part.Position.z) > COLLISION_CHECK_DISTANCE)
			{
				part.Life = std::clamp(part.Life, 0.0f, WEATHER_PARTICLE_NEAR_DEATH_LIFE);
			}

			if (!part.Stopped)
				part.Position += part.Velocity;
		}

		for (auto& part : WeatherParticles)
		{
			part.StoreInterpolationData();
			part.Life -= 2;

			// Disable particle if dead. It will be cleaned before the next spawn pass.
			if (part.Life <= 0)
			{
				part.Enabled = false;
				continue;
			}

			// Check if particle got out of collision check radius and fade out if it did.
			if (abs(Camera.pos.x - part.Position.x) > COLLISION_CHECK_DISTANCE ||
				abs(Camera.pos.z - part.Position.z) > COLLISION_CHECK_DISTANCE)
			{
				part.Life = std::clamp(part.Life, 0.0f, WEATHER_PARTICLE_NEAR_DEATH_LIFE);
			}

			// If particle was locked after landing, fade out and bypass collision and movement updates.
			if (part.Stopped)
			{
				if (part.Type == WeatherType::Snow)
					part.Size *= WEATHER_PARTICLE_NEAR_DEATH_MELT_FACTOR;

				continue;
			}

			// Backup previous position and progress new position according to velocity.
			auto prevPos = part.Position;
			part.Position.x += part.Velocity.x;
			part.Position.z += part.Velocity.z;
			part.Position.y += (part.Velocity.y / 2.0f);

			PointCollisionData pointColl;
			bool collisionCalculated = false;
			bool landed = false;
			bool outsideCurrentRoom = !IsPointInRoom(part.Position, part.RoomNumber);
			bool shouldCalculateCollision = part.CollisionCheckDelay <= 0 || outsideCurrentRoom;

			if (shouldCalculateCollision)
			{
				pointColl = GetPointCollision(part.Position, part.RoomNumber);
				collisionCalculated = true;

				// Sweep the complete unchecked rain path whenever the conservative room/sector broad phase
				// cannot prove that the segment remains inside one open interval without static geometry.
				if (part.Type == WeatherType::Rain &&
					!CanSkipExactRainSweep(part.CollisionPosition, part.RoomNumber, part.Position, pointColl))
				{
					auto surfaceHit = GetWeatherSurfaceHit(part.CollisionPosition, part.RoomNumber, part.Position);
					if (surfaceHit.has_value())
					{
						part.Position = surfaceHit->Position;
						part.CollisionPosition = surfaceHit->Position;
						part.RoomNumber = surfaceHit->RoomNumber;
						part.Stopped = true;
						part.Enabled = false;
						SpawnRainSurfaceImpact(*surfaceHit);
						continue;
					}
				}

				part.CollisionPosition = part.Position;

				// Determine collision checking frequency based on nearest floor/ceiling surface position.
				// Rain uses a lower cap because it travels much faster than snow. The accumulated sweep
				// still covers the complete distance between checks.
				auto coeff = std::min(
					std::max(0.0f, pointColl.GetFloorHeight() - part.Position.y),
					std::max(0.0f, part.Position.y - pointColl.GetCeilingHeight()));
				auto maxDelay = (part.Type == WeatherType::Rain) ? RAIN_COLLISION_CHECK_DELAY_MAX : WEATHER_PARTICLE_COLL_CHECK_DELAY_MAX;
				part.CollisionCheckDelay = std::min(
					floor(coeff / std::max(std::numeric_limits<float>::denorm_min(), abs(part.Velocity.y))),
					maxDelay);
			}
			else
			{
				part.CollisionCheckDelay--;
			}

			// Check if particle is beyond room bounds.
			if (outsideCurrentRoom)
			{
				if (!collisionCalculated)
				{
					pointColl = GetPointCollision(part.Position, part.RoomNumber);
					part.CollisionPosition = part.Position;
					collisionCalculated = true;
				}

				if (pointColl.GetRoomNumber() == part.RoomNumber)
				{
					// Not landed on door, so out of room bounds - land.
					landed = true;
				}
				else
				{
					part.RoomNumber = pointColl.GetRoomNumber();
				}
			}

			float range = (part.Type == WeatherType::Rain) ? WEATHER_SPAWN_DIST_RAIN : COLLISION_CHECK_DISTANCE;

			if (part.Type == WeatherType::Rain &&
				(abs(Camera.pos.x - part.Position.x) > range ||
				 abs(Camera.pos.z - part.Position.z) > range))
			{
				part.Life = std::clamp(part.Life, 0.0f, WEATHER_PARTICLE_NEAR_DEATH_LIFE);
			}

			// If collision was updated, process with position checks.
			if (collisionCalculated)
			{
				const auto collisionRoomNumber = pointColl.GetRoomNumber();
				const auto floorHeight = pointColl.GetFloorHeight();
				const auto ceilingHeight = pointColl.GetCeilingHeight();
				bool inSubstance = g_Level.Rooms[collisionRoomNumber].flags & (ENV_FLAG_WATER | ENV_FLAG_SWAMP);
				bool landedOnFloor = floorHeight <= part.Position.y;
				bool landedOnCeiling = ceilingHeight >= part.Position.y;
				landed = landed || landedOnFloor || landedOnCeiling;

				if (inSubstance || landed)
				{
					part.Stopped = true;
					part.RoomNumber = collisionRoomNumber;
					part.Life = std::clamp(part.Life, 0.0f, WEATHER_PARTICLE_NEAR_DEATH_LIFE);

					auto impactPosition = part.Position;
					if (inSubstance)
						impactPosition.y = pointColl.GetWaterSurfaceHeight();
					else if (landedOnFloor)
						impactPosition.y = floorHeight - WEATHER_SURFACE_OFFSET;
					else
						impactPosition = prevPos;

					part.Position = impactPosition;
					part.CollisionPosition = impactPosition;

					// Produce ripples if particle got into substance (water or swamp).
					if (inSubstance)
					{
						SpawnRipple(
							impactPosition,
							collisionRoomNumber,
							Random::GenerateFloat(16.0f, 24.0f),
							(int)RippleFlags::SlowFade | (int)RippleFlags::LowOpacity);
					}

					// Immediately disable rain particle because it doesn't need fading out.
					if (part.Type == WeatherType::Rain)
					{
						part.Enabled = false;

						// Suppress the old interior-ceiling fallback. Swept room/static collision handles
						// roof impacts at the actual exterior contact point.
						if (inSubstance || landedOnFloor)
							AddWaterSparks((int)impactPosition.x, (int)impactPosition.y, (int)impactPosition.z, 4);
					}

					continue;
				}
			}

			// Update velocities for the active weather type.
			switch (part.Type)
			{
			case WeatherType::Snow:
			{
				auto targetWindX = WindX * SNOW_WIND_SCALE + Random::GenerateFloat(-1.5f, 1.5f);
				auto targetWindZ = WindZ * SNOW_WIND_SCALE + Random::GenerateFloat(-1.5f, 1.5f);
				part.Velocity.x = ApproachVelocity(part.Velocity.x, targetWindX, SNOW_WIND_RESPONSE, 2.5f);
				part.Velocity.z = ApproachVelocity(part.Velocity.z, targetWindZ, SNOW_WIND_RESPONSE, 2.5f);

				if (part.Velocity.y < part.Size / 2)
					part.Velocity.y += part.Size / 5.0f;

				break;
			}

			case WeatherType::Rain:
			{
				auto targetWindX = WindX * RAIN_WIND_SCALE + Random::GenerateFloat(-2.0f, 2.0f);
				auto targetWindZ = WindZ * RAIN_WIND_SCALE + Random::GenerateFloat(-2.0f, 2.0f);
				part.Velocity.x = std::clamp(
					ApproachVelocity(part.Velocity.x, targetWindX, RAIN_WIND_RESPONSE, 4.0f),
					-RAIN_HORIZONTAL_VELOCITY_MAX,
					RAIN_HORIZONTAL_VELOCITY_MAX);
				part.Velocity.z = std::clamp(
					ApproachVelocity(part.Velocity.z, targetWindZ, RAIN_WIND_RESPONSE, 4.0f),
					-RAIN_HORIZONTAL_VELOCITY_MAX,
					RAIN_HORIZONTAL_VELOCITY_MAX);

				if (part.Velocity.y < part.Size * 2 * std::clamp(level.GetWeatherStrength(), 0.6f, 1.0f))
					part.Velocity.y += part.Size / 5.0f;

				break;
			}
			default:
				break;
			}
		}
	}

	void EnvironmentController::SpawnDustParticles(const ScriptInterfaceLevel& level)
	{
		if (!DustParticles.empty())
		{
			DustParticles.erase(
				std::remove_if(
					DustParticles.begin(), DustParticles.end(),
					[](const WeatherParticle& part)
					{
						return !part.Enabled;
					}),
				DustParticles.end());
		}

		for (int i = 0; i < DUST_SPAWN_DENSITY; i++)
		{
			// TODO: Use functions in Math::Random namespace.
			auto pos = Camera.pos.ToVector3i() + Vector3i(
				(rand() % DUST_SPAWN_RADIUS) - (DUST_SPAWN_RADIUS / 2),
				(rand() % DUST_SPAWN_RADIUS) - (DUST_SPAWN_RADIUS / 2),
				(rand() % DUST_SPAWN_RADIUS) - (DUST_SPAWN_RADIUS / 2));

			int roomNumber = Camera.pos.RoomNumber;
			if (!IsPointInRoom(pos, roomNumber))
				roomNumber = FindRoomNumber(pos, Camera.pos.RoomNumber, true);

			if (roomNumber == NO_VALUE || !IsPointInRoom(pos, roomNumber))
				continue;

			// Check if water room.
			if (!TestEnvironment(RoomEnvFlags::ENV_FLAG_WATER, roomNumber))
				continue;

			auto part = WeatherParticle();
			part.UniqueID = (int)DustParticles.size();
			part.Velocity = Random::GenerateDirection() * DUST_VELOCITY_MAX;
			part.Size = Random::GenerateFloat(DUST_SIZE_MAX / 2, DUST_SIZE_MAX);
			part.Type = WeatherType::None;
			part.Life = DUST_LIFE + Random::GenerateInt(-10, 10);
			part.RoomNumber = roomNumber;
			part.Position = pos.ToVector3();
			part.CollisionPosition = part.Position;
			part.Stopped = false;
			part.Enabled = true;
			part.StartLife = part.Life;
			DustParticles.push_back(part);
		}
	}

	void EnvironmentController::SpawnWeatherParticles(const ScriptInterfaceLevel& level)
	{
		// Clean up dead weather particles without touching the independent underwater dust pool.
		if (!WeatherParticles.empty())
		{
			WeatherParticles.erase(
				std::remove_if(
					WeatherParticles.begin(), WeatherParticles.end(),
					[](const WeatherParticle& part)
					{
						return !part.Enabled;
					}),
				WeatherParticles.end());
		}

		if (level.GetWeatherType() == WeatherType::None || level.GetWeatherStrength() == 0.0f)
			return;

		bool clustering = level.GetWeatherClustering();

		int newParticlesCount = 0;
		int density = WEATHER_PARTICLE_SPAWN_DENSITY * level.GetWeatherStrength();

		// Snow is falling twice as fast and must be spawned accordingly fast.
		if (level.GetWeatherType() == WeatherType::Snow)
			density *= 2;

		if (density > 0.0f && level.GetWeatherType() != WeatherType::None)
		{
			while (WeatherParticles.size() < WEATHER_PARTICLE_COUNT_MAX)
			{
				if (newParticlesCount > density)
					break;

				newParticlesCount++;

				float dist = 0;
				if (level.GetWeatherType() == WeatherType::Snow)
				{
					dist = WEATHER_SPAWN_DIST_SNOW;
				}
				else if (level.GetWeatherType() == WeatherType::Rain)
				{
					dist = WEATHER_SPAWN_DIST_RAIN;
				}
				else
				{
					dist = WEATHER_SPAWN_DIST_OTHER;
				}

				float radius = Random::GenerateInt(0, dist);
				short angle = Random::GenerateAngle();

				auto xPos = Camera.pos.x + ((int)(phd_cos(angle) * radius));
				auto zPos = Camera.pos.z + ((int)(phd_sin(angle) * radius));
				auto yPos = Camera.pos.y - (BLOCK(3) + (Random::GenerateInt() & (BLOCK(4) - 1)));

				auto outsideRoom = IsRoomOutside(xPos, yPos, zPos);

				if (outsideRoom == NO_VALUE)
					continue;

				if (g_Level.Rooms[outsideRoom].flags & (ENV_FLAG_WATER | ENV_FLAG_SWAMP))
					continue;

				auto pointColl = GetPointCollision(Vector3i(xPos, yPos, zPos), outsideRoom);

				if (!(pointColl.GetCeilingHeight() < yPos || pointColl.GetSector().GetNextRoomNumber(Vector3i(xPos, yPos, zPos), false).has_value()))
					continue;

				auto part = WeatherParticle();

				switch (level.GetWeatherType())
				{
				case WeatherType::Snow:
					part.ClusterSize = clustering ? (int)(level.GetWeatherStrength() * WEATHER_PARTICLE_CLUSTER_MULT / 2) : 1;
					part.Size = Random::GenerateFloat(SNOW_SIZE_MAX / 3, SNOW_SIZE_MAX);
					part.Velocity.y = Random::GenerateFloat(SNOW_VELOCITY_MAX / 4, SNOW_VELOCITY_MAX) * (part.Size / SNOW_SIZE_MAX);
					part.Velocity.x = WindX * SNOW_WIND_SCALE + Random::GenerateFloat(-WEATHER_PARTICLE_HORIZONTAL_VELOCITY, WEATHER_PARTICLE_HORIZONTAL_VELOCITY);
					part.Velocity.z = WindZ * SNOW_WIND_SCALE + Random::GenerateFloat(-WEATHER_PARTICLE_HORIZONTAL_VELOCITY, WEATHER_PARTICLE_HORIZONTAL_VELOCITY);
					part.Life = (SNOW_VELOCITY_MAX / 3) + ((SNOW_VELOCITY_MAX / 2) - ((int)part.Velocity.y >> 2));
					break;

				case WeatherType::Rain:
					part.ClusterSize = clustering ? (int)(level.GetWeatherStrength() * WEATHER_PARTICLE_CLUSTER_MULT) : 1;
					part.Size = Random::GenerateFloat(RAIN_SIZE_MAX / 2, RAIN_SIZE_MAX);
					part.Velocity.y = Random::GenerateFloat(RAIN_VELOCITY_MAX / 2, RAIN_VELOCITY_MAX) * (part.Size / RAIN_SIZE_MAX) * std::clamp(level.GetWeatherStrength(), 0.6f, 1.0f);
					part.Velocity.x = WindX * RAIN_WIND_SCALE + Random::GenerateFloat(-WEATHER_PARTICLE_HORIZONTAL_VELOCITY, WEATHER_PARTICLE_HORIZONTAL_VELOCITY);
					part.Velocity.z = WindZ * RAIN_WIND_SCALE + Random::GenerateFloat(-WEATHER_PARTICLE_HORIZONTAL_VELOCITY, WEATHER_PARTICLE_HORIZONTAL_VELOCITY);
					part.Life = RAIN_VELOCITY_MAX - part.Velocity.y;
					break;
				}

				part.UniqueID = (int)WeatherParticles.size();
				part.Type = level.GetWeatherType();
				part.RoomNumber = outsideRoom;
				part.Position.x = xPos;
				part.Position.y = yPos;
				part.Position.z = zPos;
				part.CollisionPosition = part.Position;
				part.Stopped = false;
				part.Enabled = true;
				part.CollisionCheckDelay = 0;
				part.StartLife = part.Life;

				WeatherParticles.push_back(part);
			}
		}
	}

	void EnvironmentController::SpawnMeteorParticles(const ScriptInterfaceLevel& level)
	{
		// Clean up dead particles.
		if (!Meteors.empty())
		{
			Meteors.erase(
				std::remove_if(
					Meteors.begin(), Meteors.end(),
					[](const MeteorParticle& part)
					{
						return !part.Active;
					}),
				Meteors.end());
		}

		if (level.GetStarfieldMeteorCount() == 0)
			return;

		int density = level.GetStarfieldMeteorSpawnDensity();
		if (density > 0)
		{
			int newParticlesCount = 0;

			while (Meteors.size() < level.GetStarfieldMeteorCount())
			{
				if (newParticlesCount > density)
					break;

				auto horizontalDir = Random::GenerateDirection2D();

				auto part = MeteorParticle();

				part.Active = true;
				part.Life = METEOR_PARTICLE_LIFE_MAX;
				part.StartPosition =
					part.Position = Random::GenerateDirectionInCone(-Vector3::UnitY, 40.0f) * BLOCK(1.5f);
				part.Fade = 0.0f;
				part.Color = Vector3(
					Random::GenerateFloat(0.6f, 1.0f),
					Random::GenerateFloat(0.6f, 1.0f),
					Random::GenerateFloat(0.6f, 1.0f));

				part.Direction = Random::GenerateDirectionInCone(Vector3(horizontalDir.x, 0, horizontalDir.y), 10.0f);
				if (part.Direction.y < 0.0f)
					part.Direction.y = -part.Direction.y;

				Meteors.push_back(part);

				newParticlesCount++;
			}
		}
	}
}