#version 450
#extension GL_ARB_separate_shader_objects : enable

// Inputs received from the Vertex Shader
layout(location = 0) in vec3 fragPos;    // Fragment position in world space
layout(location = 1) in vec3 fragNorm;   // Interpolated normal vector
layout(location = 2) in vec2 fragUV;     // Texture coordinates
layout(location = 3) in vec4 fragTan;    // Fragment tangent

const int NLAMPPOST = 256;  // Maximum number of point lights

// Output color computed by the fragment shader
layout(location = 0) out vec4 outColor;

// Structure to represent a point light source
struct PointLight {
    vec3 position;   // Light position in world space
    vec3 color;      // Light color
    float intensity; // Light intensity factor
    float padding;   // Padding for alignment (unused)
};

// Global uniforms (light and camera information)
layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    vec3 lightDir;   // Directional light direction
    vec4 lightColor; // Directional light color
    vec3 eyePos;     // Camera position in world space
    PointLight PointLights[NLAMPPOST];  // Array of point lights
} gubo;

// Uniform for the city base color texture
layout(set = 1, binding = 1) uniform sampler2D TCityBaseColor;

// Main fragment shader
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

    // Sample the base color texture for the city surface
    vec3 BaseColor = texture(TCityBaseColor, fragUV).rgb;

    // Diffuse shading (Lambertian reflection)
    float DiffInt = max(dot(N, lightDir), 0.0);
    vec3 Diffuse = BaseColor * DiffInt * lightColor;

    // Specular reflection using Blinn-Phong model
    vec3 halfwayDir = normalize(lightDir + EyeDir);
    float SpecInt = pow(max(dot(N, halfwayDir), 0.0), 8.0);  // Fixed specular exponent

    // Decay approximation
    vec3 Decay = vec3(0.15);  // Decay
    vec3 Specular = lightColor * SpecInt * Decay;

    // ---- Point Light Contribution ----
    vec3 diffusePointLight = vec3(0.0);

    for (int i = 0; i < NLAMPPOST; ++i) {
        PointLight light = gubo.PointLights[i];

        vec3 pointLightDir = light.position - fragPos;
        float distance = length(pointLightDir);  // Compute distance to the light source
        float attenuation = 1.0 / max(distance * distance, 0.01);   // Inverse square law
        float cosAngIncidence = max(dot(N, normalize(pointLightDir)), 0.0);
        vec3 intensity = light.color * light.intensity * attenuation;

        // Compute the angle between the light direction and the normal
        float angle = acos(dot(Norm, normalize(pointLightDir)));

        // Spotlight cutoff angle (in radians)
        float cutoffAngle = radians(22.5);  // Spotlight cone limit at 22.5 degrees
        float softEdge = 0.2;  // Soft transition for the spotlight edge

        // Smooth transition at the edge of the spotlight using smoothstep
        float spotEffect = smoothstep(cos(cutoffAngle + softEdge), cos(cutoffAngle), cosAngIncidence);

        diffusePointLight += intensity * cosAngIncidence * spotEffect;  // Apply spotlight effect
    }

    // Fixed ambient lighting to simulate global illumination
    vec3 ambientLight = vec3(0.05, 0.05, 0.05);
    vec3 color = diffusePointLight + Diffuse + Specular + ambientLight * BaseColor;

    // Final output color
    outColor = vec4(color, 1.0);
}
