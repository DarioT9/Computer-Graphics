#version 450
#extension GL_ARB_separate_shader_objects : enable

// Variables received from the Vertex Shader
layout(location = 0) in vec3 fragPos;  // Fragment position in world space
layout(location = 1) in vec3 fragNorm; // Interpolated normal from the vertex shader
layout(location = 2) in vec2 fragUV;   // UV coordinates for texturing
layout(location = 3) in vec4 fragTan;  // Fragment tangent for normal mapping

const int NLAMPPOST = 256; // Maximum number of point lights

// Output of the final computed color
layout(location = 0) out vec4 outColor;

// Structure representing a point light source
struct SpotLight {
	vec3 position;   // Light position in world space
	vec3 color;      // Light color
	float intensity; // Light intensity factor
	float padding;   // Padding for memory alignment
};

// Global uniforms (scene-wide lighting and camera information)
layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
	vec3 lightDir;  // Directional light direction (e.g., sunlight)
	vec4 lightColor; // Directional light color
	vec3 eyePos;    // Camera (eye) position in world space
	SpotLight SpotLights[NLAMPPOST]; // Array of point lights
} gubo;

// Scooter material-specific texture uniforms
layout(set = 1, binding = 1) uniform sampler2D TScooterBaseColor;     // Albedo texture
layout(set = 1, binding = 2) uniform sampler2D TScooterNormal;        // Normal map texture
layout(set = 1, binding = 3) uniform sampler2D TScooterHeight;        // Height map texture (not used in this shader)
layout(set = 1, binding = 4) uniform sampler2D TScooterMetallic;      // Metallic texture
layout(set = 1, binding = 5) uniform sampler2D TScooterRoughness;     // Roughness texture
layout(set = 1, binding = 6) uniform sampler2D TScooterAmbientOcclusion; // Ambient occlusion texture

// Main shader program
void main() {
	// --- Normal Mapping ---
	vec3 Norm = normalize(fragNorm); // Normalize the interpolated normal
	vec3 Tan = normalize(fragTan.xyz - Norm * dot(fragTan.xyz, Norm)); // Orthogonalize tangent
	vec3 Bitan = cross(Norm, Tan) * fragTan.w; // Compute bitangent
	mat3 tbn = mat3(Tan, Bitan, Norm); // TBN matrix (Tangent, Bitangent, Normal)

	// Sample the normal map and transform it into world space
	vec4 nMap = texture(TScooterNormal, fragUV);
	vec3 N = normalize(tbn * (nMap.rgb * 2.0 - 1.0));  // Convert from [0,1] to [-1,1] range

	// --- Light and View Direction ---
	vec3 EyeDir = normalize(gubo.eyePos - fragPos); // View direction
	vec3 lightDir = normalize(gubo.lightDir); // Directional light direction
	vec3 lightColor = gubo.lightColor.rgb; // Directional light color

	// --- Texture Data ---
	vec3 BaseColor = texture(TScooterBaseColor, fragUV).rgb; // Albedo color
	float Metallic = texture(TScooterMetallic, fragUV).r;    // Metallic factor
	float Roughness = texture(TScooterRoughness, fragUV).r;  // Roughness factor
	float AmbientOcclusion = texture(TScooterAmbientOcclusion, fragUV).r; // AO factor

	// --- Lambertian Diffuse Lighting ---
	float DiffInt = max(dot(N, lightDir), 0.0);
	vec3 Diffuse = BaseColor * DiffInt * (1.0 - Metallic);  // Reduce diffuse if metallic

	// --- Specular Reflection (Blinn-Phong with Fresnel-Schlick) ---
	vec3 halfwayDir = normalize(lightDir + EyeDir);
	float SpecInt = pow(max(dot(N, halfwayDir), 0.0), 32.0 * (1.0 - Roughness));

	// Fresnel-Schlick approximation for specular reflectance
	vec3 F0 = mix(vec3(0.04), BaseColor, Metallic);  // Base reflectance
	vec3 Specular = lightColor * SpecInt * (F0 + (1.0 - F0) * pow(1.0 - max(dot(EyeDir, halfwayDir), 0.0), 5.0));

	// --- Point Light Calculation ---
	vec3 spotLightColor = vec3(0.0);

	for (int i = 0; i < NLAMPPOST; ++i) {
		SpotLight light = gubo.SpotLights[i];

		vec3 spotLightDir = light.position.xyz - fragPos;
		float distance = length(spotLightDir);

		// Skip lights beyond max influence distance
		float maxDistance = 7.0;  // Light falloff range
		if (distance > maxDistance) continue;

		vec3 lightDirNorm = normalize(spotLightDir);
		float cosAngIncidence = max(dot(N, lightDirNorm), 0.0);

		// Linear distance attenuation (instead of quadratic)
		float attenuation = max(0.0, 1.0 - distance / maxDistance);

		vec3 intensity = light.color * light.intensity * attenuation;

		// Diffuse and specular contribution from the point light
		vec3 spotLightDiffuse = BaseColor * cosAngIncidence * (1.0 - Metallic);

		vec3 halfwayDir = normalize(lightDirNorm + EyeDir);
		float specIntensity = pow(max(dot(N, halfwayDir), 0.0), 32.0 * (1.0 - Roughness));
		vec3 spotLightSpecular = specIntensity * F0;

		// Accumulate the contribution of each point light
		spotLightColor += (spotLightDiffuse + spotLightSpecular) * intensity;
	}

	// --- Ambient Lighting & Final Color ---
	vec3 ambientLight = vec3(0.15, 0.15, 0.15); // Constant ambient lighting
	vec3 color = (Diffuse + Specular) * lightColor * AmbientOcclusion +
	ambientLight * BaseColor +
	spotLightColor;

	// Output the final computed color
	outColor = vec4(color, 1.0);
}