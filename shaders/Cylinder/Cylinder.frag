#version 450
#extension GL_ARB_separate_shader_objects : enable

// Variabili ricevute dal Vertex Shader
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec4 fragTan;  // Tangente del frammento

// Output del colore calcolato
layout(location = 0) out vec4 outColor;
//const int NLAMPPOST=256;

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
    //PointLight PointLights[NLAMPPOST]; // Point lights
} gubo;

// Uniform per la texture del colore della città
layout(set = 1, binding = 1) uniform sampler2D TCylinderBaseColor;

// Shader principale
void main() {
    // Normali e Tangenti
    vec3 Norm = normalize(fragNorm);
    vec3 Tan = normalize(fragTan.xyz - Norm * dot(fragTan.xyz, Norm));  // Ortogonalizzazione della tangente
    vec3 Bitan = cross(Norm, Tan) * fragTan.w;  // Bitangente usando la tangente e la normale
    mat3 tbn = mat3(Tan, Bitan, Norm);  // Matrice TBN (Tangente, Bitangente, Normale)

    // Normal map (se disponibile)
    // Se avessi una normal map, qui si farebbe il campionamento e la trasformazione
    // Per ora manteniamo solo la normale interpolata:
    vec3 N = normalize(Norm);  // Usa la normale interpolata

    // Direzione della luce e dell'occhio
    vec3 EyeDir = normalize(gubo.eyePos - fragPos);
    vec3 lightDir = normalize(gubo.lightDir);
    vec3 lightColor = gubo.lightColor.rgb;

    // Valori texture (solo base color per la città)
    vec3 BaseColor = texture(TCylinderBaseColor, fragUV).rgb;

    // Illuminazione diffusa
    float DiffInt = max(dot(N, lightDir), 0.0);
    vec3 Diffuse = BaseColor * max(dot(N, lightDir), 0.0) * gubo.lightColor.rgb;

    // Riflessi speculari (Blinn-Phong semplice)
    vec3 halfwayDir = normalize(lightDir + EyeDir);
    float SpecInt = pow(max(dot(N, halfwayDir), 0.0), 8.0);  // Specularità fissa

    // Fattore Decay
    vec3 Decay = vec3(0.04);  // Decay
    vec3 Specular = lightColor * SpecInt * Decay;

    // Luce ambientale fissa
    vec3 ambientLight = vec3(1, 1, 1);
    vec3 color = Diffuse + Specular + ambientLight * BaseColor;

    // Output del colore finale
    outColor = vec4(color, 0.2);
}