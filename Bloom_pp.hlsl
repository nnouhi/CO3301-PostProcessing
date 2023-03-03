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

// Algorithm found at: https://pingpoli.medium.com/the-bloom-post-processing-effect-9352fa800caf
// and https://learnopengl.com/Advanced-Lighting/Bloom , https://learnopengl.com/Advanced-Lighting/HDR


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

float Rgb2Gray(float3 colour);


// In your pixel shader:

float4 main(PostProcessingInput input) : SV_Target
{
    const float brightnessThreshHold = 0.5f;
    float3 colour = SceneTexture.Sample(PointSample, input.sceneUV).rgb;
    float colourBrightness = Rgb2Gray(colour);
    
    if (colourBrightness > brightnessThreshHold)
    {
        return float4(colour, 1.0f);
    }
    
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
}

// Convert RGB color to grayscale
float Rgb2Gray(float3 colour)
{
    return dot(colour, float3(0.2126f, 0.7152f, 0.0722f));
}
