#version 450
#extension GL_ARB_separate_shader_objects : enable

// Variables received from the Vertex Shader
layout(location = 0) in vec3 fragPos;   // Fragment position in world space
layout(location = 1) in vec3 fragNorm;  // Fragment normal
layout(location = 2) in vec2 fragUV;    // UV coordinates for texturing
layout(location = 3) in vec4 fragTan;   // Fragment tangent

// Output color computed by this shader
layout(location = 0) out vec4 outColor;

// Global uniforms (light and view-related data)
layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    vec3 lightDir;    // Directional light direction
    vec4 lightColor;  // Light color and intensity
    vec3 eyePos;      // Camera (eye) position in world space
} gubo;

// Texture uniform for the city surface color
layout(set = 1, binding = 1) uniform sampler2D TSkyScraperBaseColor;

void main() {
    // ---- Normal and Tangent Space Calculations ----
    vec3 Norm = normalize(fragNorm);
    vec3 Tan = normalize(fragTan.xyz - Norm * dot(fragTan.xyz, Norm)); // Orthogonalize tangent
    vec3 Bitan = cross(Norm, Tan) * fragTan.w;  // Compute bitangent
    mat3 tbn = mat3(Tan, Bitan, Norm);  // Create TBN matrix (Tangent, Bitangent, Normal)

    // ---- Normal Mapping (if available) ----
    // If a normal map was used, this is where sampling and transformation would happen.
    // For now, we keep only the interpolated normal:
    vec3 N = normalize(Norm);  // Use the interpolated normal

    // ---- Light and View Direction ----
    vec3 EyeDir = normalize(gubo.eyePos - fragPos); // View direction
    vec3 lightDir = normalize(gubo.lightDir); // Normalize light direction
    vec3 lightColor = gubo.lightColor.rgb; // Extract RGB from light color

    // ---- Texture Sampling ----
    vec3 BaseColor = texture(TSkyScraperBaseColor, fragUV).rgb; // Sample base texture

    // ---- Diffuse Lighting (Lambertian Reflection) ----
    float DiffInt = max(dot(N, lightDir), 0.0);
    vec3 Diffuse = BaseColor * DiffInt * lightColor; // Diffuse component

    // ---- Specular Highlights (Blinn-Phong Model) ----
    vec3 halfwayDir = normalize(lightDir + EyeDir); // Halfway vector for Blinn-Phong
    float SpecInt = pow(max(dot(N, halfwayDir), 0.0), 8.0); // Fixed shininess exponent

    // ---- Fresnel-Schlick Approximation ----
    vec3 F0 = vec3(0.04);  // Base Fresnel reflectance for non-metallic surfaces
    vec3 Specular = lightColor * SpecInt * F0; // Apply Fresnel effect

    // ---- Ambient Light ----
    vec3 ambientLight = vec3(1, 1, 1); // WARNING: Fully white ambient light may overpower other effects
    vec3 color = Diffuse + Specular + ambientLight * BaseColor; // Final color composition

    // Output final color
    outColor = vec4(color, 1.0);
}