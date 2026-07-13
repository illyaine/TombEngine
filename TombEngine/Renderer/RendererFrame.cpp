#include "framework.h"
#include "Renderer/Renderer.h"

#include <array>
#include <cstring>

#include "Game/Animation/Animation.h"
#include "Game/camera.h"
#include "Game/collision/Sphere.h"
#include "Game/effects/Decal.h"
#include "Game/effects/effects.h"
#include "Game/effects/weather.h"
#include "Game/items.h"
#include "Game/Lara/lara.h"
#include "Game/Setup.h"
#include "Game/spotcam.h"
#include "Math/Math.h"
#include "Math/Objects/GameBoundingBox.h"
#include "Objects/Effects/LensFlare.h"
#include "Renderer/RenderView.h"
#include "Scripting/Include/Flow/ScriptInterfaceFlowHandler.h"
#include "Specific/level.h"
#include "Specific/trutils.h"

using namespace TEN::Animation;
using namespace TEN::Collision::Sphere;
using namespace TEN::Effects::Decal;
using namespace TEN::Effects::Environment;
using namespace TEN::Entities::Effects;
using namespace TEN::Math;
using namespace TEN::Utils;

namespace TEN::Renderer
{
	using TEN::Memory::LinearArrayBuffer;

	namespace
	{
		constexpr auto FULL_VIEW_PORT = Vector4(-1.0f, -1.0f, 1.0f, 1.0f);
		unsigned int RoomVisibilityGeneration = 0;
		unsigned int ShadowLightSelectionGeneration = ~0u;

		void PrepareRoomForVisibility(RendererRoom& room)
		{
			if (room.VisibilityGeneration == RoomVisibilityGeneration)
				return;

			room.ItemsToDraw.clear();
			room.EffectsToDraw.clear();
			room.StaticsToDraw.clear();
			room.LightsToDraw.clear();
			room.DynamicLightCandidates.clear();
			room.DynamicLightCandidatesReady = false;
			room.Decals.clear();
			room.Visited = false;
			room.ViewPort = FULL_VIEW_PORT;
			room.VisibilityGeneration = RoomVisibilityGeneration;
		}

		void PrepareDoorForVisibility(RendererDoor& door)
		{
			if (door.VisibilityGeneration == RoomVisibilityGeneration)
				return;

			door.Visited = false;
			door.InvisibleFromCamera = false;
			door.DotProduct = FLT_MAX;
			door.VisibilityGeneration = RoomVisibilityGeneration;
		}
	}

	void Renderer::CollectRooms(RenderView& renderView, bool onlyRooms)
	{
		_visitedRoomsStack.clear();
		const bool rebuildRendererCaches = _invalidateCache;

		RoomVisibilityGeneration++;
		if (RoomVisibilityGeneration == 0)
		{
			// Generation wrap is practically unreachable, but reset markers once to preserve correctness.
			for (auto& room : _rooms)
			{
				room.VisibilityGeneration = 0;
				room.FogCollectionGeneration = 0;
				for (auto& door : room.Doors)
					door.VisibilityGeneration = 0;
			}

			RoomVisibilityGeneration = 1;
			ShadowLightSelectionGeneration = ~0u;
		}

		// Select the current shadow light before collecting items so safe off-screen casters
		// can be rejected when they are outside both the camera and shadow-light influence.
		if (!onlyRooms)
			CollectLightsForCamera();

		GetVisibleRooms(NO_VALUE, renderView.Camera.RoomNumber, FULL_VIEW_PORT, false, 0, onlyRooms, renderView);

		_invalidateCache = false;

		bool laraFound = false;

		for (auto* roomPtr : renderView.RoomsToDraw)
		{
			// Prepare real DX scissor test rectangle.
			roomPtr->ClipBounds.Left = (roomPtr->ViewPort.x + 1.0f) * _screenWidth * 0.5f;
			roomPtr->ClipBounds.Bottom = (1.0f - roomPtr->ViewPort.y) * _screenHeight * 0.5f;
			roomPtr->ClipBounds.Right = (roomPtr->ViewPort.z + 1.0f) * _screenWidth * 0.5f;
			roomPtr->ClipBounds.Top = (1.0f - roomPtr->ViewPort.w) * _screenHeight * 0.5f;

			// Indicate that Lara object is found.
			if (roomPtr->RoomNumber == LaraItem->RoomNumber)
				laraFound = true;
		}

		// HACK: Force adding Lara's room to room list, in case she is in one of camera's neighbor rooms.
		if (!laraFound && Contains(_rooms[renderView.Camera.RoomNumber].Neighbors, (int)LaraItem->RoomNumber))
		{
			auto& laraRoom = _rooms[LaraItem->RoomNumber];
			PrepareRoomForVisibility(laraRoom);
			laraRoom.Visited = true;
			renderView.RoomsToDraw.push_back(&laraRoom);
			CollectItems(LaraItem->RoomNumber, renderView);
		}

		// Reuse collection storage instead of allocating temporary vectors every frame.
		static thread_local auto tempFogBulbs = std::vector<RendererFogBulb>{};
		tempFogBulbs.clear();
		if (tempFogBulbs.capacity() < MAX_FOG_BULBS_DRAW)
			tempFogBulbs.reserve(MAX_FOG_BULBS_DRAW);

		auto collectFogBulb = [&](const RendererLight& light)
		{
			if (light.Type != LightType::FogBulb)
				return;

			// Test bigger radius to avoid bad clipping.
			if (!renderView.Camera.Frustum.SphereInFrustum(light.Position, light.Out * 1.2f))
				return;

			RendererFogBulb bulb;
			bulb.Position = light.Position;
			bulb.Density = light.Intensity;
			bulb.Color = light.Color;
			bulb.Radius = light.Out;
			bulb.FogBulbToCameraVector = bulb.Position - renderView.Camera.WorldPosition;
			bulb.Distance = bulb.FogBulbToCameraVector.LengthSquared();
			tempFogBulbs.push_back(bulb);
		};

		for (const auto& light : _dynamicLights[_dynamicLightList])
			collectFogBulb(light);

		// Preserve the original all-active-room fog semantics, but cache which rooms can actually contribute.
		static thread_local auto staticFogRooms = std::vector<RendererRoom*>{};
		static thread_local size_t cachedFogRoomCount = 0;
		if (rebuildRendererCaches || cachedFogRoomCount != _rooms.size())
		{
			staticFogRooms.clear();
			cachedFogRoomCount = _rooms.size();

			for (auto& room : _rooms)
			{
				bool hasFogBulb = false;
				for (const auto& light : room.Lights)
				{
					if (light.Type == LightType::FogBulb)
					{
						hasFogBulb = true;
						break;
					}
				}

				if (hasFogBulb)
					staticFogRooms.push_back(&room);
			}
		}

		for (auto* room : staticFogRooms)
		{
			if (room == nullptr || !g_Level.Rooms[room->RoomNumber].Active())
				continue;

			for (const auto& light : room->Lights)
				collectFogBulb(light);
		}

		auto fogBulbCompare = [](const RendererFogBulb& bulb0, const RendererFogBulb& bulb1)
		{
			return bulb0.Distance < bulb1.Distance;
		};

		const size_t fogBulbCount = std::min((size_t)MAX_FOG_BULBS_DRAW, tempFogBulbs.size());
		if (fogBulbCount < tempFogBulbs.size())
			std::partial_sort(tempFogBulbs.begin(), tempFogBulbs.begin() + fogBulbCount, tempFogBulbs.end(), fogBulbCompare);
		else
			std::sort(tempFogBulbs.begin(), tempFogBulbs.end(), fogBulbCompare);

		for (size_t i = 0; i < fogBulbCount; i++)
		{
			auto bulb = tempFogBulbs[i];
			bulb.Distance = sqrt(bulb.Distance);
			renderView.FogBulbsToDraw.push_back(bulb);
		}

		// Collect lens flares.
		static thread_local auto tempLensFlares = std::vector<RendererLensFlare>{};
		tempLensFlares.clear();
		if (tempLensFlares.capacity() < MAX_LENS_FLARES_DRAW)
			tempLensFlares.reserve(MAX_LENS_FLARES_DRAW);

		auto cameraDir = renderView.Camera.WorldDirection;
		cameraDir.Normalize();

		for (const auto& lensFlare : LensFlares)
		{
			auto lensFlareToCamera = lensFlare.Position - renderView.Camera.WorldPosition;
			float dist = lensFlareToCamera.Length();
			if (dist > EPSILON)
				lensFlareToCamera /= dist;
			else
				lensFlareToCamera = Vector3::Zero;

			if (lensFlare.IsGlobal)
				dist = 0.0f;

			if (lensFlareToCamera.Dot(cameraDir) >= 0.0f)
			{
				auto lensFlareToDraw = RendererLensFlare{};
				lensFlareToDraw.Position = lensFlare.Position;
				lensFlareToDraw.Distance = dist;
				lensFlareToDraw.Color = lensFlare.Color;
				lensFlareToDraw.SpriteID = lensFlare.SpriteID;
				lensFlareToDraw.Direction = lensFlareToCamera;
				lensFlareToDraw.IsGlobal = lensFlare.IsGlobal;

				tempLensFlares.push_back(lensFlareToDraw);
			}
		}

		auto lensFlareCompare = [](const RendererLensFlare& lensFlare0, const RendererLensFlare& lensFlare1)
		{
			if (lensFlare0.IsGlobal && !lensFlare1.IsGlobal)
				return true;

			if (!lensFlare0.IsGlobal && lensFlare1.IsGlobal)
				return false;

			return lensFlare0.Distance < lensFlare1.Distance;
		};

		const size_t lensFlareCount = std::min((size_t)MAX_LENS_FLARES_DRAW, tempLensFlares.size());
		if (lensFlareCount < tempLensFlares.size())
			std::partial_sort(tempLensFlares.begin(), tempLensFlares.begin() + lensFlareCount, tempLensFlares.end(), lensFlareCompare);
		else
			std::sort(tempLensFlares.begin(), tempLensFlares.end(), lensFlareCompare);

		for (size_t i = 0; i < lensFlareCount; i++)
			renderView.LensFlaresToDraw.push_back(tempLensFlares[i]);
	}

	bool Renderer::CheckPortal(short parentRoomNumber, RendererDoor* door, Vector4 viewPort, Vector4* clipPort, RenderView& renderView)
	{
		_numCheckPortalCalls++;

		RendererRoom* room = &_rooms[parentRoomNumber];

		int  zClip = 0;
		Vector4 p[4];

		*clipPort = Vector4(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);

		for (int i = 0; i < 4; i++)
		{
			if (!door->Visited)
			{
				p[i] = Vector4::Transform(door->AbsoluteVertices[i], renderView.Camera.ViewProjection);
				if (p[i].w > 0.0f)
				{
					p[i].x *= (1.0f / p[i].w);
					p[i].y *= (1.0f / p[i].w);

				}
				door->TransformedVertices[i] = p[i];
			}
			else
			{
				p[i] = door->TransformedVertices[i];
			}

			if (p[i].w > 0.0f)
			{
				clipPort->x = std::min(clipPort->x, p[i].x);
				clipPort->y = std::min(clipPort->y, p[i].y);
				clipPort->z = std::max(clipPort->z, p[i].x);
				clipPort->w = std::max(clipPort->w, p[i].y);
			}
			else
			{
				zClip++;
			}
		}

		door->Visited = true;

		if (zClip == 4)
			return false;

		if (zClip > 0)
		{
			for (int i = 0; i < 4; i++) 
			{
				auto a = p[i];
				auto b = p[(i + 1) % 4];

				if ((a.w > 0.0f) ^ (b.w > 0.0f))
				{

					if (a.x < 0.0f && b.x < 0.0f)
					{
						clipPort->x = -1.0f;
					}
					else
					{
						if (a.x > 0.0f && b.x > 0.0f)
						{
							clipPort->z = 1.0f;
						}
						else 
						{
							clipPort->x = -1.0f;
							clipPort->z = 1.0f;
						}
					}

					if (a.y < 0.0f && b.y < 0.0f)
					{
						clipPort->y = -1.0f;
					}
					else
					{
						if (a.y > 0.0f && b.y > 0.0f)
						{
							clipPort->w = 1.0f;
						}
						else 
						{
							clipPort->y = -1.0f;
							clipPort->w = 1.0f;
						}
					}
				}
			}
		}

		if (clipPort->x > viewPort.z ||
			clipPort->y > viewPort.w ||
			clipPort->z < viewPort.x || 
			clipPort->w < viewPort.y)
		{
			return false;
		}

		clipPort->x = std::max(clipPort->x, viewPort.x);
		clipPort->y = std::max(clipPort->y, viewPort.y);
		clipPort->z = std::min(clipPort->z, viewPort.z);
		clipPort->w = std::min(clipPort->w, viewPort.w);

		return true;
	}

	void Renderer::GetVisibleRooms(short from, short to, Vector4 viewPort, bool water, int count, bool onlyRooms, RenderView& renderView)
	{
		// FIXME: This is an urgent hack to fix stack overflow crashes.
		// See https://github.com/MontyTRC89/TombEngine/issues/947 for details.
		// NOTE by MontyTRC: I'd keep this as a failsafe solution for 0.00000001% of cases we could have problems

		int stackSize = (int)_visitedRoomsStack.size();
		int stackMinIndex = std::max(0, int(stackSize - 5));

		for (int i = stackSize - 1; i >= stackMinIndex; i--)
		{
			if (_visitedRoomsStack[i] == to)
			{
				TENLog("Circle detected in room " + std::to_string(to), LogLevel::Warning, LogConfig::Debug);
				return;
			}
		}

		auto* room = &_rooms[to];
		PrepareRoomForVisibility(*room);
		
		static constexpr int MAX_SEARCH_DEPTH = 64;
		if (room->Visited && count > MAX_SEARCH_DEPTH)
		{
			TENLog(
				"Maximum room collection depth of " + std::to_string(MAX_SEARCH_DEPTH) + " was reached with room " + std::to_string(to),
				LogLevel::Warning, LogConfig::Debug);
			return;
		}

		_visitedRoomsStack.push_back(to);

		_numGetVisibleRoomsCalls++;

		if (!room->Visited)
		{
			room->Visited = true;

			renderView.RoomsToDraw.push_back(room);

			CollectLightsForRoom(to, renderView);
			CollectDecalsForRoom(to, renderView);

			if (!onlyRooms)
			{
				CollectItems(to, renderView);
				CollectStatics(to, renderView);
				CollectEffects(to);
			}
		}

		room->ViewPort.x = std::min(room->ViewPort.x, viewPort.x);
		room->ViewPort.y = std::min(room->ViewPort.y, viewPort.y);
		room->ViewPort.z = std::max(room->ViewPort.z, viewPort.z);
		room->ViewPort.w = std::max(room->ViewPort.w, viewPort.w);

		Vector4 clipPort;
		for (int i = 0; i < room->Doors.size(); i++)
		{
			auto* door = &room->Doors[i];
			PrepareDoorForVisibility(*door);

			if (door->InvisibleFromCamera)
				continue;

			if (!door->Visited)
			{
				door->CameraToDoor = Vector3(
					Camera.pos.x - (door->AbsoluteVertices[0].x),
					Camera.pos.y - (door->AbsoluteVertices[0].y),
					Camera.pos.z - (door->AbsoluteVertices[0].z));
			}

			// IMPORTANT: dot = 0 would generate ambiguity becase door could be traversed in both directions, potentially 
			// generating endless loops. We need to exclude this.

			if (door->DotProduct == FLT_MAX)
			{
				door->DotProduct = 
					door->Normal.x * door->CameraToDoor.x +
					door->Normal.y * door->CameraToDoor.y +
					door->Normal.z * door->CameraToDoor.z;
				_numDotProducts++;
			}

			if (door->DotProduct < 0)
			{
				door->InvisibleFromCamera = true;
				continue;
			}

			if (from != door->RoomNumber && CheckPortal(to, door, viewPort, &clipPort, renderView))
				GetVisibleRooms(to, door->RoomNumber, clipPort, water, count + 1, onlyRooms, renderView);
		}

		_visitedRoomsStack.pop_back();
	}

	void Renderer::CollectMirrors(RenderView& renderView)
	{
		// Collect mirrors first because they are needed while collecting moveables.
		for (const auto& mirror : g_Level.Mirrors)
		{
			// TODO: Avoid LaraItem global.
			if (mirror.RoomNumber != Camera.pos.RoomNumber && mirror.RoomNumber != LaraItem->RoomNumber)
				continue;

			if (!mirror.Enabled)
				continue;

			auto& rendererMirror = renderView.Mirrors.emplace_back();
			rendererMirror.RoomNumber = mirror.RoomNumber;
			rendererMirror.ReflectionMatrix = mirror.ReflectionMatrix;
			rendererMirror.ReflectPlayer = mirror.ReflectPlayer;
			rendererMirror.ReflectMoveables = mirror.ReflectMoveables;
			rendererMirror.ReflectStatics = mirror.ReflectStatics;
			rendererMirror.ReflectSprites = mirror.ReflectSprites;
			rendererMirror.ReflectLights = mirror.ReflectLights;
		}
	}

	void Renderer::CollectItems(short roomNumber, RenderView& renderView)
	{
		if (_rooms.size() <= roomNumber)
			return;

		auto& rendererRoom = _rooms[roomNumber];
		const auto& room = g_Level.Rooms[rendererRoom.RoomNumber];

		bool isRoomReflected = IsRoomReflected(renderView, roomNumber);
		const bool hasShadowLight =
			_shadowLight != nullptr &&
			(_shadowLight->Type == LightType::Point || _shadowLight->Type == LightType::Spot) &&
			_shadowLight->Out > EPSILON;
		const auto shadowLightPosition = hasShadowLight ?
			((_shadowLight->Hash == 0) ? _shadowLight->Position : Vector3::Lerp(_shadowLight->PrevPosition, _shadowLight->Position, GetInterpolationFactor())) :
			Vector3::Zero;

		short itemNumber = NO_VALUE;
		for (itemNumber = room.itemNumber; itemNumber != NO_VALUE; itemNumber = g_Level.Items[itemNumber].NextItem)
		{
			const auto& item = g_Level.Items[itemNumber];

			if (item.ObjectNumber == ID_LARA && itemNumber == g_Level.Items[itemNumber].NextItem)
				break;

			if (item.Status == ITEM_INVISIBLE)
				continue;

			if (item.Model.Color.w < EPSILON)
				continue;

			if (item.ObjectNumber == ID_LARA && UseSpotCam && (SpotcamOverlay || SpotcamDontDrawLara))
				continue;

			if (item.ObjectNumber == ID_LARA && CurrentLevel == 0 && !g_GameFlow->IsLaraInTitleEnabled())
				continue;

			if (!_moveableObjects[item.ObjectNumber].has_value())
				continue;

			auto& obj = _moveableObjects[item.ObjectNumber].value();

			if (obj.Hidden)
				continue;

			const size_t transformCount = std::min(
				(size_t)MAX_BONES,
				std::max(obj.AnimationTransforms.size(), obj.ObjectMeshes.size()));

			bool inFrustum = true;
			if (!isRoomReflected)
			{
				bool canUseAnimationBounds =
					item.ObjectNumber != ID_LARA &&
					!Objects[item.ObjectNumber].Animations.empty() &&
					GetSkinningMode(obj, item.Model.SkinIndex) != SkinningMode::Full &&
					item.Model.MeshIndex.size() == obj.ObjectMeshes.size();

				if (canUseAnimationBounds)
				{
					for (const auto& mutator : item.Model.Mutators)
					{
						if (!mutator.IsEmpty())
						{
							canUseAnimationBounds = false;
							break;
						}
					}
				}

				if (canUseAnimationBounds)
				{
					for (size_t i = 0; i < obj.ObjectMeshes.size(); i++)
					{
						if (obj.ObjectMeshes[i] == nullptr || GetMesh(item.Model.MeshIndex[i]) != obj.ObjectMeshes[i])
						{
							canUseAnimationBounds = false;
							break;
						}
					}
				}

				if (canUseAnimationBounds)
				{
					auto broadSphere = GameBoundingBox(&item).ToLocalBoundingSphere();
					auto world =
						Matrix::CreateScale(item.Pose.Scale) *
						item.Pose.Orientation.ToRotationMatrix() *
						Matrix::CreateTranslation(item.Pose.Position.ToVector3());

					broadSphere.Center = Vector3::Transform(broadSphere.Center, world);
					broadSphere.Radius *= std::max({
						std::abs(item.Pose.Scale.x),
						std::abs(item.Pose.Scale.y),
						std::abs(item.Pose.Scale.z) });

					const float cullRadius = broadSphere.Radius * 1.5f;
					const bool visibleToCamera = broadSphere.Radius <= EPSILON ||
						renderView.Camera.Frustum.SphereInFrustum(broadSphere.Center, cullRadius);

					const bool shadowModeAllowsObject =
						g_Configuration.ShadowType != ShadowMode::None &&
						obj.ShadowType != ShadowMode::None &&
						(g_Configuration.ShadowType != ShadowMode::Player || obj.ShadowType == ShadowMode::Player);

					bool neededForShadow = false;
					if (!visibleToCamera && shadowModeAllowsObject && hasShadowLight)
					{
						const float shadowCullRadius = _shadowLight->Out + cullRadius;
						neededForShadow = Vector3::DistanceSquared(broadSphere.Center, shadowLightPosition) <= SQUARE(shadowCullRadius);
					}

					inFrustum = visibleToCamera || neededForShadow;
				}
			}

			auto& newItem = _items[itemNumber];

			newItem.ItemNumber = itemNumber;
			newItem.ObjectID = item.ObjectNumber;
			newItem.Color = item.Model.Color;
			newItem.Position = item.Pose.Position.ToVector3();
			newItem.Translation = Matrix::CreateTranslation(newItem.Position);
			newItem.Rotation = item.Pose.Orientation.ToRotationMatrix();
			newItem.Scale = Matrix::CreateScale(item.Pose.Scale);
			newItem.World = newItem.Scale * newItem.Rotation * newItem.Translation;

			// Keep the current world transform available to lazy bone queries, but defer animation and interpolation work until visible.
			newItem.DisableInterpolation = item.DisableInterpolation || newItem.DisableInterpolation;
			if (!inFrustum)
			{
				newItem.DisableInterpolation = true;
				continue;
			}

			if (!newItem.DoneAnimations)
			{
				if (item.ObjectNumber == ID_LARA)
					UpdateLaraAnimations(false);
				else
					UpdateItemAnimations(itemNumber, false);
			}

			// Disable interpolation when object has traveled significant distance.
			// Needed because when object goes out of frustum, previous position doesn't update.
			bool posChanged = Vector3::DistanceSquared(newItem.PrevPosition, newItem.Position) > SQUARE(BLOCK(1));

			if (newItem.DisableInterpolation || posChanged)
			{
				// NOTE: Interpolation always returns same result.
				newItem.PrevPosition = newItem.Position;
				newItem.PrevTranslation = newItem.Translation;
				newItem.PrevRotation = newItem.Rotation;
				newItem.PrevScale = newItem.Scale;
				newItem.PrevWorld = newItem.World;

				// Otherwise all frames until next ControlPhase will not be interpolated.
				newItem.DisableInterpolation = false;
				std::memcpy(newItem.PrevAnimTransforms, newItem.AnimTransforms, transformCount * sizeof(Matrix));
			}

			// Force interpolation only for player in player freeze mode.
			bool forceValue = g_GameFlow->CurrentFreezeMode == FreezeMode::Player && item.ObjectNumber == ID_LARA;
			float interpFactor = GetInterpolationFactor(forceValue);

			newItem.InterpolatedPosition = Vector3::Lerp(newItem.PrevPosition, newItem.Position, interpFactor);
			newItem.InterpolatedTranslation = Matrix::Lerp(newItem.PrevTranslation, newItem.Translation, interpFactor);
			newItem.InterpolatedRotation = Matrix::Lerp(newItem.InterpolatedRotation, newItem.Rotation, interpFactor);
			newItem.InterpolatedScale = Matrix::Lerp(newItem.InterpolatedScale, newItem.Scale, interpFactor);
			newItem.InterpolatedWorld = Matrix::Lerp(newItem.PrevWorld, newItem.World, interpFactor);
			
			for (size_t j = 0; j < transformCount; j++)
				newItem.InterpolatedAnimTransforms[j] = Matrix::Lerp(newItem.PrevAnimTransforms[j], newItem.AnimTransforms[j], interpFactor);

			CalculateLightFades(&newItem);
			CollectLightsForItem(&newItem);

			rendererRoom.ItemsToDraw.push_back(&newItem);
		}
	}

	void Renderer::CollectStatics(short roomNumber, RenderView& renderView)
	{
		if (_rooms.size() <= roomNumber)
			return;

		auto& rendererRoom = _rooms[roomNumber];
		auto& nativeRoom = g_Level.Rooms[rendererRoom.RoomNumber];

		if (nativeRoom.mesh.empty())
			return;

		bool isRoomReflected = IsRoomReflected(renderView, roomNumber);
		const bool trackStats = (_debugPage == RendererDebugPage::RendererStats);

		for (int i = 0; i < rendererRoom.Statics.size(); i++)
		{
			if (trackStats)
				_numTestedStatics++;

			auto& rendererStatic = rendererRoom.Statics[i];
			auto& nativeStatic = nativeRoom.mesh[i];

			if (nativeStatic.Dirty || _invalidateCache)
			{
				rendererStatic.ObjectNumber = nativeStatic.Slot;
				rendererStatic.Color = nativeStatic.Color;
				rendererStatic.OriginalSphere = Statics[rendererStatic.ObjectNumber].visibilityBox.ToLocalBoundingSphere();
				rendererStatic.Pose = nativeStatic.Pose;
				rendererStatic.Update(GetInterpolationFactor());

				nativeStatic.Dirty = (rendererStatic.PrevPose != rendererStatic.Pose);
			}

			if (!(nativeStatic.Flags & StaticMeshFlags::SM_VISIBLE))
				continue;

			if (nativeStatic.Color.w < EPSILON)
				continue;

			if (!_staticObjects[Statics.GetIndex(rendererStatic.ObjectNumber)].has_value())
				continue;

			const auto& rendererObj = GetStaticRendererObject(rendererStatic.ObjectNumber);
			if (rendererObj.ObjectMeshes.empty())
				continue;

			if (!isRoomReflected && !renderView.Camera.Frustum.SphereInFrustum(rendererStatic.Sphere.Center, rendererStatic.Sphere.Radius))
				continue;

			if (trackStats)
				_numVisibleStatics++;

			if (rendererObj.ObjectMeshes.front()->LightMode != LightMode::Static)
			{
				if (trackStats)
					_numDynamicLitStatics++;
				if (rendererStatic.CacheLights || _invalidateCache)
				{
					if (trackStats)
						_numStaticLightCacheMisses++;
					rendererStatic.CachedRoomLights.clear();
					CollectLights(
						rendererStatic.Pose.Position.ToVector3(), ITEM_LIGHT_COLLECTION_RADIUS,
						rendererRoom.RoomNumber, NO_VALUE, false, false,
						&rendererStatic.CachedRoomLights, &rendererStatic.LightsToDraw);
					rendererStatic.CacheLights = false;
				}
				else
				{
					if (trackStats)
						_numStaticLightCacheHits++;
					CollectLights(
						rendererStatic.Pose.Position.ToVector3(), ITEM_LIGHT_COLLECTION_RADIUS,
						rendererRoom.RoomNumber, NO_VALUE, false, true,
						&rendererStatic.CachedRoomLights, &rendererStatic.LightsToDraw);
				}
			}
			else
			{
				rendererStatic.LightsToDraw.clear();
			}

			rendererRoom.StaticsToDraw.push_back(&rendererStatic);
			renderView.SortedStaticsToDraw[rendererStatic.ObjectNumber].push_back(&rendererStatic);
		}
	}

	void Renderer::CollectLights(const Vector3& pos, float radius, int roomNumber, int prevRoomNumber, bool prioritizeShadowLight, bool useCachedRoomLights, std::vector<RendererLightNode>* roomsLights, std::vector<RendererLight*>* outputLights)
	{
		if (_rooms.size() <= roomNumber || outputLights == nullptr)
			return;

		const bool trackStaticStats = (_debugPage == RendererDebugPage::RendererStats && roomsLights != nullptr);
		auto& room = _rooms[roomNumber];
		if (!room.StaticLightCandidatesValid || _invalidateCache)
		{
			room.StaticLightCandidates.clear();
			for (int roomToCheck : room.Neighbors)
			{
				if (roomToCheck < 0 || roomToCheck >= _rooms.size())
					continue;

				for (auto& light : _rooms[roomToCheck].Lights)
				{
					if (light.Type != LightType::FogBulb)
						room.StaticLightCandidates.push_back({ &light, roomToCheck });
				}
			}
			room.StaticLightCandidatesValid = true;
		}

		constexpr size_t CANDIDATE_CAPACITY = MAX_LIGHTS_PER_ITEM + 1;
		std::array<RendererLightNode, CANDIDATE_CAPACITY> bestLights = {};
		size_t bestLightCount = 0;
		std::array<RendererLightNode, CANDIDATE_CAPACITY> bestRoomLights = {};
		size_t bestRoomLightCount = 0;

		auto isBetterLight = [](const RendererLightNode& a, const RendererLightNode& b)
		{
			return (a.Dynamic == b.Dynamic) ? (a.LocalIntensity > b.LocalIntensity) : (a.Dynamic > b.Dynamic);
		};

		auto addBestLight = [&](const RendererLightNode& node)
		{
			size_t insertIndex = 0;
			while (insertIndex < bestLightCount && !isBetterLight(node, bestLights[insertIndex]))
				insertIndex++;

			if (insertIndex >= CANDIDATE_CAPACITY)
				return;

			if (bestLightCount < CANDIDATE_CAPACITY)
				bestLightCount++;

			for (size_t i = bestLightCount - 1; i > insertIndex; i--)
				bestLights[i] = bestLights[i - 1];

			bestLights[insertIndex] = node;
		};

		auto addBestRoomLight = [&](const RendererLightNode& node)
		{
			size_t insertIndex = 0;
			while (insertIndex < bestRoomLightCount && !isBetterLight(node, bestRoomLights[insertIndex]))
				insertIndex++;

			if (insertIndex >= CANDIDATE_CAPACITY)
				return;

			if (bestRoomLightCount < CANDIDATE_CAPACITY)
				bestRoomLightCount++;

			for (size_t i = bestRoomLightCount - 1; i > insertIndex; i--)
				bestRoomLights[i] = bestRoomLights[i - 1];

			bestRoomLights[insertIndex] = node;
		};

		RendererLight* brightestLight = nullptr;
		float brightest = 0.0f;

		auto processDynamicLight = [&](RendererLight& light)
		{
			if (trackStaticStats)
				_numStaticLightCandidateChecks++;

			if (light.Out <= EPSILON)
				return;

			float distSqr = Vector3::DistanceSquared(pos, light.Position);
			if (distSqr >= SQUARE(BLOCK(20)) || distSqr > SQUARE(light.Out + radius))
				return;

			float distance = sqrt(distSqr);
			float attenuation = 1.0f - distance / light.Out;
			float intensity = attenuation * light.Intensity * light.Luma;

			if (prioritizeShadowLight && light.CastShadows && intensity >= brightest)
			{
				brightest = intensity;
				brightestLight = &light;
			}

			addBestLight({ &light, intensity, distance, 1 });
		};

		if (room.DynamicLightCandidatesReady)
		{
			for (auto* light : room.DynamicLightCandidates)
			{
				if (light != nullptr)
					processDynamicLight(*light);
			}
		}
		else
		{
			for (auto& light : _dynamicLights[_dynamicLightList])
				processDynamicLight(light);
		}

		if (!useCachedRoomLights)
		{
			if (roomsLights != nullptr)
			{
				roomsLights->clear();
				if (roomsLights->capacity() < CANDIDATE_CAPACITY)
					roomsLights->reserve(CANDIDATE_CAPACITY);
			}

			for (const auto& candidate : room.StaticLightCandidates)
			{
				if (trackStaticStats)
					_numStaticLightCandidateChecks++;
				auto* lightPtr = candidate.Light;
				if (lightPtr == nullptr)
					continue;

				auto& light = *lightPtr;
				float intensity = 0.0f;
				float distance = 0.0f;

				if (light.Type == LightType::Sun)
				{
					if (candidate.SourceRoomNumber != roomNumber &&
						(prevRoomNumber != candidate.SourceRoomNumber || prevRoomNumber == NO_VALUE))
					{
						continue;
					}

					intensity = light.Intensity * light.Luma;
				}
				else if (light.Type == LightType::Point || light.Type == LightType::Shadow || light.Type == LightType::Spot)
				{
					if (light.Out <= EPSILON)
						continue;

					float distSqr = Vector3::DistanceSquared(pos, light.Position);
					if (distSqr >= SQUARE(BLOCK(20)) || distSqr > SQUARE(light.Out + radius))
						continue;

					distance = sqrt(distSqr);
					float attenuation = 1.0f - distance / light.Out;
					intensity = attenuation * light.Intensity * light.Luma;

					if (prioritizeShadowLight && light.CastShadows && light.Type != LightType::Shadow && intensity >= brightest)
					{
						brightest = intensity;
						brightestLight = &light;
					}
				}
				else
				{
					continue;
				}

				RendererLightNode node = { &light, intensity, distance, 0 };
				if (roomsLights != nullptr)
					addBestRoomLight(node);
				addBestLight(node);
			}

			if (roomsLights != nullptr)
			{
				for (size_t i = 0; i < bestRoomLightCount; i++)
					roomsLights->push_back(bestRoomLights[i]);
			}
		}
		else if (roomsLights != nullptr)
		{
			for (const auto& node : *roomsLights)
				addBestLight(node);
		}

		outputLights->clear();
		if (outputLights->capacity() < MAX_LIGHTS_PER_ITEM)
			outputLights->reserve(MAX_LIGHTS_PER_ITEM);

		if (prioritizeShadowLight && brightestLight != nullptr)
			outputLights->push_back(brightestLight);

		for (size_t i = 0; i < bestLightCount && outputLights->size() < MAX_LIGHTS_PER_ITEM; i++)
		{
			if (prioritizeShadowLight && bestLights[i].Light == brightestLight)
				continue;

			outputLights->push_back(bestLights[i].Light);
		}
	}

	void Renderer::CollectLightsForCamera()
	{
		if (ShadowLightSelectionGeneration == RoomVisibilityGeneration)
			return;

		ShadowLightSelectionGeneration = RoomVisibilityGeneration;
		_shadowLight = nullptr;

		if (g_Configuration.ShadowType == ShadowMode::None ||
			Camera.pos.RoomNumber < 0 || Camera.pos.RoomNumber >= _rooms.size())
		{
			return;
		}

		const auto cameraPosition = Vector3(Camera.pos.x, Camera.pos.y, Camera.pos.z);
		const auto& cameraRoom = _rooms[Camera.pos.RoomNumber];
		float brightest = 0.0f;

		auto considerLight = [&](RendererLight& light)
		{
			if (!light.CastShadows ||
				(light.Type != LightType::Point && light.Type != LightType::Spot) ||
				light.Out <= EPSILON)
			{
				return;
			}

			const float distSqr = Vector3::DistanceSquared(cameraPosition, light.Position);
			if (distSqr >= SQUARE(BLOCK(20)) || distSqr > SQUARE(light.Out + CAMERA_LIGHT_COLLECTION_RADIUS))
				return;

			const float distance = sqrt(distSqr);
			const float attenuation = 1.0f - distance / light.Out;
			const float intensity = attenuation * light.Intensity * light.Luma;

			if (intensity >= brightest)
			{
				brightest = intensity;
				_shadowLight = &light;
			}
		};

		for (auto& light : _dynamicLights[_dynamicLightList])
			considerLight(light);

		if (!cameraRoom.StaticLightCandidatesValid || _invalidateCache)
		{
			auto& mutableRoom = _rooms[Camera.pos.RoomNumber];
			mutableRoom.StaticLightCandidates.clear();
			for (int roomToCheck : mutableRoom.Neighbors)
			{
				if (roomToCheck < 0 || roomToCheck >= _rooms.size())
					continue;

				for (auto& light : _rooms[roomToCheck].Lights)
				{
					if (light.Type != LightType::FogBulb)
						mutableRoom.StaticLightCandidates.push_back({ &light, roomToCheck });
				}
			}
			mutableRoom.StaticLightCandidatesValid = true;
		}

		for (const auto& candidate : _rooms[Camera.pos.RoomNumber].StaticLightCandidates)
		{
			if (candidate.Light != nullptr)
				considerLight(*candidate.Light);
		}
	}	
	
	void Renderer::CollectLightsForEffect(short roomNumber, RendererEffect* effect)
	{
		CollectLights(effect->Position, ITEM_LIGHT_COLLECTION_RADIUS, roomNumber, NO_VALUE, false, false, nullptr, &effect->LightsToDraw);
	}

	void Renderer::CollectLightsForItem(RendererItem* item)
	{
		CollectLights(item->Position, ITEM_LIGHT_COLLECTION_RADIUS, item->RoomNumber, item->PrevRoomNumber, false, false, nullptr, &item->LightsToDraw);
	}

	void Renderer::CalculateLightFades(RendererItem *item)
	{
		ItemInfo* nativeItem = &g_Level.Items[item->ItemNumber];

		// Interpolate ambient light between rooms
		if (item->PrevRoomNumber == NO_VALUE)
		{
			item->PrevRoomNumber = nativeItem->RoomNumber;
			item->RoomNumber = nativeItem->RoomNumber;
			item->LightFade = 1.0f;
		}
		else if (nativeItem->RoomNumber != item->RoomNumber)
		{
			item->PrevRoomNumber = item->RoomNumber;
			item->RoomNumber = nativeItem->RoomNumber;
			item->LightFade = 0.0f;
		}
		else if (item->LightFade < 1.0f)
		{
			item->LightFade += AMBIENT_LIGHT_INTERPOLATION_STEP;
			item->LightFade = std::clamp(item->LightFade, 0.0f, 1.0f);
		}

		if (item->PrevRoomNumber == NO_VALUE || item->LightFade == 1.0f)
			item->AmbientLight = _rooms[nativeItem->RoomNumber].AmbientLight;
		else
		{
			auto prev = _rooms[item->PrevRoomNumber].AmbientLight;
			auto next = _rooms[item->RoomNumber].AmbientLight;

			item->AmbientLight.x = Lerp(prev.x, next.x, item->LightFade);
			item->AmbientLight.y = Lerp(prev.y, next.y, item->LightFade);
			item->AmbientLight.z = Lerp(prev.z, next.z, item->LightFade);
		}

		// Multiply calculated ambient light by object tint
		item->AmbientLight *= nativeItem->Model.Color;
	}

	void Renderer::CollectDecalsForRoom(short roomNumber, RenderView& renderView)
	{
		if (_rooms.size() <= roomNumber)
			return;

		RendererRoom& room = _rooms[roomNumber];

		room.Decals.clear();

		if (Decals.empty())
			return;

		for (auto& decal : Decals)
		{
			if (!renderView.Camera.Frustum.SphereInFrustum(decal.Sphere.Center, decal.Sphere.Radius))
				continue;

			bool decalInRoom = (decal.RoomNumber == room.RoomNumber);

			if (!decalInRoom)
			{
				for (auto j : decal.Neighbors)
				{
					if (j == roomNumber)
					{
						decalInRoom = true;
						break;
					}
				}
			}

			if (decalInRoom)
			{
				RendererDecal newDecal;

				newDecal.Position = decal.Sphere.Center;
				newDecal.Radius = decal.Sphere.Radius;
				newDecal.Opacity = decal.Opacity;
				newDecal.Pattern = (int)decal.Type;

				room.Decals.push_back(newDecal);
			}
		}
	}

	void Renderer::CollectLightsForRoom(short roomNumber, RenderView &renderView)
	{
		if (_rooms.size() <= roomNumber)
			return;

		RendererRoom& room = _rooms[roomNumber];
		room.DynamicLightCandidates.clear();
		room.DynamicLightCandidatesReady = true;
		
		// Build a conservative per-room dynamic candidate list once, then reuse it for all items, statics and effects in the room.
		for (auto& dynamicLight : _dynamicLights[_dynamicLightList])
		{
			if (dynamicLight.Out <= EPSILON)
				continue;

			auto candidateSphere = dynamicLight.BoundingSphere;
			candidateSphere.Radius += ITEM_LIGHT_COLLECTION_RADIUS;
			if (room.BoundingBox.Intersects(candidateSphere))
				room.DynamicLightCandidates.push_back(&dynamicLight);
		}

		for (auto* light : room.DynamicLightCandidates)
		{
			if (renderView.LightsToDraw.size() >= NUM_LIGHTS_PER_BUFFER)
				break;

			if (!room.BoundingBox.Intersects(light->BoundingSphere))
				continue;

			if (TEN::Utils::Contains(renderView.LightsToDraw, light))
				continue;

			renderView.LightsToDraw.push_back(light);
			room.LightsToDraw.push_back(light);
		}
	}

	void Renderer::CollectEffects(short roomNumber)
	{
		if (_rooms.size() <= roomNumber)
			return;

		RendererRoom& room = _rooms[roomNumber];
		RoomData* r = &g_Level.Rooms[room.RoomNumber];
		const float interpFactor = GetInterpolationFactor();

		short fxNum = NO_VALUE;
		for (fxNum = r->fxNumber; fxNum != NO_VALUE; fxNum = EffectList[fxNum].nextFx)
		{
			FX_INFO *fx = &EffectList[fxNum];
			if (fx->objectNumber < 0 || fx->color.w <= 0)
				continue;

			ObjectInfo *obj = &Objects[fx->objectNumber];

			RendererEffect *newEffect = &_effects[fxNum];

			newEffect->Translation = Matrix::CreateTranslation(fx->pos.Position.x, fx->pos.Position.y, fx->pos.Position.z);
			newEffect->Rotation = fx->pos.Orientation.ToRotationMatrix();
			newEffect->Scale = Matrix::CreateScale(1.0f);
			newEffect->World = newEffect->Rotation * newEffect->Translation;
			newEffect->ObjectID = fx->objectNumber;
			newEffect->RoomNumber = fx->roomNumber;
			newEffect->Position = fx->pos.Position.ToVector3();
			newEffect->AmbientLight = room.AmbientLight;
			newEffect->Color = fx->color;
			newEffect->Mesh = GetMesh(obj->nmeshes ? obj->meshIndex : fx->frameNumber);

			if (fx->DisableInterpolation)
			{
				// In this way the interpolation will return always the same result
				newEffect->PrevPosition = newEffect->Position;
				newEffect->PrevTranslation = newEffect->Translation;
				newEffect->PrevRotation = newEffect->Rotation;
				newEffect->PrevWorld = newEffect->World;
				newEffect->PrevScale = newEffect->Scale;

				fx->DisableInterpolation = false;
			}

			newEffect->InterpolatedPosition = Vector3::Lerp(newEffect->PrevPosition, newEffect->Position, interpFactor);
			newEffect->InterpolatedTranslation = Matrix::Lerp(newEffect->PrevTranslation, newEffect->Translation, interpFactor);
			newEffect->InterpolatedRotation = Matrix::Lerp(newEffect->InterpolatedRotation, newEffect->Rotation, interpFactor);
			newEffect->InterpolatedWorld = Matrix::Lerp(newEffect->PrevWorld, newEffect->World, interpFactor);
			newEffect->InterpolatedScale = Matrix::Lerp(newEffect->PrevScale, newEffect->Scale, interpFactor);

			CollectLightsForEffect(fx->roomNumber, newEffect);

			room.EffectsToDraw.push_back(newEffect);
		}
	}

	void Renderer::ResetItems()
	{
		for (auto& item : _items)
			item.DoneAnimations = false;
	}

	void Renderer::SaveOldState()
	{
		for (auto& item : _items)
		{
			item.PrevPosition = item.Position;
			item.PrevWorld = item.World;
			item.PrevTranslation = item.Translation;
			item.PrevRotation = item.Rotation;
			item.PrevScale = item.Scale;

			if (item.ObjectID < 0 || item.ObjectID >= (int)_moveableObjects.size())
				continue;

			const RendererObject* obj = nullptr;
			if (item.ObjectID == ID_LARA || item.ObjectID == ID_LARA_SKIN)
			{
				obj = &GetRendererObject((GAME_OBJECT_ID)item.ObjectID);
			}
			else if (_moveableObjects[item.ObjectID].has_value())
			{
				obj = &_moveableObjects[item.ObjectID].value();
			}

			if (obj == nullptr)
				continue;

			const size_t transformCount = std::min(
				(size_t)MAX_BONES,
				std::max(obj->AnimationTransforms.size(), obj->ObjectMeshes.size()));
			std::memcpy(item.PrevAnimTransforms, item.AnimTransforms, transformCount * sizeof(Matrix));
		}

		for (auto& effect : _effects)
		{
			effect.PrevPosition = effect.Position;
			effect.PrevWorld = effect.World;
			effect.PrevTranslation = effect.Translation;
			effect.PrevRotation = effect.Rotation;
			effect.PrevScale = effect.Scale;
		}

		for (auto& room : _rooms)
		{
			for (auto& stat : room.Statics)
				stat.PrevPose = stat.Pose;
		}
	}
}