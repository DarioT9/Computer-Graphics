#version 450
#extension GL_ARB_separate_shader_objects : enable

// Input variables received from the Vertex Shader
layout(location = 0) in vec3 fragPos;   // Fragment position in world space
layout(location = 1) in vec3 fragNorm;  // Interpolated normal
layout(location = 2) in vec2 fragUV;    // Texture coordinates
layout(location = 3) in vec4 fragTan;   // Fragment tangent (xyz: direction, w: handedness)

// Output color of the fragment
layout(location = 0) out vec4 outColor;

// Global uniforms (lighting and camera properties)
layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    vec3 lightDir;    // Directional light direction
    vec4 lightColor;  // Directional light color (RGBA)
    vec3 eyePos;      // Camera position in world space
} gubo;

// Texture sampler for the base color of the object
layout(set = 1, binding = 1) uniform sampler2D TCommonObjectBaseColor;

void main() {
    // --- Normal and Tangent Space Computation ---
    vec3 Norm = normalize(fragNorm);  // Normalize the interpolated normal
    vec3 Tan = normalize(fragTan.xyz - Norm * dot(fragTan.xyz, Norm));  // Orthogonalize tangent
    vec3 Bitan = cross(Norm, Tan) * fragTan.w;  // Compute bitangent
    mat3 tbn = mat3(Tan, Bitan, Norm);  // Construct TBN matrix (Tangent-Bitangent-Normal)

    // --- Normal Mapping (if available) ---
    // If a normal map was used, it would be sampled and transformed here.
    // For now, we keep the interpolated normal:
    vec3 N = normalize(Norm);  // Use interpolated normal

    // --- Light and Eye Direction ---
    vec3 EyeDir = normalize(gubo.eyePos - fragPos); // View direction (toward camera)
    vec3 lightDir = normalize(gubo.lightDir); // Normalized directional light direction
    vec3 lightColor = gubo.lightColor.rgb; // Extract light color (ignoring alpha)

    // --- Base Color from Texture ---
    vec3 BaseColor = texture(TCommonObjectBaseColor, fragUV).rgb; // Sample the base color texture

    // --- Diffuse Lighting (Lambertian) ---
    float DiffInt = max(dot(N, lightDir), 0.0); // Lambertian diffuse intensity
    vec3 Diffuse = BaseColor * DiffInt * lightColor; // Apply light color to diffuse component

    // --- Specular Reflection (Blinn-Phong Model) ---
    vec3 halfwayDir = normalize(lightDir + EyeDir); // Halfway vector for Blinn-Phong
    float SpecInt = pow(max(dot(N, halfwayDir), 0.0), 8.0); // Specular intensity (fixed exponent)

    // --- Decay Approximation ---
    vec3 Decay = vec3(1.0);  // Decay
    vec3 Specular = lightColor * SpecInt * Decay; // Final specular contribution

    // --- Ambient Light (Constant) ---
    vec3 ambientLight = vec3(0.05, 0.05, 0.05); // Low ambient light contribution
    vec3 color = Diffuse + Specular + ambientLight * BaseColor; // Combine lighting components

    // Output final color
    outColor = vec4(color, 1.0);
}