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
Texture2D SharpTexture : register(t1);
SamplerState PointSample : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
                                          // post-processing so this sampler will use "point sampling" - no filtering

// Algorithm found at: https://www.shadertoy.com/view/WdSGRR
//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

float3 levelRange(float3 color, float minInput, float maxInput)
{
    return min(max(color - float3(minInput, minInput, minInput), float3(0.0, 0.0, 0.0)) / (float3(maxInput, maxInput, maxInput) - float3(minInput, minInput, minInput)), float3(1.0, 1.0, 1.0));
}

float saturate(float c)
{
    return clamp(c, 0.0, 1.0);
}

float4 saturate(float4 c)
{
    return float4(saturate(c.r), saturate(c.g), saturate(c.b), saturate(c.a));
}

float blendScreen(float base, float blend)
{
    return 1.0 - ((1.0 - base) * (1.0 - blend));
}

float3 blendScreen(float3 base, float3 blend)
{
    return float3(blendScreen(base.r, blend.r), blendScreen(base.g, blend.g), blendScreen(base.b, blend.b));
}

float3 blendScreen(float3 base, float3 blend, float opacity)
{
    return (blendScreen(base, blend) * opacity + base * (1.0 - opacity));
}

float3 gradient(float3 c1, float3 c2, float3 c3, float grad)
{
    float3 o = lerp(c1, c3, grad);
    float u = 1.0 - abs(grad * 2.0 - 1.0);
    
    o = lerp(o, c2, u);
    return o;
}

float4 streak(Texture2D source,
            float2 tc,
            float2 pixelSize,
            float2 dir,
            int samples,
            float attenuation,
            int iteration,
           	float4 channelMask)
{
    float att = 0.98;
    const int samp = 4;
    float pSize = 4.0;
    float scale = 1.0;
    
    float4 cOut = float4(0.0, 0.0, 0.0, 0.0);
    
    float b = pow(float(samples), float(iteration));
    
    const float nSamp = float(samp);
    
    for (float s = 0.0; s < nSamp; s++)
    {
        float weight = pow(attenuation, b * s);
        
        float2 coord = tc + (dir * b * float2(s, s) * pixelSize);
        cOut += clamp(weight, 0.0, 1.0) * source.Sample(PointSample, coord) * channelMask;
        
        float2 coord2 = tc + (dir * -1.0 * b * float2(s, s) * pixelSize);
        cOut += clamp(weight, 0.0, 1.0) * source.Sample(PointSample, coord2) * channelMask;
    }
    
   
    return saturate(cOut);
}



float4 main(PostProcessingInput input) : SV_Target
{
    float att = 0.98;
    const int samp = 4;
    float pSize = 2.0;
    float scale = 1.0;
    
    if (gKawaseIter == 4)
    {
        float4 strH = streak(SceneTexture, input.sceneUV, pSize / float2(gViewportWidth, gViewportHeight).xy, float2(1.0, 0.0), samp, att, gKawaseIter, float4(1.0, 0.0, 0.0, 0.0));
        float4 strV = streak(SceneTexture, input.sceneUV, pSize / float2(gViewportWidth, gViewportHeight).xy, float2(0.0, 1.0), samp, att, gKawaseIter, float4(0.0, 1.0, 0.0, 0.0));
        float4 strD1 = streak(SceneTexture, input.sceneUV, pSize / float2(gViewportWidth, gViewportHeight).xy, float2(1.0, 1.0), samp, att, gKawaseIter, float4(0.0, 0.0, 1.0, 0.0));
        float4 strD2 = streak(SceneTexture, input.sceneUV, pSize / float2(gViewportWidth, gViewportHeight).xy, float2(-1.0, 1.0), samp, att, gKawaseIter, float4(0.0, 0.0, 0.0, 1.0));
        
        float3 s1 = lerp(float3(0.0, 0.0, 0.0), float3(1.0, 0.65, 0.0), strH.r);
        float3 s2 = lerp(float3(0.0, 0.0, 0.0), float3(1.0, 0.65, 0.0), strV.g);
        float3 s3 = lerp(float3(0.0, 0.0, 0.0), float3(1.0, 0.3, 0.0), strD1.b);
        float3 s4 = lerp(float3(0.0, 0.0, 0.0), float3(1.0, 0.3, 0.0), strD2.a);
    
        float3 g1 = gradient(float3(1.0, 0.5, 1.0), float3(1.0, 0.91, 0.91), float3(1.0, 1.0, 1.0), strH.r);
        float3 g2 = gradient(float3(1.0, 0.5, 1.0), float3(1.0, 0.91, 0.91), float3(1.0, 1.0, 1.0), strV.g);

        float3 g3 = gradient(float3(1.0, 0.25, 0.0), float3(1.0, 0.5, 0.0), float3(1.0, 0.823, 0.65), strD1.b);
        float3 g4 = gradient(float3(1.0, 0.25, 0.0), float3(1.0, 0.5, 0.0), float3(1.0, 0.823, 0.65), strD2.a);

        float strC = strH.r + strV.g + strD1.b + strD2.a;
        float4 str = float4(strC, strC, strC, strC);

        // Blend streaks with sharp image to get final result
        return float4(blendScreen(SharpTexture.Sample(PointSample, input.sceneUV).rgb, str.rgb), 1.0f);
    }
    // Create streaks
    else
    {
        float4 strH = streak(SceneTexture, input.sceneUV, pSize / float2(gViewportWidth, gViewportHeight).xy, float2(1.0, 0.0), samp, att, gKawaseIter, float4(1.0, 0.0, 0.0, 0.0));
        float4 strV = streak(SceneTexture, input.sceneUV, pSize / float2(gViewportWidth, gViewportHeight).xy, float2(0.0, 1.0), samp, att, gKawaseIter, float4(0.0, 1.0, 0.0, 0.0));
        float4 strD1 = streak(SceneTexture, input.sceneUV, pSize / float2(gViewportWidth, gViewportHeight).xy, float2(1.0, 1.0), samp, att, gKawaseIter, float4(0.0, 0.0, 1.0, 0.0));
        float4 strD2 = streak(SceneTexture, input.sceneUV, pSize / float2(gViewportWidth, gViewportHeight).xy, float2(-1.0, 1.0), samp, att, gKawaseIter, float4(0.0, 0.0, 0.0, 1.0));
        float4 str = float4(strH.r, strV.g, strD1.b, strD2.a);    
        // Output to screen
        return str;
    }   
}

