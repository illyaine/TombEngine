#include "framework.h"
#include "Renderer/ShaderManager/ShaderManager.h"

#include "Renderer/RendererUtils.h"
#include "Renderer/Structures/RendererShader.h"
#include "Specific/configuration.h"
#include "Specific/trutils.h"
#include "Version.h"

using namespace TEN::Renderer::Structures;
using namespace TEN::Utils;

namespace TEN::Renderer::Utils
{
	ShaderManager::~ShaderManager()
	{
		_gpuEnvironmentSampler.Reset();
		_device = nullptr;
		_context = nullptr;

		for (int i = 0; i < (int)Shader::Count; i++)
			Destroy((Shader)i);
	}

	const RendererShader& ShaderManager::Get(Shader shader)
	{
		return _shaders[(int)shader];
	}

	void ShaderManager::Initialize(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context)
	{
		_device = device;
		_context = context;

		auto samplerDesc = D3D11_SAMPLER_DESC{};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		throwIfFailed(_device->CreateSamplerState(&samplerDesc, _gpuEnvironmentSampler.GetAddressOf()));
	}

	void ShaderManager::LoadPostprocessShaders()
	{
		Load(Shader::PostProcess, "PostProcess", "", ShaderType::PixelAndVertex);

		Load(Shader::PostProcessMonochrome, "PostProcess", "Monochrome", ShaderType::Pixel);
		Load(Shader::PostProcessNegative, "PostProcess", "Negative", ShaderType::Pixel);
		Load(Shader::PostProcessExclusion, "PostProcess", "Exclusion", ShaderType::Pixel);
		Load(Shader::PostProcessFinalPass, "PostProcess", "FinalPass", ShaderType::Pixel);
		Load(Shader::PostProcessLensFlare, "PostProcess", "LensFlare", ShaderType::Pixel);

		Load(Shader::Ssao, "SSAO", "", ShaderType::Pixel);
		Load(Shader::SsaoBlur, "SSAO", "Blur", ShaderType::Pixel);

		Load(Shader::Downscale, "PostProcess", "Downscale", ShaderType::Pixel);
		Load(Shader::Blur, "PostProcess", "Blur", ShaderType::Pixel);
		Load(Shader::GlowCombine, "PostProcess", "GlowCombine", ShaderType::Pixel);
	}

	void ShaderManager::LoadAAShaders(int width, int height, bool recompile)
	{
		auto string = std::stringstream{};
		auto defines = std::vector<D3D10_SHADER_MACRO>{};

		// Set up pixel size macro.
		string << "float4(1.0 / " << width << ", 1.0 / " << height << ", " << width << ", " << height << ")";
		auto pixelSizeText = string.str();
		auto renderTargetMetricsMacro = D3D10_SHADER_MACRO{ "SMAA_RT_METRICS", pixelSizeText.c_str() };
		defines.push_back(renderTargetMetricsMacro);

		if (g_Configuration.AntialiasingMode == AntialiasingMode::Medium)
		{
			defines.push_back({ "SMAA_PRESET_MEDIUM", nullptr });
		}
		else
		{
			defines.push_back({ "SMAA_PRESET_ULTRA", nullptr });
		}

		// defines.push_back({ "SMAA_PREDICATION", "1" });

		// Set up target macro.
		auto dx101Macro = D3D10_SHADER_MACRO{ "SMAA_HLSL_4_1", "1" };
		defines.push_back(dx101Macro);

		auto null = D3D10_SHADER_MACRO{ nullptr, nullptr };
		defines.push_back(null);

		Load(Shader::SmaaEdgeDetection, "SMAA", "EdgeDetection", ShaderType::Vertex, defines.data(), recompile);
		Load(Shader::SmaaLumaEdgeDetection, "SMAA", "LumaEdgeDetection", ShaderType::Pixel, defines.data(), recompile);
		Load(Shader::SmaaColorEdgeDetection, "SMAA", "ColorEdgeDetection", ShaderType::Pixel, defines.data(), recompile);
		Load(Shader::SmaaDepthEdgeDetection, "SMAA", "DepthEdgeDetection", ShaderType::Pixel, defines.data(), recompile);
		Load(Shader::SmaaBlendingWeightCalculation, "SMAA", "BlendingWeightCalculation", ShaderType::PixelAndVertex, defines.data(), recompile);
		Load(Shader::SmaaNeighborhoodBlending, "SMAA", "NeighborhoodBlending", ShaderType::PixelAndVertex, defines.data(), recompile);

		Load(Shader::Fxaa, "FXAA", "", ShaderType::Pixel);
	}

	void ShaderManager::LoadCommonShaders()
	{
		D3D_SHADER_MACRO animated[] = { "ANIMATED", "", nullptr, nullptr };
		D3D_SHADER_MACRO roomTransparent[] = { "TRANSPARENT", "", nullptr, nullptr };
		D3D_SHADER_MACRO shadowMap[] = { "SHADOW_MAP", "", nullptr, nullptr };
		
		Load(Shader::Rooms, "Rooms", "", ShaderType::PixelAndVertex);
		Load(Shader::RoomsTransparent, "Rooms", "", ShaderType::Pixel, roomTransparent);
		Load(Shader::RoomAmbient, "RoomAmbient", "", ShaderType::PixelAndVertex);
		Load(Shader::RoomAmbientSky, "RoomAmbient", "Sky", ShaderType::PixelAndVertex);
		Load(Shader::Items, "Items", "", ShaderType::PixelAndVertex);
		Load(Shader::Sky, "Sky", "", ShaderType::PixelAndVertex);
		Load(Shader::Solid, "Solid", "", ShaderType::PixelAndVertex);
		Load(Shader::Inventory, "Inventory", "", ShaderType::PixelAndVertex);

		Load(Shader::FullScreenQuad, "FullScreenQuad", "", ShaderType::PixelAndVertex);

		Load(Shader::ShadowMap, "ShadowMap", "", ShaderType::PixelAndVertex, shadowMap);

		Load(Shader::Hud, "HUD", "", ShaderType::Vertex);
		Load(Shader::HudColor, "HUD", "ColoredHUD", ShaderType::Pixel);
		Load(Shader::HudDTexture, "HUD", "TexturedHUD", ShaderType::Pixel);
		Load(Shader::HudBarColor, "HUD", "TexturedHUDBar", ShaderType::Pixel);

		Load(Shader::InstancedStatics, "InstancedStatics", "", ShaderType::PixelAndVertex);
		Load(Shader::InstancedSprites, "InstancedSprites", "", ShaderType::PixelAndVertex);
		Load(Shader::GpuEnvironment, "GpuEnvironment", "", ShaderType::PixelAndVertex);

		Load(Shader::GBuffer, "GBuffer", "", ShaderType::Pixel);
		Load(Shader::GBufferRooms, "GBuffer", "Rooms", ShaderType::Vertex);
		Load(Shader::GBufferItems, "GBuffer", "Items", ShaderType::Vertex);
		Load(Shader::GBufferInstancedStatics, "GBuffer", "InstancedStatics", ShaderType::Vertex);
	}

	void ShaderManager::LoadShaders(int width, int height, bool recompileAAShaders)
	{
		TENLog("Loading shaders...", LogLevel::Info);

		// Unbind any currently bound shader.
		Bind(Shader::None, true);

		// Reset compile counter.
		_compileCounter = 0;

		// LoadAAShaders should always be the first in the list, so that when AA settings are changed,
		// they recompile with the same index as before.

		LoadAAShaders(width, height, recompileAAShaders); 
		LoadCommonShaders();
		LoadPostprocessShaders();
	}

	void ShaderManager::Bind(Shader shader, bool forceNull)
	{
		int shaderIndex = (int)shader;

		if (shaderIndex >= _shaders.size())
		{
			TENLog("Attempt to access nonexistent shader with index " + std::to_string(shaderIndex), LogLevel::Error);
			return;
		}

		const auto& shaderObj = _shaders[shaderIndex];

		if (shaderObj.Vertex.Shader != nullptr || forceNull)
			_context->VSSetShader(shaderObj.Vertex.Shader.Get(), nullptr, 0);

		if (shaderObj.Pixel.Shader != nullptr || forceNull)
			_context->PSSetShader(shaderObj.Pixel.Shader.Get(), nullptr, 0);

		if (shaderObj.Compute.Shader != nullptr || forceNull)
			_context->CSSetShader(shaderObj.Compute.Shader.Get(), nullptr, 0);

		if (shader == Shader::GpuEnvironment && _gpuEnvironmentSampler != nullptr)
		{
			auto* sampler = _gpuEnvironmentSampler.Get();
			_context->PSSetSamplers(0, 1, &sampler);
		}
	}

	RendererShader ShaderManager::LoadOrCompile(const std::string& fileName, const std::string& funcName, ShaderType type, const D3D_SHADER_MACRO* defines, bool forceRecompile)
	{
		auto rendererShader = RendererShader{};

		// Define paths for native (uncompiled) shaders and compiled shaders.
		auto shaderPath = GetAssetPath(L"Shaders\\");
		auto compiledShaderPath = shaderPath + L"Bin\\" + ToWString(TEN_VERSION_STRING) + L"\\";
		auto wideFileName = ToWString(fileName);

		// Ensure the /Bin subdirectory exists.
		std::filesystem::create_directories(compiledShaderPath);

		// Helper function to load or compile a shader.
		auto loadOrCompileShader = [this, type, defines, forceRecompile, shaderPath, compiledShaderPath]
			(const std::wstring& baseFileName, const std::string& shaderType, const std::string& functionName, const char* model, ComPtr<ID3D10Blob>& bytecode)
		{
			// Construct full paths using GetAssetPath.
			auto prefix = ((_compileCounter < 10) ? L"0" : L"") + std::to_wstring(_compileCounter) + L"_";
			auto csoFileName = compiledShaderPath + prefix + baseFileName + L"." + std::wstring(shaderType.begin(), shaderType.end()) + L".cso";
			auto srcFileName = shaderPath + baseFileName;

			// Try both .hlsl and .fx extensions for source shader.
			auto srcFileNameWithExtension = srcFileName + L".hlsl";
			if (!std::filesystem::exists(srcFileNameWithExtension))
			{
				srcFileNameWithExtension = srcFileName + L".fx";
				if (!std::filesystem::exists(srcFileNameWithExtension))
				{
					TENLog("Shader source file not found: " + ToString(srcFileNameWithExtension), LogLevel::Error);
					throw std::runtime_error("Shader source file not found.");
				}
			}

			// Check modification dates of source and compiled files.
			if (!forceRecompile && std::filesystem::exists(csoFileName))
			{
				auto csoTime = std::filesystem::last_write_time(csoFileName);
				auto srcTime = std::filesystem::last_write_time(srcFileNameWithExtension);

				if (csoTime >= srcTime)
				{
					try
					{
						throwIfFailed(D3DReadFileToBlob(csoFileName.c_str(), &bytecode));
						TENLog("Shader " + ToString(csoFileName) + " loaded from binary file.", LogLevel::Info);
						return true;
					}
					catch (...)
					{
						// Fall back to recompilation below.
					}
				}
			}

			ComPtr<ID3DBlob> errorBlob = nullptr;
			auto flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
			flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

			auto result = D3DCompileFromFile(
				srcFileNameWithExtension.c_str(),
				defines,
				D3D_COMPILE_STANDARD_FILE_INCLUDE,
				functionName.c_str(),
				model,
				flags,
				0,
				&bytecode,
				&errorBlob);

			if (FAILED(result))
			{
				std::string errorText = (errorBlob != nullptr) ?
					std::string((const char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize()) :
					"Unknown shader compilation error.";
				TENLog("Failed to compile shader " + ToString(srcFileNameWithExtension) + ": " + errorText, LogLevel::Error);
				throw std::runtime_error(errorText);
			}

			TENLog("Shader " + ToString(srcFileNameWithExtension) + " compiled.", LogLevel::Info);

			if (bytecode != nullptr)
				D3DWriteBlobToFile(bytecode.Get(), csoFileName.c_str(), TRUE);

			return false;
		};

		const auto vertexShaderTarget = "vs_5_0";
		const auto pixelShaderTarget = "ps_5_0";
		const auto computeShaderTarget = "cs_5_0";

		auto loadStage = [&](ShaderType stageType, const std::string& stageName, const std::string& functionSuffix, const char* target, ComPtr<ID3D10Blob>& bytecode, auto createShader)
		{
			if ((type & stageType) == ShaderType::None)
				return;

			auto entryPoint = funcName + functionSuffix;
			bool loadedFromBinary = loadOrCompileShader(wideFileName, stageName, entryPoint, target, bytecode);
			createShader(bytecode, loadedFromBinary);
		};

		loadStage(ShaderType::Vertex, "VS", "VS", vertexShaderTarget, rendererShader.Vertex.Blob,
			[this, &rendererShader](ComPtr<ID3D10Blob>& bytecode, bool)
			{
				throwIfFailed(_device->CreateVertexShader(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), nullptr, &rendererShader.Vertex.Shader));
			});

		loadStage(ShaderType::Pixel, "PS", "PS", pixelShaderTarget, rendererShader.Pixel.Blob,
			[this, &rendererShader](ComPtr<ID3D10Blob>& bytecode, bool)
			{
				throwIfFailed(_device->CreatePixelShader(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), nullptr, &rendererShader.Pixel.Shader));
			});

		loadStage(ShaderType::Compute, "CS", "CS", computeShaderTarget, rendererShader.Compute.Blob,
			[this, &rendererShader](ComPtr<ID3D10Blob>& bytecode, bool)
			{
				throwIfFailed(_device->CreateComputeShader(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), nullptr, &rendererShader.Compute.Shader));
			});

		return rendererShader;
	}

	void ShaderManager::Load(Shader shader, const std::string& fileName, const std::string& funcName, ShaderType type, const D3D_SHADER_MACRO* defines, bool forceRecompile)
	{
		_shaders[(int)shader] = LoadOrCompile(fileName, funcName, type, defines, forceRecompile);
		_compileCounter++;
	}

	void ShaderManager::Destroy(Shader shader)
	{
		auto& shaderObj = _shaders[(int)shader];
		shaderObj.Vertex.Shader.Reset();
		shaderObj.Vertex.Blob.Reset();
		shaderObj.Pixel.Shader.Reset();
		shaderObj.Pixel.Blob.Reset();
		shaderObj.Compute.Shader.Reset();
		shaderObj.Compute.Blob.Reset();
	}
}
