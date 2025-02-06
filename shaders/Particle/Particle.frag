#version 450
#extension GL_ARB_separate_shader_objects : enable

// Inputs received from the Vertex Shader
layout(location = 0) in vec3 fragPos;    // Fragment position in world space
layout(location = 1) in vec3 fragNorm;   // Interpolated normal vector
layout(location = 2) in vec2 fragUV;     // Texture coordinates
layout(location = 3) in vec4 fragTan;    // Fragment tangent vector

// Output color computed by the fragment shader
layout(location = 0) out vec4 outColor;
const int NPARTICLES = 3;  // Number of point light sources

// Structure representing a point light
struct PointLight {
    vec3 position;   // Light position in world space
    vec3 color;      // Light color
    float intensity; // Light intensity factor
    float padding;   // Padding for alignment (unused)
};

// Global uniforms containing light and camera information
layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    vec3 lightDir;   // Directional light direction
    vec4 lightColor; // Directional light color
    vec3 eyePos;     // Camera position in world space
    PointLight PointLights[NPARTICLES];  // Array of point lights
} gubo;

// Uniform for the base color texture of particles
layout(set = 1, binding = 1) uniform sampler2D TParticleBaseColor;

// Main fragment shader function
void main() {
    // Compute Normal, Tangent, and Bitangent vectors
    vec3 Norm = normalize(fragNorm);
    vec3 Tan = normalize(fragTan.xyz - Norm * dot(fragTan.xyz, Norm)); // Orthogonalized tangent
    vec3 Bitan = cross(Norm, Tan) * fragTan.w;  // Compute Bitangent
    mat3 tbn = mat3(Tan, Bitan, Norm);  // TBN (Tangent, Bitangent, Normal) matrix

    // Normal mapping (if available)
    // Here, only the interpolated normal is used
    vec3 N = normalize(Norm);

    // Compute viewing direction and directional light properties
    vec3 EyeDir = normalize(gubo.eyePos - fragPos);
    vec3 lightDir = normalize(gubo.lightDir);
    vec3 lightColor = gubo.lightColor.rgb;

    // Sample the base color texture for the particles
    vec3 BaseColor = texture(TParticleBaseColor, fragUV).rgb;

    // Diffuse shading (Lambertian reflection)
    float DiffInt = max(dot(N, lightDir), 0.0);
    vec3 Diffuse = BaseColor * DiffInt * lightColor;

    // Specular reflection using Blinn-Phong model
    vec3 halfwayDir = normalize(lightDir + EyeDir);
    float SpecInt = pow(max(dot(N, halfwayDir), 0.0), 8.0);  // Fixed specular exponent

    // Fresnel-Schlick approximation (base value for non-metallic materials)
    vec3 F0 = vec3(0.04);  // Default Fresnel reflection factor for dielectrics
    vec3 Specular = lightColor * SpecInt * F0;

    // Fixed ambient lighting to simulate global illumination
    vec3 ambientLight = vec3(0.8, 0.8, 0.8);
    vec3 color = Diffuse + Specular + ambientLight * BaseColor;

    // Final output color
    outColor = vec4(color, 1.0);
}