#pragma once

#include <array>
#include <cstddef>
#include <cstring>
#include <d3d11.h>
#include <wrl/client.h>

#include "Game/Debug/Debug.h"
#include "Renderer/RendererUtils.h"

namespace TEN::Renderer::ConstantBuffers
{
	template <typename CBuff>
	class ConstantBuffer
	{
		static constexpr unsigned int CHANGE_STREAK_BEFORE_COOLDOWN = 8;
		static constexpr unsigned int COMPARISON_COOLDOWN_UPDATES = 16;

		ComPtr<ID3D11Buffer> buffer;
		std::array<std::byte, sizeof(CBuff)> _lastData = {};
		unsigned int _consecutiveChangedUpdates = 0;
		unsigned int _comparisonCooldown = 0;
		bool _lastDataValid = false;

	public:
		ConstantBuffer() = default;
		ConstantBuffer(ConstantBuffer&& other) noexcept : buffer(std::move(other.buffer)) { }

		ConstantBuffer& operator=(ConstantBuffer&& other) noexcept
		{
			if (this != &other)
			{
				buffer = std::move(other.buffer);
				_consecutiveChangedUpdates = 0;
				_comparisonCooldown = 0;
				_lastDataValid = false;
			}

			return *this;
		}

		ConstantBuffer(const ConstantBuffer&) = delete;
		ConstantBuffer& operator=(const ConstantBuffer&) = delete;

		ConstantBuffer(ID3D11Device* device)
		{
			auto desc = D3D11_BUFFER_DESC{};
			desc.ByteWidth = sizeof(CBuff);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			Utils::throwIfFailed(device->CreateBuffer(&desc, nullptr, buffer.GetAddressOf()));
			buffer->SetPrivateData(WKPDID_D3DDebugObjectName, 32, typeid(CBuff).name());
		}

		ID3D11Buffer** get()
		{
			return buffer.GetAddressOf();
		}

		void UpdateData(CBuff& data, ID3D11DeviceContext* ctx)
		{
			const auto* dataBytes = reinterpret_cast<const std::byte*>(&data);
			bool cacheCurrentData = true;

			if (_comparisonCooldown > 0)
			{
				_comparisonCooldown--;
				cacheCurrentData = (_comparisonCooldown == 0);
			}
			else if (_lastDataValid)
			{
				if (std::memcmp(_lastData.data(), dataBytes, sizeof(CBuff)) == 0)
				{
					_consecutiveChangedUpdates = 0;
					return;
				}

				_consecutiveChangedUpdates++;
				if (_consecutiveChangedUpdates >= CHANGE_STREAK_BEFORE_COOLDOWN)
				{
					_consecutiveChangedUpdates = 0;
					_comparisonCooldown = COMPARISON_COOLDOWN_UPDATES;
					_lastDataValid = false;
					cacheCurrentData = false;
				}
			}

			auto mappedResource = D3D11_MAPPED_SUBRESOURCE{};
			auto res = ctx->Map(buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			if (SUCCEEDED(res))
			{
				void* dataPtr = mappedResource.pData;
				std::memcpy(dataPtr, &data, sizeof(CBuff));
				ctx->Unmap(buffer.Get(), 0);

				if (cacheCurrentData)
				{
					std::memcpy(_lastData.data(), dataBytes, sizeof(CBuff));
					_lastDataValid = true;
				}
			}
			else
			{
				TENLog("Could not update constant buffer.", LogLevel::Error);
			}
		}
	};
}