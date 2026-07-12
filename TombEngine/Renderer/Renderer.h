#pragma once
#include <wrl/client.h>
#include <CommonStates.h>
#include <SpriteFont.h>
#include <PrimitiveBatch.h>
#include <d3d9types.h>
#include <SimpleMath.h>
#include <PostProcess.h>
#include <limits>
#include <type_traits>

#include "Math/Math.h"
#include "Game/control/box.h"
#include "Game/items.h"
#include "Game/Animation/Animation.h"
#include "Game/Gui.h"
#include "Game/Hud/DrawItems/DrawItems.h"
#include "Game/Hud/Hud.h"
#include "Game/Hud/PickupSummary.h"
#include "Game/effects/effects.h"
#include "Game/effects/Electricity.h"
#include "Game/Setup.h"
#include "Specific/level.h"
#include "Specific/fast_vector.h"
#include "Renderer/Frustum.h"
#include "Renderer/RendererEnums.h"
#include "Renderer/RenderView.h"
#include "Renderer/Structures/RendererLight.h"
#include "Renderer/ConstantBuffers/LightBuffer.h"
#include "Renderer/ConstantBuffers/HUDBarBuffer.h"
#include "Renderer/ConstantBuffers/HUDBuffer.h"
#include "Renderer/ConstantBuffers/ShadowLightBuffer.h"
#include "Renderer/ConstantBuffers/RoomBuffer.h"
#include "Renderer/ConstantBuffers/ItemBuffer.h"
#include "Renderer/ConstantBuffers/AnimatedBuffer.h"
#include "Renderer/ConstantBuffers/BlendingBuffer.h"
#include "Renderer/ConstantBuffers/CameraMatrixBuffer.h"
#include "Renderer/ConstantBuffers/MaterialBuffer.h"
#include "Renderer/ConstantBuffers/InstancedStaticBuffer.h"
#include "Renderer/ConstantBuffers/InstancedSpriteBuffer.h"
#include "Renderer/ConstantBuffers/GpuEnvironmentBuffer.h"
#include "Renderer/ConstantBuffers/ConstantBuffer.h"
#include "Renderer/ConstantBuffers/PostProcessBuffer.h"
#include "Renderer/ConstantBuffers/SMAABuffer.h"
#include "Renderer/ConstantBuffers/SkyBuffer.h"
#include "Renderer/Structures/RendererBone.h"
#include "Renderer/Structures/RendererDoor.h"
#include "Renderer/Structures/RendererStringToDraw.h"
#include "Renderer/Structures/RendererRoom.h"
#include "Renderer/Structures/RendererSprite.h"
#include "Renderer/Structures/RendererAnimatedTexture.h"
#include "Renderer/Structures/RendererAnimatedTextureSet.h"
#include "Renderer/Graphics/Texture2D.h"
#include "Renderer/Graphics/IndexBuffer.h"
#include "Renderer/Graphics/RenderTarget2D.h"
#include "Renderer/Graphics/RenderTargetCube.h"
#include "Renderer/Graphics/Texture2DArray.h"
#include "Renderer/Graphics/VertexBuffer.h"
#include "Renderer/Graphics/Vertices/PostProcessVertex.h"
#include "Renderer/ShaderManager/ShaderManager.h"
#include "Renderer/Structures/RendererItem.h"
#include "Renderer/Structures/RendererEffect.h"
#include "Renderer/Structures/RendererLine3D.h"
#include "Renderer/Structures/RendererTriangle3D.h"
#include "Renderer/Structures/RendererMesh.h"
#include "Renderer/Structures/RendererSpriteSequence.h"
#include "Renderer/Structures/RendererSpriteBucket.h"
#include "Renderer/Structures/RendererLine2D.h"
#include "Renderer/Structures/RendererHudBar.h"
#include "Renderer/Structures/RendererRoomAmbientMap.h"
#include "Renderer/Structures/RendererObject.h"
#include "Renderer/Structures/RendererStar.h"
#include "Structures/RendererShader.h"

using namespace TEN::Animation;

enum GAME_OBJECT_ID : short;
enum class SphereSpaceType;
class EulerAngles;
struct AnimFrameInterpData;
struct CAMERA_INFO;

namespace TEN::Renderer
{
	using namespace TEN::Effects::Electricity;
	using namespace TEN::Gui;
	using namespace TEN::Hud;
	using namespace TEN::Renderer::ConstantBuffers;
	using namespace TEN::Renderer::Graphics;
	using namespace TEN::Renderer::Structures;
	using namespace TEN::Renderer::Utils;
	using namespace DirectX::SimpleMath;

	using AtlasTexturesSet = std::tuple<Texture2D, Texture2D, Texture2D, Texture2D>;

	struct AdapterInfo
	{
		std::string Name = {};
		unsigned int VendorId = 0;
		unsigned int DeviceId = 0;
		unsigned int SubSysId = 0;
		unsigned int Revision = 0;
		size_t DedicatedVideoMemory = 0;
		size_t DedicatedSystemMemory = 0;
		size_t SharedSystemMemory = 0;
	};

	struct RendererSphereView
	{
		std::array<BoundingSphere, MAX_BONES> Spheres = {};
		size_t Count = 0;

		bool empty() const { return Count == 0; }
		size_t size() const { return Count; }
		auto begin() const { return Spheres.begin(); }
		auto end() const { return Spheres.begin() + Count; }
		const BoundingSphere& operator[](size_t index) const { return Spheres[index]; }

		operator std::vector<BoundingSphere>() const
		{
			return std::vector<BoundingSphere>(begin(), end());
		}
	};

	class Renderer
	{
	private:
		// Core DX11 objects

		ComPtr<ID3D11Device> _device = nullptr;
		ComPtr<ID3D11DeviceContext> _context = nullptr;
		ComPtr<IDXGISwapChain> _swapChain = nullptr;
		std::unique_ptr<CommonStates> _renderStates = nullptr;
		ComPtr <ID3D11SamplerState> _pointWrapSamplerState = nullptr;
		ComPtr<ID3D11BlendState> _subtractiveBlendState = nullptr;
		ComPtr<ID3D11BlendState> _screenBlendState = nullptr;
		ComPtr<ID3D11BlendState> _lightenBlendState = nullptr;
		ComPtr<ID3D11BlendState> _excludeBlendState = nullptr;
		ComPtr<ID3D11BlendState> _transparencyBlendState = nullptr;
		ComPtr<ID3D11BlendState> _finalTransparencyBlendState = nullptr;
		ComPtr<ID3D11RasterizerState> _cullCounterClockwiseRasterizerState = nullptr;
		ComPtr<ID3D11RasterizerState> _cullClockwiseRasterizerState = nullptr;
		ComPtr<ID3D11RasterizerState> _cullNoneRasterizerState = nullptr;
		ComPtr<ID3D11InputLayout> _inputLayout = nullptr;
		D3D11_VIEWPORT _viewport;
		D3D11_VIEWPORT _shadowMapViewport;
		Viewport _viewportToolkit;

		// Adapter info
		AdapterInfo _adapterInfo = {};

		// Render targets

		RenderTarget2D _normalsAndMaterialIndexRenderTarget;
		RenderTarget2D _depthRenderTarget;
		RenderTarget2D _emissiveAndRoughnessRenderTarget;
		RenderTarget2D _backBuffer;
		RenderTarget2D _dumpScreenRenderTarget;
		RenderTarget2D _renderTarget;
		RenderTarget2D _postProcessRenderTarget[2];
		RenderTarget2D _glowRenderTarget[2];
		RenderTarget2D _tempRoomAmbientRenderTarget1;
		RenderTarget2D _tempRoomAmbientRenderTarget2;
		RenderTarget2D _tempRoomAmbientRenderTarget3;
		RenderTarget2D _tempRoomAmbientRenderTarget4;
		Texture2DArray _shadowMap;
		RenderTarget2D _legacyReflectionsRenderTarget;
		RenderTarget2D _roomAmbientMapFront;
		RenderTarget2D _roomAmbientMapBack;
		RenderTarget2D _SSAORenderTarget;
		RenderTarget2D _SSAOBlurredRenderTarget;
		RenderTarget2D _SMAASceneRenderTarget;
		RenderTarget2D _SMAASceneSRGBRenderTarget;
		RenderTarget2D _SMAADepthRenderTarget;
		RenderTarget2D _SMAAEdgesRenderTarget;
		RenderTarget2D _SMAABlendRenderTarget;
		Texture2DArray _skyboxRenderTarget;

		// Constant buffers

		RenderView _gameCamera;
		RenderView _oldGameCamera;
		RenderView _currentGameCamera;
		ConstantBuffer<CCameraMatrixBuffer> _cbCameraMatrices;
		CItemBuffer _stItem;
		ConstantBuffer<CItemBuffer> _cbItem;
		CLightBuffer _stLights;
		ConstantBuffer<CLightBuffer> _cbLights;
		CRoomBuffer _stRoom;
		ConstantBuffer<CRoomBuffer> _cbRoom;
		CAnimatedBuffer _stAnimated;
		ConstantBuffer<CAnimatedBuffer> _cbAnimated;
		CShadowLightBuffer _stShadowMap;
		ConstantBuffer<CShadowLightBuffer> _cbShadowMap;
		CHUDBuffer _stHUD;
		ConstantBuffer<CHUDBuffer> _cbHUD;
		CHUDBarBuffer _stHUDBar;
		ConstantBuffer<CHUDBarBuffer> _cbHUDBar;
		CPostProcessBuffer _stPostProcessBuffer;
		ConstantBuffer<CPostProcessBuffer> _cbPostProcessBuffer;
		CInstancedSpriteBuffer _stInstancedSpriteBuffer;
		ConstantBuffer<CInstancedSpriteBuffer> _cbInstancedSpriteBuffer;
		CGpuEnvironmentBuffer _stGpuEnvironment;
		ConstantBuffer<CGpuEnvironmentBuffer> _cbGpuEnvironment;
		CBlendingBuffer _stBlending;
		ConstantBuffer<CBlendingBuffer> _cbBlending;
		CInstancedStaticMeshBuffer _stInstancedStaticMeshBuffer;
		ConstantBuffer<CInstancedStaticMeshBuffer> _cbInstancedStaticMeshBuffer;
		CSMAABuffer _stSMAABuffer;
		ConstantBuffer<CSMAABuffer> _cbSMAABuffer;
		CSkyBuffer _stSky;
		ConstantBuffer<CSkyBuffer> _cbSky;
		CMaterialBuffer _stMaterial;
		ConstantBuffer<CMaterialBuffer> _cbMaterial;

		// Primitive batches

		std::unique_ptr<SpriteBatch> _spriteBatch;
		std::unique_ptr<PrimitiveBatch<Vertex>> _primitiveBatch;

		// Text

		std::unique_ptr<SpriteFont> _gameFont;
		std::vector<RendererStringToDraw> _stringsToDraw;
		Vector4 _blinkColorValue = Vector4::Zero;
		float _blinkTime = 0.0f;
		float _oldBlinkTime = 0.0f;

		// Sprites
		std::vector<Vertex> _spriteVertices;
		VertexBuffer<Vertex> _spriteVertexBuffer;

		// Graphics resources

		Texture2D _logo;
		Texture2D _skyTexture;
		Texture2D _whiteTexture;
		RendererSprite _whiteSprite;
		Texture2D _loadingBarBorder;
		Texture2D _loadingBarInner;
		Texture2D _loadingScreenTexture;

		VertexBuffer<Vertex> _roomsVertexBuffer;
		IndexBuffer _roomsIndexBuffer;
		VertexBuffer<Vertex> _moveablesVertexBuffer;
		IndexBuffer _moveablesIndexBuffer;
		VertexBuffer<Vertex> _staticsVertexBuffer;
		IndexBuffer _staticsIndexBuffer;
		VertexBuffer<Vertex> _skyVertexBuffer;
		IndexBuffer _skyIndexBuffer;
		VertexBuffer<Vertex> _quadVertexBuffer;

		// Starfield

		ComPtr<ID3D11Buffer> _starfieldBuffer = nullptr;
		ComPtr<ID3D11ShaderResourceView> _starfieldBufferView = nullptr;
		unsigned int _starfieldRevision = std::numeric_limits<unsigned int>::max();
		int _starfieldCount = 0;

		std::vector<Vertex> _roomsVertices;
		std::vector<int> _roomsIndices;
		std::vector<Vertex> _moveablesVertices;
		std::vector<int> _moveablesIndices;
		std::vector<Vertex> _staticsVertices;
		std::vector<int> _staticsIndices;

		// Rooms and collector

		std::vector<RendererRoom> _rooms;
		bool _invalidateCache;
		std::vector<short> _visitedRoomsStack;

		// Lights

		int _dynamicLightList = 0;
		std::vector<RendererLight> _dynamicLights[2];
		RendererLight* _shadowLight;

		// Lines

		std::vector<RendererLine2D> 		_lines2DToDraw = {};
		std::vector<RendererLine3D> 		_lines3DToDraw = {};
		std::vector<RendererTriangle3D> _triangles3DToDraw = {};

		// Textures, objects and sprites

		std::vector<std::optional<RendererObject>> 			   _moveableObjects;
		std::vector<std::optional<RendererObject>> 			   _staticObjects; // Key = static ID, value = renderer object.
		std::vector<RendererSprite> 							   _sprites;
		std::vector<RendererSpriteSequence> 				   _spriteSequences;
		std::vector<RendererAnimatedTextureSet> 			   _animatedTextureSets;
		std::vector<RendererMesh*> 							   _meshes;
		std::vector<AtlasTexturesSet> 						   _roomTextures;
		std::vector<AtlasTexturesSet> 						   _animatedTextures;
		std::vector<AtlasTexturesSet> 						   _moveablesTextures;
		std::vector<AtlasTexturesSet> 						   _staticTextures;
		std::vector<Texture2D> 								   _spritesTextures;
		RendererSprite										   _videoSprite; // Video texture is an unique case

		Matrix _playerWorldMatrix;

		// Preallocated pools of objects for avoiding new/delete.
		// Items and effects are safe (can't be more than 1024 items in TR), 
		// lights should be oversized (eventually ignore lights more than MAX_LIGHTS)

		std::vector<RendererItem> _items;
		std::vector<RendererEffect> _effects;

		// Debug variables

		int _numDrawCalls = 0;
		int _numRoomDrawCalls = 0;
		int _numMoveableDrawCalls = 0;
		int _numStaticDrawCalls = 0;
		int _numSpriteDrawCalls = 0;
		int _numShadowMapDrawCalls = 0;
		int _numSortedRoomDrawCalls = 0;
		int _numSortedMoveablesDrawCalls = 0;
		int _numSortedStaticsDrawCalls = 0;
		int _numSortedSpriteDrawCalls = 0;
		int _numSortedTriangles = 0;
		int _numTriangles = 0;
		int _numGetVisibleRoomsCalls = 0;
		int _numCheckPortalCalls = 0;
		int _numDotProducts = 0;
		int _numConstantBufferUpdates = 0;
		int _numRequestedMaterialsUpdates = 0;
		int _numExecutedMaterialsUpdates = 0;

		int _timePrepare;
		int _timeDraw;
		int _timeFrame;
		float _fps;
		int _currentCausticsFrame;

		// Screen settings

		int _screenWidth;
		int _screenHeight;
		int _refreshRate;
		bool _isWindowed;
		float _farView = DEFAULT_FAR_VIEW;

		// A flag to prevent extra renderer object additions

		bool _isLocked = false;

		// Caching state changes

		TextureBase* _lastTexture;
		BlendMode _lastBlendMode;
		DepthState _lastDepthState;
		CullMode _lastCullMode;
		int _lastMaterialIndex;

		std::vector<RendererSpriteBucket> _spriteBuckets;

		ComPtr<ID3D11SamplerState> _shadowSampler;

		// Antialiasing

		Texture2D _SMAAAreaTexture;
		Texture2D _SMAASearchTexture;

		// Post-process

		PostProcessMode _postProcessMode = PostProcessMode::None;
		float _postProcessStrength = 1.0f;
		Vector3 _postProcessTint = Vector3::One;

		VertexBuffer<PostProcessVertex> _fullscreenTriangleVertexBuffer;
		ComPtr<ID3D11InputLayout> _fullscreenTriangleInputLayout = nullptr;

		bool _doingFullscreenPass = false;

		// SSAO

		Texture2D _SSAONoiseTexture;
		std::vector<Vector4> _SSAOKernel;

		// Special effects

		//std::vector<Texture2D> _causticTextures;
		RendererMirror* _currentMirror = nullptr;

		// Transparency

		fast_vector<Vertex> _sortedPolygonsVertices;
		fast_vector<int> _sortedPolygonsIndices;
		VertexBuffer<Vertex> _sortedPolygonsVertexBuffer;
		IndexBuffer _sortedPolygonsIndexBuffer;

		// High framerate

		float _interpolationFactor = 0.0f;
		bool  _graphicsSettingsChanged = false;

		// Shader manager

		ShaderManager _shaders;

		void CollectAdapterInfo();

		void ApplySMAA(RenderTarget2D* renderTarget, RenderView& view);
		void ApplyFXAA(RenderTarget2D* renderTarget, RenderView& view);
		void ApplyAntialiasing(RenderTarget2D* renderTarget, RenderView& view);
		void BindTexture(TextureRegister registerType, TextureBase* texture, SamplerStateRegister samplerType);
		int  BindLight(RendererLight& light, ShaderLight* lights, int index);
		void BindRoomLights(std::vector<RendererLight*>& lights);
		void BindInstancedStaticLights(std::vector<RendererLight*>& lights, int instanceID);
		void BindMoveableLights(std::vector<RendererLight*>& lights, int roomNumber, int prevRoomNumber, float fade, bool shadow);
		void BindRoomDecals(const std::vector<RendererDecal>& decals);
		void BindRenderTargetAsTexture(TextureRegister registerType, RenderTarget2D* target, SamplerStateRegister samplerType);
		void BindConstantBufferVS(ConstantBufferRegister constantBufferType, ID3D11Buffer** buffer);
		void BindConstantBufferPS(ConstantBufferRegister constantBufferType, ID3D11Buffer** buffer);
		void BindMaterial(int materialIndex, bool force);
		void BuildHierarchy(RendererObject* obj);
		void BuildHierarchyRecursive(RendererObject* obj, RendererBone* node, RendererBone* parentNode);
		void UpdateAnimation(RendererItem* item, RendererObject& obj, const KeyframeInterpolationData& interpData, int mask, bool useObjectWorldRotation = false);
		bool CheckPortal(short parentRoomNumber, RendererDoor* door, Vector4 viewPort, Vector4* clipPort, RenderView& renderView);
		void GetVisibleRooms(short from, short to, Vector4 viewPort, bool water, int count, bool onlyRooms, RenderView& renderView);
		void CollectMirrors(RenderView& renderView);
		void CollectRooms(RenderView& renderView, bool onlyRooms);
		void CollectItems(short roomNumber, RenderView& renderView);
		void CollectStatics(short roomNumber, RenderView& renderView);
		void CollectLights(const Vector3& pos, float radius, int roomNumber, int prevRoomNumber, bool prioritizeShadowLight, bool useCachedRoomLights, std::vector<RendererLightNode>* roomsLights, std::vector<RendererLight*>* outputLights);
		void CollectLightsForItem(RendererItem* item);
		void CollectLightsForEffect(short roomNumber, RendererEffect* effect);
		void CollectLightsForRoom(short roomNumber, RenderView& renderView);
		void CollectLightsForCamera();
		void CalculateLightFades(RendererItem* item);
		void CollectDecalsForRoom(short roomNumber, RenderView& renderView);
		void CollectEffects(short roomNumber, RenderView& renderView);
		void ClearShadowMap();
		void CalculateSSAO(RenderView& view);
		void UpdateItemAnimations(RenderView& view);
		void InitializeScreen(int w, int h, HWND handle, bool reset);
		void InitializeCommonTextures();
		void InitializeGameBars();
		void InitializeMenuBars(int y);
		void InitializeSky();
		void DrawAllStrings();
		void PrepareDynamicLight(RendererLight& light);
		void PrepareLaserBarriers(RenderView& view);
		void PrepareSingleLaserBeam(RenderView& view);
		void DrawHorizonAndSky(ID3D11DepthStencilView* depthStencilView, RenderView& renderView, bool reflectionPass = false);
		void DrawStarfield();
		void UpdateStarfieldBuffer();
		void DrawHorizonAndSkyForReflections(RenderView& renderView);
		void DrawRooms(RenderView& view, RendererPass rendererPass);
		void DrawItems(RenderView& view, RendererPass rendererPass, bool onlyPlayer = false);
		void DrawAnimatingItem(RendererItem* item, RenderView& view, RendererPass rendererPass);
		void DrawWaterfalls(RendererItem* item, RenderView& view, float speed, RendererPass rendererPass);
		void DrawBaddyGunflashes(RenderView& view);
		void DrawStatics(RenderView& view, RendererPass rendererPass);
		void DrawLara(RenderView& view, RendererPass rendererPass);
		void PrepareFires(RenderView& view);
		void PrepareParticles(RenderView& view);
		void PrepareSmokes(RenderView& view);
		void PrepareFireflies(RenderView& view);
		void PrepareElectricity(RenderView& view);
		void PrepareHelicalLasers(RenderView& view);
		void PrepareBlood(RenderView& view);
		void PrepareWeatherParticles(RenderView& view);
		void PrepareDrips(RenderView& view);
		void PrepareBubbles(RenderView& view);
		void DoRenderPass(RendererPass pass, RenderView& view, bool drawMirrors);
		void DrawObjects(RendererPass pass, RenderView& view, bool player, bool moveables, bool statics, bool sprites);
		void DrawEffects(RenderView& view, RendererPass rendererPass);
		void DrawEffect(RenderView& view, RendererEffect* effect, RendererPass rendererPass);
		void PrepareSplashes(RenderView& view);
		void DrawSprites(RenderView& view, RendererPass rendererPass);
		void DrawDisplaySprites(RenderView& view, bool negativePriority);
		void DrawDisplayItems();
		void DrawSortedFaces(RenderView& view);
		void DrawSingleSprite(RendererSortableObject* object, RendererObjectType lastObjectType, RenderView& view);
		void DrawRoomSorted(RendererSortableObject* objectInfo, RendererObjectType lastObjectType, RenderView& view);
		void DrawItemSorted(RendererSortableObject* objectInfo, RendererObjectType lastObjectType, RenderView& view);
		void DrawStaticSorted(RendererSortableObject* objectInfo, RendererObjectType lastObjectType, RenderView& view);
		void DrawSpriteSorted(RendererSortableObject* objectInfo, RendererObjectType lastObjectType, RenderView& view);
		void DrawMoveableAsStaticSorted(RendererSortableObject* objectInfo, RendererObjectType lastObjectType, RenderView& view);
		void DrawHairSorted(RendererSortableObject* objectInfo, RendererObjectType lastObjectType, RenderView& view, int index);
		void DrawLines2D();
		void DrawLines3D(RenderView& view);
		void DrawTriangles3D(RenderView& view);
		void DrawOverlays(RenderView& view);
		void PrepareRopes(RenderView& view);
		void DrawFishSwarm(RenderView& view, RendererPass rendererPass);
		void DrawBats(RenderView& view, RendererPass rendererPass);
		void DrawRats(RenderView& view, RendererPass rendererPass);
		void DrawScarabs(RenderView& view, RendererPass rendererPass);
		void DrawSpiders(RenderView& view, RendererPass rendererPass);
		bool DrawGunFlashes(RenderView& view);
		void DrawGunShells(RenderView& view, RendererPass rendererPass);
		void DrawLocusts(RenderView& view, RendererPass rendererPass);
		void DrawStatistics();
		void DrawExamines();
		void DrawDebris(RenderView& view, RendererPass rendererPass);
		void DrawFullScreenImage(ID3D11ShaderResourceView* texture, float fade, ID3D11RenderTargetView* target,
			ID3D11DepthStencilView* depthTarget);
		void PrepareShockwaves(RenderView& view);
		void PrepareRipples(RenderView& view);
		void PrepareUnderwaterBloodParticles(RenderView& view);
		void DrawFullScreenQuad(ID3D11ShaderResourceView* texture, Vector3 color, bool fit = true, float customAspect = 0.0f);
		void DrawFullScreenSprite(RendererSprite* sprite, DirectX::SimpleMath::Vector3 color, bool fit = true);
		void PrepareSmokeParticles(RenderView& view);
		void PrepareSparkParticles(RenderView& view);
		void PrepareExplosionParticles(RenderView& view);
		void DrawLaraHolsters(RendererItem* itemToDraw, RendererRoom* room, RenderView& view, RendererPass rendererPass);
		void DrawLaraJoints(RendererItem* itemToDraw, RendererRoom* room, RenderView& view, RendererPass rendererPass);
		void DrawLaraHair(RendererItem* itemToDraw, RendererRoom* room, RenderView& view, RendererPass rendererPass);
		void DrawMesh(RendererItem* itemToDraw, RendererMesh* mesh, RendererObjectType type, int boneIndex, bool skinned, RenderView& view, RendererPass rendererPass);
		void PrepareSimpleParticles(RenderView& view);
		void PrepareStreamers(RenderView& view);
		void PrepareFootprints(RenderView& view);
		void DrawLoadingBar(float percent);
		void DrawPostprocess(RenderTarget2D* renderTarget, RenderView& view, SceneRenderMode renderMode);
		void RenderInventoryScene(RenderTarget2D* renderTarget, TextureBase* background, float backgroundFade);
		void RenderTitleMenu(Menu menu);
		void RenderPauseMenu(Menu menu);
		void RenderLoadSaveMenu();
		void RenderOptionsMenu(Menu menu, int initialY);
		void RenderNewInventory();
		void RenderToCubemap(const RenderTargetCube& dest, const Vector3& pos, int roomNumber);
		void RenderBlobShadows(RenderView& renderView);
		void RenderShadowMap(RendererItem* item, RenderView& view);
		void RenderItemShadows(RenderView& renderView);
		void SetBlendMode(BlendMode blendMode, bool force = false);
		void SetDepthState(DepthState depthState, bool force = false);
		void SetCullMode(CullMode cullMode, bool force = false);
		void SetAlphaTest(AlphaTestMode mode, float threshold, bool force = false);
		void SetScissor(RendererRectangle rectangle);
		bool SetupBlendModeAndAlphaTest(BlendMode blendMode, RendererPass rendererPass, int drawPass);
		void SortAndPrepareSprites(RenderView& view);
		void SortTransparentFaces(RenderView& view);
		void ResetItems();
		void ResetScissor();
		void ResetDebugVariables();
		float CalculateFrameRate();
		void InterpolateCamera(float interpFactor);
		void CopyRenderTarget(RenderTarget2D* source, RenderTarget2D* dest, RenderView& view);
		void CopyRenderTargetAndDownscale(RenderTarget2D* source, RenderTarget2D* dest, float factor, RenderView& view);
		void BindBucketTextures(const RendererBucket& bucket, TextureSource textureSource, bool animated);
		void BindAtlasTextures(const RendererBucket& bucket, TextureSource textureSource);
		void PackSpriteTextureCoordinates(int instanceId, RendererSprite* sprite);
		void ApplyGlow(RenderTarget2D* renderTarget, RenderView& view);

		void AddSpriteBillboard(RendererSprite* sprite, const Vector3& pos, const Vector4& color, float orient2D, float scale,
			Vector2 size, BlendMode blendMode, bool isSoftParticle, RenderView& view, SpriteRenderType renderType = SpriteRenderType::Default);
		void AddSpriteBillboardConstrained(RendererSprite* sprite, const Vector3& pos, const Vector4& color, float orient2D,
			float scale, Vector2 size, BlendMode blendMode, const Vector3& constrainAxis,
			bool isSoftParticle, RenderView& view, SpriteRenderType renderType = SpriteRenderType::Default);
		void AddSpriteBillboardConstrainedLookAt(RendererSprite* sprite, const Vector3& pos, const Vector4& color, float orient2D,
			float scale, Vector2 size, BlendMode blendMode, const Vector3& lookAtAxis,
			bool isSoftParticle, RenderView& view, SpriteRenderType renderType = SpriteRenderType::Default);
		void AddQuad(RendererSprite* sprite, const Vector3& vertex0, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3,
			const Vector4 color, float orient2D, float scale, Vector2 size, BlendMode blendMode, bool softParticles,
			RenderView& view);
		void AddQuad(RendererSprite* sprite, const Vector3& vertex0, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3,
			const Vector4& color0, const Vector4& color1, const Vector4& color2, const Vector4& color3, float orient2D,
			float scale, Vector2 size, BlendMode blendMode, bool isSoftParticle, RenderView& view, SpriteRenderType renderType = SpriteRenderType::Default);
		void AddColoredQuad(const Vector3& vertex0, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3,
			const Vector4& color, BlendMode blendMode, RenderView& view);
		void AddColoredQuad(const Vector3& vertex0, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3,
			const Vector4& color0, const Vector4& color1, const Vector4& color2, const Vector4& color3,
			BlendMode blendMode, RenderView& view, SpriteRenderType renderType = SpriteRenderType::Default);

		Matrix GetWorldMatrixForSprite(const RendererSpriteToDraw& sprite, RenderView& view);
		RendererObject& GetRendererObject(GAME_OBJECT_ID id);
		RendererMesh* GetMesh(int meshIndex);
		Vector4 GetPortalRect(Vector4 v, Vector4 vp);
		float GetFramerateMultiplier() const;
		float GetInterpolationFactor(bool forceRawValue = false) const;
		std::optional<Vector2> Get2DPosition(const Vector3& pos) const;
		std::pair<Vector3, Vector3> GetRay(const Vector2& pos) const;
		Vector3 GetMoveableBonePosition(int itemNumber, int boneID, const Vector3& relOffset);
		Quaternion GetMoveableBoneOrientation(int itemNumber, int boneID);
		bool IsRoomReflected(RenderView& renderView, int roomNumber);
		void SaveScreenshot();
		void SortTransparentPolygons(RenderView& view, int indexCount, int index, RendererObjectType objectType,
			RendererRoom* room = nullptr, RendererItem* item = nullptr, RendererStatic* staticMesh = nullptr, RendererEffect* effect = nullptr,
			int boneIndex = 0, int indexInPolyList = 0, Matrix world = Matrix::Identity, float depth = 0, int materialIndex = 0,
			LightMode lightMode = LightMode::Dynamic, bool animated = false, bool skinned = false);
		void SwapDynamicLightBuffer();
		std::optional<Vector2> ProjectDisplayItemPointToScreen(const Vector3& worldPos) const;
		std::optional<std::pair<Vector2, Vector2>> GetDisplayItemBounds(const DisplayItem& item) const;

	public:
		Renderer();
		~Renderer();

		void Initialize();
		void Render();
		void RenderLoadingScreen(float value);
		void UpdateProgress(float value);
		void Lock();
		void UpdateVideoTexture(Texture2D* texture);
		void UpdateCameraMatrices(CAMERA_INFO* cam, float farView);
		void UpdateItemAnimations(int itemNumber, bool force);
		void UpdateLaraAnimations(bool force);
		void GetBoneMatrix(short itemNumber, int jointIndex, Matrix* outMatrix);
		SkinningMode GetSkinningMode(const RendererObject& obj, int skinIndex);
		void CollectRendererDataForLevel(int numItems, int numEffects);
		void FreeRendererData();
		void FreeLevelResources();
		void PrepareDataForTheRenderer();
		void SwapAnimatedTextures();
		void UpdateRendererRoom(int roomNumber, bool updateFliprooms);
		void FlipRooms(short roomNumber1, short roomNumber2);
		void SaveOldState();
		void SetGraphicsSettingsChanged();
		void AddDynamicPointLight(const Vector3& position, const Vector3& color, float radius, float intensity, bool castShadows, int hash);
		void AddDynamicSpotLight(const Vector3& position, const Vector3& direction, const Vector3& color, float radius, float falloff, float distance, float intensity, bool castShadows, int hash);
		void AddDynamicFogBulb(const Vector3& position, const Vector3& color, float radius, float density);
		void AddDisplaySprite(const std::shared_ptr<DisplaySprite> sprite);
		bool SphereBoxIntersection(BoundingBox box, Vector3 sphereCentre, float sphereRadius);
		RendererSphereView GetSpheres(int itemNumber);
		void RenderBar(int x, int y, int width, int height, int barHeight, int value, int maxValue, Vector4 color1, Vector4 color2, Vector4 color3, HudBarMode mode, HudBarType type, BarConfiguration config, long timer = 0);
		void AddString(StringToDraw* stringToDraw);
		void AddLine2D(Line2D line);
		void AddLine3D(Line3D line);
		void AddTriangle3D(Triangle3D triangle);
		void ReloadShaders(bool recompileAAShaders = false);
		void DrawDebugPage();
		void TakeScreenshot();

		RendererDebugPage GetDebugPage() const;
		int GetScreenWidth() const { return _screenWidth; };
		int GetScreenHeight() const { return _screenHeight; };
		int GetScreenRefreshRate();
		const AdapterInfo& GetAdapterInfo() const { return _adapterInfo; }
		int GetCurrentCausticsFrame() const { return _currentCausticsFrame; };
		int GetCausticsTextureCount() const { return _animatedTextures.empty() ? 0 : (int)std::get<0>(_animatedTextures[0]).NumTextures; };
		ID3D11Device* GetDevice() { return _device.Get(); };
		ID3D11DeviceContext* GetContext() { return _context.Get(); };
		RenderTarget2D& GetBackBuffer() { return _backBuffer; };
		Viewport* GetViewport() { return &_viewportToolkit; };
	};

	extern Renderer g_Renderer;
}
