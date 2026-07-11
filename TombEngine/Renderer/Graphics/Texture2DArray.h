#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include "Renderer/Graphics/Texture2D.h"
#include "Renderer/Graphics/TextureBase.h"
#include "Renderer/Graphics/VRAMTracker.h"
#include <vector>

namespace TEN::Renderer::Graphics
{
	using namespace TEN::Renderer::Utils;

	using Microsoft::WRL::ComPtr;

	class Texture2DArray : public TextureBase
	{
	public:

		std::vector<ComPtr<ID3D11RenderTargetView>> RenderTargetView;
		ComPtr<ID3D11Texture2D> Texture;
		std::vector<ComPtr<ID3D11DepthStencilView>> DepthStencilView;
		ComPtr<ID3D11Texture2D> DepthStencilTexture;
		int Resolution;
		int Width;
		int Height;
		D3D11_VIEWPORT Viewport;

		Texture2DArray() : Resolution(0), Width(0), Height(0), Viewport({}) {};

		Texture2DArray(Texture2DArray&& other) noexcept
			: TextureBase(std::move(other)),
			  RenderTargetView(std::move(other.RenderTargetView)),
			  Texture(std::move(other.Texture)),
			  DepthStencilView(std::move(other.DepthStencilView)),
			  DepthStencilTexture(std::move(other.DepthStencilTexture)),
			  Resolution(other.Resolution), Width(other.Width), Height(other.Height), Viewport(other.Viewport),
			  _vramSize(other._vramSize), _vramCategory(other._vramCategory)
		{
			other._vramSize = 0;
		}

		Texture2DArray& operator=(Texture2DArray&& other) noexcept
		{
			if (this != &other)
			{
				if (_vramSize > 0)
					VRAMTracker::Get().Remove(_vramCategory, _vramSize);

				TextureBase::operator=(std::move(other));
				RenderTargetView = std::move(other.RenderTargetView);
				Texture = std::move(other.Texture);
				DepthStencilView = std::move(other.DepthStencilView);
				DepthStencilTexture = std::move(other.DepthStencilTexture);
				Resolution = other.Resolution;
				Width = other.Width;
				Height = other.Height;
				Viewport = other.Viewport;
				_vramSize = other._vramSize;
				_vramCategory = other._vramCategory;
				other._vramSize = 0;
			}
			return *this;
		}

		Texture2DArray(const Texture2DArray&) = delete;
		Texture2DArray& operator=(const Texture2DArray&) = delete;

		static bool AreCompatible(const std::vector<Texture2D*>& textures)
		{
			if (textures.empty() || textures[0] == nullptr || textures[0]->Texture == nullptr || textures[0]->ShaderResourceView == nullptr)
				return false;

			auto referenceDesc = D3D11_TEXTURE2D_DESC{};
			textures[0]->Texture->GetDesc(&referenceDesc);

			auto referenceViewDesc = D3D11_SHADER_RESOURCE_VIEW_DESC{};
			textures[0]->ShaderResourceView->GetDesc(&referenceViewDesc);

			if (referenceDesc.ArraySize != 1 || referenceDesc.SampleDesc.Count != 1)
				return false;

			for (const auto* texture : textures)
			{
				if (texture == nullptr || texture->Texture == nullptr || texture->ShaderResourceView == nullptr)
					return false;

				auto desc = D3D11_TEXTURE2D_DESC{};
				texture->Texture->GetDesc(&desc);

				auto viewDesc = D3D11_SHADER_RESOURCE_VIEW_DESC{};
				texture->ShaderResourceView->GetDesc(&viewDesc);

				if (desc.Width != referenceDesc.Width ||
					desc.Height != referenceDesc.Height ||
					desc.MipLevels != referenceDesc.MipLevels ||
					desc.ArraySize != 1 ||
					desc.Format != referenceDesc.Format ||
					desc.SampleDesc.Count != 1 ||
					viewDesc.Format != referenceViewDesc.Format)
				{
					return false;
				}
			}

			return true;
		}

		Texture2DArray(ID3D11Device* device, ID3D11DeviceContext* context, const std::vector<Texture2D*>& textures)
		{
			if (!AreCompatible(textures))
				throw std::invalid_argument("Texture2DArray source textures are incompatible.");

			auto sourceDesc = D3D11_TEXTURE2D_DESC{};
			textures[0]->Texture->GetDesc(&sourceDesc);

			auto sourceViewDesc = D3D11_SHADER_RESOURCE_VIEW_DESC{};
			textures[0]->ShaderResourceView->GetDesc(&sourceViewDesc);

			Width = sourceDesc.Width;
			Height = sourceDesc.Height;
			Resolution = (Width == Height) ? Width : 0;

			if (textures.size() == 1)
			{
				Texture = textures[0]->Texture;
			}
			else
			{
				auto desc = sourceDesc;
				desc.ArraySize = static_cast<UINT>(textures.size());
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				desc.CPUAccessFlags = 0;
				desc.MiscFlags = 0;

				throwIfFailed(device->CreateTexture2D(&desc, nullptr, Texture.GetAddressOf()), device, "CreateTexture2D (source texture array)");

				for (UINT slice = 0; slice < textures.size(); slice++)
				{
					for (UINT mip = 0; mip < sourceDesc.MipLevels; mip++)
					{
						context->CopySubresourceRegion(
							Texture.Get(),
							D3D11CalcSubresource(mip, slice, sourceDesc.MipLevels),
							0, 0, 0,
							textures[slice]->Texture.Get(),
							D3D11CalcSubresource(mip, 0, sourceDesc.MipLevels),
							nullptr);
					}
				}

				_vramSize = VRAMTracker::ComputeTexture2DSize(desc);
				_vramCategory = VRAMCategory::Texture;
				VRAMTracker::Get().Add(_vramCategory, _vramSize);
			}

			auto shaderDesc = D3D11_SHADER_RESOURCE_VIEW_DESC{};
			shaderDesc.Format = sourceViewDesc.Format;
			shaderDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			shaderDesc.Texture2DArray.MostDetailedMip = 0;
			shaderDesc.Texture2DArray.MipLevels = sourceDesc.MipLevels;
			shaderDesc.Texture2DArray.ArraySize = static_cast<UINT>(textures.size());
			shaderDesc.Texture2DArray.FirstArraySlice = 0;
			throwIfFailed(device->CreateShaderResourceView(Texture.Get(), &shaderDesc, ShaderResourceView.GetAddressOf()), device, "CreateSRV (source texture array)");
		}

		Texture2DArray(ID3D11Device* device, int resolution, int count, DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat)
			: Resolution(resolution), Width(resolution), Height(resolution)
		{
			DepthStencilView.resize(count);
			RenderTargetView.resize(count);
			D3D11_TEXTURE2D_DESC desc = {};
			desc.Width = resolution;
			desc.Height = resolution;
			desc.MipLevels = 1;
			desc.ArraySize = count;
			desc.Format = colorFormat;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0x0;

			HRESULT res = device->CreateTexture2D(&desc, NULL, Texture.GetAddressOf());
			throwIfFailed(res, device, "CreateTexture2D (texture array color)");

			_vramSize = VRAMTracker::ComputeTexture2DSize(desc);

			D3D11_RENDER_TARGET_VIEW_DESC viewDesc = {};
			viewDesc.Format = desc.Format;
			viewDesc.Texture2DArray.ArraySize = 1;
			viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;

			for (int i = 0; i < count; i++)
			{
				viewDesc.Texture2DArray.FirstArraySlice = D3D11CalcSubresource(0, i, 1);
				res = device->CreateRenderTargetView(Texture.Get(), &viewDesc, RenderTargetView[i].GetAddressOf());
				throwIfFailed(res, device, "CreateRenderTargetView (texture array slice " + std::to_string(i) + ")");
			}

			// Setup the description of the shader resource view.
			D3D11_SHADER_RESOURCE_VIEW_DESC shaderDesc = {};
			shaderDesc.Format = desc.Format;
			shaderDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			shaderDesc.Texture2DArray.MostDetailedMip = 0;
			shaderDesc.Texture2DArray.MipLevels = 1;
			shaderDesc.Texture2DArray.ArraySize = count;
			shaderDesc.Texture2DArray.FirstArraySlice = 0;
			res = device->CreateShaderResourceView(Texture.Get(), &shaderDesc, ShaderResourceView.GetAddressOf());
			throwIfFailed(res, device, "CreateSRV (texture array)");

			D3D11_TEXTURE2D_DESC depthTexDesc = {};
			depthTexDesc.Width = resolution;
			depthTexDesc.Height = resolution;
			depthTexDesc.MipLevels = 1;
			depthTexDesc.ArraySize = count;
			depthTexDesc.SampleDesc.Count = 1;
			depthTexDesc.SampleDesc.Quality = 0;
			depthTexDesc.Format = depthFormat;
			depthTexDesc.Usage = D3D11_USAGE_DEFAULT;
			depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			depthTexDesc.CPUAccessFlags = 0;
			depthTexDesc.MiscFlags = 0x0;

			res = device->CreateTexture2D(&depthTexDesc, NULL, DepthStencilTexture.GetAddressOf());
			throwIfFailed(res, device, "CreateTexture2D (texture array depth)");

			_vramSize += VRAMTracker::ComputeTexture2DSize(depthTexDesc);

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = depthTexDesc.Format;
			dsvDesc.Flags = 0;
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			dsvDesc.Texture2DArray.ArraySize = 1;

			for (int i = 0; i < count; i++)
			{
				dsvDesc.Texture2DArray.FirstArraySlice = D3D11CalcSubresource(0, i, 1);
				res = device->CreateDepthStencilView(DepthStencilTexture.Get(), &dsvDesc, DepthStencilView[i].GetAddressOf());
				throwIfFailed(res, device, "CreateDepthStencilView (texture array slice " + std::to_string(i) + ")");
			}

			_vramCategory = VRAMCategory::RenderTarget;
			VRAMTracker::Get().Add(_vramCategory, _vramSize);
		}

		~Texture2DArray()
		{
			if (_vramSize > 0)
				VRAMTracker::Get().Remove(_vramCategory, _vramSize);
		}

	private:
		int _vramSize = 0;
		VRAMCategory _vramCategory = VRAMCategory::RenderTarget;
	};
}
