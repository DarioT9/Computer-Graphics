#version 450
#extension GL_ARB_separate_shader_objects : enable

// Vertex attributes received from the input mesh
// These must match the locations defined in the Vertex Descriptor
layout(location = 0) in vec3 inPosition; // Vertex position (in model space)
layout(location = 1) in vec2 inUV;       // UV texture coordinates

// Output variables sent to the Fragment Shader
// These locations must match the corresponding input locations in the Fragment Shader
layout(location = 0) out vec2 fragUV; // Interpolated UV coordinates

// Uniform Buffer Object containing transformation matrices
// - `mvpMat`: Model-View-Projection matrix used to transform vertex positions
layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 mvpMat;
} ubo;

void main() {
	// Compute the final clip-space position by applying the MVP matrix
	gl_Position = ubo.mvpMat * vec4(inPosition, 1.0);

	// Pass the UV coordinates to the Fragment Shader (interpolated across the triangle)
	fragUV = inUV;
}