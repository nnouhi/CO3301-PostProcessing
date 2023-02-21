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
    // Define the kernel size and weights for the Gaussian blur
    const int KernelSize = 9;
    float weights[] = { 0.05, 0.09, 0.12, 0.15, 0.16, 0.15, 0.12, 0.09, 0.05 };
    
    // Initialize the color and weight accumulators
    float4 colorAccumulator = float4(0, 0, 0, 0);
    float weightAccumulator = 0;
    
    // Iterate over the kernel and accumulate the color and weight

    for (int i = 0; i < KernelSize; i++)
    {
        // Calculate the offset from the center pixel
        float2 offset = float2((i - (KernelSize - 1) / 2) * gBlurAmount, 0);
        
        // Sample the texture at the offset position and apply the kernel weight
        colorAccumulator += weights[i] * SceneTexture.Sample(PointSample, input.sceneUV + offset);
        weightAccumulator += weights[i];
    }
    
    // Normalize the color by the weight accumulator
    float4 color = colorAccumulator / weightAccumulator;
    
    return color;
}