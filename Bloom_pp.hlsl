//--------------------------------------------------------------------------------------
// Colour Tint Post-Processing Pixel Shader
//--------------------------------------------------------------------------------------
// Just samples a pixel from the scene texture and multiplies it by a fixed colour to tint the scene

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// The scene has been rendered to a texture, these variables allow access to that texture
Texture2D SceneTexture : register(t0);
SamplerState PointSample : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
                                          // post-processing so this sampler will use "point sampling" - no filtering

// Algorithm found at: https://www.shadertoy.com/view/WdSGRR

//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

float3 gammaCorrect(float3 color, float gamma)
{
    return pow(color, float3(1.0 / gamma, 1.0 / gamma, 1.0 / gamma));
}

float3 levelRange(float3 color, float minInput, float maxInput)
{
    return min(max(color - float3(minInput, minInput, minInput), float3(0.0, 0.0, 0.0)) / (float3(maxInput, maxInput, maxInput) - float3(minInput, minInput, minInput)), float3(1.0, 1.0f, 1.0f));
}

float3 finalLevels(float3 color, float minInput, float gamma, float maxInput)
{
    return gammaCorrect(levelRange(color, minInput, maxInput), gamma);
}

float luma(float3 color)
{
    return dot(color, float3(0.299, 0.587, 0.114));
}

float luma(float4 color)
{
    return dot(color.rgb, float3(0.299, 0.587, 0.114));
}

// This shader does some thresholding and edge detection to find highlights.
float4 main(PostProcessingInput input) : SV_Target
{
	
    
    float4 t = SceneTexture.Sample(PointSample, input.sceneUV);
    float4 tCol = t;
    float l = luma(t.rgb);
    float3 soft = finalLevels(float3(l, l, l), 0.0, 2.8, 183.0 / 255.0);
    t.rgb = smoothstep(0.56, 0.63, float3(l, l, l));
    t.rgb -= 0.9997;
    
    return saturate(float4(t.rgb, t.r));
}