#version 450
#extension GL_ARB_separate_shader_objects : enable

// Vertex attributes received from the pipeline input assembler.
// These must match the layout specified in the Vertex Descriptor
// and the corresponding C++ data structure.
layout(location = 0) in vec3 inPosition;  // Vertex position in model space
layout(location = 1) in vec3 inNorm;      // Vertex normal in model space
layout(location = 2) in vec2 inUV;        // Vertex UV texture coordinates
layout(location = 3) in vec4 inTan;       // Vertex tangent in model space (w component stores handedness)

// Variables passed to the Fragment Shader.
// The locations must match those declared in the Fragment Shader.
layout(location = 0) out vec3 fragPos;  // Vertex position in world space
layout(location = 1) out vec3 fragNorm; // Transformed normal vector in world space
layout(location = 2) out vec2 fragUV;   // Interpolated UV coordinates
layout(location = 3) out vec4 fragTan;  // Transformed tangent vector in world space

// Uniform buffer containing transformation matrices (Set 1, Binding 0).
// The layout must match the definition used in the C++ code.
layout(set = 1, binding = 0) uniform UniformBufferObject {
	mat4 mvpMat;  // Model-View-Projection matrix (transforms to clip space)
	mat4 mMat;    // Model matrix (transforms from model space to world space)
	mat4 nMat;    // Normal matrix (used to transform normals correctly)
} ubo;

// Main vertex shader function
void main() {
	// Compute the final clip-space position by transforming the vertex with the MVP matrix.
	// This value is stored in the built-in `gl_Position` variable.
	gl_Position = ubo.mvpMat * vec4(inPosition, 1.0);

	// Compute world-space position by transforming the vertex with the model matrix.
	fragPos = (ubo.mMat * vec4(inPosition, 1.0)).xyz;

	// Transform the normal vector using the normal matrix to ensure correct lighting calculations.
	fragNorm = (ubo.nMat * vec4(inNorm, 0.0)).xyz;

	// Transform the tangent vector using the normal matrix, keeping the handedness (w component).
	fragTan = vec4((ubo.nMat * vec4(inTan.xyz, 0.0)).xyz, inTan.w);

	// Pass through UV coordinates without modification (they are interpolated in the fragment shader).
	fragUV = inUV;
}
