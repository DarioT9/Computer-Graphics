#include "headers/tiny_obj_loader.h"
#include "modules/Starter.hpp"
#include "WVP.hpp"

#include "modules/TextMaker.hpp"

#include <thread>
#include <chrono>

#define PI 3.14159265359

// TextMaker instance for creating text
TextMaker txt;

// Vector of SingleText objects for output text
std::vector<SingleText> outText = {{2, {"test", "", "", ""}, 0, 0}};

// Struct representing a 2D bounding box (minimum and maximum points)
struct BoundingBox {
    glm::vec2 min;  // Minimum coordinates (x, y)
    glm::vec2 max;  // Maximum coordinates (x, y)
};

// Global uniform buffer object holding light direction, light color, and eye position
struct GlobalUniformBufferObject {
    alignas(16) glm::vec3 lightDir;  // Direction of light
    alignas(16) glm::vec4 lightColor; // Color of the light
    alignas(16) glm::vec3 eyePos;  // Eye (camera) position
};

// Struct for object matrices, including Model-View-Projection (MVP), Model, and Normal matrices
struct SingleObjectMatricesUniformBufferObject {
    alignas(16) glm::mat4 mvpMat; // Model-View-Projection matrix
    alignas(16) glm::mat4 mMat;   // Model matrix
    alignas(16) glm::mat4 nMat;   // Normal matrix
};

#define NSKYSCRAPER 16
// Arrays holding indices for each skyscraper model
int indicesSkyScraper1[NSKYSCRAPER] = {3, 15, 8, 30, 12, 5, 26, 1, 9, 16, 22, 37, 40, 49, 61};
int indicesSkyScraper2[NSKYSCRAPER] = {2, 6, 10, 18, 20, 25, 33, 41, 45, 50, 52, 53, 54, 56, 58, 60};
int indicesSkyScraper3[NSKYSCRAPER] = {4, 7, 11, 14, 19, 21, 23, 29, 31, 32, 34, 39, 42, 44, 47, 48};
int indicesSkyScraper4[NSKYSCRAPER] = {0, 9, 13, 17, 24, 27, 35, 36, 38, 43, 46, 51, 55, 57, 59, 62};

// Struct for skyscraper matrices, including MVP, Model, and Normal matrices
struct SkyScraperMatricesUniformBufferObject {
    alignas(16) glm::mat4 mvpMat[NSKYSCRAPER];  // Model-View-Projection matrix for each skyscraper
    alignas(16) glm::mat4 mMat[NSKYSCRAPER];    // Model matrix for each skyscraper
    alignas(16) glm::mat4 nMat[NSKYSCRAPER];    // Normal matrix for each skyscraper
};

#define NTREE 64
// Arrays holding indices for each tree model
int indicesTree1[16] = {3, 15, 8, 30, 12, 5, 26, 1, 9, 16, 22, 28, 37, 40, 49, 61};
int indicesTree2[16] = {2, 6, 10, 18, 20, 25, 33, 41, 45, 50, 52, 53, 54, 56, 58, 60};
int indicesTree3[16] = {4, 7, 11, 14, 19, 21, 23, 29, 31, 32, 34, 39, 42, 44, 47, 48};
int indicesTree4[16] = {0, 9, 13, 17, 24, 27, 35, 36, 38, 43, 46, 51, 55, 57, 59, 62};

// Struct for tree matrices, including MVP, Model, and Normal matrices
struct TreeMatricesUniformBufferObject {
    alignas(16) glm::mat4 mvpMat[NTREE];  // Model-View-Projection matrix for each tree
    alignas(16) glm::mat4 mMat[NTREE];    // Model matrix for each tree
    alignas(16) glm::mat4 nMat[NTREE];    // Normal matrix for each tree
};

// Function to update tree matrices, including translation, scaling, and matrix calculations
void updateTreeMatrices(TreeMatricesUniformBufferObject* treeUbo, int& counter, int i, int offset, const glm::mat4& ViewPrj, bool applyScale) {
    for (int k = 0; k < 4; ++k) {
        glm::vec3 translation;
        // Define translation based on the case value (k)
        switch (k) {
            case 0: translation = glm::vec3(75 - (24 * (i % 8)), 0.0, 85 - offset - (24 * (i / 8))); break;
            case 1: translation = glm::vec3(84 + offset - (24 * (i % 8)), 0.0, 75 - (24 * (i / 8))); break;
            case 2: translation = glm::vec3(93 - (24 * (i % 8)), 0.0, 85 + offset - (24 * (i / 8))); break;
            case 3: translation = glm::vec3(84 - offset - (24 * (i % 8)), 0.0, 93 - (24 * (i / 8))); break;
        }

        int idx = counter * 4 + k;
        treeUbo->mMat[idx] = glm::translate(glm::mat4(1.0f), translation);

        // Apply scaling if requested
        if (applyScale) {
            treeUbo->mMat[idx] = glm::scale(treeUbo->mMat[idx], glm::vec3(0.5f, 0.5f, 0.5f));
        }

        // Calculate the Model-View-Projection matrix
        treeUbo->mvpMat[idx] = ViewPrj * treeUbo->mMat[idx];
        // Calculate the Normal matrix (transpose of the inverse of the Model matrix)
        treeUbo->nMat[idx] = glm::inverse(glm::transpose(treeUbo->mMat[idx]));
    }

    ++counter;
}

#define NLAMPPOST 256
// Struct for lamp post matrices, including MVP, Model, and Normal matrices
struct LampPostMatricesUniformBufferObject {
    alignas(16) glm::mat4 mvpMat[NLAMPPOST];  // Model-View-Projection matrix for each lamp post
    alignas(16) glm::mat4 mMat[NLAMPPOST];    // Model matrix for each lamp post
    alignas(16) glm::mat4 nMat[NLAMPPOST];    // Normal matrix for each lamp post
};

// Struct for skybox matrices (used for rendering the skybox)
struct skyBoxUniformBufferObject {
    alignas(16) glm::mat4 mvpMat;  // Model-View-Projection matrix for the skybox
};

// Struct for emission matrices (used for objects that emit light)
struct EmissionUniformBufferObject {
    alignas(16) glm::mat4 mvpMat;  // Model-View-Projection matrix for emission objects
};

// Object parameters structure for controlling light emission (e.g., power and angle)
struct ObjectParametersUniformBufferObject {
    alignas(4) float Pow;  // Power of the emission
    alignas(4) float Ang;  // Angle of the emission
};

// Vertex structure for objects (position, normal, texture coordinates, and tangent vector)
struct ObjectVertex {
    glm::vec3 pos;  // Position in 3D space
    glm::vec3 norm; // Normal vector
    glm::vec2 UV;   // Texture coordinates
    glm::vec4 tan;  // Tangent vector for normal mapping
};

// Vertex structure for the skybox (only position needed)
struct skyBoxVertex {
    glm::vec3 pos;  // Position in 3D space
};

// Vertex structure for emission objects (position and texture coordinates)
struct EmissionVertex {
    glm::vec3 pos;  // Position in 3D space
    glm::vec2 UV;   // Texture coordinates
};

// Structure to store model offsets (name, vertex offset, index offset, and index count)
struct ModelOffsets {
    std::string name;      // Name of the model
    uint32_t vertexOffset; // Starting vertex offset in the buffer
    uint32_t indexOffset;  // Starting index offset in the buffer
    uint32_t indexCount;   // Number of indices for this model
};

// Function to calculate model offsets for each shape in the OBJ file
std::vector<ModelOffsets> calculateOffsets(const std::string& filename) {
    tinyobj::ObjReader reader;

    // Parse the OBJ file
    if (!reader.ParseFromFile(filename)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader Error: " << reader.Error() << std::endl;
        }
        throw std::runtime_error("Failed to parse OBJ file.");
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();

    std::vector<ModelOffsets> modelOffsets;

    uint32_t globalVertexOffset = 0;
    uint32_t globalIndexOffset = 0;

    // Iterate over each shape in the OBJ file
    for (const auto& shape : shapes) {
        ModelOffsets offsets;
        offsets.name = shape.name;

        offsets.vertexOffset = globalVertexOffset;
        offsets.indexOffset = globalIndexOffset;
        offsets.indexCount = static_cast<uint32_t>(shape.mesh.indices.size());

        // Update cumulative offsets
        // Note: Index data is reused; vertex data must advance by unique vertices used
        std::set<int> uniqueVertices;
        for (const auto& index : shape.mesh.indices) {
            uniqueVertices.insert(index.vertex_index);
        }
        globalVertexOffset += static_cast<uint32_t>(uniqueVertices.size());
        globalIndexOffset += offsets.indexCount;

        modelOffsets.push_back(offsets);
    }

    return modelOffsets;
}



// MAIN
class A10 : public BaseProject {
protected:

    // Descriptor Layouts ["classes" of what will be passed to the shaders]
    DescriptorSetLayout DSLGlobal;	// For global values (lighting, camera, etc.)

    // Descriptor layouts for specific objects in the scene
    DescriptorSetLayout DSLScooter;    // Descriptor layout for the Scooter object
    DescriptorSetLayout DSLScooterFWheel; // Descriptor layout for the Scooter front wheel
    DescriptorSetLayout DSLCity;        // Descriptor layout for the City model
    DescriptorSetLayout DSLPizzeria;    // Descriptor layout for the Pizzeria model
    DescriptorSetLayout DSLSkyScraper[4]; // Descriptor layouts for the 4 skyscrapers
    DescriptorSetLayout DSLTree[4];     // Descriptor layouts for the 4 trees
    DescriptorSetLayout DSLLampPost;    // Descriptor layout for LampPost
    DescriptorSetLayout DSLskyBox;      // Descriptor layout for the SkyBox
    DescriptorSetLayout DSLEmission;    // Descriptor layout for emission materials

    // Vertex descriptors for defining the layout of vertex data for various objects
    VertexDescriptor VDScooter;         // Vertex descriptor for the Scooter model
    VertexDescriptor VDScooterFWheel;   // Vertex descriptor for the Scooter front wheel
    VertexDescriptor VDCity;            // Vertex descriptor for the City model
    VertexDescriptor VDPizzeria;        // Vertex descriptor for the Pizzeria model
    VertexDescriptor VDSkyScraper[4];   // Vertex descriptors for the 4 skyscrapers
    VertexDescriptor VDTree[4];         // Vertex descriptors for the 4 trees
    VertexDescriptor VDLampPost;        // Vertex descriptor for the LampPost model
    VertexDescriptor VDskyBox;          // Vertex descriptor for the SkyBox
    VertexDescriptor VDEmission;        // Vertex descriptor for the emission model

    // Pipelines for rendering the various objects
    Pipeline PScooter;                  // Pipeline for rendering the Scooter
    Pipeline PScooterFWheel;            // Pipeline for rendering the Scooter front wheel
    Pipeline PCity;                     // Pipeline for rendering the City
    Pipeline PPizzeria;                 // Pipeline for rendering the Pizzeria
    Pipeline PSkyScraper[4];            // Pipelines for rendering the 4 skyscrapers
    Pipeline PTree[4];                  // Pipelines for rendering the 4 trees
    Pipeline PLampPost;                 // Pipeline for rendering the LampPost
    Pipeline PskyBox;                   // Pipeline for rendering the SkyBox
    Pipeline PEmission;                 // Pipeline for rendering the emission objects

    // Models, textures, and descriptor sets for various objects
    DescriptorSet DSGlobal;             // Global descriptor set for uniforms like lighting and camera position

    // Model and texture variables for different objects in the scene
    Model MScooter;                     // Model for the Scooter
    Model MFWheel;                      // Model for the Scooter's front wheel
    Model MCity;                        // Model for the City
    Model MPizzeria;                    // Model for the Pizzeria
    Model MSkyScraper[4];               // Models for the 4 skyscrapers
    Model MTree[4];                     // Models for the 4 trees
    Model MLampPost;                    // Model for the LampPost
    Model MskyBox;                      // Model for the SkyBox
    Model Msun;                         // Model for the Sun (possibly for lighting)

    // Textures for the various objects in the scene
    Texture TScooterBaseColor, TScooterNormal, TScooterHeight, TScooterMetallic, TScooterRoughness, TScooterAmbientOcclusion; // Textures for the Scooter
    Texture TCity;                      // Texture for the City
    Texture TPizzeria;                  // Texture for the Pizzeria
    Texture TSkyScraper[4];             // Textures for the 4 skyscrapers
    Texture TTree[4];                   // Textures for the 4 trees
    Texture TLampPost;                  // Texture for the LampPost
    Texture TskyBox;                    // Texture for the SkyBox
    Texture Tsun;                       // Texture for the Sun

    // Descriptor sets for the various objects
    DescriptorSet DSScooter;            // Descriptor set for the Scooter
    DescriptorSet DSScooterFWheel;      // Descriptor set for the Scooter front wheel
    DescriptorSet DSCity;               // Descriptor set for the City
    DescriptorSet DSPizzeria;           // Descriptor set for the Pizzeria
    DescriptorSet DSSkyScraper[4];      // Descriptor sets for the 4 skyscrapers
    DescriptorSet DSTree[4];            // Descriptor sets for the 4 trees
    DescriptorSet DSLampPost;           // Descriptor set for the LampPost
    DescriptorSet DSskyBox;             // Descriptor set for the SkyBox
    DescriptorSet DSsun;                // Descriptor set for the Sun

    // Other application parameters for scene management and camera controls
    int currScene = 0;                  // Current scene index
    int subpass = 0;                    // Current subpass in the rendering pipeline

    glm::vec3 CamPos = glm::vec3(0.0, 0.1, 5.0); // Camera position in the scene
    glm::mat4 ViewMatrix;               // View matrix for camera transformations

    float Ar;                           // Aspect ratio of the window
    glm::vec3 ScooterPos;               // Position of the Scooter in the scene
    glm::vec3 InitialPos;               // Initial position of the objects
    float Yaw = 0;                      // Yaw (rotation around the Y-axis)
    float Roll = 0;                     // Roll (rotation around the Z-axis)

    // Generates centers for the buildings, going from -84 to 84 with a step of 24
    std::vector<glm::vec2> centers = generateCenters(84, 24);

    // List of barriers (objects that may block the movement of other objects)
    std::vector<BoundingBox> barriers = getBarriers(centers, 20);
    BoundingBox externalBarriers = {{-98.0f, -98.0f}, {98.0f, 98.0f}}; // External barriers

    // Set the main application parameters for the window and display
    void setWindowParameters() {
        // Set the window size, title, and initial background color
        windowWidth = 800;
        windowHeight = 600;
        windowTitle = "A10 - Adding an object"; // Window title
        windowResizable = GLFW_TRUE;           // Allow window resizing
        initialBackgroundColor = {0.1f, 0.1f, 0.1f, 1.0f}; // Initial background color (dark gray)

        Ar = (float)windowWidth / (float)windowHeight; // Calculate aspect ratio
    }

    // Adjust aspect ratio when the window size changes
    void onWindowResize(int w, int h) {
        std::cout << "Window resized to: " << w << " x " << h << "\n";
        Ar = (float)w / (float)h; // Update aspect ratio
    }


    // Here the Vulkan Models and Textures are loaded and set up.
// Descriptor set layouts are created and shaders are loaded for the pipelines.
    void localInit() {

        // Initialize the Descriptor Layout for Global values.
        DSLGlobal.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject),
                 1}
        });

        // Initialize the Descriptor Set Layout for the Scooter model with necessary bindings for uniform buffers and textures.
        DSLScooter.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT,   sizeof(SingleObjectMatricesUniformBufferObject), 1},  // Uniform matrix for the scooter model
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,                                               1},  // Base Color Texture
                {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1,                                               1},  // Normal Map Texture
                {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2,                                               1},  // Height Map Texture
                {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3,                                               1},  // Metallic Texture
                {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4,                                               1},  // Roughness Texture
                {6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5,                                               1},  // Ambient Occlusion Texture
                {7, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectParametersUniformBufferObject),     1},
        });

        // Initialize the Descriptor Set Layout for the City model with uniform buffers and textures.
        DSLCity.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT,   sizeof(SingleObjectMatricesUniformBufferObject), 1},  // Uniform matrix for the city model
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,                                               1},  // Base Color Texture
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectParametersUniformBufferObject),     1},
        });

        // Initialize the Descriptor Set Layout for the Pizzeria model with uniform buffers and textures.
        DSLPizzeria.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT,   sizeof(SingleObjectMatricesUniformBufferObject), 1},  // Uniform matrix for the pizzeria model
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,                                               1},  // Base Color Texture
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectParametersUniformBufferObject),     1},
        });

        // Initialize Descriptor Set Layouts for multiple SkyScraper models with appropriate bindings.
        for (int i = 0; i < 4; i++) {
            DSLSkyScraper[i].init(this, {
                    {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT,   sizeof(SkyScraperMatricesUniformBufferObject), 1},
                    {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,                                             1},
                    {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectParametersUniformBufferObject),   1},
            });
        }

        // Initialize Descriptor Set Layouts for multiple Tree models with appropriate bindings.
        for (int i = 0; i < 4; i++) {
            DSLTree[i].init(this, {
                    {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT,   sizeof(TreeMatricesUniformBufferObject),     1},  // Uniform matrix for the tree model
                    {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,                                           1},  // Texture for the tree model
                    {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectParametersUniformBufferObject), 1},
            });
        }

        // Initialize Descriptor Set Layout for the LampPost model with uniform buffers and textures.
        DSLLampPost.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT,   sizeof(LampPostMatricesUniformBufferObject), 1},  // Uniform matrix for the lamp post model
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,                                           1},  // Base Color Texture
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectParametersUniformBufferObject), 1},
        });

        // Initialize Descriptor Set Layout for the SkyBox with uniform buffers and textures.
        DSLskyBox.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT,   sizeof(skyBoxUniformBufferObject), 1},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,                                 1}
        });

        // Initialize Descriptor Set Layout for the Emission object with uniform buffers and textures.
        DSLEmission.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT,   sizeof(EmissionUniformBufferObject), 1},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,                                   1}
        });

        // Initialize the Vertex Descriptor for the Scooter model with vertex attributes and input rate.
        VDScooter.init(this, {
                {0, sizeof(ObjectVertex),
                 VK_VERTEX_INPUT_RATE_VERTEX}  // Describes the size of the vertex and input rate
        }, {
                               // Describes the vertex attribute layout
                               {0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(ObjectVertex,
                                                                              pos),                sizeof(glm::vec3), POSITION},  // Position (3 float components)
                               {0, 1, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(ObjectVertex,
                                                                              norm),               sizeof(glm::vec3), NORMAL},    // Normal (3 float components)
                               {0, 2, VK_FORMAT_R32G32_SFLOAT,       offsetof(ObjectVertex,
                                                                              UV),                 sizeof(glm::vec2), UV},             // UV (2 float components)
                               {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ObjectVertex,
                                                                              tan),                sizeof(glm::vec4), TANGENT}  // Tangent (4 float components)
                       });

        // Initialize the Vertex Descriptor for the City model with vertex attributes and input rate.
        VDCity.init(this, {
                {0, sizeof(ObjectVertex),
                 VK_VERTEX_INPUT_RATE_VERTEX}  // Describes the size of the vertex and input rate
        }, {
                            // Describes the vertex attribute layout
                            {0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(ObjectVertex,
                                                                           pos),                sizeof(glm::vec3), POSITION},  // Position (3 float components)
                            {0, 1, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(ObjectVertex,
                                                                           norm),               sizeof(glm::vec3), NORMAL},    // Normal (3 float components)
                            {0, 2, VK_FORMAT_R32G32_SFLOAT,       offsetof(ObjectVertex,
                                                                           UV),                 sizeof(glm::vec2), UV},             // UV (2 float components)
                            {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ObjectVertex,
                                                                           tan),                sizeof(glm::vec4), TANGENT}  // Tangent (4 float components)
                    });

        // Initialize the Vertex Descriptor for the Pizzeria model with vertex attributes and input rate.
        VDPizzeria.init(this, {
                {0, sizeof(ObjectVertex),
                 VK_VERTEX_INPUT_RATE_VERTEX}  // Describes the size of the vertex and input rate
        }, {
                                // Describes the vertex attribute layout
                                {0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(ObjectVertex,
                                                                               pos),                sizeof(glm::vec3), POSITION},  // Position (3 float components)
                                {0, 1, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(ObjectVertex,
                                                                               norm),               sizeof(glm::vec3), NORMAL},    // Normal (3 float components)
                                {0, 2, VK_FORMAT_R32G32_SFLOAT,       offsetof(ObjectVertex,
                                                                               UV),                 sizeof(glm::vec2), UV},             // UV (2 float components)
                                {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ObjectVertex,
                                                                               tan),                sizeof(glm::vec4), TANGENT}  // Tangent (4 float components)
                        });

        // Initialize the Vertex Descriptors for multiple SkyScraper models with vertex attributes and input rate.
        for (int i = 0; i < 4; i++) {
            VDSkyScraper[i].init(this, {
                    {0, sizeof(ObjectVertex),
                     VK_VERTEX_INPUT_RATE_VERTEX}  // Describes the size of the vertex and input rate
            }, {
                                         // Describes the vertex attribute layout
                                         {0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(ObjectVertex,
                                                                                        pos),                sizeof(glm::vec3), POSITION},  // Position (3 float components)
                                         {0, 1, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(ObjectVertex,
                                                                                        norm),               sizeof(glm::vec3), NORMAL},    // Normal (3 float components)
                                         {0, 2, VK_FORMAT_R32G32_SFLOAT,       offsetof(ObjectVertex,
                                                                                        UV),                 sizeof(glm::vec2), UV},             // UV (2 float components)
                                         {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ObjectVertex,
                                                                                        tan),                sizeof(glm::vec4), TANGENT}  // Tangent (4 float components)
                                 });
        }

        // Initialize the Vertex Descriptors for multiple Tree models with vertex attributes and input rate.
        for (int i = 0; i < 4; i++) {
            VDTree[i].init(this, {
                    {0, sizeof(ObjectVertex),
                     VK_VERTEX_INPUT_RATE_VERTEX}  // Describes the size of the vertex and input rate
            }, {
                                   // Describes the vertex attribute layout
                                   {0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(ObjectVertex,
                                                                                  pos),                sizeof(glm::vec3), POSITION},  // Position (3 float components)
                                   {0, 1, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(ObjectVertex,
                                                                                  norm),               sizeof(glm::vec3), NORMAL},    // Normal (3 float components)
                                   {0, 2, VK_FORMAT_R32G32_SFLOAT,       offsetof(ObjectVertex,
                                                                                  UV),                 sizeof(glm::vec2), UV},             // UV (2 float components)
                                   {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ObjectVertex,
                                                                                  tan),                sizeof(glm::vec4), TANGENT}  // Tangent (4 float components)
                           });
        }

        // Initialize the Vertex Descriptor for the LampPost model with vertex attributes and input rate.
        VDLampPost.init(this, {
                {0, sizeof(ObjectVertex),
                 VK_VERTEX_INPUT_RATE_VERTEX}  // Describes the size of the vertex and input rate
        }, {
                                // Describes the vertex attribute layout
                                {0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(ObjectVertex,
                                                                               pos),                sizeof(glm::vec3), POSITION},  // Position (3 float components)
                                {0, 1, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(ObjectVertex,
                                                                               norm),               sizeof(glm::vec3), NORMAL},    // Normal (3 float components)
                                {0, 2, VK_FORMAT_R32G32_SFLOAT,       offsetof(ObjectVertex,
                                                                               UV),                 sizeof(glm::vec2), UV},             // UV (2 float components)
                                {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ObjectVertex,
                                                                               tan),                sizeof(glm::vec4), TANGENT}  // Tangent (4 float components)
                        });

        // Initialize the Vertex Descriptor for the SkyBox with vertex attributes and input rate.
        VDskyBox.init(this, {
                {0, sizeof(skyBoxVertex), VK_VERTEX_INPUT_RATE_VERTEX}
        }, {
                              {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(skyBoxVertex, pos),
                               sizeof(glm::vec3), POSITION}
                      });

        // Initialize the Vertex Descriptor for the Emission object with vertex attributes and input rate.
        VDEmission.init(this, {
                {0, sizeof(EmissionVertex), VK_VERTEX_INPUT_RATE_VERTEX}
        }, {
                                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(EmissionVertex, pos),
                                        sizeof(glm::vec3), POSITION},
                                {0, 1, VK_FORMAT_R32G32_SFLOAT,    offsetof(EmissionVertex, UV),
                                        sizeof(glm::vec2), UV}
                        });

        // Initialize the pipelines. Shaders are loaded from the specified files and use the newly defined VertexDescriptor.
        // Each pipeline is associated with its respective DescriptorSetLayout, such as DSLGlobal for the common descriptors
        // and specific layouts like DSLScooter, DSLCity, etc., for the unique objects.
        PScooter.init(this, &VDScooter, "shaders/NormalMapVert.spv", "shaders/NormalMapFrag.spv",
                      {&DSLGlobal, &DSLScooter});
        PCity.init(this, &VDCity, "shaders/NormalMapVert.spv", "shaders/CityFrag.spv", {&DSLGlobal, &DSLCity});
        PPizzeria.init(this, &VDPizzeria, "shaders/NormalMapVert.spv", "shaders/PizzeriaFrag.spv",
                       {&DSLGlobal, &DSLPizzeria});
        for (int i = 0; i < 4; i++) {
            PSkyScraper[i].init(this, &VDSkyScraper[i], "shaders/SkyScraperVert.spv", "shaders/SkyScraperFrag.spv",
                                {&DSLGlobal, &DSLSkyScraper[i]});
        }
        for (int i = 0; i < 4; i++) {
            PTree[i].init(this, &VDTree[i], "shaders/TreeVert.spv", "shaders/TreeFrag.spv", {&DSLGlobal, &DSLTree[i]});
        }
        PLampPost.init(this, &VDLampPost, "shaders/LampPostVert.spv", "shaders/LampPostFrag.spv",
                       {&DSLGlobal, &DSLLampPost});
        PskyBox.init(this, &VDskyBox, "shaders/SkyBoxVert.spv", "shaders/SkyBoxFrag.spv", {&DSLskyBox});
        PskyBox.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
                                    VK_CULL_MODE_BACK_BIT, false);
        PEmission.init(this, &VDEmission, "shaders/EmissionVert.spv", "shaders/EmissionFrag.spv", {&DSLEmission});

        // Load models from the specified paths. Each model is initialized with its respective VertexDescriptor and format.
        // The models include different objects such as scooter, city, pizzeria, skyscrapers, trees, lamp post, and the skybox.
        MScooter.init(this, &VDScooter, "models/Scooter.obj", OBJ);
        MCity.init(this, &VDCity, "models/road.obj", OBJ);
        MPizzeria.init(this, &VDPizzeria, "models/Pizzeria/pizzeria.obj", OBJ);
        const char *modelPaths[4] = {
                "models/apartment_1.mgcg",
                "models/apartment_2.mgcg",
                "models/apartment_3.mgcg",
                "models/apartment_4.mgcg"
        };
        for (int i = 0; i < 4; ++i) {
            MSkyScraper[i].init(this, &VDSkyScraper[i], modelPaths[i], MGCG);
        }
        const char *treeModelPaths[4] = {
                "models/vegetation.021.mgcg",
                "models/vegetation.023.mgcg",
                "models/vegetation.024.mgcg",
                "models/vegetation.025.mgcg"
        };

        for (int i = 0; i < 4; ++i) {
            MTree[i].init(this, &VDTree[i], treeModelPaths[i], MGCG);
        }
        MLampPost.init(this, &VDLampPost, "models/lamppost.mgcg", MGCG);
        MskyBox.init(this, &VDskyBox, "models/SkyBoxCube.obj", OBJ);
        Msun.init(this, &VDEmission, "models/Sphere.obj", OBJ);

        // Load textures for various objects. Each texture is initialized with the respective file path.
        // This includes textures for the scooter, city, pizzeria, skyscrapers, trees, lamp post, skybox, and the sun.
        TScooterBaseColor.init(this, "textures/scooter/new/KR51BaseColor.png");

        // Normal map initialization with a special feature to support normal mapping.
        TScooterNormal.init(this, "textures/scooter/new/KR51Normal.png", VK_FORMAT_R8G8B8A8_UNORM);

        TScooterHeight.init(this, "textures/scooter/new/KR51Height.png");

        TScooterMetallic.init(this, "textures/scooter/new/KR51Metallic.png");

        TScooterRoughness.init(this, "textures/scooter/new/KR51Roughness.png");

        TScooterAmbientOcclusion.init(this, "textures/scooter/new/KR51AO.png");

        TCity.init(this, "textures/city/road.png");

        TPizzeria.init(this, "textures/pizzeria/TPizzeria.jpeg");

        for (int i = 0; i < 4; i++) {
            TSkyScraper[i].init(this, "textures/Textures_Skyscrapers.png");
        }
        for (int i = 0; i < 4; i++) {
            TTree[i].init(this, "textures/Textures_Vegetation.png");
        }

        TLampPost.init(this, "textures/Textures_Skyscrapers.png");

        TskyBox.init(this, "textures/starMap/starmap_g4k.jpg");

        Tsun.init(this, "textures/sun/2k_sun.jpg");

        // Set up descriptor pool sizes based on the number of uniform blocks, textures, and descriptor sets required for the scene.
        DPSZs.uniformBlocksInPool =
                1 + 2 + 2 + 2 + 8 + 8 + 2 + 1 + 1; // Total number of uniform blocks (for scooter, city, pizzeria, etc.)
        DPSZs.texturesInPool =
                6 + 1 + 1 + 4 + 4 + 1 + 1 + 1; // Total number of textures (for scooter, city, pizzeria, etc.)
        DPSZs.setsInPool = 1 + 1 + 1 + 4 + 4 + 1 + 1 + 1 +
                           1; // Total number of descriptor sets (for scooter, city, pizzeria, etc.)

        std::cout << "Initialization completed!\n";
        std::cout << "Uniform Blocks in the Pool  : " << DPSZs.uniformBlocksInPool << "\n";
        std::cout << "Textures in the Pool        : " << DPSZs.texturesInPool << "\n";
        std::cout << "Descriptor Sets in the Pool : " << DPSZs.setsInPool << "\n";

        // Set the initial view matrix, translating by the camera position.
        ViewMatrix = glm::translate(glm::mat4(1), -CamPos);

        // Set the initial position for the scooter.
        ScooterPos = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    // Create pipelines and descriptor sets.
    void pipelinesAndDescriptorSetsInit() {
        // Create the pipelines for each object type (scooter, city, pizzeria, etc.).
        PScooter.create();
        PCity.create();
        PPizzeria.create();
        for (int i = 0; i < 4; i++) {
            PSkyScraper[i].create();
        }
        for (int i = 0; i < 4; i++) {
            PTree[i].create();
        }
        PLampPost.create();
        PskyBox.create();
        PEmission.create();

        // Create descriptor sets for each object type with their respective textures.
        DSScooter.init(this, &DSLScooter,
                       {&TScooterBaseColor, &TScooterNormal, &TScooterHeight, &TScooterMetallic, &TScooterRoughness,
                        &TScooterAmbientOcclusion});
        DSCity.init(this, &DSLCity, {&TCity});
        DSPizzeria.init(this, &DSLPizzeria, {&TPizzeria});
        for (int i = 0; i < 4; i++) {
            DSSkyScraper[i].init(this, &DSLSkyScraper[i], {&TSkyScraper[i]});
        }
        for (int i = 0; i < 4; i++) {
            DSTree[i].init(this, &DSLTree[i], {&TTree[i]});
        }
        DSLampPost.init(this, &DSLLampPost, {&TLampPost});
        DSskyBox.init(this, &DSLskyBox, {&TskyBox});
        DSsun.init(this, &DSLEmission, {&Tsun});

        DSGlobal.init(this, &DSLGlobal, {});
    }

    // This function handles the destruction of pipelines and Descriptor Sets.
    // All object classes defined in Starter.hpp have a .cleanup() method for this purpose.
    void pipelinesAndDescriptorSetsCleanup() {
        // Clean up the pipeline objects for each entity.
        PScooter.cleanup();
        PCity.cleanup();
        PPizzeria.cleanup();
        for(int i = 0; i < 4; i++)
        {
            PSkyScraper[i].cleanup();
        }
        for(int i = 0; i < 4; i++)
        {
            PTree[i].cleanup();
        }
        PLampPost.cleanup();
        PskyBox.cleanup();
        PEmission.cleanup();

        // Clean up the global descriptor set.
        DSGlobal.cleanup();

        // Clean up descriptor sets for each entity.
        DSScooter.cleanup();
        DSCity.cleanup();
        DSPizzeria.cleanup();
        for(int i = 0; i < 4; i++)
        {
            DSSkyScraper[i].cleanup();
        }
        for(int i = 0; i < 4; i++)
        {
            DSTree[i].cleanup();
        }
        DSLampPost.cleanup();
        DSskyBox.cleanup();
        DSsun.cleanup();
    }

    // This function handles the cleanup of models, textures, and Descriptor Set Layouts.
    // All object classes defined in Starter.hpp have a .cleanup() method for this purpose.
    // Pipelines also need to be cleaned up. The .cleanup() method recreates them, while .destroy() deletes them completely.
    void localCleanup() {
        // Clean up texture resources for each entity.
        TScooterBaseColor.cleanup();
        TScooterNormal.cleanup();
        TScooterHeight.cleanup();
        TScooterMetallic.cleanup();
        TScooterRoughness.cleanup();
        TScooterAmbientOcclusion.cleanup();
        MScooter.cleanup();

        // Clean up model and texture resources for city.
        TCity.cleanup();
        MCity.cleanup();

        // Clean up model and texture resources for pizzeria.
        TPizzeria.cleanup();
        MPizzeria.cleanup();

        // Clean up textures and models for skyscrapers.
        for(int i = 0; i < 4; i++)
        {
            TSkyScraper[i].cleanup();
            MSkyScraper[i].cleanup();
        }

        // Clean up textures and models for trees.
        for(int i = 0; i < 4; i++)
        {
            TTree[i].cleanup();
            MTree[i].cleanup();
        }

        // Clean up texture and model resources for lamp posts.
        TLampPost.cleanup();
        MLampPost.cleanup();

        // Clean up texture and model resources for skybox.
        TskyBox.cleanup();
        MskyBox.cleanup();

        // Clean up texture and model resources for the sun.
        Tsun.cleanup();
        Msun.cleanup();

        // Clean up the global descriptor set layout.
        DSLGlobal.cleanup();

        // Clean up descriptor set layouts for each entity.
        DSLScooter.cleanup();
        DSLCity.cleanup();
        DSLPizzeria.cleanup();
        for(int i = 0; i < 4; i++)
        {
            DSLSkyScraper[i].cleanup();
        }
        for(int i = 0; i < 4; i++)
        {
            DSLTree[i].cleanup();
        }
        DSLLampPost.cleanup();
        DSLskyBox.cleanup();
        DSLEmission.cleanup();

        // Clean up the pipelines by destroying them completely.
        PScooter.destroy();
        PCity.destroy();
        PPizzeria.destroy();
        for(int i = 0; i < 4; i++)
        {
            PSkyScraper[i].destroy();
        }
        for(int i = 0; i < 4; i++)
        {
            PTree[i].destroy();
        }
        PLampPost.destroy();
        PskyBox.destroy();
        PEmission.destroy();
    }

    // This function handles the population of the command buffer.
    // It sends all the objects to the GPU that need to be drawn, along with their buffers and textures.
    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

        std::cout << "test\n";

        // Binding the pipeline, model, descriptor sets, and issuing the draw call for the scooter.
        std::vector<ModelOffsets> ScooterOffsets = calculateOffsets("models/Scooter.obj");
        PScooter.bind(commandBuffer);
        MScooter.bind(commandBuffer);

        DSGlobal.bind(commandBuffer, PScooter, 0, currentImage);
        DSScooter.bind(commandBuffer, PScooter, 1, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MScooter.indices.size()), 1, 0, 0, 0);

        // Binding the pipeline and model for the city.
        PCity.bind(commandBuffer);                // Pipeline for the city
        MCity.bind(commandBuffer);                // Model for the city

        // Binding global and specific descriptor sets for the city.
        DSGlobal.bind(commandBuffer, PCity, 0, currentImage);    // Global Descriptor Set for the city
        DSCity.bind(commandBuffer, PCity, 1, currentImage);      // Specific Descriptor Set for the city

        // Draw call for the city.
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MCity.indices.size()), 1, 0, 0, 0);

        for(int i = 0; i < 4; i++)
        {
            // Binding the pipeline and model for the skyscraper.
            PSkyScraper[i].bind(commandBuffer);           // Pipeline for the skyscraper
            MSkyScraper[i].bind(commandBuffer);           // Model for the skyscraper

            // Binding global and specific descriptor sets for the skyscraper.
            DSGlobal.bind(commandBuffer, PSkyScraper[i], 0, currentImage);    // Global Descriptor Set for the skyscraper
            DSSkyScraper[i].bind(commandBuffer, PSkyScraper[i], 1, currentImage);      // Specific Descriptor Set for the skyscraper

            // Draw call for the skyscraper.
            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(MSkyScraper[i].indices.size()), NSKYSCRAPER, 0, 0, 0);
        }

        for(int i = 0; i < 4; i++)
        {
            // Binding the pipeline and model for the trees.
            PTree[i].bind(commandBuffer);           // Pipeline for the tree
            MTree[i].bind(commandBuffer);           // Model for the tree

            // Binding global and specific descriptor sets for the tree.
            DSGlobal.bind(commandBuffer, PTree[i], 0, currentImage);    // Global Descriptor Set for the tree
            DSTree[i].bind(commandBuffer, PTree[i], 1, currentImage);      // Specific Descriptor Set for the tree

            // Draw call for the tree.
            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(MTree[i].indices.size()), NTREE, 0, 0, 0);
        }

        // Binding the pipeline and model for the lamp post.
        PLampPost.bind(commandBuffer);                // Pipeline for the lamp post
        MLampPost.bind(commandBuffer);                // Model for the lamp post

        // Binding global and specific descriptor sets for the lamp post.
        DSGlobal.bind(commandBuffer, PLampPost, 0, currentImage);    // Global Descriptor Set for the lamp post
        DSLampPost.bind(commandBuffer, PLampPost, 1, currentImage);      // Specific Descriptor Set for the lamp post

        // Draw call for the lamp post.
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MLampPost.indices.size()), NLAMPPOST, 0, 0, 0);

        // Binding the pipeline and model for the skybox.
        PskyBox.bind(commandBuffer);
        MskyBox.bind(commandBuffer);
        DSskyBox.bind(commandBuffer, PskyBox, 0, currentImage);
        // Draw the skybox.
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MskyBox.indices.size()), 1, 0, 0, 0);

        // Binding the pipeline and model for the sun.
        PEmission.bind(commandBuffer);
        Msun.bind(commandBuffer);
        DSsun.bind(commandBuffer, PEmission, 0, currentImage);
        // Draw the sun.
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(Msun.indices.size()), 1, 0, 0, 0);

        // Binding the pipeline and model for the pizzeria.
        PPizzeria.bind(commandBuffer);                // Pipeline for the pizzeria
        MPizzeria.bind(commandBuffer);                // Model for the pizzeria

        // Binding global and specific descriptor sets for the pizzeria.
        DSGlobal.bind(commandBuffer, PPizzeria, 0, currentImage);    // Global Descriptor Set for the pizzeria
        DSPizzeria.bind(commandBuffer, PPizzeria, 1, currentImage);      // Specific Descriptor Set for the pizzeria

        // Draw call for the pizzeria.
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MPizzeria.indices.size()), 1, 0, 0, 0);
    }


    // This function handles the update of the uniform buffers.
// It contains the logic for the application, including camera control, movement, and collision checks.

    void updateUniformBuffer(uint32_t currentImage) {
        static bool debounce = false;
        static int curDebounce = 0;

        float deltaT;
        glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
        bool fire = false;

        // Gets input from the six-axis controller for movement and rotation.
        getSixAxis(deltaT, m, r, fire);

        static float ang;
        static float lookAng = 0;
        static float DlookAng = 0;
        static int subpass = 0;
        static float subpassTimer = 0.0;

        static float CamPitch = glm::radians(20.0f);
        static float CamYaw   = M_PI;
        static float CamDist  = 10.0f;
        static float CamRoll  = 0.0f;
        const glm::vec3 CamTargetDelta = glm::vec3(0,2,0);
        const glm::vec3 Cam1stPos = glm::vec3(0.49061f, 2.07f, 2.7445f);

        glm::vec3 CamPos = ScooterPos;
        static glm::vec3 dampedCamPos = CamPos;
        static float SteeringAng = 0.0f;
        static float wheelRoll = 0.0f;
        static float dampedVel = 0.0f;

        glm::vec3 proposedPos = ScooterPos;

        glm::mat4 M;

        // Defines the velocity and steering parameters for the movement.
        const float STEERING_SPEED = glm::radians(80.0f);
        const float ROT_SPEED = glm::radians(140.0f);
        const float MOVE_SPEED = 10.0f;

        // Updates the steering angle based on the user input.
        SteeringAng += -m.x * STEERING_SPEED * deltaT;
        SteeringAng = (SteeringAng < glm::radians(-35.0f) ? glm::radians(-35.0f) :
                       (SteeringAng > glm::radians(35.0f)  ? glm::radians(35.0f)  : SteeringAng));

        // Applies damping to the velocity and calculates the wheel roll.
        double lambdaVel = 8.0f;
        double dampedVelEpsilon = 0.001f;
        dampedVel =  MOVE_SPEED * deltaT * m.z * (1 - exp(-lambdaVel * deltaT)) +
                     dampedVel * exp(-lambdaVel * deltaT);
        dampedVel = ((fabs(dampedVel) < dampedVelEpsilon) ? 0.0f : dampedVel);
        wheelRoll -= dampedVel / 0.4;
        wheelRoll = (wheelRoll < 0.0 ? wheelRoll + 2*M_PI : (wheelRoll > 2*M_PI ? wheelRoll - 2*M_PI : wheelRoll));

        // If there is movement, update the position based on the steering angle.
        if(dampedVel != 0.0f) {

            glm::vec3 oldPos = proposedPos;
            if(SteeringAng != 0.0f) {
                const float l = 2.78f;
                float r = l / tan(SteeringAng);
                float cx = proposedPos.x + r * cos(Yaw);
                float cz = proposedPos.z - r * sin(Yaw);
                float Dbeta = dampedVel / r;
                Yaw = Yaw - Dbeta;
                Roll = Roll - Dbeta;
                proposedPos.x = cx - r * cos(Yaw);
                proposedPos.z = cz + r * sin(Yaw);
            } else {
                proposedPos.x = proposedPos.x - sin(Yaw) * dampedVel;
                proposedPos.z = proposedPos.z - cos(Yaw) * dampedVel;
            }
            if(m.x == 0) {
                if(SteeringAng > STEERING_SPEED * deltaT) {
                    SteeringAng -= STEERING_SPEED * deltaT;
                } else if(SteeringAng < -STEERING_SPEED * deltaT) {
                    SteeringAng += STEERING_SPEED * deltaT;
                } else {
                    SteeringAng = 0.0f;
                }
            }

        }

        // Checks for collisions with the new position.
        if (!checkCollision(proposedPos, ang, 0.7f)) {
            // Only updates the position if no collision is detected.
            ScooterPos = proposedPos;
        }

        glm::mat4 Mv;

        // Calculates the transformation matrix for the scooter's movement.
        glm::mat4 Wm = glm::translate(glm::mat4(1), ScooterPos) * glm::rotate(glm::mat4(1), ang + Yaw, glm::vec3(0,1,0))
                       * glm::rotate(glm::mat4(1), ang - SteeringAng, glm::vec3(0,0,1));

        glm::mat4 FWm = glm::translate(glm::mat4(1), ScooterPos) * glm::rotate(glm::mat4(1), ang + Yaw*3.0f, glm::vec3(0,1,0));

        // Toggles between scenes when the spacebar is pressed.
        if(glfwGetKey(window, GLFW_KEY_SPACE)){
            if(!debounce) {
                debounce = true;
                curDebounce = GLFW_KEY_SPACE;
                if(currScene != 1) {
                    currScene = 1;
                } else if(currScene == 1) {
                    currScene = 0;
                }

                // Rebuilds the pipeline after a scene change.
                RebuildPipeline();
            }
        } else {
            if((curDebounce == GLFW_KEY_SPACE) && debounce) {
                debounce = false;
                curDebounce = 0;
            }
        }

        // Closes the window when the Escape key is pressed.
        if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        // Perspective camera control when in scene 0.
        if(currScene == 0) {
            CamYaw += ROT_SPEED * deltaT * r.y;
            CamPitch -= ROT_SPEED * deltaT * r.x;
            CamRoll -= ROT_SPEED * deltaT * r.z;
            CamDist -= MOVE_SPEED * deltaT * m.y;

            // Clamps camera angles to prevent over-rotation.
            CamYaw = (CamYaw < 0.0f ? 0.0f : (CamYaw > 2*M_PI ? 2*M_PI : CamYaw));
            CamPitch = (CamPitch < 0.0f ? 0.0f : (CamPitch > M_PI_2-0.01f ? M_PI_2-0.01f : CamPitch));
            CamRoll = (CamRoll < -M_PI ? -M_PI : (CamRoll > M_PI ? M_PI : CamRoll));
            CamDist = 5.0f;

            // Updates camera position and view target based on scooter position.
            glm::vec3 CamTarget = ScooterPos + glm::vec3(0,2,-5);
            CamPos = CamTarget + glm::vec3(glm::rotate(glm::mat4(1), Yaw + CamYaw, glm::vec3(0,1,0)) * glm::rotate(glm::mat4(1), -CamPitch, glm::vec3(1,0,0)) *
                                           glm::vec4(0, 0, CamDist,1));

            const float lambdaCam = 10.0f;
            dampedCamPos = CamPos * (1 - exp(-lambdaCam * deltaT)) +
                           dampedCamPos * exp(-lambdaCam * deltaT);
            M = MakeViewProjectionLookAt(dampedCamPos, CamTarget, glm::vec3(0,1,0), CamRoll, glm::radians(90.0f), Ar, 0.1f, 500.0f);
            Mv = ViewMatrix;
        } else if(currScene == 1){ //Orthogonal camera

            M = glm::rotate(glm::mat4(1.0f / 15.0f,0,0,0,  0,-4.0f / 45.f,0,0,   0,0,1.0f/(-500.0f-500.0f),0, 0,0,-500.0f/(-500.0f-500.0f),1), glm::radians(90.0f), glm::vec3(1,0,0));
            Mv =  glm::inverse(
                    glm::translate(glm::mat4(1), ScooterPos) *
                    glm::rotate(glm::mat4(1), DlookAng, glm::vec3(0,1,0))
            );
        }

        // This section updates the uniforms used by the shaders

        glm::mat4 ViewPrj = M * Mv; // Calculates the view-projection matrix

        glm::mat4 baseTr = glm::mat4(1.0f); // Defines a base transformation matrix

        // Updates the global uniforms
        GlobalUniformBufferObject gubo{};
        gubo.lightDir = glm::vec3(-1.0f, 1.0f, 1.0f);  // Sets light direction
        gubo.lightColor = glm::vec4(1.0f);                // Sets light color
        gubo.eyePos = glm::vec3(glm::inverse(ViewMatrix) * glm::vec4(0, 3, 0, 1)); // Sets eye position
        DSGlobal.map(currentImage, &gubo, 0); // Maps the global uniform buffer object

        EmissionUniformBufferObject emissionUbo{};
        emissionUbo.mvpMat = ViewPrj * glm::translate(glm::mat4(1), gubo.lightDir * 90.0f) * baseTr; // Calculates emission matrix
        emissionUbo.mvpMat = glm::scale(emissionUbo.mvpMat, glm::vec3(3.0f, 3.0f, 3.0f)); // Scales the emission matrix
        DSsun.map(currentImage, &emissionUbo, 0); // Maps the emission uniform buffer object

        // Initializes matrices and parameters for different objects
        SingleObjectMatricesUniformBufferObject ScooterUbo{};
        ObjectParametersUniformBufferObject ScooterParUbo{};

        SingleObjectMatricesUniformBufferObject CityUbo{};
        ObjectParametersUniformBufferObject CityParUbo{};

        SingleObjectMatricesUniformBufferObject PizzeriaUbo{};
        ObjectParametersUniformBufferObject PizzeriaParUbo{};

        SkyScraperMatricesUniformBufferObject skyScraperUbos[4]{};
        ObjectParametersUniformBufferObject skyScraperParamUbos[4]{};

        TreeMatricesUniformBufferObject treeUbos[4]; // Array of UBOs for trees
        ObjectParametersUniformBufferObject treeParamUbos[4]; // Array of parameters for trees

        LampPostMatricesUniformBufferObject LampPostUbo{};
        ObjectParametersUniformBufferObject LampPostParUbo{};

        skyBoxUniformBufferObject SkyBoxUbo{};

        // World and normal matrices should be identity, while the WVP matrix is variable (ViewPrj)
        ScooterUbo.mMat = Wm * glm::mat4(1.0f); // Sets model matrix for the scooter
        ScooterUbo.mvpMat = ViewPrj * ScooterUbo.mMat; // Calculates MVP matrix for the scooter
        ScooterUbo.nMat = glm::inverse(glm::transpose(ScooterUbo.mMat)); // Calculates normal matrix for the scooter

        CityUbo.mMat = glm::mat4(1.0f); // Sets identity matrix for the city
        CityUbo.nMat = glm::mat4(1.0f); // Sets identity normal matrix for the city
        CityUbo.mvpMat = ViewPrj; // Sets MVP matrix for the city

        // Updates the model matrix for the pizzeria with a scaling factor of 1.8 and a rotation of 45 degrees
        PizzeriaUbo.mMat = glm::translate(glm::mat4(1.0f),
                                          glm::vec3(84.0 - (24.0 * (28 % 8)), 0.0, 84 - (24 * (28 / 8))));
        PizzeriaUbo.mMat = glm::rotate(PizzeriaUbo.mMat, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotates the pizzeria
        PizzeriaUbo.mMat = glm::scale(PizzeriaUbo.mMat, glm::vec3(1.8f, 1.8f, 1.8f)); // Scales the pizzeria

        // Calculates the MVP matrix for the pizzeria
        PizzeriaUbo.mvpMat = ViewPrj * PizzeriaUbo.mMat;

        // Calculates the normal matrix for the pizzeria
        PizzeriaUbo.nMat = glm::inverse(glm::transpose(PizzeriaUbo.mMat));

        int currentIndex = -1;
        int counterSkyScraper[4] = {0, 0, 0, 0}; // Array of counters for skyscrapers

        // Loops over each skyscraper and updates their matrices
        for (int i = 0; i < NSKYSCRAPER * 4; i++) {
            for (int j = 0; j < NSKYSCRAPER; j++) {
                if (indicesSkyScraper1[j] == i) {
                    currentIndex = 0;
                } else if (indicesSkyScraper2[j] == i) {
                    currentIndex = 1;
                } else if (indicesSkyScraper3[j] == i) {
                    currentIndex = 2;
                } else if (indicesSkyScraper4[j] == i) {
                    currentIndex = 3;
                }
            }

            // Excludes the pizzeria tile
            if (i == 28) {
                currentIndex = -1;
            }

            if (currentIndex != -1) {
                // Updates the model matrix for the current skyscraper
                skyScraperUbos[currentIndex].mMat[counterSkyScraper[currentIndex]] = glm::translate(glm::mat4(1.0f),
                                                                                                    glm::vec3(84.0 - (24.0 * (i % 8)), 0.0, 84 - (24 * (i / 8))));

                skyScraperUbos[currentIndex].mMat[counterSkyScraper[currentIndex]] = glm::rotate(
                        skyScraperUbos[currentIndex].mMat[counterSkyScraper[currentIndex]],
                        glm::radians(90.0f * (float)(i % 4)), // Rotation angle in degrees
                        glm::vec3(0.0f, 1.0f, 0.0f));         // Y-axis rotation

                // Calculates the MVP matrix for the current skyscraper
                skyScraperUbos[currentIndex].mvpMat[counterSkyScraper[currentIndex]] =
                        ViewPrj * skyScraperUbos[currentIndex].mMat[counterSkyScraper[currentIndex]];

                // Calculates the normal matrix for the current skyscraper
                skyScraperUbos[currentIndex].nMat[counterSkyScraper[currentIndex]] =
                        glm::inverse(glm::transpose(skyScraperUbos[currentIndex].mMat[counterSkyScraper[currentIndex]]));

                // Increments the counter for the skyscraper
                counterSkyScraper[currentIndex]++;
            }
        }


        // Iterates through the lamp posts and updates the model matrices, MVP matrices, and normal matrices for each lamp post.
        for(int i = 0; i < NLAMPPOST/4; i++) {
            // Updates the model matrix for the first lamp post in the set
            LampPostUbo.mMat[i * 4] =
                    glm::translate(glm::mat4(1.0f), glm::vec3(75 - (24 * (i % 8)), 0.0, 85 - (24 * (i / 8)))) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            // Calculates the MVP matrix for the first lamp post
            LampPostUbo.mvpMat[i * 4] = ViewPrj * LampPostUbo.mMat[i * 4];
            // Calculates the normal matrix for the first lamp post
            LampPostUbo.nMat[i * 4] = glm::inverse(glm::transpose(LampPostUbo.mMat[i * 4]));

            // Updates the model matrix for the second lamp post in the set
            LampPostUbo.mMat[i * 4 + 1] =
                    glm::translate(glm::mat4(1.0f), glm::vec3(84 - (24 * (i % 8)), 0.0, 75 - (24 * (i / 8)))) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(-180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            // Calculates the MVP matrix for the second lamp post
            LampPostUbo.mvpMat[i * 4 + 1] = ViewPrj * LampPostUbo.mMat[i * 4 + 1];
            // Calculates the normal matrix for the second lamp post
            LampPostUbo.nMat[i * 4 + 1] = glm::inverse(glm::transpose(LampPostUbo.mMat[i * 4 + 1]));

            // Updates the model matrix for the third lamp post in the set
            LampPostUbo.mMat[i * 4 + 2] =
                    glm::translate(glm::mat4(1.0f), glm::vec3(93 - (24 * (i % 8)), 0.0, 85 - (24 * (i / 8)))) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(-270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            // Calculates the MVP matrix for the third lamp post
            LampPostUbo.mvpMat[i * 4 + 2] = ViewPrj * LampPostUbo.mMat[i * 4 + 2];
            // Calculates the normal matrix for the third lamp post
            LampPostUbo.nMat[i * 4 + 2] = glm::inverse(glm::transpose(LampPostUbo.mMat[i * 4 + 2]));

            // Updates the model matrix for the fourth lamp post in the set
            LampPostUbo.mMat[i * 4 + 3] = glm::translate(glm::mat4(1.0f), glm::vec3(84 - (24 * (i % 8)), 0.0, 93 - (24 * (i / 8))));
            // Calculates the MVP matrix for the fourth lamp post
            LampPostUbo.mvpMat[i * 4 + 3] = ViewPrj * LampPostUbo.mMat[i * 4 + 3];
            // Calculates the normal matrix for the fourth lamp post
            LampPostUbo.nMat[i * 4 + 3] = glm::inverse(glm::transpose(LampPostUbo.mMat[i * 4 + 3]));
        }


        // Initializes and sets up tree indices and counters for positioning and transformation
        currentIndex = -1;
        int offset = 6;
        int counterTree[4] = {0, 0, 0, 0}; // Counter for each tree
        int* indicesTree[4] = {indicesTree1, indicesTree2, indicesTree3, indicesTree4}; // Array of indices for trees

        // Iterates over all tree indices and updates the matrices for each tree
        for (int i = 0; i < 64; ++i) {
            currentIndex = -1;

            // Determines the current tree index
            for (int j = 0; j < 4; ++j) {
                for (int k = 0; k < 16; ++k) {
                    if (indicesTree[j][k] == i) {
                        currentIndex = j;
                        break;
                    }
                }
                if (currentIndex != -1) break;
            }

            // Excludes the pizzeria tile from tree updates
            if (i == 28) {
                currentIndex = -1;
            }

            // Updates the matrices for the current tree, applying scale only for Tree1 and Tree4
            if (currentIndex != -1) {
                bool applyScale = (currentIndex == 0 || currentIndex == 3); // Apply scale only for Tree1 and Tree4
                updateTreeMatrices(&treeUbos[currentIndex], counterTree[currentIndex], i, offset, ViewPrj, applyScale);
            }
        }

        // Sets the scale factor for the SkyBox and adjusts its translation to center it in the scene
        float SkyBox_scale_factor = 98.0f;
        glm::vec3 skybox_center_offset = glm::vec3(0, 3, -5); // Adjust the translation values to center the SkyBox

        // Sets up the model-view-projection matrix for the SkyBox with scaling and translation
        SkyBoxUbo.mvpMat = M * glm::mat4(glm::mat3(ViewMatrix))
                           * glm::translate(glm::mat4(1.0f), skybox_center_offset) // Translation
                           * glm::scale(glm::mat4(1.0f), glm::vec3(SkyBox_scale_factor, SkyBox_scale_factor, SkyBox_scale_factor)); // Scaling

        // Maps the Uniform Buffer Objects (UBOs) for each object to the corresponding DSL bindings
        DSScooter.map(currentImage, &ScooterUbo, 0);
        DSCity.map(currentImage, &CityUbo, 0);
        DSPizzeria.map(currentImage, &PizzeriaUbo, 0);

        // Maps the UBOs for the skyscrapers
        for(int i = 0; i < 4; i++) {
            DSSkyScraper[i].map(currentImage, &skyScraperUbos[i], 0);
        }

        // Maps the UBOs for the trees
        for(int i = 0; i < 4; i++) {
            DSTree[i].map(currentImage, &treeUbos[i], 0);
        }

        // Maps the UBO for the lamp posts
        DSLampPost.map(currentImage, &LampPostUbo, 0);

        // Maps the UBO for the SkyBox
        DSskyBox.map(currentImage, &SkyBoxUbo, 0);

        // Sets the specular power for each material's uniform buffer object
        ScooterParUbo.Pow = 200.0f;
        CityParUbo.Pow = 200.0f;
        PizzeriaParUbo.Pow = 200.0f;
        for (int i = 0; i < 4; ++i) {
            skyScraperParamUbos[i].Pow = 200.0f;
        }
        for (int i = 0; i < 4; ++i) {
            treeParamUbos[i].Pow = 200.0f;
        }
        LampPostParUbo.Pow = 200.0f;

        // Sets the texture angle parameter for each object based on time
        ScooterParUbo.Ang = 0.0f;
        CityParUbo.Ang = 0.0f;
        PizzeriaParUbo.Ang = 0.0f;
        for (int i = 0; i < 4; ++i) {
            skyScraperParamUbos[i].Ang = 0.0f;
        }
        for (int i = 0; i < 4; ++i) {
            treeParamUbos[i].Ang = 0.0f;
        }
        LampPostParUbo.Ang = 0.0f;

        // Maps the material parameter UBOs for each object to the corresponding DSL bindings
        DSScooter.map(currentImage, &ScooterParUbo, 7);
        DSCity.map(currentImage, &CityParUbo, 2);
        DSPizzeria.map(currentImage, &PizzeriaParUbo, 2);

        // Maps the material parameter UBOs for the skyscrapers
        for(int i = 0; i < 4; i++) {
            DSSkyScraper[i].map(currentImage, &skyScraperParamUbos[i], 2);
        }

        // Maps the material parameter UBOs for the trees
        for(int i = 0; i < 4; i++) {
            DSTree[i].map(currentImage, &treeParamUbos[i], 2);
        }

        // Maps the material parameter UBO for the lamp posts
        DSLampPost.map(currentImage, &LampPostParUbo, 2);
    }

    bool checkCollision(glm::vec3 centerPos, float angle, float halfLength) {
        // The direction of the scooter based on its orientation
        glm::vec2 direction(-sin(angle), cos(angle));

        // Calculate the points in front and behind the center position
        glm::vec2 frontPoint = glm::vec2(centerPos.x, centerPos.z) + halfLength * direction;
        glm::vec2 backPoint = glm::vec2(centerPos.x, centerPos.z) - halfLength * direction;

        // Check if the scooter is outside the external barriers
        if (
                frontPoint.x <= externalBarriers.min.x ||
                frontPoint.x >= externalBarriers.max.x ||
                frontPoint.y <= externalBarriers.min.y ||
                frontPoint.y >= externalBarriers.max.y ||
                backPoint.x <= externalBarriers.min.x ||
                backPoint.x >= externalBarriers.max.x ||
                backPoint.y <= externalBarriers.min.y ||
                backPoint.y >= externalBarriers.max.y
                )
        {
            return true; // Collision detected with external barriers
        }

        // Check for collisions with internal barriers for both front and back points
        for (const auto& barrier : barriers) {
            if ((frontPoint.x >= barrier.min.x && frontPoint.x <= barrier.max.x &&
                 frontPoint.y >= barrier.min.y && frontPoint.y <= barrier.max.y) || // Check if front point is inside barrier
                (backPoint.x >= barrier.min.x && backPoint.x <= barrier.max.x &&
                 backPoint.y >= barrier.min.y && backPoint.y <= barrier.max.y))     // Check if back point is inside barrier
            {
                return true;  // Collision detected with an internal barrier
            }
        }
        return false;  // No collision detected
    }

    std::vector<glm::vec2> generateCenters(int coord, int step) {
        std::vector<glm::vec2> centers;

        // Generate a grid of center points based on the provided coordinate range and step
        for (int i = -coord; i <= coord; i+=step) {
            for (int j = -coord; j <= coord; j+=step) {
                centers.push_back(glm::vec2(static_cast<float>(i), static_cast<float>(j)));
            }
        }

        return centers;  // Return the generated list of center points
    }

    std::vector<BoundingBox> getBarriers(const std::vector<glm::vec2>& centers, float sideLength) {
        std::vector<BoundingBox> barriers;
        float halfSide = sideLength / 2.0f;

        // Generate bounding boxes based on the center points and the provided side length
        for (const auto& center : centers) {
            glm::vec2 min = {center.x - halfSide, center.y - halfSide};
            glm::vec2 max = {center.x + halfSide, center.y + halfSide};
            barriers.push_back({min, max});
        }

        return barriers;  // Return the list of generated barriers
    }


};

// Main function: The application runs here, and any errors will be caught and displayed
int main() {
    A10 app;

    try {
        app.run();  // Run the application
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;  // Print the exception message if an error occurs
        return EXIT_FAILURE;  // Return failure status on error
    }

    return EXIT_SUCCESS;  // Return success status when the application completes successfully
}



