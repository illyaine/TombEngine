#pragma once

#include <map>
#include <unordered_map>
#include <vector>

#include "Game/camera.h"
#include "Renderer/ConstantBuffers/CameraMatrixBuffer.h"
#include "Renderer/Frustum.h"
#include "Renderer/RendererEnums.h"
#include "Specific/memory/LinearArrayBuffer.h"
#include "Renderer/Structures/RendererSprite2D.h"
#include "Renderer/Structures/RendererSprite.h"
#include "Renderer/Structures/RendererFogBulb.h"
#include "Renderer/Structures/RendererStatic.h"
#include "Renderer/Structures/RendererItem.h"
#include "Renderer/Structures/RendererLight.h"
#include "Renderer/Structures/RendererEffect.h"
#include "Renderer/Structures/RendererRoom.h"
#include "Renderer/Structures/RendererSortableObject.h"
#include "Renderer/Structures/RendererSpriteToDraw.h"
#include "Renderer/Structures/RendererLensFlare.h"
#include "Renderer/Structures/RendererMirror.h"

namespace TEN::Renderer 
{
	using namespace TEN::Renderer::ConstantBuffers;
	using namespace TEN::Renderer::Structures;

	class ReusableStaticDrawGroups
	{
	private:
		using GroupMap = std::map<int, std::vector<RendererStatic*>>;
		using GroupLookup = std::unordered_map<int, GroupMap::iterator>;

		GroupMap _groups;
		GroupLookup _lookup;

		void RebuildLookup()
		{
			_lookup.clear();
			_lookup.reserve(_groups.size());

			for (auto it = _groups.begin(); it != _groups.end(); ++it)
				_lookup.emplace(it->first, it);
		}

		template <typename TIterator>
		class BasicIterator
		{
		private:
			TIterator _it;
			TIterator _end;

			void SkipEmpty()
			{
				while (_it != _end && _it->second.empty())
					++_it;
			}

		public:
			BasicIterator(TIterator it, TIterator end) : _it(it), _end(end)
			{
				SkipEmpty();
			}

			auto& operator*() const { return *_it; }
			auto* operator->() const { return &(*_it); }

			BasicIterator& operator++()
			{
				++_it;
				SkipEmpty();
				return *this;
			}

			bool operator==(const BasicIterator& other) const { return _it == other._it; }
			bool operator!=(const BasicIterator& other) const { return _it != other._it; }
		};

	public:
		using iterator = BasicIterator<GroupMap::iterator>;
		using const_iterator = BasicIterator<GroupMap::const_iterator>;

		ReusableStaticDrawGroups() = default;

		ReusableStaticDrawGroups(const ReusableStaticDrawGroups& other) :
			_groups(other._groups)
		{
			RebuildLookup();
		}

		ReusableStaticDrawGroups& operator=(const ReusableStaticDrawGroups& other)
		{
			if (this != &other)
			{
				_groups = other._groups;
				RebuildLookup();
			}

			return *this;
		}

		ReusableStaticDrawGroups(ReusableStaticDrawGroups&&) noexcept = default;
		ReusableStaticDrawGroups& operator=(ReusableStaticDrawGroups&&) noexcept = default;

		std::vector<RendererStatic*>& operator[](int objectNumber)
		{
			auto lookupIt = _lookup.find(objectNumber);
			if (lookupIt != _lookup.end())
				return lookupIt->second->second;

			// Preserve sorted iteration while avoiding a tree lookup for every visible static.
			auto groupIt = _groups.try_emplace(objectNumber).first;
			_lookup.emplace(objectNumber, groupIt);
			return groupIt->second;
		}

		void clear()
		{
			for (auto& [objectNumber, statics] : _groups)
				statics.clear();
		}

		size_t size() const
		{
			size_t count = 0;
			for (const auto& [objectNumber, statics] : _groups)
			{
				if (!statics.empty())
					count++;
			}

			return count;
		}

		iterator begin() { return iterator(_groups.begin(), _groups.end()); }
		iterator end() { return iterator(_groups.end(), _groups.end()); }
		const_iterator begin() const { return const_iterator(_groups.begin(), _groups.end()); }
		const_iterator end() const { return const_iterator(_groups.end(), _groups.end()); }
	};

	struct RenderViewCamera
	{
		Matrix ViewProjection;
		Matrix View;
		Matrix Projection;
		Vector3 WorldPosition;
		Vector3 WorldDirection;
		Vector2 ViewSize;
		Vector2 InvViewSize;
		int RoomNumber;
		Frustum Frustum;
		float NearPlane;
		float FarPlane;
		float FOV;

		RenderViewCamera(CAMERA_INFO* cam, float roll, float fov, float n, float f, int w, int h);
		RenderViewCamera(const Vector3& pos, const Vector3& dir, const Vector3& up, int room, int width, int height, float fov, float n, float f);
	};

	struct RenderView
	{
		RenderViewCamera Camera;
		D3D11_VIEWPORT	 Viewport;

		std::vector<RendererRoom*>					RoomsToDraw				 = {};
		std::vector<RendererLight*>					LightsToDraw			 = {};
		std::vector<RendererFogBulb>				FogBulbsToDraw			 = {};
		std::vector<RendererSpriteToDraw>			SpritesToDraw			 = {};
		std::vector<RendererDisplaySpriteToDraw>	DisplaySpritesToDraw	 = {};
		ReusableStaticDrawGroups					SortedStaticsToDraw		 = {};
		std::vector<RendererSortableObject>			TransparentObjectsToDraw = {};
		std::vector<RendererLensFlare>				LensFlaresToDraw		 = {};
		std::vector<RendererMirror>					Mirrors					 = {};

		RenderView(CAMERA_INFO* cam, float roll, float fov, float nearPlane, float farPlane, int w, int h);
		RenderView(const Vector3& pos, const Vector3& dir, const Vector3& up, int w, int h, int room, float nearPlane, float farPlane, float fov);

		void FillConstantBuffer(CCameraMatrixBuffer& bufferToFill);
		void Clear();
	};
}
