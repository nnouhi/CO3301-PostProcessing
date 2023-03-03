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



// Combines together two scene textures (in this case used to combine default scene texture with bloom + gaussian blur texture)

// Algorithm found at: file:///D:/Downloads/siggraph2015-mmg-marius-notes.pdf

//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------
float4 Downsample(float2 uv, float2 halfpixel);
float4 Upsample(float2 uv, float2 halfpixel);

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    float4 colour = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if (gDualFilterIteration < 4)
    {
        float2 halfpixelOffset = 0.5f / (float2(gViewportHeight, gViewportWidth) / 2.0f);
        colour = Downsample(input.sceneUV, halfpixelOffset);
    }
    else
    {
        float2 halfpixelOffset = 0.5f / (float2(gViewportHeight, gViewportWidth) * 2.0f);
        colour = Upsample(input.sceneUV, halfpixelOffset);
    }

    return colour;
}

float4 Downsample(float2 uv, float2 halfpixel)
{
    float4 sum = SceneTexture.Sample(PointSample, uv) * 4.0;
    sum += SceneTexture.Sample(PointSample, uv - halfpixel.xy);
    sum += SceneTexture.Sample(PointSample, uv + halfpixel.xy);
    sum += SceneTexture.Sample(PointSample, uv + float2(halfpixel.x, -halfpixel.y));
    sum += SceneTexture.Sample(PointSample, uv - float2(halfpixel.x, -halfpixel.y));
    return sum / 8.0;
}
float4 Upsample(float2 uv, float2 halfpixel)
{
    float4 sum = SceneTexture.Sample(PointSample, uv + float2(-halfpixel.x * 2.0, 0.0));
    sum += SceneTexture.Sample(PointSample, uv + float2(-halfpixel.x, halfpixel.y)) * 2.0;
    sum += SceneTexture.Sample(PointSample, uv + float2(0.0, halfpixel.y * 2.0));
    sum += SceneTexture.Sample(PointSample, uv + float2(halfpixel.x, halfpixel.y)) * 2.0;
    sum += SceneTexture.Sample(PointSample, uv + float2(halfpixel.x * 2.0, 0.0));
    sum += SceneTexture.Sample(PointSample, uv + float2(halfpixel.x, -halfpixel.y)) * 2.0;
    sum += SceneTexture.Sample(PointSample, uv + float2(0.0, -halfpixel.y * 2.0));
    sum += SceneTexture.Sample(PointSample, uv + float2(-halfpixel.x, -halfpixel.y)) * 2.0;
    return sum / 12.0;
}
