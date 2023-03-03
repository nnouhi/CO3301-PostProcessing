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


// Algorithm found at: https://ivanskodje.com/gameboy-shaders/

//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

float3 Rgb2Gray(float3 colour);
float3 Gray2Ggb(float3 gray, float offset);

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    
    // Colour offset - changes threshold for colour adjustments
    const float offset = 0.3f;
    
    const int pixelWidth = 7;
    const int pixelHeight = 4;
    
    // Calcualte new pixel width and height in texure coord system
    const float newTexelWidth = pixelWidth * (1.0f / gViewportWidth);
    const float newTexelHeight = pixelHeight * (1.0f / gViewportHeight);
    const float2 newCoord = float2(floor(input.sceneUV.x / newTexelWidth + 0.5f) * newTexelWidth, floor(input.sceneUV.y / newTexelHeight + 0.5f) * newTexelHeight);
    
	// Sample a pixel from the scene texture and multiply it with the tint colour (comes from a constant buffer defined in Common.hlsli)
    float3 colour = SceneTexture.Sample(PointSample, newCoord).rgb;
	
	// Got the RGB from the scene texture, set alpha to 1 for final output
    return float4(Gray2Ggb(Rgb2Gray(colour), offset), 1.0f);
}

// Convert RGB color to grayscale
float3 Rgb2Gray(float3 colour)
{
    float gray = (colour.r + colour.g + colour.b) / 3.0f;
    
    return float3(gray, gray, gray);
}

// Convert grayscale value to RGB color
float3 Gray2Ggb(float3 gray, float offset)
{
    const float3 colour1 = float3(0.505f, 0.529f, 0.407f);
    const float3 colour2 = float3(0.294f, 0.313f, 0.247f);
    const float3 colour3 = float3(0.152f, 0.172f, 0.145f);
    const float3 colour4 = float3(0.070f, 0.090f, 0.086f);
    float3 newColour = float3(0.0f, 0.0f, 0.0f);
    
    // Set darkest colour 4
    newColour = colour4;
    
    // Colour greater that (default) 0.25 in value?
    if (gray.r > offset * 0.5f)
    {
        newColour = colour3;
        
        // Colour greater than (default) 0.50 in value? 
        if (gray.r > offset)
        {
            newColour = colour2;
        }
        
        // Colour greater that (default) 0.75 in value?
        if (gray.r > offset * 1.5f)
        {
            newColour = colour1;
        }
    }
    
    return newColour;
}