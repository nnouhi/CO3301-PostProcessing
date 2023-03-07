//--------------------------------------------------------------------------------------
// Colour Tint Post-Processing Pixel Shader
//--------------------------------------------------------------------------------------
// Just samples a pixel from the scene texture and multiplies it by a fixed colour to tint the scene

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// The scene has been rendered to a texture, these variables allow access to that texture
Texture2D CurrentFrameTexture : register(t0);
SamplerState PointSample : register(s0); 
Texture2D PreviousFrameTexture : register(t1);



//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    const float AmountOfBlur = 0.5f;
    
    // Sample the current frame and the last frame
    float4 currentColor = CurrentFrameTexture.SampleLevel(PointSample, input.sceneUV, 0);
    float4 lastColor = PreviousFrameTexture.SampleLevel(PointSample, input.sceneUV, 0);
    
    // Calculate the motion vector
    float2 motionVector = (currentColor.xy - lastColor.xy) * AmountOfBlur;
    
    // Calculate the texture coordinates for the last frame
    float2 lastTexcoord = input.sceneUV - motionVector * (1.0f / float2(gViewportWidth, gViewportHeight));
    
    // Sample the last frame at the calculated texture coordinates
    float4 blendedColor = PreviousFrameTexture.SampleLevel(PointSample, lastTexcoord, 0);
    
    // Combine the current and last frame colors with motion blur
    float4 outputColor = lerp(currentColor, blendedColor, AmountOfBlur);
    
    // Output the final color
    return outputColor;
}