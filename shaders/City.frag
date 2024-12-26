#version 450
#extension GL_ARB_separate_shader_objects : enable

// Variabili ricevute dal Vertex Shader
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec4 fragTan;  // Tangente del frammento
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

// Uniform per la texture del colore della città
layout(set = 1, binding = 1) uniform sampler2D TCityBaseColor;


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
    vec3 BaseColor = texture(TCityBaseColor, fragUV).rgb;

    // Illuminazione diffusa
    float DiffInt = max(dot(N, lightDir), 0.0);
    vec3 Diffuse = BaseColor * max(dot(N, lightDir), 0.0) * gubo.lightColor.rgb;

    // Riflessi speculari (Blinn-Phong semplice)
    vec3 halfwayDir = normalize(lightDir + EyeDir);
    float SpecInt = pow(max(dot(N, halfwayDir), 0.0), 8.0);  // Specularità fissa

    // Fattore Fresnel-Schlick (valore base per oggetti non metallici)
    vec3 F0 = vec3(0.04);  // Fresnel di base per materiali dielettrici (non metallici)
    vec3 Specular = lightColor * SpecInt * F0;

     // ---- Luci Puntiformi ----
    vec3 diffusePointLight = vec3(0.0);

    for (int i = 0; i < NLAMPPOST; ++i) {
        PointLight light = gubo.PointLights[i];

        vec3 pointLightDir = light.position.xyz - fragPos;
        float distance = length(pointLightDir);  // Distance between the light and the fragment
        float attenuation = 1.0 / (distance * distance);
        float cosAngIncidence = max(dot(Norm, normalize(pointLightDir)), 0);
        vec3 intensity = light.color.xyz * light.intensity * attenuation;

        // Calcolare l'angolo tra la direzione della luce e la direzione del punto
        float angle = acos(dot(Norm, pointLightDir));

        // Spotlight cone cutoff angle (in radians)
        float cutoffAngle = radians(30.0);  // 30 degrees as the spotlight's cutoff angle
        float softEdge = 0.1;  // The softness of the edge (increase this for a softer falloff)

        // Use smoothstep for a smooth transition on the edge of the spotlight cone
        float spotEffect = smoothstep(cos(cutoffAngle + softEdge), cos(cutoffAngle), cosAngIncidence);

        diffusePointLight += intensity * cosAngIncidence * spotEffect;  // Apply spotlight with smooth edge
        

    }

    // Luce ambientale fissa
    vec3 ambientLight = vec3(0.1, 0.1, 0.2);
    vec3 color = diffusePointLight + Diffuse + Specular + ambientLight * BaseColor;

    // Output del colore finale
    outColor = vec4(color, 1.0);
}
