from __future__ import annotations

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def write(path: str, content: str) -> None:
    target = ROOT / path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(content, encoding="utf-8", newline="")


def replace_once(path: str, old: str, new: str) -> None:
    content = read(path)
    count = content.count(old)
    if count != 1:
        raise RuntimeError(f"Expected one match in {path}, found {count}: {old!r}")
    write(path, content.replace(old, new, 1))


def replace_regex_once(path: str, pattern: str, replacement: str) -> None:
    content = read(path)
    updated, count = re.subn(pattern, replacement, content, count=1, flags=re.DOTALL)
    if count != 1:
        raise RuntimeError(f"Expected one regex match in {path}, found {count}: {pattern!r}")
    write(path, updated)


def insert_project_entry(path: str, anchor: str, entry: str) -> None:
    content = read(path)
    if entry in content:
        return
    if anchor not in content:
        raise RuntimeError(f"Project anchor not found in {path}: {anchor}")
    write(path, content.replace(anchor, anchor + "\n" + entry, 1))


def clone_filter_entry(path: str, source: str, target: str) -> None:
    content = read(path)
    if target in content:
        return

    escaped = re.escape(source)
    pattern = rf'(?P<indent>\s*)<(?P<tag>ClCompile|ClInclude|None) Include="{escaped}"(?P<body>\s*/>|>.*?</(?P=tag)>)'
    match = re.search(pattern, content, flags=re.DOTALL)
    if not match:
        raise RuntimeError(f"Filter entry not found in {path}: {source}")

    block = match.group(0).replace(source, target, 1)
    write(path, content[:match.end()] + block + content[match.end():])


WEATHER_UPDATE_STARFIELD = r'''	void EnvironmentController::UpdateStarfield(const ScriptInterfaceLevel& level)
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
'''

RENDERER_STAR = r'''#pragma once
#include <SimpleMath.h>

namespace TEN::Renderer::Structures
{
	using namespace DirectX::SimpleMath;

	struct alignas(16) RendererStar
	{
		Vector3 Direction;
		float Scale;
		Vector3 Color;
		float Extinction;
	};

	static_assert(sizeof(RendererStar) == 32);
}
'''

STARFIELD_BUFFER = r'''#pragma once
#include <SimpleMath.h>

namespace TEN::Renderer::ConstantBuffers
{
	using namespace DirectX::SimpleMath;

	struct alignas(16) CStarfieldBuffer
	{
		Vector4 UV[2];
	};
}
'''

RENDERER_STARFIELD_CPP = r'''#include "framework.h"
#include "Renderer/Renderer.h"

#include "Game/effects/weather.h"
#include "Game/Setup.h"
#include "Objects/game_object_ids.h"

using namespace TEN::Effects::Environment;
using namespace TEN::Renderer::Structures;

namespace TEN::Renderer
{
	constexpr UINT STARFIELD_BUFFER_SLOT = 14;

	void Renderer::UpdateStarfieldBuffer()
	{
		const auto revision = Weather.GetStarfieldRevision();
		if (_starfieldRevision == revision)
			return;

		_starfieldRevision = revision;
		_starfieldCount = 0;
		_starfieldBuffer.Reset();
		_starfieldBufferView.Reset();

		const auto& stars = Weather.GetStars();
		if (stars.empty())
			return;

		auto rendererStars = std::vector<RendererStar>{};
		rendererStars.reserve(stars.size());

		for (const auto& star : stars)
		{
			rendererStars.push_back(
			{
				star.Direction,
				star.Scale,
				star.Color,
				star.Extinction
			});
		}

		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = (UINT)(sizeof(RendererStar) * rendererStars.size());
		bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferDesc.StructureByteStride = sizeof(RendererStar);

		D3D11_SUBRESOURCE_DATA initialData = {};
		initialData.pSysMem = rendererStars.data();

		Utils::throwIfFailed(_device->CreateBuffer(&bufferDesc, &initialData, _starfieldBuffer.GetAddressOf()));

		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
		viewDesc.Format = DXGI_FORMAT_UNKNOWN;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		viewDesc.Buffer.FirstElement = 0;
		viewDesc.Buffer.NumElements = (UINT)rendererStars.size();

		Utils::throwIfFailed(_device->CreateShaderResourceView(_starfieldBuffer.Get(), &viewDesc, _starfieldBufferView.GetAddressOf()));
		_starfieldCount = (int)rendererStars.size();
	}

	void Renderer::DrawStarfield()
	{
		UpdateStarfieldBuffer();

		if (_starfieldCount == 0 || _starfieldBufferView == nullptr)
			return;

		auto* sprite = &_sprites[Objects[ID_DEFAULT_SPRITES].meshIndex + SPR_LENS_FLARE_3];

		// NOTE: Strange packing due to particular HLSL 16 byte alignment requirements.
		_stStarfield.UV[0].x = sprite->UV[0].x;
		_stStarfield.UV[0].y = sprite->UV[1].x;
		_stStarfield.UV[0].z = sprite->UV[2].x;
		_stStarfield.UV[0].w = sprite->UV[3].x;
		_stStarfield.UV[1].x = sprite->UV[0].y;
		_stStarfield.UV[1].y = sprite->UV[1].y;
		_stStarfield.UV[1].z = sprite->UV[2].y;
		_stStarfield.UV[1].w = sprite->UV[3].y;

		UpdateConstantBuffer(_stStarfield, _cbStarfield);
		BindConstantBufferVS(ConstantBufferRegister::Starfield, _cbStarfield.get());

		SetDepthState(DepthState::Read);
		SetBlendMode(BlendMode::Additive);
		SetCullMode(CullMode::None);

		_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		_context->IASetInputLayout(_inputLayout.Get());

		unsigned int stride = sizeof(Vertex);
		unsigned int offset = 0;
		_context->IASetVertexBuffers(0, 1, _quadVertexBuffer.Buffer.GetAddressOf(), &stride, &offset);

		_shaders.Bind(Shader::Starfield);
		BindTexture(TextureRegister::ColorMap, sprite->Texture, SamplerStateRegister::LinearClamp);

		auto* starfieldView = _starfieldBufferView.Get();
		_context->VSSetShaderResources(STARFIELD_BUFFER_SLOT, 1, &starfieldView);

		DrawInstancedTriangles(4, _starfieldCount, 0);

		ID3D11ShaderResourceView* nullView = nullptr;
		_context->VSSetShaderResources(STARFIELD_BUFFER_SLOT, 1, &nullView);

		_numInstancedSpritesDrawCalls++;
	}
}
'''

CB_STARFIELD_HLSL = r'''#ifndef CBSTARFIELD
#define CBSTARFIELD

cbuffer CBStarfield : register(b14)
{
	float4 StarfieldUV[2];
};

#endif
'''

STARFIELD_HLSL = r'''#include "./CBCamera.hlsli"
#include "./CBStarfield.hlsli"
#include "./Math.hlsli"
#include "./ShaderLight.hlsli"
#include "./VertexInput.hlsli"

struct StarfieldInstance
{
	float3 Direction;
	float Scale;
	float3 Color;
	float Extinction;
};

struct PixelShaderInput
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD1;
	float4 Color : COLOR;
	float4 FogBulbs : TEXCOORD3;
	float DistanceFog : FOG;
};

StructuredBuffer<StarfieldInstance> Stars : register(t14);

Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

float Hash(float value)
{
	return frac(sin(value) * 43758.5453f);
}

float GetTwinkle(StarfieldInstance star, uint instanceID)
{
	float seed = Hash(dot(star.Direction, float3(12.9898f, 78.233f, 37.719f)) + instanceID);
	float frequency = lerp(0.035f, 0.11f, Hash(seed * 91.17f));
	float phase = seed * PI * 2.0f;
	return 0.75f + sin(InterpolatedFrame * frequency + phase) * 0.25f;
}

PixelShaderInput VS(VertexShaderInput input, uint instanceID : SV_InstanceID)
{
	PixelShaderInput output;
	StarfieldInstance star = Stars[instanceID];

	const float starDistance = 1024.0f;
	const float starSize = 2.0f * star.Scale;

	float3 cameraRight = normalize(InverseView[0].xyz);
	float3 cameraUp = normalize(InverseView[1].xyz);
	float3 center = CamPositionWS.xyz + star.Direction * starDistance;
	float3 worldPosition = center + cameraRight * input.Position.x * starSize + cameraUp * input.Position.y * starSize;

	output.Position = mul(float4(worldPosition, 1.0f), ViewProjection);

	int polyIndex = DecodeIndexInPoly(input.Effects);
	output.UV = float2(StarfieldUV[0][polyIndex], StarfieldUV[1][polyIndex]);
	output.Color = float4(star.Color, GetTwinkle(star, instanceID) * star.Extinction);
	output.FogBulbs = DoFogBulbsForVertex(float4(worldPosition, 1.0f));
	output.DistanceFog = DoDistanceFogForVertex(float4(worldPosition, 1.0f));

	return output;
}

float4 PS(PixelShaderInput input) : SV_TARGET
{
	float4 output = Texture.Sample(Sampler, input.UV) * input.Color;

	output.xyz *= 1.0f - Luma(input.FogBulbs.xyz);
	output.xyz = saturate(output.xyz);
	output = DoDistanceFogForPixel(output, float4(0.0f, 0.0f, 0.0f, 0.0f), input.DistanceFog);

	return output;
}
'''


def main() -> None:
    replace_once(
        "TombEngine/Game/effects/weather.h",
        "\t\tfloat Extinction = 1.0f;\n\t\tfloat Scale\t\t = 1.0f;\n\t\tfloat Blinking\t = 1.0f;",
        "\t\tfloat Extinction = 1.0f;\n\t\tfloat Scale\t\t = 1.0f;")
    replace_once(
        "TombEngine/Game/effects/weather.h",
        "\t\tbool\t\t\t\t\t\tResetStarField = true;",
        "\t\tbool\t\t\t\t\t\tResetStarField = true;\n\t\tunsigned int\t\t\t\tStarfieldRevision = 0;")
    replace_once(
        "TombEngine/Game/effects/weather.h",
        "\t\tconst std::vector<StarParticle>&\tGetStars() const { return Stars; }",
        "\t\tconst std::vector<StarParticle>&\tGetStars() const { return Stars; }\n\t\tunsigned int GetStarfieldRevision() const { return StarfieldRevision; }")

    replace_once(
        "TombEngine/Game/effects/weather.cpp",
        "\t\tResetStarField = true;\n\t\tStars.clear();\n\t\tMeteors.clear();",
        "\t\tResetStarField = true;\n\t\tStars.clear();\n\t\tStarfieldRevision++;\n\t\tMeteors.clear();")
    replace_regex_once(
        "TombEngine/Game/effects/weather.cpp",
        r"\tvoid EnvironmentController::UpdateStarfield\(const ScriptInterfaceLevel& level\)\n\t\{.*?\n\t\}\n\n\tvoid EnvironmentController::UpdateWeather",
        WEATHER_UPDATE_STARFIELD + "\n\tvoid EnvironmentController::UpdateWeather")

    write("TombEngine/Renderer/Structures/RendererStar.h", RENDERER_STAR)
    write("TombEngine/Renderer/ConstantBuffers/StarfieldBuffer.h", STARFIELD_BUFFER)
    write("TombEngine/Renderer/RendererStarfield.cpp", RENDERER_STARFIELD_CPP)
    write("TombEngine/Shaders/CBStarfield.hlsli", CB_STARFIELD_HLSL)
    write("TombEngine/Shaders/Starfield.hlsl", STARFIELD_HLSL)

    replace_once(
        "TombEngine/Renderer/RendererEnums.h",
        "\tInstancedSprites = 13\n};",
        "\tInstancedSprites = 13,\n\tStarfield = 14\n};")
    replace_once(
        "TombEngine/Renderer/ShaderManager/ShaderManager.h",
        "\t\tInstancedSprites,\n\t\tSky,",
        "\t\tInstancedSprites,\n\t\tStarfield,\n\t\tSky,")
    replace_once(
        "TombEngine/Renderer/ShaderManager/ShaderManager.cpp",
        "\t\tLoad(Shader::InstancedSprites, \"InstancedSprites\", \"\", ShaderType::PixelAndVertex);",
        "\t\tLoad(Shader::InstancedSprites, \"InstancedSprites\", \"\", ShaderType::PixelAndVertex);\n\t\tLoad(Shader::Starfield, \"Starfield\", \"\", ShaderType::PixelAndVertex);")

    replace_once(
        "TombEngine/Renderer/Renderer.h",
        "#include <PostProcess.h>\n",
        "#include <PostProcess.h>\n#include <limits>\n")
    replace_once(
        "TombEngine/Renderer/Renderer.h",
        "#include \"Renderer/ConstantBuffers/InstancedSpriteBuffer.h\"\n",
        "#include \"Renderer/ConstantBuffers/InstancedSpriteBuffer.h\"\n#include \"Renderer/ConstantBuffers/StarfieldBuffer.h\"\n")
    replace_once(
        "TombEngine/Renderer/Renderer.h",
        "\t\tCInstancedSpriteBuffer _stInstancedSpriteBuffer;\n\t\tConstantBuffer<CInstancedSpriteBuffer> _cbInstancedSpriteBuffer;",
        "\t\tCInstancedSpriteBuffer _stInstancedSpriteBuffer;\n\t\tConstantBuffer<CInstancedSpriteBuffer> _cbInstancedSpriteBuffer;\n\t\tCStarfieldBuffer _stStarfield;\n\t\tConstantBuffer<CStarfieldBuffer> _cbStarfield;")
    replace_once(
        "TombEngine/Renderer/Renderer.h",
        "\t\tVertexBuffer<Vertex> _quadVertexBuffer;",
        "\t\tVertexBuffer<Vertex> _quadVertexBuffer;\n\n\t\t// Starfield\n\n\t\tComPtr<ID3D11Buffer> _starfieldBuffer = nullptr;\n\t\tComPtr<ID3D11ShaderResourceView> _starfieldBufferView = nullptr;\n\t\tunsigned int _starfieldRevision = std::numeric_limits<unsigned int>::max();\n\t\tint _starfieldCount = 0;")
    replace_once(
        "TombEngine/Renderer/Renderer.h",
        "\t\tvoid DrawHorizonAndSky(ID3D11DepthStencilView* depthStencilView, RenderView& renderView, bool reflectionPass = false);",
        "\t\tvoid DrawHorizonAndSky(ID3D11DepthStencilView* depthStencilView, RenderView& renderView, bool reflectionPass = false);\n\t\tvoid DrawStarfield();\n\t\tvoid UpdateStarfieldBuffer();")

    replace_once(
        "TombEngine/Renderer/RendererInit.cpp",
        "\t\t_cbInstancedSpriteBuffer = CreateConstantBuffer<CInstancedSpriteBuffer>();",
        "\t\t_cbInstancedSpriteBuffer = CreateConstantBuffer<CInstancedSpriteBuffer>();\n\t\t_cbStarfield = CreateConstantBuffer<CStarfieldBuffer>();")

    replace_regex_once(
        "TombEngine/Renderer/RendererDraw.cpp",
        r"\n\t\t\tint drawnStars = 0;\n\t\t\tint starCount = \(int\)Weather\.GetStars\(\)\.size\(\);\n\n\t\t\twhile \(drawnStars < starCount\)\n\t\t\t\{.*?\n\t\t\t\}\n\n\t\t\t// Draw meteors",
        "\n\t\t\tDrawStarfield();\n\t\t\t_shaders.Bind(Shader::InstancedSprites);\n\n\t\t\t// Draw meteors")

    project = "TombEngine/TombEngine.vcxproj"
    insert_project_entry(project,
        '    <ClCompile Include="Renderer\\RendererSprites.cpp" />',
        '    <ClCompile Include="Renderer\\RendererStarfield.cpp" />')
    insert_project_entry(project,
        '    <ClInclude Include="Renderer\\ConstantBuffers\\SkyBuffer.h" />',
        '    <ClInclude Include="Renderer\\ConstantBuffers\\StarfieldBuffer.h" />')

    project_content = read(project)
    shader_anchor_match = re.search(r'^(\s*)<(None|FxCompile) Include="Shaders\\Sky\.hlsl".*$', project_content, flags=re.MULTILINE)
    if not shader_anchor_match:
        raise RuntimeError("Sky shader project entry not found")
    shader_line = shader_anchor_match.group(0)
    shader_tag = shader_anchor_match.group(2)
    if 'Shaders\\Starfield.hlsl' not in project_content:
        project_content = project_content.replace(shader_line, shader_line + f'\n{shader_anchor_match.group(1)}<{shader_tag} Include="Shaders\\Starfield.hlsl" />', 1)

    include_anchor_match = re.search(r'^(\s*)<(None|ClInclude) Include="Shaders\\CBSky\.hlsli".*$', project_content, flags=re.MULTILINE)
    if not include_anchor_match:
        raise RuntimeError("CBSky project entry not found")
    include_line = include_anchor_match.group(0)
    include_tag = include_anchor_match.group(2)
    if 'Shaders\\CBStarfield.hlsli' not in project_content:
        project_content = project_content.replace(include_line, include_line + f'\n{include_anchor_match.group(1)}<{include_tag} Include="Shaders\\CBStarfield.hlsli" />', 1)
    write(project, project_content)

    filters = "TombEngine/TombEngine.vcxproj.filters"
    clone_filter_entry(filters, "Renderer\\RendererSprites.cpp", "Renderer\\RendererStarfield.cpp")
    clone_filter_entry(filters, "Renderer\\ConstantBuffers\\SkyBuffer.h", "Renderer\\ConstantBuffers\\StarfieldBuffer.h")
    clone_filter_entry(filters, "Shaders\\Sky.hlsl", "Shaders\\Starfield.hlsl")
    clone_filter_entry(filters, "Shaders\\CBSky.hlsli", "Shaders\\CBStarfield.hlsli")

    # Static regression checks.
    weather_cpp = read("TombEngine/Game/effects/weather.cpp")
    renderer_draw = read("TombEngine/Renderer/RendererDraw.cpp")
    shader = read("TombEngine/Shaders/Starfield.hlsl")

    if "star.Blinking" in weather_cpp or ".Blinking" in renderer_draw:
        raise RuntimeError("CPU star blinking path still exists")
    if "GetWorldMatrixForSprite(rDrawSprite, renderView)" in renderer_draw[renderer_draw.index("void Renderer::DrawHorizonAndSky"):]:
        raise RuntimeError("CPU star world-matrix generation still exists")
    if "StructuredBuffer<StarfieldInstance>" not in shader:
        raise RuntimeError("Structured starfield buffer is missing")
    if "Weather.GetStars().size()" not in renderer_draw:
        raise RuntimeError("Starfield draw guard was unexpectedly removed")


if __name__ == "__main__":
    main()
