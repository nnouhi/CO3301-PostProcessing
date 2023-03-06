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

// Algorithm found at: https://lettier.github.io/3d-game-shaders-for-beginners/dilation.html

//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

float Rgb2Gray(float3 colour);


// In your pixel shader:

float4 main(PostProcessingInput input) : SV_Target
{
    const float brightnessThreshold = 0.5f;
    const float2 texelSize = 1.0f / float2(gViewportWidth, gViewportHeight);
    float3 colour = SceneTexture.Sample(PointSample, input.sceneUV).rgb;
    float3 result = colour;
    
    // Calculate the grayscale value of the center pixel
    float centerBrightness = Rgb2Gray(colour);
    
    // Calculate the dilation factor based on the brightness of the center pixel
    float dilationFactor = (centerBrightness > brightnessThreshold) ? 1.0f: 2.0f;
    
    // Apply dilation to neighbour pixels
    float3 dilatedPixel = SceneTexture.Sample(PointSample, input.sceneUV + float2(texelSize.x, 0)).rgb; // right
    dilatedPixel = max(dilatedPixel, SceneTexture.Sample(PointSample, input.sceneUV + float2(-texelSize.x, 0)).rgb); // left
    dilatedPixel = max(dilatedPixel, SceneTexture.Sample(PointSample, input.sceneUV + float2(0, texelSize.y)).rgb); // up
    dilatedPixel = max(dilatedPixel, SceneTexture.Sample(PointSample, input.sceneUV + float2(0, -texelSize.y)).rgb); // down
    result += dilationFactor * (dilatedPixel - colour);
    
    return float4(result, 1.0f);
}

// Convert RGB color to grayscale
float Rgb2Gray(float3 colour)
{
    return dot(colour, float3(0.2126f, 0.7152f, 0.0722f));
}

