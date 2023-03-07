//--------------------------------------------------------------------------------------
// Colour Tint Post-Processing Pixel Shader
//--------------------------------------------------------------------------------------
// Just samples a pixel from the scene texture and multiplies it by a fixed colour to tint the scene

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// The scene has been rendered to a texture, these variables allow access to that texture
// The scene has been rendered to a texture, these variables allow access to that texture
Texture2D BlurredTexture : register(t0); // This texture is the default HDR scene before any post processing applied to it
SamplerState PointSample : register(s0);
Texture2D SharpTexture : register(t1); 
Texture2D DepthTexture : register(t2); 


//--------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Helper functions
//--------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

float to_distance(float depth)
{
    float near = 1.0f;
    float far = 20000.0f;
    float dist = (2.0 * near * far) / (far + near - depth * (far - near));
    return dist;
}

float to_depth(float dist)
{
    float near = 1.0f;
    float far = 20000.0f;
    float depth = (far * (dist - 2.0 * near) + near * dist) / (dist * (far - near));
    return depth;
}

// Post-processing shader that applies depth of field effect to the scene texture
float4 main(PostProcessingInput input) : SV_Target
{
    const float pi_times_2 = 6.28318530718f; // Pi*2
    const float directions = 24.0f; // BLUR directions (Default 16.0 - More is better but slower)
    const float quality = 4.0f; // BLUR quality (Default 4.0 - More is better but slower)
    const float size = 10.0f; // BLUR size (radius)
    float4 unblurred_colour = SharpTexture.Sample(PointSample, input.sceneUV);
    float depth = DepthTexture.Sample(PointSample, input.sceneUV).r;
 
    float distance_to_pixel = to_distance(depth);
    float x = clamp(abs(distance_to_pixel - gDistanceToFocusedObject) / gDistanceToFocusedObject, 0.0, 1.0);
    x = max(1.0 - x, 0.0);
    x = 1.0 - pow(x, 1.0 / 10.0);

    // Blurring
    float2 radius = float2(size / gViewportWidth, size / gViewportHeight);
    
    float4 blurred_colour = unblurred_colour;
    
    for (float d = 0.0; d < pi_times_2; d += pi_times_2 / directions)
    {
        for (float i = 1.0 / quality; i <= 1.0; i += 1.0 / quality)
        {
            blurred_colour += SharpTexture.Sample(PointSample, input.sceneUV + float2(cos(d), sin(d)) * radius * x * i);
        }
    }
    
    // Output to screen
    blurred_colour /= quality * directions - 15.0f;

    float4 frag_colour;
    frag_colour.rgb = blurred_colour.rgb;
    frag_colour.a = 1.0;
    
    return frag_colour;
}


