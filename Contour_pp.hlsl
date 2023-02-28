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

// Convert RGB color to grayscale
float3 rgb2gray(float3 color)
{
    return (color.r + color.g + color.b) / 3.0f;
}

// Convert grayscale value to RGB color
float3 gray2rgb(float gray)
{
    return float3(gray, gray, gray);
}

// Sobel edge detection function
float3 sobelEdgeDetect(float2 texCoord)
{
    // Sobel filter kernels
    float3x3 sobelX = float3x3(-1, 0, 1, -2, 0, 2, -1, 0, 1);
    float3x3 sobelY = float3x3(-1, -2, -1, 0, 0, 0, 1, 2, 1);
    const float2 texelSize = 1.0f / float2(gViewportWidth, gViewportHeight);
    
    // Compute the Sobel X and Sobel Y gradients
    float3 gx = 0;
    float3 gy = 0;
    for (int i = -1; i <= 1; i++)
    {
        for (int j = -1; j <= 1; j++)
        {
            float3 color = rgb2gray(SceneTexture.Sample(PointSample, texCoord + float2(i, j) * texelSize).rgb);
            gx += sobelX[i + 1][j + 1] * color;
            gy += sobelY[i + 1][j + 1] * color;
        }
    }

    // Compute the gradient magnitudes for each channel separately
    float gx_mag = dot(gx, gx);
    float gy_mag = dot(gy, gy);
    float gradient_mag = sqrt(gx_mag * gx_mag + gy_mag * gy_mag);

    // Convert the gradient magnitude to grayscale and return
    return gray2rgb(gradient_mag);
}

// Algorithm found at: https://alexanderameye.github.io/notes/rendering-outlines/
// and https://jameshfisher.com/2020/08/31/edge-detection-with-sobel-filters/

//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    // Apply Sobel edge detection to the input texture
    float3 color = sobelEdgeDetect(input.sceneUV);

    // Output the result
    return float4(color, 1);
}