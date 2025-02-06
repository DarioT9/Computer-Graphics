#version 450
#extension GL_ARB_separate_shader_objects : enable

// Vertex attributes received from the input mesh
// These must match the locations defined in the Vertex Descriptor
layout(location = 0) in vec3 inPosition; // Vertex position (in model space)
layout(location = 1) in vec3 inNorm;     // Vertex normal (in model space)
layout(location = 2) in vec2 inUV;       // UV texture coordinates
layout(location = 3) in vec4 inTan;      // Tangent vector (for normal mapping)

// Output variables sent to the Fragment Shader
// These locations must match the corresponding input locations in the Fragment Shader
layout(location = 0) out vec3 fragPos;  // Position in world space
layout(location = 1) out vec3 fragNorm; // Normal in world space
layout(location = 2) out vec2 fragUV;   // UV coordinates
layout(location = 3) out vec4 fragTan;  // Tangent vector in world space (not used here)

// Maximum number of lamp posts (instances) supported
const int NLAMPPOST = 256;

// Uniform Buffer Object containing transformation matrices for instanced rendering
// Each instance (lamp post) has its own set of transformation matrices
layout(set = 1, binding = 0) uniform UniformBufferObject {
	mat4 mvpMat[NLAMPPOST]; // Model-View-Projection matrix for each instance
	mat4 mMat[NLAMPPOST];   // Model matrix (transforms from model space to world space)
	mat4 nMat[NLAMPPOST];   // Normal matrix (for transforming normals correctly)
} ubo;

void main() {
	// Instance index determines which transformation matrix to use
	int i = gl_InstanceIndex;

	// Compute the final clip-space position by applying the MVP matrix for the current instance
	gl_Position = ubo.mvpMat[i] * vec4(inPosition, 1.0);

	// Compute world-space attributes and pass them to the Fragment Shader
	fragPos = (ubo.mMat[i] * vec4(inPosition, 1.0)).xyz; // Transform position to world space
	fragNorm = (ubo.nMat[i] * vec4(inNorm, 0.0)).xyz;     // Transform normal to world space
	fragUV = inUV;                                       // Pass UV coordinates unchanged
}