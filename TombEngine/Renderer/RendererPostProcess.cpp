#include "framework.h"
#include "Renderer/Renderer.h"
#include "Game/LightingSettingsInput.h"
#include "Game/LightingSettingsRender.h"
#include "Game/spotcam.h"
#include "Specific/configuration.h"

namespace TEN::Renderer
{
	static const GameConfiguration& GetPostProcessConfiguration()
	{
		if (TEN::Gui::g_Gui.GetMenuToDisplay() == TEN::Gui::Menu::LightingHDR)
			return TEN::Gui::g_Gui.GetCurrentSettings().Configuration;

		return g_Configuration;
	}

	static void SetUserPostProcessSettings(CPostProcessBuffer& buffer)
	{
		const auto& configuration = GetPostProcessConfiguration();

		buffer.HDRExposure = std::clamp(configuration.HDRExposure / 100.0f, 0.25f, 4.0f);
		buffer.HDRStrength = std::clamp(configuration.HDRStrength / 100.0f, 0.0f, 1.0f);
		buffer.BloomThreshold = std::clamp(configuration.BloomThreshold / 100.0f, 0.25f, 3.0f);
		buffer.BloomStrength = std::clamp(configuration.BloomStrength / 100.0f, 0.0f, 3.0f);
		buffer.GlareStrength = std::clamp(configuration.GlareStrength / 100.0f, 0.0f, 3.0f);
		buffer.GlareLength = std::clamp(configuration.GlareLength / 100.0f, 0.25f, 3.0f) * 8.0f;
		buffer.EnableHDR = configuration.EnableHDRRendering ? 1 : 0;
		buffer.EnableBloom = configuration.EnableLightBloom ? 1 : 0;
	}

	static bool IsHDRRenderTarget(const RenderTarget2D& renderTarget)
	{
		D3D11_TEXTURE2D_DESC description = {};
		renderTarget.Texture->GetDesc(&description);
		return description.Format == DXGI_FORMAT_R16G16B16A16_FLOAT ||
			description.Format == DXGI_FORMAT_R16G16B16A16_TYPELESS;
	}

	void Renderer::DrawPostprocess(RenderTarget2D* renderTarget, RenderView& view, SceneRenderMode renderMode)
	{
		static bool lightingRestartRequired = false;
		const bool hdrRenderTargetsEnabled = IsHDRRenderTarget(_renderTarget);
		const bool lightingMenuActive = TEN::Gui::UpdateLightingSettingsInput(lightingRestartRequired, hdrRenderTargetsEnabled);

		_doingFullscreenPass = true;
		SetBlendMode(BlendMode::Opaque);
		SetCullMode(CullMode::CounterClockwise);
		SetDepthState(DepthState::Write);
		_context->RSSetViewports(1, &view.Viewport);
		ResetScissor();

		const float finalScreenFadeFactor = renderMode == SceneRenderMode::Full ? ScreenFadeCurrent : 1.0f;
		const float finalCinematicBarsHeight = renderMode == SceneRenderMode::Full ? CinematicBarsHeight : 0.0f;
		const Vector3 finalTint = _postProcessTint;

		_stPostProcessBuffer.ScreenFadeFactor = finalScreenFadeFactor;
		_stPostProcessBuffer.CinematicBarsHeight = finalCinematicBarsHeight;
		_stPostProcessBuffer.ViewportSize = Vector2i(_screenWidth, _screenHeight);
		_stPostProcessBuffer.EffectStrength = _postProcessStrength;
		_stPostProcessBuffer.Tint = finalTint;
		SetUserPostProcessSettings(_stPostProcessBuffer);

		_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_context->IASetInputLayout(_fullscreenTriangleInputLayout.Get());
		unsigned int stride = sizeof(PostProcessVertex);
		unsigned int offset = 0;
		_context->IASetVertexBuffers(0, 1, _fullscreenTriangleVertexBuffer.Buffer.GetAddressOf(), &stride, &offset);

		float clearColor[4] = { 0, 0, 0, 0 };
		_context->ClearRenderTargetView(_postProcessRenderTarget[0].RenderTargetView.Get(), clearColor);
		_context->OMSetRenderTargets(1, _postProcessRenderTarget[0].RenderTargetView.GetAddressOf(), nullptr);
		BindRenderTargetAsTexture(TextureRegister::ColorMap, &_renderTarget, SamplerStateRegister::PointWrap);

		if (hdrRenderTargetsEnabled)
		{
			// Convert the FP16 scene to SDR before entering the SDR postprocess chain.
			// Fade, tint and cinematic bars remain deferred to the final output pass.
			_stPostProcessBuffer.ScreenFadeFactor = 1.0f;
			_stPostProcessBuffer.CinematicBarsHeight = 0.0f;
			_stPostProcessBuffer.Tint = Vector3::One;
			UpdateConstantBuffer(_stPostProcessBuffer, _cbPostProcessBuffer);
			_shaders.Bind(Shader::PostProcessFinalPass);
		}
		else
		{
			UpdateConstantBuffer(_stPostProcessBuffer, _cbPostProcessBuffer);
			_shaders.Bind(Shader::PostProcess);
		}

		DrawTriangles(3, 0);

		// Restore final output controls. An FP16 scene has already been tone mapped
		// above, so the last pass must not apply HDR conversion a second time.
		_stPostProcessBuffer.ScreenFadeFactor = finalScreenFadeFactor;
		_stPostProcessBuffer.CinematicBarsHeight = finalCinematicBarsHeight;
		_stPostProcessBuffer.Tint = finalTint;
		if (hdrRenderTargetsEnabled)
			_stPostProcessBuffer.EnableHDR = 0;
		UpdateConstantBuffer(_stPostProcessBuffer, _cbPostProcessBuffer);

		int currentRenderTarget = 0;
		int destRenderTarget = 1;

		if (!view.LensFlaresToDraw.empty())
		{
			_context->ClearRenderTargetView(_postProcessRenderTarget[destRenderTarget].RenderTargetView.Get(), clearColor);
			_context->OMSetRenderTargets(1, _postProcessRenderTarget[destRenderTarget].RenderTargetView.GetAddressOf(), nullptr);
			_shaders.Bind(Shader::PostProcessLensFlare);
			for (int i = 0; i < view.LensFlaresToDraw.size(); i++)
			{
				_stPostProcessBuffer.LensFlares[i].Position = view.LensFlaresToDraw[i].Position;
				_stPostProcessBuffer.LensFlares[i].Color = view.LensFlaresToDraw[i].Color.ToVector3();
			}
			_stPostProcessBuffer.NumLensFlares = (int)view.LensFlaresToDraw.size();
			UpdateConstantBuffer(_stPostProcessBuffer, _cbPostProcessBuffer);
			BindRenderTargetAsTexture(TextureRegister::ColorMap, &_postProcessRenderTarget[currentRenderTarget], SamplerStateRegister::PointWrap);
			DrawTriangles(3, 0);
			destRenderTarget = destRenderTarget == 1 ? 0 : 1;
			currentRenderTarget = currentRenderTarget == 1 ? 0 : 1;
		}

		if (_postProcessMode != PostProcessMode::None && _postProcessStrength > EPSILON)
		{
			_context->ClearRenderTargetView(_postProcessRenderTarget[destRenderTarget].RenderTargetView.Get(), clearColor);
			_context->OMSetRenderTargets(1, _postProcessRenderTarget[destRenderTarget].RenderTargetView.GetAddressOf(), nullptr);
			switch (_postProcessMode)
			{
			case PostProcessMode::Monochrome:
				_shaders.Bind(Shader::PostProcessMonochrome);
				break;

			case PostProcessMode::Negative:
				_shaders.Bind(Shader::PostProcessNegative);
				break;

			case PostProcessMode::Exclusion:
				_shaders.Bind(Shader::PostProcessExclusion);
				break;

			default:
				_shaders.Bind(Shader::PostProcess);
				break;
			}
			BindRenderTargetAsTexture(TextureRegister::ColorMap, &_postProcessRenderTarget[currentRenderTarget], SamplerStateRegister::PointWrap);
			DrawTriangles(3, 0);
			destRenderTarget = destRenderTarget == 1 ? 0 : 1;
			currentRenderTarget = currentRenderTarget == 1 ? 0 : 1;
		}

		_shaders.Bind(Shader::PostProcessFinalPass);
		_context->ClearRenderTargetView(renderTarget->RenderTargetView.Get(), Colors::Black);
		_context->ClearDepthStencilView(renderTarget->DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		_context->OMSetRenderTargets(1, renderTarget->RenderTargetView.GetAddressOf(), renderTarget->DepthStencilView.Get());
		BindTexture(TextureRegister::ColorMap, &_postProcessRenderTarget[currentRenderTarget], SamplerStateRegister::PointWrap);
		DrawTriangles(3, 0);

		_doingFullscreenPass = false;

		if (lightingMenuActive)
		{
			// Draw settings after tone mapping so UI brightness and text remain stable.
			_stringsToDraw.clear();
			TEN::Gui::RenderLightingSettings(*this, lightingRestartRequired);
			_context->OMSetRenderTargets(1, renderTarget->RenderTargetView.GetAddressOf(), renderTarget->DepthStencilView.Get());
			_context->RSSetViewports(1, &view.Viewport);
			ResetScissor();
			DrawAllStrings();
			_stringsToDraw.clear();
		}
	}

	PostProcessMode Renderer::GetPostProcessMode() { return _postProcessMode; }
	float Renderer::GetPostProcessStrength() { return _postProcessStrength; }
	Vector3 Renderer::GetPostProcessTint() { return _postProcessTint; }
	void Renderer::SetPostProcessMode(PostProcessMode mode) { _postProcessMode = mode; }
	void Renderer::SetPostProcessStrength(float strength) { _postProcessStrength = strength; }
	void Renderer::SetPostProcessTint(Vector3 tint) { _postProcessTint = tint; }

	void Renderer::CopyRenderTarget(RenderTarget2D* source, RenderTarget2D* dest, RenderView& view)
	{
		SetBlendMode(BlendMode::Opaque, true);
		SetCullMode(CullMode::CounterClockwise, true);
		SetDepthState(DepthState::Write, true);
		_context->RSSetViewports(1, &view.Viewport);
		ResetScissor();
		_shaders.Bind(Shader::PostProcess);
		_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_context->IASetInputLayout(_fullscreenTriangleInputLayout.Get());
		unsigned int stride = sizeof(PostProcessVertex);
		unsigned int offset = 0;
		_context->IASetVertexBuffers(0, 1, _fullscreenTriangleVertexBuffer.Buffer.GetAddressOf(), &stride, &offset);
		float clearColor[4] = { 0, 0, 0, 0 };
		_context->ClearRenderTargetView(dest->RenderTargetView.Get(), clearColor);
		_context->OMSetRenderTargets(1, dest->RenderTargetView.GetAddressOf(), nullptr);
		BindRenderTargetAsTexture(TextureRegister::ColorMap, source, SamplerStateRegister::PointWrap);
		DrawTriangles(3, 0);
	}

	void Renderer::CopyRenderTargetAndDownscale(RenderTarget2D* source, RenderTarget2D* dest, float factor, RenderView& view)
	{
		D3D11_VIEWPORT viewport = { 0, 0, _screenWidth / factor, _screenHeight / factor, 0, 1 };
		_context->RSSetViewports(1, &viewport);
		D3D11_RECT rect = { 0, 0, (LONG)viewport.Width, (LONG)viewport.Height };
		_context->RSSetScissorRects(1, &rect);
		SetBlendMode(BlendMode::Opaque, true);
		SetCullMode(CullMode::CounterClockwise, true);
		SetDepthState(DepthState::Write, true);
		_shaders.Bind(Shader::PostProcess);
		_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_context->IASetInputLayout(_fullscreenTriangleInputLayout.Get());
		unsigned int stride = sizeof(PostProcessVertex);
		unsigned int offset = 0;
		_context->IASetVertexBuffers(0, 1, _fullscreenTriangleVertexBuffer.Buffer.GetAddressOf(), &stride, &offset);
		float clearColor[4] = { 0, 0, 0, 0 };
		_context->ClearRenderTargetView(dest->RenderTargetView.Get(), clearColor);
		_context->OMSetRenderTargets(1, dest->RenderTargetView.GetAddressOf(), nullptr);
		BindRenderTargetAsTexture(TextureRegister::ColorMap, source, SamplerStateRegister::PointWrap);
		DrawTriangles(3, 0);
		ResetScissor();
		_context->RSSetViewports(1, &view.Viewport);
	}

	void Renderer::ApplyGlow(RenderTarget2D* renderTarget, RenderView& view)
	{
		const auto& configuration = GetPostProcessConfiguration();
		if (!configuration.EnableLightBloom ||
			(configuration.BloomStrength <= 0 && configuration.GlareStrength <= 0))
		{
			return;
		}

		SetBlendMode(BlendMode::Opaque, true);
		SetCullMode(CullMode::CounterClockwise, true);
		SetDepthState(DepthState::Write, true);
		D3D11_VIEWPORT viewport = { 0, 0, _screenWidth / GLOW_DOWNSCALE_FACTOR, _screenHeight / GLOW_DOWNSCALE_FACTOR, 0, 1 };
		_context->RSSetViewports(1, &viewport);
		D3D11_RECT rect = { 0, 0, (LONG)viewport.Width, (LONG)viewport.Height };
		_context->RSSetScissorRects(1, &rect);
		_shaders.Bind(Shader::PostProcess);
		_stPostProcessBuffer.ViewportSize = Vector2i(_screenWidth, _screenHeight);
		SetUserPostProcessSettings(_stPostProcessBuffer);
		_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_context->IASetInputLayout(_fullscreenTriangleInputLayout.Get());
		unsigned int stride = sizeof(PostProcessVertex);
		unsigned int offset = 0;
		_context->IASetVertexBuffers(0, 1, _fullscreenTriangleVertexBuffer.Buffer.GetAddressOf(), &stride, &offset);

		_shaders.Bind(Shader::Downscale);
		_stPostProcessBuffer.DownscaleFactor = GLOW_DOWNSCALE_FACTOR;
		UpdateConstantBuffer(_stPostProcessBuffer, _cbPostProcessBuffer);
		float clearColor[4] = { 0, 0, 0, 0 };
		_context->ClearRenderTargetView(_glowRenderTarget[0].RenderTargetView.Get(), clearColor);
		_context->OMSetRenderTargets(1, _glowRenderTarget[0].RenderTargetView.GetAddressOf(), nullptr);
		BindRenderTargetAsTexture(TextureRegister::ColorMap, renderTarget, SamplerStateRegister::LinearClamp);
		BindRenderTargetAsTexture(static_cast<TextureRegister>(5), &_emissiveAndRoughnessRenderTarget, SamplerStateRegister::LinearClamp);
		DrawTriangles(3, 0);

		_shaders.Bind(Shader::Blur);
		const float bloomRadiusScale = std::clamp(configuration.BloomRadius / 100.0f, 0.25f, 3.0f);
		_stPostProcessBuffer.TexelSize = Vector2(1.0f / viewport.Width, 1.0f / viewport.Height);
		_stPostProcessBuffer.BlurSigma = GLOW_BLUR_SIGMA * bloomRadiusScale;
		_stPostProcessBuffer.BlurRadius = std::clamp((int)(GLOW_BLUR_RADIUS * bloomRadiusScale + 0.5f), 1, 100);
		_context->ClearRenderTargetView(_glowRenderTarget[1].RenderTargetView.Get(), clearColor);
		_context->OMSetRenderTargets(1, _glowRenderTarget[1].RenderTargetView.GetAddressOf(), nullptr);
		_stPostProcessBuffer.BlurDirection = Vector2(1, 0);
		UpdateConstantBuffer(_stPostProcessBuffer, _cbPostProcessBuffer);
		BindRenderTargetAsTexture(TextureRegister::ColorMap, &_glowRenderTarget[0], SamplerStateRegister::LinearClamp);
		DrawTriangles(3, 0);
		_context->ClearRenderTargetView(_glowRenderTarget[0].RenderTargetView.Get(), clearColor);
		_context->OMSetRenderTargets(1, _glowRenderTarget[0].RenderTargetView.GetAddressOf(), nullptr);
		_stPostProcessBuffer.BlurDirection = Vector2(0, 1);
		UpdateConstantBuffer(_stPostProcessBuffer, _cbPostProcessBuffer);
		BindRenderTargetAsTexture(TextureRegister::ColorMap, &_glowRenderTarget[1], SamplerStateRegister::LinearClamp);
		DrawTriangles(3, 0);

		_context->RSSetViewports(1, &view.Viewport);
		ResetScissor();

		// Keep the scene copy in FP16 until tone mapping. The SMAA scene target is
		// full-sized and is overwritten again by SMAA later in the frame.
		RenderTarget2D* sceneCopyTarget = IsHDRRenderTarget(*renderTarget) ?
			&_SMAASceneRenderTarget : &_postProcessRenderTarget[0];
		CopyRenderTarget(renderTarget, sceneCopyTarget, view);

		_shaders.Bind(Shader::GlowCombine);
		_stPostProcessBuffer.GlowSoftAdd = configuration.EnableHDRRendering ? 0 : 1;
		_stPostProcessBuffer.GlowIntensity = 1.0f;
		SetUserPostProcessSettings(_stPostProcessBuffer);
		UpdateConstantBuffer(_stPostProcessBuffer, _cbPostProcessBuffer);
		_context->ClearRenderTargetView(renderTarget->RenderTargetView.Get(), clearColor);
		_context->OMSetRenderTargets(1, renderTarget->RenderTargetView.GetAddressOf(), nullptr);
		BindRenderTargetAsTexture(static_cast<TextureRegister>(0), sceneCopyTarget, SamplerStateRegister::LinearClamp);
		BindRenderTargetAsTexture(static_cast<TextureRegister>(3), &_glowRenderTarget[0], SamplerStateRegister::LinearClamp);
		DrawTriangles(3, 0);
	}
}
