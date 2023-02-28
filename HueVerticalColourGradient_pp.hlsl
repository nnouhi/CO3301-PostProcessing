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

// Conversion found at: https://www.chilliant.com/rgb2hsv.html 

const float Epsilon = 1e-10;

// Declare method signatures
float3 GetNewColour(float3 startingColour);
float ShiftHue(float amountOfHueShift);
float3 RGBtoHSL(in float3 RGB);
float3 RGBtoHCV(in float3 RGB);
float3 HSLtoRGB(in float3 HSL);
float3 HUEtoRGB(in float H);

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    float t = input.sceneUV.y; 
 
	// Sample a pixel from the scene texture and multiply it with interpolated colour
    float3 colour = SceneTexture.Sample(PointSample, input.sceneUV).rgb * lerp(GetNewColour(gTopColour), GetNewColour(gBottomColour), t);
	
	// Got the RGB from the scene texture, set alpha to 1 for final output
    return float4(colour, 1.0f);
}

float3 GetNewColour(float3 startingColour)
{  
    float3 hsl = RGBtoHSL(startingColour);
    float t = gElapsedTime + hsl.x * gPeriod;
    hsl.x = (t - floor(t / gPeriod) * gPeriod) / gPeriod;
    return HSLtoRGB(hsl);
}

// Methods used for RGB -> HSL conversion
float3 RGBtoHSL(in float3 RGB)
{
    float3 HCV = RGBtoHCV(RGB);
    float L = HCV.z - HCV.y * 0.5;
    float S = HCV.y / (1 - abs(L * 2 - 1) + Epsilon);
    return float3(HCV.x, S, L);
}

float3 RGBtoHCV(in float3 RGB)
{
    // Based on work by Sam Hocevar and Emil Persson
    float4 P = (RGB.g < RGB.b) ? float4(RGB.bg, -1.0, 2.0 / 3.0) : float4(RGB.gb, 0.0, -1.0 / 3.0);
    float4 Q = (RGB.r < P.x) ? float4(P.xyw, RGB.r) : float4(RGB.r, P.yzx);
    float C = Q.x - min(Q.w, Q.y);
    float H = abs((Q.w - Q.y) / (6 * C + Epsilon) + Q.z);
    return float3(H, C, Q.x);
}

// Method used for HSL -> RGB conversion
float3 HSLtoRGB(in float3 HSL)
{
    float3 RGB = HUEtoRGB(HSL.x);
    float C = (1 - abs(2 * HSL.z - 1)) * HSL.y;
    return (RGB - 0.5) * C + HSL.z;
}

float3 HUEtoRGB(in float H)
{
    float R = abs(H * 6 - 3) - 1;
    float G = 2 - abs(H * 6 - 2);
    float B = 2 - abs(H * 6 - 4);
    return saturate(float3(R, G, B));
}