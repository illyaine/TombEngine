#pragma once

#include "Math/Objects/GameBoundingBox.h"
#include "Math/Objects/Pose.h"
#include "Renderer/Structures/RendererLight.h"

namespace TEN::Renderer::Structures
{
	class RendererStaticLightCache
	{
	private:
		std::array<RendererLightNode, MAX_LIGHTS_PER_ITEM> _nodes = {};
		size_t _size = 0;

	public:
		void clear()
		{
			_size = 0;
		}

		size_t capacity() const
		{
			return MAX_LIGHTS_PER_ITEM;
		}

		void reserve(size_t)
		{
			// Fixed-size cache; no allocation is required.
		}

		void push_back(const RendererLightNode& node)
		{
			if (_size < MAX_LIGHTS_PER_ITEM)
			{
				_nodes[_size++] = node;
				return;
			}

			size_t weakestIndex = 0;
			for (size_t i = 1; i < _size; i++)
			{
				if (_nodes[i].LocalIntensity < _nodes[weakestIndex].LocalIntensity)
					weakestIndex = i;
			}

			if (node.LocalIntensity > _nodes[weakestIndex].LocalIntensity)
				_nodes[weakestIndex] = node;
		}

		auto begin() { return _nodes.begin(); }
		auto end() { return _nodes.begin() + _size; }
		auto begin() const { return _nodes.begin(); }
		auto end() const { return _nodes.begin() + _size; }
	};

	struct RendererStatic
	{
		int ObjectNumber;
		int RoomNumber;
		int IndexInRoom;

		Pose 	PrevPose;
		Pose	Pose;
		Matrix	World;
		Vector4 Color;
		Vector4 AmbientLight;

		std::vector<RendererLight*> LightsToDraw;
		RendererStaticLightCache CachedRoomLights;
		bool CacheLights;

		BoundingSphere OriginalSphere;
		BoundingSphere Sphere;

		void Update(float interpolationFactor)
		{
			auto pos = Vector3::Lerp(PrevPose.Position.ToVector3(), Pose.Position.ToVector3(), interpolationFactor);
			auto scale = Vector3::Lerp(PrevPose.Scale, Pose.Scale, interpolationFactor);
			
			auto translationMatrix = Matrix::CreateTranslation(pos);
			auto scaleMatrix = Matrix::CreateScale(scale);
			auto rotMatrix = Matrix::Lerp(PrevPose.Orientation.ToRotationMatrix(), Pose.Orientation.ToRotationMatrix(), interpolationFactor);
			
			auto worldMatrix = rotMatrix * scaleMatrix * translationMatrix;

			auto sphereCenter = Vector3::Transform(OriginalSphere.Center, worldMatrix);
			float sphereScale = std::max({ Pose.Scale.x, Pose.Scale.y, Pose.Scale.z });
			float sphereRadius = OriginalSphere.Radius * sphereScale;

			World = worldMatrix;
			Sphere = BoundingSphere(sphereCenter, sphereRadius);
			
			CacheLights = true;
		}
	};
}