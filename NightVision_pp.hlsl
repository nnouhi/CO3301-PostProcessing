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

// Algorithm found at: https://developer.valvesoftware.com/wiki/Vision_Nocturna

//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    float3 finalColour = float3(0.0f, 0.0f, 0.0f);
    float4 vAdd = float4(0.1, 0.1, 0.1, 0); // just a float4 for use later
	
    float4 colour = SceneTexture.Sample(PointSample, input.sceneUV); // this takes our sampler and turns the rgba into floats between 0 and 1
    
    colour += SceneTexture.Sample(PointSample, input.sceneUV + 0.001); // these 3 lines blur the image slightly
    colour += SceneTexture.Sample(PointSample, input.sceneUV + 0.002);
    colour += SceneTexture.Sample(PointSample, input.sceneUV + 0.003);
    
    if (((colour.r + colour.g + colour.b) / 3) < 0.9f) // if the pixel is bright leave it bright
    {
        colour = colour / 4; //otherwise set it to an average color of the 4 images we just added together
    }
	
	
    finalColour = colour + vAdd; //adds the floats together

    finalColour.r = 0; // removes red and blue colors
    finalColour.g = (colour.r + colour.g + colour.b) / 3; // sets green the the average of rgb
    finalColour.b = 0;
    
    return float4(finalColour /* * gBrightness*/, 1.0f); // brighten the final image and return it 
}