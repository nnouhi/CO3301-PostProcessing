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
Texture2D SecondSceneTexture : register(t1);
SamplerState PointSample : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
                                          // post-processing so this sampler will use "point sampling" - no filtering

// Algorithm found at: https://pingpoli.medium.com/the-bloom-post-processing-effect-9352fa800caf
// and https://learnopengl.com/Advanced-Lighting/Bloom , https://learnopengl.com/Advanced-Lighting/HDR

// Combines together two scene textures (in this case used to combine default scene texture with bloom + gaussian blur texture)

//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    const float gamma = 2.2f;
    float3 hdrColour = SceneTexture.Sample(PointSample, input.sceneUV).rgb;
    float3 bloomColour = SecondSceneTexture.Sample(PointSample, input.sceneUV).rgb;

    // Apply Reinhard tone mapping to HDR colors
    hdrColour = hdrColour / (hdrColour + 1.0f);
    bloomColour = bloomColour / (bloomColour + 1.0f);

    // Convert HDR colors to LDR colors using gamma correction
    hdrColour = pow(hdrColour, 1.0f / gamma);
    bloomColour = pow(bloomColour, 1.0f / gamma);

    // Combine HDR colors and bloom colors
    float3 finalColour = hdrColour + bloomColour;

    // Gamma correct the final color for output
    finalColour = pow(finalColour, gamma);

    // Set alpha to 1 for final output
    return float4(finalColour, 1.0f);
}