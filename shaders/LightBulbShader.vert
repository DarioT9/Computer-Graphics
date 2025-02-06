#version 450
#extension GL_ARB_separate_shader_objects : enable

// Vertex attributes received from the input mesh
// These must match the locations defined in the Vertex Descriptor
layout(location = 0) in vec3 inPosition; // Vertex position in model space
layout(location = 1) in vec2 inUV;       // UV texture coordinates

// Output variable passed to the Fragment Shader
// The location must match the corresponding input location in the Fragment Shader
layout(location = 0) out vec2 fragUV; // UV coordinates (interpolated for the fragment shader)

// Maximum number of instances (lamp posts) supported
const int NLAMPPOST = 256;

// Uniform Buffer Object containing the Model-View-Projection (MVP) matrices
// Each instance (lamp post) has its own MVP matrix
layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 mvpMat[NLAMPPOST]; // MVP matrix for each instance
} ubo;

void main() {
	// Instance index determines which transformation matrix to use
	int i = gl_InstanceIndex;

	// Compute the final clip-space position by applying the MVP matrix for the current instance
	gl_Position = ubo.mvpMat[i] * vec4(inPosition, 1.0);

	// Pass the UV coordinates to the Fragment Shader unchanged
	fragUV = inUV;
}