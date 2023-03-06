//--------------------------------------------------------------------------------------
// Colour Tint Post-Processing Pixel Shader
//--------------------------------------------------------------------------------------
// Just samples a pixel from the scene texture and multiplies it by a fixed colour to tint the scene

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// The scene has been rendered to a texture, these variables allow access to that texture
Texture2D SceneTexture : register(t0); // This texture is the default HDR scene before any post processing applied to it
SamplerState PointSample : register(s0); 
Texture2D DepthTexture : register(t1); // This texture is the blurred version of the focus texture with dilation applied to it


// Algorithm found at: https://lettier.github.io/3d-game-shaders-for-beginners/depth-of-field.html

//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    
    float depth = DepthTexture.Sample(PointSample, input.sceneUV).r;
    return float4(depth, depth, depth, 1.0f);
}

