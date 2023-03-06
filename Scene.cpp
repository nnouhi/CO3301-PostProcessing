//--------------------------------------------------------------------------------------
// Scene geometry and layout preparation
// Scene rendering & update
//--------------------------------------------------------------------------------------

#include "Scene.h"
#include "Mesh.h"
#include "Model.h"
#include "Camera.h"
#include "State.h"
#include "Shader.h"
#include "Input.h"
#include "Common.h"

#include "CVector2.h" 
#include "CVector3.h" 
#include "CMatrix4x4.h"
#include "MathHelpers.h"     // Helper functions for maths
#include "GraphicsHelpers.h" // Helper functions to unclutter the code here
#include "ColourRGBA.h" 

#include <array>
#include <sstream>
#include <memory>

#include <map>
#include <vector>
#include <stack>


//--------------------------------------------------------------------------------------
// Scene Data
//--------------------------------------------------------------------------------------

//********************
// Available post-processes
enum class PostProcess
{
	None,
	NightVision,
	VerticalColourGradient,
	GaussianBlurHorizontal,
	GaussianBlurVertical,
	UnderWater,
	HueVerticalColourGradient,
	Sepia,
	Inverted,
	Contour,
	GameBoy,
	Bloom,
	MergeTextures,
	Dilation,
	DualFiltering,
	DepthOfField,

	Copy,
	Tint,
	GreyNoise,
	Burn,
	Distort,
	Spiral,
	HeatHaze,
};

enum class PostProcessMode
{
	Fullscreen,
	Area,
	Polygon,
};

auto gCurrentPostProcess     = PostProcess::None;
auto gCurrentPostProcessMode = PostProcessMode::Fullscreen;
std::vector<std::pair<PostProcess, PostProcessMode>> gPostProcessAndModeStack;
std::vector<PostProcess> windowPostProcesses;
const int NUM_OF_WINDOWS = 4;
//********************


// Constants controlling speed of movement/rotation (measured in units per second because we're using frame time)
const float ROTATION_SPEED = 1.5f;  // Radians per second for rotation
const float MOVEMENT_SPEED = 50.0f; // Units per second for movement (what a unit of length is depends on 3D model - i.e. an artist decision usually)

// Lock FPS to monitor refresh rate, which will typically set it to 60fps. Press 'p' to toggle to full fps
bool lockFPS = true;
//std::map<PostProcess, bool> isActivePostProcessMap
//{
//	{PostProcess::NightVision, false},
//	{PostProcess::GaussianBlurHorizontal, false},
//	{PostProcess::GaussianBlurVertical, false},
//	{PostProcess::VerticalColourGradient, false},
//	{PostProcess::UnderWater, false},
//	{PostProcess::HueVerticalColourGradient, false},
//	{PostProcess::Copy, false},
//	{PostProcess::Tint, false},
//	{PostProcess::GreyNoise, false},
//	{PostProcess::Burn, false},
//	{PostProcess::Distort, false},
//	{PostProcess::Spiral, false},
//	{PostProcess::HeatHaze, false},
//
//};



// Meshes, models and cameras, same meaning as TL-Engine. Meshes prepared in InitGeometry function, Models & camera in InitScene
Mesh* gStarsMesh;
Mesh* gGroundMesh;
Mesh* gCubeMesh;
Mesh* gCrateMesh;
Mesh* gLightMesh;
Mesh* gWallMesh;

Model* gStars;
Model* gGround;
Model* gCube;
Model* gCrate;
Model* gWall;

Camera* gCamera;


// Store lights in an array in this exercise
const int NUM_LIGHTS = 2;
struct Light
{
	Model*   model;
	CVector3 colour;
	float    strength;
};
Light gLights[NUM_LIGHTS];


// Additional light information
CVector3 gAmbientColour = { 0.3f, 0.3f, 0.4f }; // Background level of light (slightly bluish to match the far background, which is dark blue)
float    gSpecularPower = 256; // Specular power controls shininess - same for all models in this app

ColourRGBA gBackgroundColor = { 0.3f, 0.3f, 0.4f, 1.0f };

// Variables controlling light1's orbiting of the cube
const float gLightOrbitRadius = 20.0f;
const float gLightOrbitSpeed = 0.7f;



//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
// Variables sent over to the GPU each frame
// The structures are now in Common.h
// IMPORTANT: Any new data you add in C++ code (CPU-side) is not automatically available to the GPU
//            Anything the shaders need (per-frame or per-model) needs to be sent via a constant buffer

PerFrameConstants gPerFrameConstants;      // The constants (settings) that need to be sent to the GPU each frame (see common.h for structure)
ID3D11Buffer*     gPerFrameConstantBuffer; // The GPU buffer that will recieve the constants above

PerModelConstants gPerModelConstants;      // As above, but constants (settings) that change per-model (e.g. world matrix)
ID3D11Buffer*     gPerModelConstantBuffer; // --"--

//**************************
PostProcessingConstants gPostProcessingConstants;       // As above, but constants (settings) for each post-process
ID3D11Buffer*           gPostProcessingConstantBuffer; // --"--
//**************************


//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

// DirectX objects controlling textures used in this lab
ID3D11Resource*           gStarsDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gStarsDiffuseSpecularMapSRV = nullptr;
ID3D11Resource*           gGroundDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gGroundDiffuseSpecularMapSRV = nullptr;
ID3D11Resource*           gCrateDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gCrateDiffuseSpecularMapSRV = nullptr;
ID3D11Resource*           gCubeDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gCubeDiffuseSpecularMapSRV = nullptr;
ID3D11Resource*			  gWallDifuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gWallDifuseSpecularMapSRV = nullptr;

ID3D11Resource*           gLightDiffuseMap = nullptr;
ID3D11ShaderResourceView* gLightDiffuseMapSRV = nullptr;

/* NOTE TO SELF

The relation between a render target view (RTV) and a shader resource view (SRV) is that they both reference the same underlying texture resource in GPU memory, but with different access and usage semantics.

A render target view (RTV) is a view object that is bound to a texture resource in GPU memory and allows rendering to that texture. It provides a way for the graphics pipeline to write data to the texture, such as pixel colors, depth values, or stencil values, during the rendering process.

A shader resource view (SRV) is a view object that is bound to a texture resource in GPU memory and allows shaders to access the contents of that texture. It provides a way for the graphics pipeline to read data from the texture, such as pixel colors or depth values, during the rendering process.

In the context of post-processing, a typical workflow involves rendering the scene to a texture using an RTV, then using an SRV to access the texture in a post-processing shader that applies some effect to the rendered image.

This allows the post-processing shader to read the colors or other data from the rendered image and manipulate it in some way before outputting the final result to the screen.


The SRV does not get modified directly by the pixel shader.

Instead, the pixel shader reads from the SRV, which is connected to a render target through the output merger stage.

When the scene is rendered to the render target, the output from the pixel shader is written to the render target, which updates the texture stored in the GPU memory.

The updated texture is then passed to the SRV through the shader resource view.

In this way, the SRV is indirectly updated when the scene is rendered to the render target.

*/

//****************************
// Post processing textures

// Dimensions of shadow map texture - controls quality of shadows
int gShadowMapSize = 1024;

// The shadow texture - effectively a depth buffer of the scene **from the light's point of view**
//                      Each frame it is rendered to, then the texture is used to help the per-pixel lighting shader identify pixels in shadow
ID3D11Texture2D*		  gShadowMap1Texture = nullptr; // This object represents the memory used by the texture on the GPU
ID3D11DepthStencilView*   gShadowMap1DepthStencil = nullptr; // This object is used when we want to render to the texture above **as a depth buffer**
ID3D11ShaderResourceView* gShadowMap1SRV = nullptr; // This object is used to give shaders access to the texture above (SRV = shader resource view)

// This texture will have the scene renderered on it. Then the texture is then used for post-processing
ID3D11Texture2D*          gSceneTexture      = nullptr; // This object represents the memory used by the texture on the GPU
ID3D11RenderTargetView*   gSceneRenderTarget = nullptr; // This object is used when we want to render to the texture above
ID3D11ShaderResourceView* gSceneTextureSRV   = nullptr; // This object is used to give shaders access to the texture above (SRV = shader resource view)

// This texture will have the scene renderered on it. Then the texture is then used for post-processing
ID3D11Texture2D*          gSceneTextureTwo = nullptr; // This object represents the memory used by the texture on the GPU
ID3D11RenderTargetView*   gSceneRenderTargetTwo = nullptr; // This object is used when we want to render to the texture above
ID3D11ShaderResourceView* gSceneTextureSRVTwo = nullptr; // This object is used to give shaders access to the texture above (SRV = shader resource view)

// This texture will have the scene renderered on it. Then the texture is then used for post-processing
ID3D11Texture2D*          gSceneTextureCopy = nullptr; // This object represents the memory used by the texture on the GPU
ID3D11RenderTargetView*   gSceneRenderTargetCopy = nullptr; // This object is used when we want to render to the texture above
ID3D11ShaderResourceView* gSceneTextureSRVCopy = nullptr; // This object is used to give shaders access to the texture above (SRV = shader resource view)

// Additional textures used for specific post-processes
ID3D11Resource*			  gStarLensMap = nullptr;
ID3D11ShaderResourceView* gStarLensMapSRV = nullptr;
ID3D11Resource*           gNoiseMap = nullptr;
ID3D11ShaderResourceView* gNoiseMapSRV = nullptr;
ID3D11Resource*           gBurnMap = nullptr;
ID3D11ShaderResourceView* gBurnMapSRV = nullptr;
ID3D11Resource*           gDistortMap = nullptr;
ID3D11ShaderResourceView* gDistortMapSRV = nullptr;

ID3D11ShaderResourceView* nullSRV = nullptr;


//****************************

// Helper method signatures
void SaveCurrentSceneToTexture(int index);
void AddProcessAndMode(PostProcess process, PostProcessMode mode);
void RemoveProcessAndMode();
std::array<CVector3, 4> GetWindowPoint(int windowIndex);
void CreateWindowPostProcesses(std::vector<PostProcess> windowPostProcesses);

//--------------------------------------------------------------------------------------
// Light Helper Functions
//--------------------------------------------------------------------------------------

// Get "camera-like" view matrix for a spotlight
CMatrix4x4 CalculateLightViewMatrix(int lightIndex)
{
	return InverseAffine(gLights[lightIndex].model->WorldMatrix());
}

// Get "camera-like" projection matrix for a spotlight
CMatrix4x4 CalculateLightProjectionMatrix(int lightIndex)
{
	return MakeProjectionMatrix(1.0f, ToRadians(90)); // Helper function in Utility\GraphicsHelpers.cpp
}

//--------------------------------------------------------------------------------------
// Initialise scene geometry, constant buffers and states
//--------------------------------------------------------------------------------------

// Prepare the geometry required for the scene
// Returns true on success
bool InitGeometry()
{
	////--------------- Load meshes ---------------////

	// Load mesh geometry data, just like TL-Engine this doesn't create anything in the scene. Create a Model for that.
	try
	{
		gStarsMesh  = new Mesh("Stars.x");
		gGroundMesh = new Mesh("Floor.x");
		gCubeMesh   = new Mesh("Cube.x");
		gCrateMesh  = new Mesh("CargoContainer.x");
		gLightMesh = new Mesh("Light.x");
		gWallMesh  = new Mesh("Wall2.x");
	}
	catch (std::runtime_error e)  // Constructors cannot return error messages so use exceptions to catch mesh errors (fairly standard approach this)
	{
		gLastError = e.what(); // This picks up the error message put in the exception (see Mesh.cpp)
		return false;
	}


	////--------------- Load / prepare textures & GPU states ---------------////

	// Load textures and create DirectX objects for them
	// The LoadTexture function requires you to pass a ID3D11Resource* (e.g. &gCubeDiffuseMap), which manages the GPU memory for the
	// texture and also a ID3D11ShaderResourceView* (e.g. &gCubeDiffuseMapSRV), which allows us to use the texture in shaders
	// The function will fill in these pointers with usable data. The variables used here are globals found near the top of the file.
	if (!LoadTexture("Stars.jpg",                &gStarsDiffuseSpecularMap,  &gStarsDiffuseSpecularMapSRV) ||
		!LoadTexture("GrassDiffuseSpecular.dds", &gGroundDiffuseSpecularMap, &gGroundDiffuseSpecularMapSRV) ||
		!LoadTexture("StoneDiffuseSpecular.dds", &gCubeDiffuseSpecularMap, &gCubeDiffuseSpecularMapSRV) ||
		!LoadTexture("brick_35.jpg", &gWallDifuseSpecularMap,   &gWallDifuseSpecularMapSRV) ||
		!LoadTexture("CargoA.dds",               &gCrateDiffuseSpecularMap,  &gCrateDiffuseSpecularMapSRV) ||
		!LoadTexture("Flare.jpg",                &gLightDiffuseMap,          &gLightDiffuseMapSRV) ||
		!LoadTexture("Noise.png",                &gNoiseMap,   &gNoiseMapSRV) ||
		!LoadTexture("Flare.jpg",				 &gStarLensMap, &gStarLensMapSRV) ||
		!LoadTexture("Burn.png",                 &gBurnMap,    &gBurnMapSRV) ||
		!LoadTexture("Distort.png",              &gDistortMap, &gDistortMapSRV))
	{
		gLastError = "Error loading textures";
		return false;
	}


	// Create all filtering modes, blending modes etc. used by the app (see State.cpp/.h)
	if (!CreateStates())
	{
		gLastError = "Error creating states";
		return false;
	}


	////--------------- Prepare shaders and constant buffers to communicate with them ---------------////

	// Load the shaders required for the geometry we will use (see Shader.cpp / .h)
	if (!LoadShaders())
	{
		gLastError = "Error loading shaders";
		return false;
	}

	// Create GPU-side constant buffers to receive the gPerFrameConstants and gPerModelConstants structures above
	// These allow us to pass data from CPU to shaders such as lighting information or matrices
	// See the comments above where these variable are declared and also the UpdateScene function
	gPerFrameConstantBuffer       = CreateConstantBuffer(sizeof(gPerFrameConstants));
	gPerModelConstantBuffer       = CreateConstantBuffer(sizeof(gPerModelConstants));
	gPostProcessingConstantBuffer = CreateConstantBuffer(sizeof(gPostProcessingConstants));
	if (gPerFrameConstantBuffer == nullptr || gPerModelConstantBuffer == nullptr || gPostProcessingConstantBuffer == nullptr)
	{
		gLastError = "Error creating constant buffers";
		return false;
	}

	//********************************************
	//**** Create Scene Texture

	// We will render the scene to this texture instead of the back-buffer (screen), then we post-process the texture onto the screen
	// This is exactly the same code we used in the graphics module when we were rendering the scene onto a cube using a texture

	// Using a helper function to load textures from files above. Here we create the scene texture manually
	// as we are creating a special kind of texture (one that we can render to). Many settings to prepare:
	D3D11_TEXTURE2D_DESC sceneTextureDesc = {};
	sceneTextureDesc.Width = gViewportWidth;  // Full-screen post-processing - use full screen size for texture
	sceneTextureDesc.Height = gViewportHeight;
	sceneTextureDesc.MipLevels = 1; // No mip-maps when rendering to textures (or we would have to render every level)
	sceneTextureDesc.ArraySize = 1;
	//sceneTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA texture (8-bits each)
	sceneTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // Enables HDR (Comment to disable and uncomment above line)
	sceneTextureDesc.SampleDesc.Count = 1;
	sceneTextureDesc.SampleDesc.Quality = 0;
	sceneTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	sceneTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; // IMPORTANT: Indicate we will use texture as render target, and pass it to shaders
	sceneTextureDesc.CPUAccessFlags = 0;
	sceneTextureDesc.MiscFlags = 0;
	if (FAILED(gD3DDevice->CreateTexture2D(&sceneTextureDesc, NULL, &gSceneTexture)))
	{
		gLastError = "Error creating scene texture";
		return false;
	}

	// Second texture
	if (FAILED(gD3DDevice->CreateTexture2D(&sceneTextureDesc, NULL, &gSceneTextureTwo)))
	{
		gLastError = "Error creating scene texture two";
		return false;
	}

	// Copy texture
	if (FAILED(gD3DDevice->CreateTexture2D(&sceneTextureDesc, NULL, &gSceneTextureCopy)))
	{
		gLastError = "Error creating scene texture two";
		return false;
	}

	// We created the scene texture above, now we get a "view" of it as a render target, i.e. get a special pointer to the texture that
	// we use when rendering to it (see RenderScene function below)
	if (FAILED(gD3DDevice->CreateRenderTargetView(gSceneTexture, NULL, &gSceneRenderTarget)))
	{
		gLastError = "Error creating scene render target view";
		return false;
	}

	// Second texture
	if (FAILED(gD3DDevice->CreateRenderTargetView(gSceneTextureTwo, NULL, &gSceneRenderTargetTwo)))
	{
		gLastError = "Error creating scene render target view";
		return false;
	}

	// Copy texture
	if (FAILED(gD3DDevice->CreateRenderTargetView(gSceneTextureCopy, NULL, &gSceneRenderTargetCopy)))
	{
		gLastError = "Error creating scene render target view";
		return false;
	}

	// We also need to send this texture (resource) to the shaders. To do that we must create a shader-resource "view"
	D3D11_SHADER_RESOURCE_VIEW_DESC srDesc = {};
	srDesc.Format = sceneTextureDesc.Format;
	srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srDesc.Texture2D.MostDetailedMip = 0;
	srDesc.Texture2D.MipLevels = 1;
	if (FAILED(gD3DDevice->CreateShaderResourceView(gSceneTexture, &srDesc, &gSceneTextureSRV)))
	{
		gLastError = "Error creating scene shader resource view";
		return false;
	}

	// Second texture
	if (FAILED(gD3DDevice->CreateShaderResourceView(gSceneTextureTwo, &srDesc, &gSceneTextureSRVTwo)))
	{
		gLastError = "Error creating scene shader resource view";
		return false;
	}

	// Copy texture
	if (FAILED(gD3DDevice->CreateShaderResourceView(gSceneTextureCopy, &srDesc, &gSceneTextureSRVCopy)))
	{
		gLastError = "Error creating scene shader resource view";
		return false;
	}



	//**** Create Shadow Map texture ****//

	// We also need a depth buffer to go with our portal
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = gShadowMapSize; // Size of the shadow map determines quality / resolution of shadows
	textureDesc.Height = gShadowMapSize;
	textureDesc.MipLevels = 1; // 1 level, means just the main texture, no additional mip-maps. Usually don't use mip-maps when rendering to textures (or we would have to render every level)
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32_TYPELESS; // The shadow map contains a single 32-bit value [tech gotcha: have to say typeless because depth buffer and shaders see things slightly differently]
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D10_BIND_DEPTH_STENCIL | D3D10_BIND_SHADER_RESOURCE; // Indicate we will use texture as a depth buffer and also pass it to shaders
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	if (FAILED(gD3DDevice->CreateTexture2D(&textureDesc, NULL, &gShadowMap1Texture)))
	{
		gLastError = "Error creating shadow map texture";
		return false;
	}

	// Create the depth stencil view, i.e. indicate that the texture just created is to be used as a depth buffer
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // See "tech gotcha" above. The depth buffer sees each pixel as a "depth" float
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	dsvDesc.Flags = 0;
	if (FAILED(gD3DDevice->CreateDepthStencilView(gShadowMap1Texture, &dsvDesc, &gShadowMap1DepthStencil)))
	{
		gLastError = "Error creating shadow map depth stencil view";
		return false;
	}


	// We also need to send this texture (resource) to the shaders. To do that we must create a shader-resource "view"
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT; // See "tech gotcha" above. The shaders see textures as colours, so shadow map pixels are not seen as depths
	// but rather as "red" floats (one float taken from RGB). Although the shader code will use the value as a depth
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	if (FAILED(gD3DDevice->CreateShaderResourceView(gShadowMap1Texture, &srvDesc, &gShadowMap1SRV)))
	{
		gLastError = "Error creating shadow map shader resource view";
		return false;
	}


	return true;
}


// Prepare the scene
// Returns true on success
bool InitScene()
{
	////--------------- Set up scene ---------------////

	gStars  = new Model(gStarsMesh);
	gGround = new Model(gGroundMesh);
	gCube   = new Model(gCubeMesh);
	gCrate  = new Model(gCrateMesh);
	gWall	= new Model(gWallMesh);

	// Initial positions
	gCube->SetPosition({ 42, 5, -10 });
	gCube->SetRotation({ 0.0f, ToRadians(-110.0f), 0.0f });
	gCube->SetScale(1.5f);
	gCrate->SetPosition({ -10, 0, 90 });
	gCrate->SetRotation({ 0.0f, ToRadians(40.0f), 0.0f });
	gCrate->SetScale(6.0f);
	gStars->SetScale(8000.0f);
	gWall->SetPosition({ 50, 0, -50 });
	gWall->SetRotation({ 0.0f, ToRadians(-180.0f), 0.0f });
	gWall->SetScale(50.0f);


	// Light set-up - using an array this time
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		gLights[i].model = new Model(gLightMesh);
	}

	gLights[0].colour = { 0.8f, 0.8f, 1.0f };
	gLights[0].strength = 10;
	gLights[0].model->SetPosition({ 30, 10, 0 });
	gLights[0].model->SetScale(pow(gLights[0].strength, 1.0f)); // Convert light strength into a nice value for the scale of the light - equation is ad-hoc.

	gLights[1].colour = { 1.0f, 0.8f, 0.2f };
	gLights[1].strength = 40;
	gLights[1].model->SetPosition({ -70, 30, 100 });
	gLights[1].model->SetScale(pow(gLights[1].strength, 1.0f));


	////--------------- Set up camera ---------------////

	gCamera = new Camera();
	gCamera->SetPosition({ 125, 20, 75 });
	gCamera->SetRotation({ ToRadians(10.0f), ToRadians(270.0f), 0.0f });

	// Create post processes for windows
	windowPostProcesses.push_back(PostProcess::NightVision);
	windowPostProcesses.push_back(PostProcess::Contour);
	windowPostProcesses.push_back(PostProcess::Sepia);
	windowPostProcesses.push_back(PostProcess::Inverted);
	
	CreateWindowPostProcesses(windowPostProcesses);

	return true;
}


// Release the geometry and scene resources created above
void ReleaseResources()
{
	ReleaseStates();

	if (gShadowMap1DepthStencil)  gShadowMap1DepthStencil->Release();
	if (gShadowMap1SRV)           gShadowMap1SRV->Release();
	if (gShadowMap1Texture)       gShadowMap1Texture->Release();

	if (gSceneTextureSRV)              gSceneTextureSRV->Release();
	if (gSceneRenderTarget)            gSceneRenderTarget->Release();
	if (gSceneTexture)                 gSceneTexture->Release();

	if (gSceneTextureSRVTwo)              gSceneTextureSRVTwo->Release();
	if (gSceneRenderTargetTwo)            gSceneRenderTargetTwo->Release();
	if (gSceneTextureTwo)                 gSceneTextureTwo->Release();

	if (gDistortMapSRV)                gDistortMapSRV->Release();
	if (gDistortMap)                   gDistortMap->Release();
	if (gBurnMapSRV)                   gBurnMapSRV->Release();
	if (gBurnMap)                      gBurnMap->Release();
	if (gNoiseMapSRV)                  gNoiseMapSRV->Release();
	if (gNoiseMap)                     gNoiseMap->Release();
	if (gStarLensMapSRV)                  gStarLensMapSRV->Release();
	if (gStarLensMap)                     gStarLensMap->Release();


	if (gLightDiffuseMapSRV)           gLightDiffuseMapSRV->Release();
	if (gLightDiffuseMap)              gLightDiffuseMap->Release();
	if (gCrateDiffuseSpecularMapSRV)   gCrateDiffuseSpecularMapSRV->Release();
	if (gCrateDiffuseSpecularMap)      gCrateDiffuseSpecularMap->Release();
	if (gCubeDiffuseSpecularMapSRV)    gCubeDiffuseSpecularMapSRV->Release();
	if (gCubeDiffuseSpecularMap)       gCubeDiffuseSpecularMap->Release();
	if (gWallDifuseSpecularMapSRV)       gWallDifuseSpecularMapSRV->Release();
	if (gWallDifuseSpecularMap)    gWallDifuseSpecularMap->Release();
	if (gGroundDiffuseSpecularMapSRV)  gGroundDiffuseSpecularMapSRV->Release();
	if (gGroundDiffuseSpecularMap)     gGroundDiffuseSpecularMap->Release();
	if (gStarsDiffuseSpecularMapSRV)   gStarsDiffuseSpecularMapSRV->Release();
	if (gStarsDiffuseSpecularMap)      gStarsDiffuseSpecularMap->Release();

	if (gPostProcessingConstantBuffer)  gPostProcessingConstantBuffer->Release();
	if (gPerModelConstantBuffer)        gPerModelConstantBuffer->Release();
	if (gPerFrameConstantBuffer)        gPerFrameConstantBuffer->Release();

	ReleaseShaders();

	// See note in InitGeometry about why we're not using unique_ptr and having to manually delete
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		delete gLights[i].model;  gLights[i].model = nullptr;
	}
	delete gCamera;  gCamera = nullptr;
	delete gCrate;   gCrate = nullptr;
	delete gCube;    gCube = nullptr;
	delete gGround;  gGround = nullptr;
	delete gStars;   gStars = nullptr;
	delete gWall;	 gWall = nullptr;

	delete gLightMesh;   gLightMesh = nullptr;
	delete gCrateMesh;   gCrateMesh = nullptr;
	delete gCubeMesh;    gCubeMesh = nullptr;
	delete gWallMesh;    gWallMesh = nullptr;
	delete gGroundMesh;  gGroundMesh = nullptr;
	delete gStarsMesh;   gStarsMesh = nullptr;
}



//--------------------------------------------------------------------------------------
// Scene Rendering
//--------------------------------------------------------------------------------------

void RenderDepthBufferFromCamera(Camera* camera)
{
	// Set camera matrices in the constant buffer and send over to GPU
	/*gPerFrameConstants.cameraMatrix = camera->WorldMatrix();
	gPerFrameConstants.viewMatrix = camera->ViewMatrix();
	gPerFrameConstants.projectionMatrix = camera->ProjectionMatrix();
	gPerFrameConstants.viewProjectionMatrix = camera->ViewProjectionMatrix();
	UpdateConstantBuffer(gPerFrameConstantBuffer, gPerFrameConstants);*/

	// Get camera-like matrices from the spotlight, seet in the constant buffer and send over to GPU
	gPerFrameConstants.viewMatrix = CalculateLightViewMatrix(0);
	gPerFrameConstants.projectionMatrix = CalculateLightProjectionMatrix(0);
	gPerFrameConstants.viewProjectionMatrix = gPerFrameConstants.viewMatrix * gPerFrameConstants.projectionMatrix;
	UpdateConstantBuffer(gPerFrameConstantBuffer, gPerFrameConstants);


	// Indicate that the constant buffer we just updated is for use in the vertex shader (VS), geometry shader (GS) and pixel shader (PS)
	gD3DContext->VSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer); // First parameter must match constant buffer number in the shader 
	gD3DContext->PSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);

	// Depth only technique
	gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gPixelDepthPixelShader, nullptr, 0);

	// States - no blending, normal depth buffer and back-face culling (standard set-up for opaque models)
	gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gUseDepthBufferState, 0);
	gD3DContext->RSSetState(gCullBackState);	

	gGround->Render();
	gCrate->Render();
	gCube->Render();
	gWall->Render();
	gStars->Render();

	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		gLights[i].model->Render();
	}
}

// Render everything in the scene from the given camera
void RenderSceneFromCamera(Camera* camera)
{
	// Set camera matrices in the constant buffer and send over to GPU
	gPerFrameConstants.cameraMatrix = camera->WorldMatrix();
	gPerFrameConstants.viewMatrix = camera->ViewMatrix();
	gPerFrameConstants.projectionMatrix = camera->ProjectionMatrix();
	gPerFrameConstants.viewProjectionMatrix = camera->ViewProjectionMatrix();
	UpdateConstantBuffer(gPerFrameConstantBuffer, gPerFrameConstants);

	// Indicate that the constant buffer we just updated is for use in the vertex shader (VS), geometry shader (GS) and pixel shader (PS)
	gD3DContext->VSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer); // First parameter must match constant buffer number in the shader 
	gD3DContext->GSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);
	gD3DContext->PSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);

	gD3DContext->PSSetShader(gPixelLightingPixelShader, nullptr, 0);


	////--------------- Render ordinary models ---------------///

	// Select which shaders to use next
	gD3DContext->VSSetShader(gPixelLightingVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gPixelLightingPixelShader, nullptr, 0);
	gD3DContext->GSSetShader(nullptr, nullptr, 0);  // Switch off geometry shader when not using it (pass nullptr for first parameter)

	// States - no blending, normal depth buffer and back-face culling (standard set-up for opaque models)
	gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gUseDepthBufferState, 0);
	gD3DContext->RSSetState(gCullBackState);

	// Render lit models, only change textures for each onee
	gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);

	gD3DContext->PSSetShaderResources(0, 1, &gGroundDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gGround->Render();

	gD3DContext->PSSetShaderResources(0, 1, &gCrateDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gCrate->Render();

	gD3DContext->PSSetShaderResources(0, 1, &gCubeDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gCube->Render();

	gD3DContext->PSSetShaderResources(0, 1, &gWallDifuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gWall->Render();


	////--------------- Render sky ---------------////

	// Select which shaders to use next
	gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gTintedTexturePixelShader, nullptr, 0);

	// Using a pixel shader that tints the texture - don't need a tint on the sky so set it to white
	gPerModelConstants.objectColour = { 1, 1, 1 };

	// Stars point inwards
	gD3DContext->RSSetState(gCullNoneState);

	// Render sky
	gD3DContext->PSSetShaderResources(0, 1, &gStarsDiffuseSpecularMapSRV);
	gStars->Render();



	////--------------- Render lights ---------------////

	// Select which shaders to use next (actually same as before, so we could skip this)
	gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gTintedTexturePixelShader, nullptr, 0);

	// Select the texture and sampler to use in the pixel shader
	gD3DContext->PSSetShaderResources(0, 1, &gLightDiffuseMapSRV); // First parameter must match texture slot number in the shaer

	// States - additive blending, read-only depth buffer and no culling (standard set-up for blending)
	gD3DContext->OMSetBlendState(gAdditiveBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
	gD3DContext->RSSetState(gCullNoneState);

	// Render all the lights in the array
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		gPerModelConstants.objectColour = gLights[i].colour; // Set any per-model constants apart from the world matrix just before calling render (light colour here)
		gLights[i].model->Render();
	}
}



//**************************

//--------------------------------------------------------------------------------------
// Helper Methods
//--------------------------------------------------------------------------------------


// Select the appropriate shader plus any additional textures required for a given post-process
// Helper function shared by full-screen, area and polygon post-processing functions below
void SelectPostProcessShaderAndTextures(PostProcess postProcess, float frameTime)
{
	if (postProcess == PostProcess::Copy)
	{
		gD3DContext->PSSetShader(gCopyPostProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::DepthOfField)
	{
		gD3DContext->PSSetShader(gDepthOfFieldProcess, nullptr, 0);
		gD3DContext->PSSetShaderResources(1, 1, &gShadowMap1SRV);
	}
	else if (postProcess == PostProcess::DualFiltering)
	{
		gPostProcessingConstants.dualFilterIteration = gPostProcessingConstants.dualFilterIteration + 1;
		gD3DContext->PSSetShader(gDualFilteringProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::Dilation)
	{
		gD3DContext->PSSetShader(gDilationProcess, nullptr, 0);
		gPostProcessingConstants.elapsedTime += frameTime;
	}
	else if (postProcess == PostProcess::MergeTextures)
	{
		gD3DContext->PSSetShader(gMergeTexturesProcess, nullptr, 0);
		gD3DContext->PSSetShaderResources(1, 1, &gSceneTextureSRVCopy);
	}
	else if (postProcess == PostProcess::Bloom)
	{
		gD3DContext->PSSetShader(gBloomProcess, nullptr, 0);
		gPostProcessingConstants.dualFilterIteration = 0;
	}
	else if (postProcess == PostProcess::GameBoy)
	{
		gD3DContext->PSSetShader(gGameBoyProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::Contour)
	{
		gD3DContext->PSSetShader(gContourProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::Inverted)
	{
		gD3DContext->PSSetShader(gInvertedProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::Sepia)
	{
		gD3DContext->PSSetShader(gSepiaProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::NightVision)
	{
		gD3DContext->PSSetShader(gNightVisionProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::HueVerticalColourGradient)
	{
		gD3DContext->PSSetShader(gHueVerticalColourGradientProcess, nullptr, 0);

		gPostProcessingConstants.elapsedTime += frameTime;
		gPostProcessingConstants.period = 4;

		// Set the top and bottom colours of the gradient
		gPostProcessingConstants.topColour = { 0.0f, 0.0f, 1.0f };
		gPostProcessingConstants.bottomColour = { 0.0f, 1.0f, 1.0f };
	}
	else if (postProcess == PostProcess::UnderWater)
	{
		gD3DContext->PSSetShader(gUnderWaterProcess, nullptr, 0);

		// Update the underwater timer
		gPostProcessingConstants.underWaterTimer += frameTime;
	}
	else if (postProcess == PostProcess::GaussianBlurHorizontal)
	{
		gPostProcessingConstants.blurAmount = 1.0f;
		gD3DContext->PSSetShader(gGaussianBlurHorizontalProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::GaussianBlurVertical)
	{
		gPostProcessingConstants.blurAmount = 1.0f;
		gD3DContext->PSSetShader(gGaussianBlurVerticalProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::VerticalColourGradient)
	{
		gD3DContext->PSSetShader(gVerticalColourGradientProcess, nullptr, 0);

		// Set the top and bottom colours of the gradient
		gPostProcessingConstants.topColour = { 0.0f, 0.0f, 1.0f };
		gPostProcessingConstants.bottomColour = { 0.0f, 1.0f, 1.0f };
	}
	else if (postProcess == PostProcess::GreyNoise)
	{
		gD3DContext->PSSetShader(gGreyNoisePostProcess, nullptr, 0);

		// Noise scaling adjusts how fine the grey noise is.
		const float grainSize = 140; // Fineness of the noise grain
		gPostProcessingConstants.noiseScale = { gViewportWidth / grainSize, gViewportHeight / grainSize };

		// The noise offset is randomised to give a constantly changing noise effect (like tv static)
		gPostProcessingConstants.noiseOffset = { Random(0.0f, 1.0f), Random(0.0f, 1.0f) };

		// Give pixel shader access to the noise texture
		gD3DContext->PSSetShaderResources(1, 1, &gNoiseMapSRV);
		gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);
	}
	else if (postProcess == PostProcess::Burn)
	{
		gD3DContext->PSSetShader(gBurnPostProcess, nullptr, 0);

		// Set and increase the burn level (cycling back to 0 when it reaches 1.0f)
		const float burnSpeed = 0.2f;
		gPostProcessingConstants.burnHeight = fmod(gPostProcessingConstants.burnHeight + burnSpeed * frameTime, 1.0f);

		// Give pixel shader access to the burn texture (basically a height map that the burn level ascends)
		gD3DContext->PSSetShaderResources(1, 1, &gBurnMapSRV);
		gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);
	}
	else if (postProcess == PostProcess::Distort)
	{
		gD3DContext->PSSetShader(gDistortPostProcess, nullptr, 0);

		// Set the level of distortion
		gPostProcessingConstants.distortLevel = 0.03f;

		// Give pixel shader access to the distortion texture (containts 2D vectors (in R & G) to shift the texture UVs to give a cut-glass impression)
		gD3DContext->PSSetShaderResources(1, 1, &gDistortMapSRV);
		gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);
	}
	else if (postProcess == PostProcess::Spiral)
	{
		gD3DContext->PSSetShader(gSpiralPostProcess, nullptr, 0);

		// Set and increase the amount of spiral - use a tweaked cos wave to animate
		static float wiggle = 0.0f;
		const float wiggleSpeed = 1.0f;
		gPostProcessingConstants.spiralLevel = ((1.0f - cos(wiggle)) * 4.0f);
		wiggle += wiggleSpeed * frameTime;
	}
	else if (postProcess == PostProcess::HeatHaze)
	{
		gD3DContext->PSSetShader(gHeatHazePostProcess, nullptr, 0);

		// Update heat haze timer
		gPostProcessingConstants.heatHazeTimer += frameTime;
	}
	else if (postProcess == PostProcess::Tint)
	{
		gD3DContext->PSSetShader(gTintPostProcess, nullptr, 0);

		// Colour for tint shader
		gPostProcessingConstants.tintColour = { 1.0f, 0.0f, 0.0f };
	}
}


//**********************
// Post Process Modes

// Perform a full-screen post process from "scene texture" to back buffer
void FullScreenPostProcess(PostProcess postProcess, float frameTime, int processIndex)
{
	// Using special vertex shader that creates its own data for a 2D screen quad
	gD3DContext->VSSetShader(g2DQuadVertexShader, nullptr, 0);
	gD3DContext->GSSetShader(nullptr, nullptr, 0);  // Switch off geometry shader when not using it (pass nullptr for first parameter)


	// States - no blending, don't write to depth buffer and ignore back-face culling
	gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
	gD3DContext->RSSetState(gCullNoneState);


	// No need to set vertex/index buffer (see 2D quad vertex shader), just indicate that the quad will be created as a triangle strip
	gD3DContext->IASetInputLayout(NULL); // No vertex data
	gD3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// These lines unbind the scene texture from the pixel shader to stop DirectX issuing a warning when we try to render to it again next frame
	gD3DContext->PSSetShaderResources(0, 1, &nullSRV);

	if (processIndex % 2 == 0)
	{
		// Render first post-process to second render target texture
		
		// Select the back buffer to use for rendering. Not going to clear the back-buffer because we're going to overwrite it all
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTargetTwo, gDepthStencil); // Where to render

		// Give the pixel shader (post-processing shader) access to the scene texture 
		// Use the 'first' texture's contents on the 'second' render pass
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV); // Which input (previous in this case)
	}
	else
	{
		// Select the back buffer to use for rendering. Not going to clear the back-buffer because we're going to overwrite it all
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget, gDepthStencil);

		// Give the pixel shader (post-processing shader) access to the scene texture 
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRVTwo);
	}

	gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)

	// Select shader and textures needed for the required post-processes (helper function above)
	SelectPostProcessShaderAndTextures(postProcess, frameTime);


	// Set 2D area for full-screen post-processing (coordinates in 0->1 range)
	gPostProcessingConstants.area2DTopLeft = { 0, 0 }; // Top-left of entire screen
	gPostProcessingConstants.area2DSize    = { 1, 1 }; // Full size of screen
	gPostProcessingConstants.area2DDepth   = 0;        // Depth buffer value for full screen is as close as possible


	// Pass over the above post-processing settings (also the per-process settings prepared in UpdateScene function below)
	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
	gD3DContext->VSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
	gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);


	// Draw a quad
	gD3DContext->Draw(4, 0);

	// Select the back buffer to use for rendering. Not going to clear the back-buffer because we're going to overwrite it all
	gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);

	// Draw a quad
	gD3DContext->Draw(4, 0);
}


// Perform an area post process from "scene texture" to back buffer at a given point in the world, with a given size (world units)
void AreaPostProcess(PostProcess postProcess, CVector3 worldPoint, CVector2 areaSize, float frameTime, int processIndex)
{
	// First perform a full-screen copy of the scene to back-buffer
	FullScreenPostProcess(PostProcess::Copy, frameTime, processIndex);
	

	// Now perform a post-process of a portion of the scene to the back-buffer (overwriting some of the copy above)
	// Note: The following code relies on many of the settings that were prepared in the FullScreenPostProcess call above, it only
	//       updates a few things that need to be changed for an area process. If you tinker with the code structure you need to be
	//       aware of all the work that the above function did that was also preparation for this post-process area step

	// Select shader/textures needed for required post-process
	SelectPostProcessShaderAndTextures(postProcess, frameTime);

	// Enable alpha blending - area effects need to fade out at the edges or the hard edge of the area is visible
	// A couple of the shaders have been updated to put the effect into a soft circle
	// Alpha blending isn't enabled for fullscreen and polygon effects so it doesn't affect those (except heat-haze, which works a bit differently)
	gD3DContext->OMSetBlendState(gAlphaBlendingState, nullptr, 0xffffff);


	// Use picking methods to find the 2D position of the 3D point at the centre of the area effect
	auto worldPointTo2D = gCamera->PixelFromWorldPt(worldPoint, gViewportWidth, gViewportHeight);
	CVector2 area2DCentre = { worldPointTo2D.x, worldPointTo2D.y };
	float areaDistance = worldPointTo2D.z;
	
	// Nothing to do if given 3D point is behind the camera
	if (areaDistance < gCamera->NearClip())  return;
	
	// Convert pixel coordinates to 0->1 coordinates as used by the shader
	area2DCentre.x /= gViewportWidth;
	area2DCentre.y /= gViewportHeight;



	// Using new helper function here - it calculates the world space units covered by a pixel at a certain distance from the camera.
	// Use this to find the size of the 2D area we need to cover the world space size requested
	CVector2 pixelSizeAtPoint = gCamera->PixelSizeInWorldSpace(areaDistance, gViewportWidth, gViewportHeight);
	CVector2 area2DSize = { areaSize.x / pixelSizeAtPoint.x, areaSize.y / pixelSizeAtPoint.y };

	// Again convert the result in pixels to a result to 0->1 coordinates
	area2DSize.x /= gViewportWidth;
	area2DSize.y /= gViewportHeight;

	// Send the area top-left and size into the constant buffer - the 2DQuad vertex shader will use this to create a quad in the right place
	gPostProcessingConstants.area2DTopLeft = area2DCentre - 0.5f * area2DSize; // Top-left of area is centre - half the size
	gPostProcessingConstants.area2DSize = area2DSize;

	// Manually calculate depth buffer value from Z distance to the 3D point and camera near/far clip values. Result is 0->1 depth value
	// We've never seen this full calculation before, it's occasionally useful. It is derived from the material in the Picking lecture
	// Having the depth allows us to have area effects behind normal objects
	gPostProcessingConstants.area2DDepth = gCamera->FarClip() * (areaDistance - gCamera->NearClip()) / (gCamera->FarClip() - gCamera->NearClip());
	gPostProcessingConstants.area2DDepth /= areaDistance;

	// Pass over this post-processing area to shaders (also sends the per-process settings prepared in UpdateScene function below)
	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
	gD3DContext->VSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
	gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);


	// Draw a quad
	gD3DContext->Draw(4, 0);
}


// Perform an post process from "scene texture" to back buffer within the given four-point polygon and a world matrix to position/rotate/scale the polygon
void PolygonPostProcess(PostProcess postProcess, const std::array<CVector3, 4>& points, const CMatrix4x4& worldMatrix, float frameTime, int processIndex)
{
	// First perform a full-screen copy of the scene to back-buffer
	FullScreenPostProcess(PostProcess::Copy, frameTime, processIndex);


	// These lines unbind the scene texture from the pixel shader to stop DirectX issuing a warning when we try to render to it again next frame
	gD3DContext->PSSetShaderResources(0, 1, &nullSRV);

	if (processIndex % 2 == 0)
	{
		// Render first post-process to second render target texture

		// Select the back buffer to use for rendering. Not going to clear the back-buffer because we're going to overwrite it all
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTargetTwo, gDepthStencil); // Where to render

		// Give the pixel shader (post-processing shader) access to the scene texture 
		// Use the 'first' texture's contents on the 'second' render pass
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV); // Which input (previous in this case)
	}
	else
	{
		// Select the back buffer to use for rendering. Not going to clear the back-buffer because we're going to overwrite it all
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget, gDepthStencil);

		// Give the pixel shader (post-processing shader) access to the scene texture 
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRVTwo);
	}

	gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)

	// Now perform a post-process of a portion of the scene to the back-buffer (overwriting some of the copy above)
	// Note: The following code relies on many of the settings that were prepared in the FullScreenPostProcess call above, it only
	//       updates a few things that need to be changed for an area process. If you tinker with the code structure you need to be
	//       aware of all the work that the above function did that was also preparation for this post-process area step

	// Select shader/textures needed for required post-process
	SelectPostProcessShaderAndTextures(postProcess, frameTime);

	// Loop through the given points, transform each to 2D (this is what the vertex shader normally does in most labs)
	for (unsigned int i = 0; i < points.size(); ++i)
	{
		CVector4 modelPosition = CVector4(points[i], 1);
		CVector4 worldPosition = modelPosition * worldMatrix;
		CVector4 viewportPosition = worldPosition * gCamera->ViewProjectionMatrix();

		gPostProcessingConstants.polygon2DPoints[i] = viewportPosition;
	}

	// Pass over the polygon points to the shaders (also sends the per-process settings prepared in UpdateScene function below)
	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
	gD3DContext->VSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
	gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);

	// Select the special 2D polygon post-processing vertex shader and draw the polygon
	gD3DContext->VSSetShader(g2DPolygonVertexShader, nullptr, 0);

	// Draw a quad
	gD3DContext->Draw(4, 0);

	// Select the back buffer to use for rendering. Not going to clear the back-buffer because we're going to overwrite it all
	gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);

	// Draw a quad
	gD3DContext->Draw(4, 0);
}

//**********************

//**************************


// Rendering the scene
void RenderScene(float frameTime)
{
	//// Common settings ////

	// Set up the light information in the constant buffer
	// Don't send to the GPU yet, the function RenderSceneFromCamera will do that
	gPerFrameConstants.light1Colour   = gLights[0].colour * gLights[0].strength;
	gPerFrameConstants.light1Position = gLights[0].model->Position();
	gPerFrameConstants.light2Colour   = gLights[1].colour * gLights[1].strength;
	gPerFrameConstants.light2Position = gLights[1].model->Position();

	gPerFrameConstants.ambientColour  = gAmbientColour;
	gPerFrameConstants.specularPower  = gSpecularPower;
	gPerFrameConstants.cameraPosition = gCamera->Position();

	gPerFrameConstants.viewportWidth  = static_cast<float>(gViewportWidth);
	gPerFrameConstants.viewportHeight = static_cast<float>(gViewportHeight);



	////--------------- Main scene rendering ---------------////


	// Setup the viewport to the size of the shadow map texture
	D3D11_VIEWPORT vp;
	vp.Width = static_cast<FLOAT>(gShadowMapSize);
	vp.Height = static_cast<FLOAT>(gShadowMapSize);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	gD3DContext->RSSetViewports(1, &vp);

	gD3DContext->OMSetRenderTargets(0, nullptr, gShadowMap1DepthStencil);
	gD3DContext->ClearDepthStencilView(gShadowMap1DepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

	RenderDepthBufferFromCamera(gCamera);

	// Set the target for rendering and select the main depth buffer.
	// If using post-processing then render to the scene texture, otherwise to the usual back buffer
	// Also clear the render target to a fixed colour and the depth buffer to the far distance
	if (gPostProcessAndModeStack.size() != 0)
	{
		// Render scene to first render target texture
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget, gDepthStencil);
		gD3DContext->ClearRenderTargetView(gSceneRenderTarget, &gBackgroundColor.r);
	}
	else
	{
		gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);
		gD3DContext->ClearRenderTargetView(gBackBufferRenderTarget, &gBackgroundColor.r);
	}

	gD3DContext->ClearDepthStencilView(gDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);
	
	// Setup the viewport to the size of the main window
	vp.Width = static_cast<FLOAT>(gViewportWidth);
	vp.Height = static_cast<FLOAT>(gViewportHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	gD3DContext->RSSetViewports(1, &vp);

	
	gD3DContext->PSSetShaderResources(1, 1, &gShadowMap1SRV);
	gD3DContext->PSSetSamplers(1, 1, &gPointSampler);

	// Render the scene from the main camera
	RenderSceneFromCamera(gCamera);


	////--------------- Scene completion ---------------////

	if (gPostProcessAndModeStack.size() != 0)
	{
		int processIndex = 0;
		for (std::pair<PostProcess, PostProcessMode> postProcessAndMode : gPostProcessAndModeStack)
		{
			gCurrentPostProcess = postProcessAndMode.first;
			gCurrentPostProcessMode = postProcessAndMode.second;

			if (gCurrentPostProcessMode == PostProcessMode::Fullscreen)
			{
				if (gCurrentPostProcess == PostProcess::Bloom)
				{
					SaveCurrentSceneToTexture(processIndex);
				}

				FullScreenPostProcess(gCurrentPostProcess, frameTime, processIndex++);
			}
			else if (gCurrentPostProcessMode == PostProcessMode::Polygon)
			{

				// A rotating matrix placing the model above in the scene
				static CMatrix4x4 polyMatrix = MatrixTranslation({ 0, 0, 0 });
			
				// Pass an array of 4 points and a matrix. Only supports 4 points.
				PolygonPostProcess(gCurrentPostProcess, GetWindowPoint(processIndex), polyMatrix, frameTime, processIndex++);
			}
			else if (gCurrentPostProcessMode == PostProcessMode::Area)
			{
				AreaPostProcess(gCurrentPostProcess, gCube->Position(), { 10, 10 }, frameTime, processIndex++);
			}
		}
	}


	//*****************************//
	// Temporary demonstration code for visualising the light's view of the scene
	ColourRGBA white = {1,1,1};
	gD3DContext->ClearRenderTargetView(gBackBufferRenderTarget, &white.r);
	RenderDepthBufferFromCamera(gCamera);
	//*****************************//
	
	// When drawing to the off-screen back buffer is complete, we "present" the image to the front buffer (the screen)
	// Set first parameter to 1 to lock to vsync
	gSwapChain->Present(lockFPS ? 1 : 0, 0);
}

//--------------------------------------------------------------------------------------
// Scene Update
//--------------------------------------------------------------------------------------


// Update models and camera. frameTime is the time passed since the last frame
void UpdateScene(float frameTime)
{
	//***********

	// Select post process on keys
	// Switched from F1, F2, F3 to Z, X, C because I don't have F keys on mey keyboard
	//if (KeyHit(Key_Z))  gCurrentPostProcessMode = PostProcessMode::Fullscreen; // F1
	//if (KeyHit(Key_X))  gCurrentPostProcessMode = PostProcessMode::Area; // F2
	//if (KeyHit(Key_C))  gCurrentPostProcessMode = PostProcessMode::Polygon; // F3

	if (KeyHit(Key_1)) { AddProcessAndMode(PostProcess::VerticalColourGradient, PostProcessMode::Fullscreen); }
	
	if (KeyHit(Key_2)) 
	{ 
		AddProcessAndMode(PostProcess::GaussianBlurHorizontal, PostProcessMode::Fullscreen);
		AddProcessAndMode(PostProcess::GaussianBlurVertical, PostProcessMode::Fullscreen);
	}
	
	if (KeyHit(Key_3)) { AddProcessAndMode(PostProcess::UnderWater, PostProcessMode::Fullscreen); }
	
	if (KeyHit(Key_4)) { AddProcessAndMode(PostProcess::DepthOfField, PostProcessMode::Fullscreen); }
	
	if (KeyHit(Key_5)) { AddProcessAndMode(PostProcess::Distort, PostProcessMode::Fullscreen); }
	
	if (KeyHit(Key_6)) { AddProcessAndMode(PostProcess::Spiral, PostProcessMode::Fullscreen); }
	
	if (KeyHit(Key_7)) { AddProcessAndMode(PostProcess::HeatHaze, PostProcessMode::Fullscreen); }
	
	if (KeyHit(Key_8)) { AddProcessAndMode(PostProcess::Tint, PostProcessMode::Fullscreen); }
	
	if (KeyHit(Key_9)) { AddProcessAndMode(PostProcess::GreyNoise, PostProcessMode::Fullscreen); }
	
	if (KeyHit(Key_Q)) { AddProcessAndMode(PostProcess::Copy, PostProcessMode::Fullscreen); }

	if (KeyHit(Key_Q)) { AddProcessAndMode(PostProcess::Copy, PostProcessMode::Fullscreen); }

	if (KeyHit(Key_E)) { AddProcessAndMode(PostProcess::HueVerticalColourGradient, PostProcessMode::Fullscreen); }

	if (KeyHit(Key_R)) { AddProcessAndMode(PostProcess::NightVision, PostProcessMode::Fullscreen); }

	if (KeyHit(Key_T)) { AddProcessAndMode(PostProcess::Sepia, PostProcessMode::Fullscreen); }

	if (KeyHit(Key_Y)) { AddProcessAndMode(PostProcess::Inverted, PostProcessMode::Fullscreen); }

	if (KeyHit(Key_U)) { AddProcessAndMode(PostProcess::Contour, PostProcessMode::Fullscreen); }

	if (KeyHit(Key_I)) { AddProcessAndMode(PostProcess::GameBoy, PostProcessMode::Fullscreen); }

	if (KeyHit(Key_O)) 
	{ 
		AddProcessAndMode(PostProcess::Bloom, PostProcessMode::Fullscreen);
		//AddProcessAndMode(PostProcess::Dilation, PostProcessMode::Fullscreen);
		int numOfDualFilterings = 8;
		for (int i = 0; i < numOfDualFilterings; i++)
		{
			AddProcessAndMode(PostProcess::DualFiltering, PostProcessMode::Fullscreen);	
		}
		AddProcessAndMode(PostProcess::MergeTextures, PostProcessMode::Fullscreen);


		////AddProcessAndMode(PostProcess::StarLens, PostProcessMode::Fullscreen);
		//AddProcessAndMode(PostProcess::GaussianBlurHorizontal, PostProcessMode::Fullscreen);
		//AddProcessAndMode(PostProcess::GaussianBlurVertical, PostProcessMode::Fullscreen);
	}

	if (KeyHit(Key_P)) { AddProcessAndMode(PostProcess::Burn, PostProcessMode::Fullscreen); }
	
	if (KeyHit(Key_0)) { gPostProcessAndModeStack.clear(); CreateWindowPostProcesses(windowPostProcesses); }

	if (KeyHit(Key_Back)) { RemoveProcessAndMode(); }

	// Orbit one light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float lightRotate = 0.0f;
	static bool go = true;
	gLights[0].model->SetRotation(CVector3(0.0f, ToRadians(180.f), 0.0f));
	gLights[0].model->SetPosition({ 20 + cos(lightRotate) * gLightOrbitRadius, 10, 20 + sin(lightRotate) * gLightOrbitRadius });
	if (go)  lightRotate -= gLightOrbitSpeed * frameTime;
	if (KeyHit(Key_L))  go = !go;

	// Control of camera
	gCamera->Control(frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D);

	// Toggle FPS limiting
	if (KeyHit(Key_P))  lockFPS = !lockFPS;

	// Show frame time / FPS in the window title //
	const float fpsUpdateTime = 0.5f; // How long between updates (in seconds)
	static float totalFrameTime = 0;
	static int frameCount = 0;
	totalFrameTime += frameTime;
	++frameCount;
	if (totalFrameTime > fpsUpdateTime)
	{
		// Displays FPS rounded to nearest int, and frame time (more useful for developers) in milliseconds to 2 decimal places
		float avgFrameTime = totalFrameTime / frameCount;
		std::ostringstream frameTimeMs;
		frameTimeMs.precision(2);
		frameTimeMs << std::fixed << avgFrameTime * 1000;
		std::string windowTitle = "CO3303 Post Process Assingment - Nicolas Nouhi - Frame Time: " + frameTimeMs.str() +
			"ms, FPS: " + std::to_string(static_cast<int>(1 / avgFrameTime + 0.5f));
		SetWindowTextA(gHWnd, windowTitle.c_str());
		totalFrameTime = 0;
		frameCount = 0;
	}
}

void SaveCurrentSceneToTexture(int index)
{
	// Using special vertex shader that creates its own data for a 2D screen quad
	gD3DContext->VSSetShader(g2DQuadVertexShader, nullptr, 0);
	gD3DContext->GSSetShader(nullptr, nullptr, 0);  // Switch off geometry shader when not using it (pass nullptr for first parameter)


	// States - no blending, don't write to depth buffer and ignore back-face culling
	gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
	gD3DContext->RSSetState(gCullNoneState);


	// No need to set vertex/index buffer (see 2D quad vertex shader), just indicate that the quad will be created as a triangle strip
	gD3DContext->IASetInputLayout(NULL); // No vertex data
	gD3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// These lines unbind the scene texture from the pixel shader to stop DirectX issuing a warning when we try to render to it again next frame
	gD3DContext->PSSetShaderResources(0, 1, &nullSRV);

	// Select the back buffer to use for rendering. Not going to clear the back-buffer because we're going to overwrite it all
	gD3DContext->OMSetRenderTargets(1, &gSceneRenderTargetCopy, gDepthStencil); // Where to render

	if (index % 2 == 0)
	{
		// Give the pixel shader (post-processing shader) access to the scene texture 
		// Use the 'first' texture's contents on the 'second' render pass
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV); // Which input (previous in this case)
	}
	else
	{
		// Give the pixel shader (post-processing shader) access to the scene texture 
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRVTwo);
	}

	gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)

	// Use the copy ps
	gD3DContext->PSSetShader(gCopyPostProcess, nullptr, 0);

	// Set 2D area for full-screen post-processing (coordinates in 0->1 range)
	gPostProcessingConstants.area2DTopLeft = { 0, 0 }; // Top-left of entire screen
	gPostProcessingConstants.area2DSize = { 1, 1 }; // Full size of screen
	gPostProcessingConstants.area2DDepth = 0;        // Depth buffer value for full screen is as close as possible


	// Pass over the above post-processing settings (also the per-process settings prepared in UpdateScene function below)
	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
	gD3DContext->VSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
	gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);


	// Draw a quad
	gD3DContext->Draw(4, 0);

	// Select the back buffer to use for rendering. Not going to clear the back-buffer because we're going to overwrite it all
	gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);

	// Draw a quad
	gD3DContext->Draw(4, 0);
}

void CreateWindowPostProcesses(std::vector<PostProcess> windowPostProcesses)
{
	for (PostProcess windowPostProcess : windowPostProcesses)
	{
		AddProcessAndMode(windowPostProcess, PostProcessMode::Polygon);
	}
}

void AddProcessAndMode(PostProcess process, PostProcessMode mode)
{
	gPostProcessAndModeStack.push_back(std::pair<PostProcess, PostProcessMode>(std::make_pair(process, mode)));
}

void RemoveProcessAndMode()
{
	if (gPostProcessAndModeStack.size() > NUM_OF_WINDOWS)
	{
		// If we're removing a vertical Gaussian blur, we need to remove the horizontal one too since its a 2 pass process
		if (gCurrentPostProcess == PostProcess::GaussianBlurVertical)
		{
			gPostProcessAndModeStack.pop_back();
			gPostProcessAndModeStack.pop_back();
		}
		else if (gCurrentPostProcess == PostProcess::MergeTextures)
		{
			gPostProcessAndModeStack.pop_back();
			gPostProcessAndModeStack.pop_back(); 
			gPostProcessAndModeStack.pop_back();
			gPostProcessAndModeStack.pop_back();
		}
		else
		{
			gPostProcessAndModeStack.pop_back();
		}
	}
}

std::array<CVector3, 4> GetWindowPoint(int windowIndex)
{

	switch (windowIndex)
	{
		case 0:
			return{{ { 22, 25, -50 }, {22, 5, -50 }, {33, 25, -50 }, {33, 5, -50 } }};
		case 1:
			return{{ { 36, 25, -50 }, { 36, 5, -50 }, { 49, 25, -50 }, { 49, 5, -50 } }};
		case 2:
			return{{ { 50, 25, -50 }, { 50, 5,-50 }, { 63, 25, -50 }, { 63, 5, -50 } }};
		case 3:
			return{{  {64, 25, -50 }, { 64, 5, -50 }, { 78 ,25, -50 }, { 78, 5, -50 } }};
	}

	return std::array<CVector3, 4>();
}
