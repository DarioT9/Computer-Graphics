#version 450
#extension GL_ARB_separate_shader_objects : enable

// Uniform Buffer Object containing the transformation matrix
// This matrix is used to transform the vertex position
layout(binding = 0) uniform UniformBufferObject {
    mat4 mvpMat; // Model-View-Projection (MVP) matrix
} ubo;

// Input vertex attributes
layout(location = 0) in vec3 inPosition; // Vertex position in model space

// Output variable passed to the Fragment Shader
layout(location = 0) out vec3 fragTexCoord; // Used for texture lookup in the Fragment Shader

void main()
{
    // Pass the original vertex position to the Fragment Shader
    // This is typically used for sampling a cube map in a skybox shader
    fragTexCoord = inPosition;

    // Compute the transformed vertex position
    vec4 pos = ubo.mvpMat * vec4(inPosition, 1.0);

    // Assign the final position to gl_Position
    // The .xyww swizzle is used to ensure correct depth values in a skybox
    gl_Position = pos.xyww;
}
