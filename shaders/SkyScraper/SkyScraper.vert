#version 450
#extension GL_ARB_separate_shader_objects : enable

// Vertex attributes received from the input mesh
// These must match the locations defined in the Vertex Descriptor
layout(location = 0) in vec3 inPosition;  // Vertex position in model space
layout(location = 1) in vec3 inNorm;      // Normal vector in model space
layout(location = 2) in vec2 inUV;        // UV texture coordinates
layout(location = 3) in vec4 inTan;       // Tangent vector (XYZ: direction, W: handedness)

// Output variables passed to the Fragment Shader
// The locations must match the corresponding input locations in the Fragment Shader
layout(location = 0) out vec3 fragPos;   // Vertex position in world space
layout(location = 1) out vec3 fragNorm;  // Normal vector in world space
layout(location = 2) out vec2 fragUV;    // UV texture coordinates
layout(location = 3) out vec4 fragTan;   // Transformed tangent vector

// Maximum number of skyscrapers supported in the scene
const int NSKYSCRAPER = 16;

// Uniform Buffer Object containing transformation matrices for each instance
// Each instance (skyscraper) has its own set of matrices
layout(set = 1, binding = 0) uniform UniformBufferObject {
	mat4 mvpMat[NSKYSCRAPER]; // Model-View-Projection (MVP) matrix for each instance
	mat4 mMat[NSKYSCRAPER];   // Model matrix (transforms from local to world space)
	mat4 nMat[NSKYSCRAPER];   // Normal matrix (for correct normal transformations)
} ubo;

void main() {
	// Get the instance index to apply the correct transformation matrices
	int i = gl_InstanceIndex;

	// Compute the final clip-space position using the instance's MVP matrix
	gl_Position = ubo.mvpMat[i] * vec4(inPosition, 1.0);

	// Transform the vertex position into world space
	fragPos = (ubo.mMat[i] * vec4(inPosition, 1.0)).xyz;

	// Transform the normal vector using the normal matrix (ignoring translation)
	fragNorm = (ubo.nMat[i] * vec4(inNorm, 0.0)).xyz;

	// Transform the tangent vector using the normal matrix (keeping the handedness value)
	fragTan = vec4((ubo.nMat[i] * vec4(inTan.xyz, 0.0)).xyz, inTan.w);

	// Pass the UV coordinates to the Fragment Shader unchanged
	fragUV = inUV;
}