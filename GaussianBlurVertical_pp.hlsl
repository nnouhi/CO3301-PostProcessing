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

// Algorithm found at: https://www.rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/ 
// and https://stackoverflow.com/questions/36303950/hlsl-gaussian-blur-effect

// Post-processing shader that performs a vertical Gaussian blur on the scene texture
float4 main(PostProcessingInput input) : SV_Target
{   
    
    const float offset[5] = { 0.0, 1.0, 2.0, 3.0, 4.0 };
    const float weight[5] = { 0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162 };
    
    float3 colour = SceneTexture.Sample(PointSample, input.sceneUV).rgb * weight[0];
    float3 fragmentColour = float3(0.0f, 0.0f, 0.0f);

    // We are doing a vertical blur so hStep is 0
    float hStep = 0.0f;
    float vStep = 1.0f;

    for (int i = 1; i < 5; i++)
    {
        float2 normalizedOffset = float2( /*float2(0.0f, 1.0f * offset[i])*/hStep * offset[i], vStep * offset[i] * gBlurAmount) / gViewportHeight;
        
        fragmentColour +=
        SceneTexture.Sample(PointSample, input.sceneUV + normalizedOffset) * weight[i] +
        SceneTexture.Sample(PointSample, input.sceneUV - normalizedOffset) * weight[i];
    }
    
    colour += fragmentColour;
    return float4(colour, 1.0);
}