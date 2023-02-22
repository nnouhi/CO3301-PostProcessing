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


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    const float3 underWaterColour = float3(0.0f, 0.6f, 0.8f);
	const float effectStrength = 0.01f;
    

	// Water is a combination of sine waves in x and y dimensions
    float SinX = sin(input.areaUV.x * radians(360.0f) + gUnderWaterTimer * 3.0f);
    float SinY = sin(input.areaUV.y * radians(360.0f) + gUnderWaterTimer * 3.7f);
	
	// Offset for scene texture UV based on haze effect
	// Adjust size of UV offset based on the constant EffectStrength, the overall size of area being processed, and the alpha value calculated above
    float2 waterOffset = float2(SinY, SinX) * effectStrength * gArea2DSize;

	// Get pixel from scene texture, offset using haze
    float3 colour = SceneTexture.Sample(PointSample, input.sceneUV + waterOffset).rgb * underWaterColour;

    return float4(colour, 1.0f);
}
