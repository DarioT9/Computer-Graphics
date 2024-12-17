#version 450
layout(location = 0) in vec3 inPos;  // Posizione del vertice
layout(set = 0, binding = 0) uniform DebugMatricesUBO {
    mat4 mvpMat;
} ubo;

void main() {
    gl_Position = ubo.mvpMat * vec4(inPos, 1.0);
}
