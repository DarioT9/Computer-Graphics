#version 450
#extension GL_ARB_separate_shader_objects : enable

// Variabili ricevute dal Vertex Shader
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec4 fragTan;
const int NLAMPPOST=256;

// Output del colore calcolato
layout(location = 0) out vec4 outColor;

struct PointLight {
    vec3 position;   // Posizione della luce
    vec3 color;      // Colore della luce
    float intensity; // Intensità della luce
    float padding;   // Padding per allineamento
};

// Uniforms globali (luce e vista)
layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    vec3 lightDir;
    vec4 lightColor;
    vec3 eyePos;
    PointLight PointLights[NLAMPPOST];
} gubo;

// Uniforms specifici del materiale Scooter
layout(set = 1, binding = 1) uniform sampler2D TScooterBaseColor;
layout(set = 1, binding = 2) uniform sampler2D TScooterNormal;
layout(set = 1, binding = 3) uniform sampler2D TScooterHeight;
layout(set = 1, binding = 4) uniform sampler2D TScooterMetallic;
layout(set = 1, binding = 5) uniform sampler2D TScooterRoughness;
layout(set = 1, binding = 6) uniform sampler2D TScooterAmbientOcclusion;

// Shader principale
void main() {
	// Normali e Tangenti
	vec3 Norm = normalize(fragNorm);
	vec3 Tan = normalize(fragTan.xyz - Norm * dot(fragTan.xyz, Norm));
	vec3 Bitan = cross(Norm, Tan) * fragTan.w;
	mat3 tbn = mat3(Tan, Bitan, Norm);

	// Normal map
	vec4 nMap = texture(TScooterNormal, fragUV);
	vec3 N = normalize(tbn * (nMap.rgb * 2.0 - 1.0));  // Da [0,1] a [-1,1]

	// Direzioni luce e occhio
	vec3 EyeDir = normalize(gubo.eyePos - fragPos);
	vec3 lightDir = normalize(gubo.lightDir);
	vec3 lightColor = gubo.lightColor.rgb;

	// Valori texture
	vec3 BaseColor = texture(TScooterBaseColor, fragUV).rgb;
	float Metallic = texture(TScooterMetallic, fragUV).r;
	float Roughness = texture(TScooterRoughness, fragUV).r;
	float AmbientOcclusion = texture(TScooterAmbientOcclusion, fragUV).r;

	// Calcoli di illuminazione
	float DiffInt = max(dot(N, lightDir), 0.0);
	vec3 Diffuse = BaseColor * DiffInt * (1.0 - Metallic);  // Riduzione diffusione con Metallic

	// Riflessi speculari (Blinn-Phong con Fresnel-Schlick)
	vec3 halfwayDir = normalize(lightDir + EyeDir);
	float SpecInt = pow(max(dot(N, halfwayDir), 0.0), 32.0 * (1.0 - Roughness));

	// Effetto Fresnel-Schlick
	vec3 F0 = mix(vec3(0.04), BaseColor, Metallic);  // Fattore Fresnel
	vec3 Specular = lightColor * SpecInt * (F0 + (1.0 - F0) * pow(1.0 - max(dot(EyeDir, halfwayDir), 0.0), 5.0));

	// ---- Luci Spotlight ----
	vec3 pointLightColor = vec3(0.0);

	for (int i = 0; i < NLAMPPOST; ++i) {
		PointLight light = gubo.PointLights[i];

		vec3 pointLightDir = light.position.xyz - fragPos;
		float distance = length(pointLightDir);

		// Adjust max distance and intensity falloff
		float maxDistance = 7.0;  // Increased range
		if (distance > maxDistance) continue;

		vec3 lightDirNorm = normalize(pointLightDir);
		float cosAngIncidence = max(dot(N, lightDirNorm), 0.0);

		// Linear distance attenuation instead of quadratic
		float attenuation = max(0.0, 1.0 - distance / maxDistance);

		vec3 intensity = light.color * light.intensity * attenuation;

		// Diffuse and specular contributions
		vec3 pointLightDiffuse = BaseColor * cosAngIncidence * (1.0 - Metallic);

		vec3 halfwayDir = normalize(lightDirNorm + EyeDir);
		float specIntensity = pow(max(dot(N, halfwayDir), 0.0), 32.0 * (1.0 - Roughness));
		vec3 pointLightSpecular = specIntensity * F0;

		pointLightColor += (pointLightDiffuse + pointLightSpecular) * intensity;
	}

	// Luce ambientale e occlusione
	vec3 ambientLight = vec3(0.15, 0.15, 0.15);
	vec3 color = (Diffuse + Specular) * lightColor * AmbientOcclusion +
	ambientLight * BaseColor +
	pointLightColor;

	// Output del colore finale
	outColor = vec4(color, 1.0);
}
