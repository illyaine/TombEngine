#include "framework.h"
#include "Renderer/Renderer.h"
#include "Game/spotcam.h"
#include "Specific/configuration.h"

namespace TEN::Renderer
{
	static void SetUserPostProcessSettings(CPostProcessBuffer& buffer)
	{
		buffer.HDRExposure = std::clamp(g_Configuration.HDRExposure / 100.0f, 0.25f, 4.0f);
		buffer.HDRStrength = std::clamp(g_Configuration.HDRStrength / 100.0f, 0.0f, 1.0f);
		buffer.BloomThreshold = std::clamp(g_Configuration.BloomThreshold / 100.0f, 0.25f, 3.0f);
		buffer.BloomStrength = std::clamp(g_Configuration.BloomStrength / 100.0f, 0.0f, 3.0f);
		buffer.GlareStrength = std::clamp(g_Configuration.GlareStrength / 100.0f, 0.0f, 3.0f);
		buffer.GlareLength = std::clamp(g_Configuration.GlareLength / 100.0f, 0.25f, 3.0f) * 8.0f;
		buffer.EnableHDR = g_Configuration.EnableHDRRendering ? 1 : 0;
		buffer.EnableBloom = g_Configuration.EnableLightBloom ? 1 : 0;
	}

	void Renderer::DrawPostprocess(RenderTarget2D* renderTarget, RenderView& view, SceneRenderMode renderMode)
	{
		_doingFullscreenPass = true;
		SetBlendMode(BlendMode::Opaque);
		SetCullMode(CullMode::CounterClockwise);
		SetDepthState(DepthState::Write);
		_context->RSSetViewports(1, &view.Viewport);
		ResetScissor();

		_stPostProcessBuffer.ScreenFadeFactor = renderMode == SceneRenderMode::Full ? ScreenFadeCurrent : 1.0f;
		_stPostProcessBuffer.CinematicBarsHeight = renderMode == SceneRenderMode::Full ? CinematicBarsHeight : 0.0f;
		_stPostProcessBuffer.ViewportSize = Vector2i(_screenWidth, _screenHeight);
		_stPostProcessBuffer.EffectStrength = _postProcessStrength;
		_stPostProcessBuffer.Tint = _postProcessTint;
		SetUserPostProcessSettings(_stPostProcessBuffer);
		UpdateConstantBuffer(_stPostProcessBuffer, _cbPostProcessBuffer);

		_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_context->IASetInputLayout(_fullscreenTriangleInputLayout.Get());
		unsigned int stride = sizeof(PostProcessVertex);
		unsigned int offset = 0;
		_context->IASetVertexBuffers(0, 1, _fullscreenTriangleVertexBuffer.Buffer.GetAddressOf(), &stride, &offset);
		_shaders.Bind(Shader::PostProcess);

		float clearColor[4] = { 0, 0, 0, 0 };
		_context->ClearRenderTargetView(_postProcessRenderTarget[0].RenderTargetView.Get(), clearColor);
		_context->OMSetRenderTargets(1, _postProcessRenderTarget[0].RenderTargetView.GetAddressOf(), nullptr);
		BindRenderTargetAsTexture(TextureRegister::ColorMap, &_renderTarget, SamplerStateRegister::PointWrap);
		DrawTriangles(3, 0);

		int currentRenderTarget = 0;
		int destRenderTarget = 1;
		_shaders.Bind(Shader::PostProcess);

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
			case PostProcessMode::Monochrome: _shaders.Bind(Shader::PostProcessMonochrome); break;
			case PostProcessMode::Negative: _shaders.Bind(Shader::PostProcessNegative); break;
			case PostProcessMode::Exclusion: _shaders.Bind(Shader::PostProcessExclusion); break;
			default: return;
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
		if (!g_Configuration.EnableLightBloom)
			return;

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
		BindRenderTargetAsTexture(TextureRegister::EmissiveMap, &_emissiveAndRoughnessRenderTarget, SamplerStateRegister::LinearClamp);
		DrawTriangles(3, 0);

		_shaders.Bind(Shader::Blur);
		_stPostProcessBuffer.TexelSize = Vector2(1.0f / viewport.Width, 1.0f / viewport.Height);
		_stPostProcessBuffer.BlurSigma = GLOW_BLUR_SIGMA;
		_stPostProcessBuffer.BlurRadius = GLOW_BLUR_RADIUS;
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
		CopyRenderTarget(renderTarget, &_postProcessRenderTarget[0], view);
		_shaders.Bind(Shader::GlowCombine);
		_stPostProcessBuffer.GlowSoftAdd = g_Configuration.EnableHDRRendering ? 0 : 1;
		_stPostProcessBuffer.GlowIntensity = 1.0f;
		SetUserPostProcessSettings(_stPostProcessBuffer);
		UpdateConstantBuffer(_stPostProcessBuffer, _cbPostProcessBuffer);
		_context->ClearRenderTargetView(renderTarget->RenderTargetView.Get(), clearColor);
		_context->OMSetRenderTargets(1, renderTarget->RenderTargetView.GetAddressOf(), nullptr);
		BindRenderTargetAsTexture(static_cast<TextureRegister>(0), &_postProcessRenderTarget[0], SamplerStateRegister::LinearClamp);
		BindRenderTargetAsTexture(static_cast<TextureRegister>(3), &_glowRenderTarget[0], SamplerStateRegister::LinearClamp);
		DrawTriangles(3, 0);
	}
}