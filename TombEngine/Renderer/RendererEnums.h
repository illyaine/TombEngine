#pragma once
#include <SimpleMath.h>

#include "Math/Math.h"

using namespace DirectX::SimpleMath;

#define SHAPE_RECTANGLE 0
#define SHAPE_TRIANGLE	1

constexpr auto MAX_LINES_2D		= 256;
constexpr auto MAX_LINES_3D		= 16384;
constexpr auto MAX_TRIANGLES_3D = 16384;

constexpr auto FADE_FACTOR = 0.0625f;

constexpr auto NUM_LIGHTS_PER_BUFFER = 48;
constexpr auto MAX_LIGHTS_PER_ITEM = 8;
constexpr auto MAX_LIGHTS_PER_ROOM = 48;
constexpr auto MAX_LIGHTS = 100;
constexpr auto AMBIENT_LIGHT_INTERPOLATION_STEP = 1.0f / 10.0f;
constexpr auto MAX_DYNAMIC_SHADOWS = 1;
constexpr auto MAX_DYNAMIC_LIGHTS = 1024;
constexpr auto ITEM_LIGHT_COLLECTION_RADIUS = BLOCK(2);
constexpr auto CAMERA_LIGHT_COLLECTION_RADIUS = BLOCK(4);

constexpr auto MAX_TRANSPARENT_FACES		  = 16384;
constexpr auto MAX_TRANSPARENT_VERTICES		  = MAX_TRANSPARENT_FACES * 6;
constexpr auto MAX_TRANSPARENT_FACES_PER_ROOM = 16384;
constexpr auto TRANSPARENT_BUCKET_SIZE		  = 3840 * 16;
constexpr auto ALPHA_TEST_THRESHOLD			  = 0.5f;
constexpr auto ALPHA_BLEND_THRESHOLD		  = 1.0f - EPSILON;
constexpr auto FAST_ALPHA_BLEND_THRESHOLD	  = 0.5f;

constexpr auto MAX_BONES = 32;
constexpr auto MAX_BONE_WEIGHTS = 4;

constexpr auto DISPLAY_SPACE_RES = Vector2(800.0f, 600.0f);
constexpr auto REFERENCE_FONT_SIZE = 35.0f;
constexpr auto HUD_ZERO_Y = -DISPLAY_SPACE_RES.y;

constexpr float DISPLAY_ITEM_NEAR_PLANE = 0.1f;
constexpr float DISPLAY_ITEM_FAR_PLANE = BLOCK(100);

constexpr auto UNDERWATER_FOG_MIN_DISTANCE = 4;
constexpr auto UNDERWATER_FOG_MAX_DISTANCE = 30;
constexpr auto MAX_ROOM_BOUNDS = 256;

constexpr auto MIN_FAR_VIEW = 3200.0f;
constexpr auto DEFAULT_FAR_VIEW = 102400.0f;

constexpr auto INSTANCED_SPRITES_BUCKET_SIZE = 512;
constexpr auto MAX_SPRITE_VERTICES 			 = INSTANCED_SPRITES_BUCKET_SIZE * 6;

constexpr auto SKY_TILES_COUNT = 20;
constexpr auto SKY_SIZE = 10240.0f;
constexpr auto SKY_VERTICES_COUNT = 4 * SKY_TILES_COUNT * SKY_TILES_COUNT;
constexpr auto SKY_INDICES_COUNT = 6 * SKY_TILES_COUNT * SKY_TILES_COUNT;
constexpr auto SKY_TRIANGLES_COUNT = 2 * SKY_TILES_COUNT * SKY_TILES_COUNT;

constexpr auto MAX_ROOMS_DRAW = 256;
constexpr auto MAX_ITEMS_DRAW = 128;
constexpr auto MAX_LIGHTS_DRAW = 48;
constexpr auto MAX_FOG_BULBS_DRAW = 32;
constexpr auto MAX_SPRITES_DRAW = 512;
constexpr auto MAX_LENS_FLARES_DRAW = 8;

constexpr auto ROOM_AMBIENT_MAP_SIZE = 512;
constexpr auto LEGACY_REFLECTIONS_DOWNSCALE_FACTOR = 2.0f;
constexpr auto MAX_ROOM_AMBIENT_MAPS = 10;

constexpr auto GLOW_DOWNSCALE_FACTOR = 4.0f;
constexpr auto GLOW_BLUR_SIGMA = 10.0f;
constexpr auto GLOW_BLUR_RADIUS = 24.0f;
constexpr auto INVENTORY_GLOW_BLUR_SIGMA = 4.0f;
constexpr auto INVENTORY_GLOW_BLUR_RADIUS = 8.0f;

constexpr auto GLOW_VERTEX_SHIFT = 0;
constexpr auto MOVE_VERTEX_SHIFT = 8;
constexpr auto SHININESS_VERTEX_SHIFT = 16;
constexpr auto LOCKED_VERTEX_SHIFT = 24;
constexpr auto INDEX_IN_POLY_VERTEX_SHIFT = 25;

enum class LightType
{
	Sun = 0,
	Point = 1,
	Spot = 2,
	Shadow = 3,
	FogBulb = 4,
	HDR = 5
};

enum class BlendMode
{
	Unknown = -1,
	Opaque = 0,
	AlphaTest = 1,
	Additive = 2,
	NoDepthTest = 4,
	Subtractive = 5,
	Wireframe = 6,
	Exclude = 8,
	Screen = 9,
	Lighten = 10,
	AlphaBlend = 11,
	FastAlphaBlend = 12
};

enum class SkinningMode
{
	None = 0,
	Full = 1,
	Classic = 2
};

enum class CullMode
{
	Unknown = -1,
	None = 0,
	Clockwise = 1,
	CounterClockwise = 2
};

enum class ShadowMode
{
	None,
	Player,
	All
};

enum class AntialiasingMode
{
	None,
	Low,
	Medium,
	High
};

enum class LightMode
{
	Dynamic,
	Static
};

enum class DepthState
{
	Unknown = -1,
	Write = 0,
	Read = 1,
	None = 2
};

enum class SpriteType
{
	Billboard,
	ThreeD,
	CustomBillboard,
	LookAtBillboard
};

enum class RendererDebugPage
{
	None,
	RendererStats,
	MemoryStats,
	DimensionStats,
	PlayerStats,
	InputStats,
	CollisionStats,
	CollisionMeshStats,
	PortalStats,
	PathfindingStats,
	WireframeMode,

	Count
};

enum class TransparentFaceType
{
	Room,
	Moveable,
	Static,
	Sprite,
	None
};

enum class TextureRegister
{
	ColorMap = 0,
	NormalMap = 1,
	CausticsMap = 2,
	ShadowMap = 3,
	GBufferNormalMap = 4,
	Hud = 5,
	GBufferDepthMap = 6,
	EnvironmentMapFront = 7,
	EnvironmentMapBack = 8,
	SSAO = 9,
	ORSHMap = 10,
	EmissiveMap = 11,
	LegacyEnvironmentReflections = 12,
	SkyboxEnvironmentReflections = 13
};

enum class SamplerStateRegister
{
	None = 0,
	PointWrap = 1,
	LinearWrap = 2,
	LinearClamp = 3,
	AnisotropicWrap = 4,
	AnisotropicClamp = 5,
	ShadowMap = 6
};

enum class ConstantBufferRegister
{
	Camera = 0,
	Item = 1,
	Material = 2,
	Room = 3,
	Animated = 4,
	Blending = 5,
	PostProcess = 6,
	ShadowMap = 7,
	Static = 8,
	FogBulb = 9,
	AlphaTest = 10,
	SMAA = 13
};

enum class RendererPass
{
	CollectTransparentFaces,
	GBuffer,
	Opaque,
	Additive,
	Transparent,
	ShadowMap,
	Water
};

enum class SceneRenderMode
{
	Full,
	NoHud,
	NoPostProcess
};

enum class TextureSource
{
	Room,
	Moveables,
	Statics,
	Sprites,
	Animated
};

enum class PostProcessMode
{
	None,
	Monochrome,
	Negative,
	Exclusion
};

enum class AlphaTestMode
{
	None,
	GreatherThan,
	LessThan
};

enum class RendererObjectType
{
	Room,
	Item,
	Static,
	Sprite,
	MoveableAsStatic,
	Hair
};

enum class SpriteRenderType
{
	Default,
	LaserBarrier,
	LaserBeam
};
