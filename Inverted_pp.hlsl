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


// Algorithm found at: https://medium.com/geekculture/shader-journey-3-basic-post-processing-effects-e9feb900ceff

//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    float3 finalColour = float3(0.0f, 0.0f, 0.0f);
	
    // Sample a pixel from the scene texture and multiply it with the tint colour (comes from a constant buffer defined in Common.hlsli)
    float3 colour = SceneTexture.Sample(PointSample, input.sceneUV).rgb;
    
    finalColour.r = 1.0f - colour.r;
    finalColour.g = 1.0f - colour.g;
    finalColour.b = 1.0f - colour.b;
	
	// Got the RGB from the scene texture, set alpha to 1 for final output
    return float4(finalColour, 1.0f);
}