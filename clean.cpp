// This has been adapted from the Vulkan tutorial

#include "headers/tiny_obj_loader.h"
#include "modules/Starter.hpp"
#include "WVP.hpp"

#define PI 3.14159265359

struct BoundingBox {
    glm::vec2 min;
    glm::vec2 max;
};

/*
std::vector<BoundingBox> barriers = {
//        {{-2.0f, -2.0f}, {2.0f, 2.0f}},  // Rettangolo tra (-2, -2) e (2, 2)
        {{2.0f, 2.0f}, {22.0f, 22.0f}},    // Un altro rettangolo
        // Aggiungi qui altri muri invisibili
};
*/

struct GlobalUniformBufferObject {
    alignas(16) glm::vec3 lightDir;
    alignas(16) glm::vec4 lightColor;
    alignas(16) glm::vec3 eyePos;
};

// **A10** Place here the CPP struct for the uniform buffer for the matrices
struct ScooterMatricesUniformBufferObject {
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::mat4 mMat;
    alignas(16) glm::mat4 nMat;
};

struct ScooterFWheelMatricesUniformBufferObject {
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::mat4 mMat;
    alignas(16) glm::mat4 nMat;
};

struct CityMatricesUniformBufferObject {
    alignas(16) glm::mat4 mvpMat;  // Model-View-Projection matrix
    alignas(16) glm::mat4 mMat;    // Model matrix
    alignas(16) glm::mat4 nMat;    // Normal matrix
};

#define NSKYSCRAPER 64
struct SkyScraperMatricesUniformBufferObject {
    alignas(16) glm::mat4 mvpMat[NSKYSCRAPER];  // Model-View-Projection matrix
    alignas(16) glm::mat4 mMat[NSKYSCRAPER];    // Model matrix
    alignas(16) glm::mat4 nMat[NSKYSCRAPER];    // Normal matrix
};

#define NLAMPPOST 256
struct LampPostMatricesUniformBufferObject {
    alignas(16) glm::mat4 mvpMat[NLAMPPOST];  // Model-View-Projection matrix
    alignas(16) glm::mat4 mMat[NLAMPPOST];    // Model matrix
    alignas(16) glm::mat4 nMat[NLAMPPOST];    // Normal matrix
};

struct skyBoxUniformBufferObject {
    alignas(16) glm::mat4 mvpMat;           //usata per il rendering della skybox (la scatola che rappresenta il cielo o lo sfondo)
};

// **A10** Place here the CPP struct for the uniform buffer for the parameters
struct ScooterParametersUniformBufferObject {
    alignas(4) float Pow;
    alignas(4) float Ang;
};

struct ScooterFWheelParametersUniformBufferObject {
    alignas(4) float Pow;
    alignas(4) float Ang;
};

struct CityParametersUniformBufferObject {
    alignas(4) float Pow;  // Parametro di potenza speculare, simile a quello dello scooter
    alignas(4) float Ang;  // Parametro per eventuali trasformazioni basate sul tempo (opzionale)
};

struct SkyScraperParametersUniformBufferObject {
    alignas(4) float Pow;  // Parametro di potenza speculare, simile a quello dello scooter
    alignas(4) float Ang;  // Parametro per eventuali trasformazioni basate sul tempo (opzionale)
};

struct LampPostParametersUniformBufferObject {
    alignas(4) float Pow;  // Parametro di potenza speculare, simile a quello dello scooter
    alignas(4) float Ang;  // Parametro per eventuali trasformazioni basate sul tempo (opzionale)
};



// **A10** Place here the CPP struct for the vertex definition
struct ScooterVertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 UV;
    glm::vec4 tan;
};

struct ScooterFWheelVertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 UV;
    glm::vec4 tan;
};

struct CityVertex {
    glm::vec3 pos;   // Posizione del vertex
    glm::vec3 norm;  // Normale del vertex
    glm::vec2 UV;    // Coordinate UV per la texture
    glm::vec4 tan;   // Tangente per il normal mapping (opzionale, se il modello lo richiede)
};

struct SkyScraperVertex {
    glm::vec3 pos;   // Posizione del vertex
    glm::vec3 norm;  // Normale del vertex
    glm::vec2 UV;    // Coordinate UV per la texture
    glm::vec4 tan;   // Tangente per il normal mapping (opzionale, se il modello lo richiede)
};

struct LampPostVertex {
    glm::vec3 pos;   // Posizione del vertex
    glm::vec3 norm;  // Normale del vertex
    glm::vec2 UV;    // Coordinate UV per la texture
    glm::vec4 tan;   // Tangente per il normal mapping (opzionale, se il modello lo richiede)
};

struct skyBoxVertex {
    glm::vec3 pos;
};

struct ModelOffsets {
    std::string name;      // Name of the model
    uint32_t vertexOffset; // Starting vertex offset in the buffer
    uint32_t indexOffset;  // Starting index offset in the buffer
    uint32_t indexCount;   // Number of indices for this model
};


std::vector<ModelOffsets> calculateOffsets(const std::string& filename) {
    tinyobj::ObjReader reader;

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



// MAIN !
class A10 : public BaseProject {
protected:


    // Descriptor Layouts ["classes" of what will be passed to the shaders]
    DescriptorSetLayout DSLGlobal;	// For Global values

// **A10** Place here the variable for the DescriptorSetLayout
    DescriptorSetLayout DSLScooter;
    DescriptorSetLayout DSLScooterFWheel;
    DescriptorSetLayout DSLCity;
    DescriptorSetLayout DSLSkyScraper;
    DescriptorSetLayout DSLLampPost;
    DescriptorSetLayout DSLskyBox;	// For skyBox


// **A10** Place here the variable for the VertexDescriptor
    VertexDescriptor VDScooter;
    VertexDescriptor VDScooterFWheel;
    VertexDescriptor VDCity;
    VertexDescriptor VDSkyScraper;
    VertexDescriptor VDLampPost;
    VertexDescriptor VDskyBox;

// **A10** Place here the variable for the Pipeline
    Pipeline PScooter;
    Pipeline PScooterFWheel;
    Pipeline PCity;
    Pipeline PSkyScraper;
    Pipeline PLampPost;
    Pipeline PskyBox;

/*
	// Scenes and texts
    TextMaker txt;
*/

    // Models, textures and Descriptor Sets (values assigned to the uniforms)
    DescriptorSet DSGlobal;

// **A10** Place here the variables for the Model, the five texture (diffuse, specular, normal map, emission and clouds) and the Descrptor Set
    Model MScooter;
    Model MFWheel;
    Model MCity;
    Model MSkyScraper;
    Model MLampPost;
    Model MskyBox;

    Texture TScooterBaseColor, TScooterNormal, TScooterHeight, TScooterMetallic, TScooterRoughness, TScooterAmbientOcclusion;
    Texture TCity;
    Texture TSkyScraper;
    Texture TLampPost;
    Texture TskyBox;

    DescriptorSet DSScooter;
    DescriptorSet DSScooterFWheel;
    DescriptorSet DSCity;
    DescriptorSet DSSkyScraper;
    DescriptorSet DSLampPost;
    DescriptorSet DSskyBox;

    // Other application parameters
    int currScene = 0;
    int subpass = 0;

    glm::vec3 CamPos = glm::vec3(0.0, 0.1, 5.0);
    glm::mat4 ViewMatrix;

    float Ar;
	glm::vec3 ScooterPos;
	glm::vec3 InitialPos;
    float Yaw = 0;
    float Roll = 0;

    // Generates centers for the buildings, going from -84 to 84 with a step of 24
    std::vector<glm::vec2> centers = generateCenters(84, 24);

    std::vector<BoundingBox> barriers = getBarriers(centers, 20);
    BoundingBox externalBarriers = {{-98.0f, -98.0f}, {98.0f, 98.0f}};



    // Here you set the main application parameters
    void setWindowParameters() {
        // window size, titile and initial background
        windowWidth = 800;
        windowHeight = 600;
        windowTitle = "A10 - Adding an object";
        windowResizable = GLFW_TRUE;
        initialBackgroundColor = {0.1f, 0.1f, 0.1f, 1.0f};

        Ar = (float)windowWidth / (float)windowHeight;
    }

    // What to do when the window changes size
    void onWindowResize(int w, int h) {
        std::cout << "Window resized to: " << w << " x " << h << "\n";
        Ar = (float)w / (float)h;
    }

    // Here you load and setup all your Vulkan Models and Texutures.
    // Here you also create your Descriptor set layouts and load the shaders for the pipelines
    void localInit() {

        // Descriptor Layouts [what will be passed to the shaders]
        DSLGlobal.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject), 1}
        });

// **A10** Place here the initialization of the the DescriptorSetLayout
        DSLScooter.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(ScooterMatricesUniformBufferObject), 1},  // Matrice uniforme del modello scooter
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},  // Texture Base Color
                {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},  // Texture Normal
                {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1},  // Texture Height
                {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1},  // Texture Metallic
                {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1},  // Texture Roughness
                {6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5, 1},  // Texture Ambient Occlusion
                {7, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ScooterParametersUniformBufferObject), 1},
        });
        DSLCity.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(CityMatricesUniformBufferObject), 1},  // Matrice uniforme del modello scooter
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},  // Texture Base Color
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(CityParametersUniformBufferObject), 1},
        });
        DSLSkyScraper.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(SkyScraperMatricesUniformBufferObject), 1},  // Matrice uniforme del modello scooter
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},  // Texture Base Color
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(SkyScraperParametersUniformBufferObject), 1},
        });
        DSLLampPost.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(LampPostMatricesUniformBufferObject), 1},  // Matrice uniforme del modello scooter
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},  // Texture Base Color
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(LampPostParametersUniformBufferObject), 1},
        });
        DSLskyBox.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(skyBoxUniformBufferObject), 1},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
        });

// **A10** Place here the initialization for the VertexDescriptor
        VDScooter.init(this, {
                {0, sizeof(ScooterVertex), VK_VERTEX_INPUT_RATE_VERTEX}  // Descrive la dimensione del vertice e la frequenza di input
        }, {
                               // Descrizione degli attributi dei vertici
                               {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ScooterVertex, pos), sizeof(glm::vec3), POSITION},  // Posizione (3 componenti float)
                               {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ScooterVertex, norm), sizeof(glm::vec3), NORMAL},    // Normale (3 componenti float)
                               {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(ScooterVertex, UV), sizeof(glm::vec2), UV},             // UV (2 componenti float)
                               {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ScooterVertex, tan), sizeof(glm::vec4), TANGENT}  // Tangente (4 componenti float)
                       });

        VDCity.init(this, {
                {0, sizeof(CityVertex), VK_VERTEX_INPUT_RATE_VERTEX}  // Descrive la dimensione del vertice e la frequenza di input
        }, {
                            // Descrizione degli attributi dei vertici
                            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(CityVertex, pos), sizeof(glm::vec3), POSITION},  // Posizione (3 componenti float)
                            {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(CityVertex, norm), sizeof(glm::vec3), NORMAL},    // Normale (3 componenti float)
                            {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(CityVertex, UV), sizeof(glm::vec2), UV},             // UV (2 componenti float)
                            {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(CityVertex, tan), sizeof(glm::vec4), TANGENT}  // Tangente (4 componenti float)
                    });
        VDSkyScraper.init(this, {
                {0, sizeof(SkyScraperVertex), VK_VERTEX_INPUT_RATE_VERTEX}  // Descrive la dimensione del vertice e la frequenza di input
        }, {
                                  // Descrizione degli attributi dei vertici
                                  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SkyScraperVertex, pos), sizeof(glm::vec3), POSITION},  // Posizione (3 componenti float)
                                  {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SkyScraperVertex, norm), sizeof(glm::vec3), NORMAL},    // Normale (3 componenti float)
                                  {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(SkyScraperVertex, UV), sizeof(glm::vec2), UV},             // UV (2 componenti float)
                                  {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(SkyScraperVertex, tan), sizeof(glm::vec4), TANGENT}  // Tangente (4 componenti float)
                          });
        VDLampPost.init(this, {
                {0, sizeof(LampPostVertex), VK_VERTEX_INPUT_RATE_VERTEX}  // Descrive la dimensione del vertice e la frequenza di input
        }, {
                                  // Descrizione degli attributi dei vertici
                                  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(LampPostVertex, pos), sizeof(glm::vec3), POSITION},  // Posizione (3 componenti float)
                                  {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(LampPostVertex, norm), sizeof(glm::vec3), NORMAL},    // Normale (3 componenti float)
                                  {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(LampPostVertex, UV), sizeof(glm::vec2), UV},             // UV (2 componenti float)
                                  {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(LampPostVertex, tan), sizeof(glm::vec4), TANGENT}  // Tangente (4 componenti float)
                          });
        VDskyBox.init(this, {
                {0, sizeof(skyBoxVertex), VK_VERTEX_INPUT_RATE_VERTEX}
        }, {
                              {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(skyBoxVertex, pos),
                               sizeof(glm::vec3), POSITION}
                      });

// **A10** Place here the initialization of the pipeline. Remember that it should use shaders in files
//		"shaders/NormalMapVert.spv" and "shaders/NormalMapFrag.spv", it should receive the new VertexDescriptor you defined
//		And should receive two DescriptorSetLayout, the first should be DSLGlobal, while the other must be the one you defined
        PScooter.init(this, &VDScooter, "shaders/NormalMapVert.spv", "shaders/NormalMapFrag.spv", {&DSLGlobal, &DSLScooter});
        PCity.init(this, &VDCity, "shaders/NormalMapVert.spv", "shaders/CityFrag.spv", {&DSLGlobal, &DSLCity});
        PSkyScraper.init(this, &VDSkyScraper, "shaders/SkyScraperVert.spv", "shaders/SkyScraperFrag.spv", {&DSLGlobal, &DSLSkyScraper});
        PLampPost.init(this, &VDLampPost, "shaders/LampPostVert.spv", "shaders/LampPostFrag.spv", {&DSLGlobal, &DSLLampPost});
        PskyBox.init(this, &VDskyBox, "shaders/SkyBoxVert.spv", "shaders/SkyBoxFrag.spv", {&DSLskyBox});
        PskyBox.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
                                    VK_CULL_MODE_BACK_BIT, false);

// **A10** Place here the loading of the model. It should be contained in file "models/Sphere.gltf", it should use the
//		Vertex descriptor you defined, and be of GLTF format.
        MScooter.init(this, &VDScooter, "models/Scooter.obj", OBJ);
        MCity.init(this, &VDCity, "models/road.obj", OBJ);
        MSkyScraper.init(this, &VDSkyScraper, "models/apartment_001.mgcg", MGCG);
        MLampPost.init(this, &VDLampPost, "models/lamppost.mgcg", MGCG);
        MskyBox.init(this, &VDskyBox, "models/SkyBoxCube.obj", OBJ);

// **A10** Place here the loading of the four textures

//        Texture TScooterBaseColor, TScooterNormal, TScooterHeight, TScooterMetallic, TScooterRoughness, TScooterAmbientOcclusion;
        TScooterBaseColor.init(this, "textures/scooter/new/KR51BaseColor.png");

        // Normal map
        // note that it must add a special feature to support the normal map, in particular
        // the init function should be the following: .init(this, "path/to/textures/", VK_FORMAT_R8G8B8A8_UNORM);
        TScooterNormal.init(this, "textures/scooter/new/KR51Normal.png", VK_FORMAT_R8G8B8A8_UNORM);

        TScooterHeight.init(this, "textures/scooter/new/KR51Height.png");

        TScooterMetallic.init(this, "textures/scooter/new/KR51Metallic.png");

        TScooterRoughness.init(this, "textures/scooter/new/KR51Roughness.png");

        TScooterAmbientOcclusion.init(this, "textures/scooter/new/KR51AO.png");

        TCity.init(this, "textures/city/road.png");

        TSkyScraper.init(this, "textures/skyScraper/apartment_001.png");

        TLampPost.init(this, "textures/lampPost/lamppost.png");

        TskyBox.init(this, "textures/starMap/starmap_g4k.jpg");

        // Descriptor pool sizes
        // WARNING!!!!!!!!
        // Must be set before initializing the text and the scene
// **A10** Update the number of elements to correctly size the descriptor sets pool
        DPSZs.uniformBlocksInPool = 1 + 2 + 2 + 2 + 2 + 1; // ScooterMatrixUniformBufferObject and ScooterShaderParameterdsUniformBufferObject, same for City, same for SkyScraper, same for LampPost, 1 for SkyBox
        DPSZs.texturesInPool = 6 + 1 + 1 + 1 + 1; // Textures (TScooterBaseColor, TScooterNormal, TScooterHeight, TScooterMetallic, TScooterRoughness, TScooterAmbientOcclusion) + 1 for city + 1 for SkyScraper + 1 for LampPost + 1 for Skybox
        DPSZs.setsInPool = 1 + 1 + 1 + 1 + 1 + 1; // DSScooter and DSCity and DSSkyScraper and DSLampPost and DSSkyBox

        std::cout << "Initialization completed!\n";
        std::cout << "Uniform Blocks in the Pool  : " << DPSZs.uniformBlocksInPool << "\n";
        std::cout << "Textures in the Pool        : " << DPSZs.texturesInPool << "\n";
        std::cout << "Descriptor Sets in the Pool : " << DPSZs.setsInPool << "\n";

        ViewMatrix = glm::translate(glm::mat4(1), -CamPos);

        ScooterPos = glm::vec3(0.0f, 0.0f, 0.0f);  // Imposta la posizione iniziale dello scooter
    }

    // Here you create your pipelines and Descriptor Sets!
    void pipelinesAndDescriptorSetsInit() {
// **A10** Add the pipeline creation
        PScooter.create();
        PCity.create();
        PSkyScraper.create();
        PLampPost.create();
        PskyBox.create();

// **A10** Add the descriptor set creation

//        Texture TScooterAmbientOcclusion, TScooterBaseColor, TScooterNormal, TScooterHeight, TScooterMetallic, TScooterRoughness;
        DSScooter.init(this,&DSLScooter,{&TScooterBaseColor, &TScooterNormal, &TScooterHeight, &TScooterMetallic, &TScooterRoughness, &TScooterAmbientOcclusion});
        DSCity.init(this,&DSLCity,{&TCity});
        DSSkyScraper.init(this,&DSLSkyScraper,{&TSkyScraper});
        DSLampPost.init(this,&DSLLampPost,{&TLampPost});
        DSskyBox.init(this, &DSLskyBox, {&TskyBox});

        DSGlobal.init(this, &DSLGlobal, {});
    }

    // Here you destroy your pipelines and Descriptor Sets!
    // All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
    void pipelinesAndDescriptorSetsCleanup() {
// **A10** Add the pipeline cleanup
        PScooter.cleanup();
        PCity.cleanup();
        PSkyScraper.cleanup();
        PLampPost.cleanup();
        PskyBox.cleanup();

        DSGlobal.cleanup();

// **A10** Add the descriptor set cleanup
        DSScooter.cleanup();
        DSCity.cleanup();
        DSSkyScraper.cleanup();
        DSLampPost.cleanup();
        DSskyBox.cleanup();
    }

    // Here you destroy all the Models, Texture and Desc. Set Layouts you created!
    // All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
    // You also have to destroy the pipelines: since they need to be rebuilt, they have two different
    // methods: .cleanup() recreates them, while .destroy() delete them completely
    void localCleanup() {
// **A10** Add the cleanup for models and textures
//        Texture TScooterBaseColor, TScooterNormal, TScooterHeight, TScooterMetallic, TScooterRoughness, TScooterAmbientOcclusion;
        TScooterBaseColor.cleanup();
        TScooterNormal.cleanup();
        TScooterHeight.cleanup();
        TScooterMetallic.cleanup();
        TScooterRoughness.cleanup();
        TScooterAmbientOcclusion.cleanup();
        MScooter.cleanup();

        TCity.cleanup();
        MCity.cleanup();

        TSkyScraper.cleanup();
        MSkyScraper.cleanup();

        TLampPost.cleanup();
        MLampPost.cleanup();

        TskyBox.cleanup();
        MskyBox.cleanup();

        DSLGlobal.cleanup();

// **A10** Add the cleanup for the descriptor set layout
        DSLScooter.cleanup();
        DSLCity.cleanup();
        DSLSkyScraper.cleanup();
        DSLLampPost.cleanup();
        DSLskyBox.cleanup();

// **A10** Add the cleanup for the pipeline
        PScooter.destroy();
        PCity.destroy();
        PSkyScraper.destroy();
        PLampPost.destroy();
        PskyBox.destroy();

    }

    // Here it is the creation of the command buffer:
    // You send to the GPU all the objects you want to draw,
    // with their buffers and textures

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

// **A10** Add the commands to bind the pipeline, the mesh its two descriptor setes, and the draw call of the planet

        std::cout << "test\n";

        std::vector<ModelOffsets> ScooterOffsets = calculateOffsets("models/Scooter.obj");
        PScooter.bind(commandBuffer);
        MScooter.bind(commandBuffer);

        DSGlobal.bind(commandBuffer, PScooter, 0, currentImage);
        DSScooter.bind(commandBuffer, PScooter, 1, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MScooter.indices.size()), 1, 0, 0, 0);


// 2. Binding del pipeline e del modello per la città
        PCity.bind(commandBuffer);                // Pipeline per la città
        MCity.bind(commandBuffer);                // Modello della città

// Binding dei descriptor set globali e quelli specifici della città
        DSGlobal.bind(commandBuffer, PCity, 0, currentImage);    // Descriptor Set globale per la città
        DSCity.bind(commandBuffer, PCity, 1, currentImage);      // Descriptor Set per la città

// Comando di disegno per la città
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MCity.indices.size()), 1, 0, 0, 0);

// 3. Binding del pipeline e del modello per lo SkyScraper
        PSkyScraper.bind(commandBuffer);                // Pipeline per la città
        MSkyScraper.bind(commandBuffer);                // Modello della città

// Binding dei descriptor set globali e quelli specifici dello SkyScraper
        DSGlobal.bind(commandBuffer, PSkyScraper, 0, currentImage);    // Descriptor Set globale per lo SkyScraper
        DSSkyScraper.bind(commandBuffer, PSkyScraper, 1, currentImage);      // Descriptor Set per lo SkyScraper

// Comando di disegno per lo SkyScraper
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MSkyScraper.indices.size()), NSKYSCRAPER, 0, 0, 0);

// 4. Binding del pipeline e del modello per il LampPost
        PLampPost.bind(commandBuffer);                // Pipeline per la città
        MLampPost.bind(commandBuffer);                // Modello della città

// Binding dei descriptor set globali e quelli specifici del LampPost
        DSGlobal.bind(commandBuffer, PLampPost, 0, currentImage);    // Descriptor Set globale per il LampPost
        DSLampPost.bind(commandBuffer, PLampPost, 1, currentImage);      // Descriptor Set per il LampPost

// Comando di disegno per il LampPost
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MLampPost.indices.size()), NLAMPPOST, 0, 0, 0);

//5. Binding del pipeline e del modello per lo SkyBox
        PskyBox.bind(commandBuffer);
        MskyBox.bind(commandBuffer);
        DSskyBox.bind(commandBuffer, PskyBox, 0, currentImage);
        // Disegna il cielo
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MskyBox.indices.size()), 1, 0, 0, 0);

    }


    // Here is where you update the uniforms.
    // Very likely this will be where you will be writing the logic of your application.
    void updateUniformBuffer(uint32_t currentImage) {
        static bool debounce = false;
		static int curDebounce = 0;

        float deltaT;
        glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
        bool fire = false;
//        bool print = false;

//        bool print = false;

        getSixAxis(deltaT, m, r, fire);


        //const float ROT_SPEED = glm::radians(360.0f);
		//const float MOVE_SPEED = 10.0f;
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


        //Velocity and steering equations
        const float STEERING_SPEED = glm::radians(60.0f);
		const float ROT_SPEED = glm::radians(120.0f);
		const float MOVE_SPEED = 10.0f;

		SteeringAng += -m.x * STEERING_SPEED * deltaT;
		SteeringAng = (SteeringAng < glm::radians(-35.0f) ? glm::radians(-35.0f) :
					  (SteeringAng > glm::radians(35.0f)  ? glm::radians(35.0f)  : SteeringAng));

        double lambdaVel = 8.0f;
		double dampedVelEpsilon = 0.001f;
		dampedVel =  MOVE_SPEED * deltaT * m.z * (1 - exp(-lambdaVel * deltaT)) +
					 dampedVel * exp(-lambdaVel * deltaT);
		dampedVel = ((fabs(dampedVel) < dampedVelEpsilon) ? 0.0f : dampedVel);
		wheelRoll -= dampedVel / 0.4;
		wheelRoll = (wheelRoll < 0.0 ? wheelRoll + 2*M_PI : (wheelRoll > 2*M_PI ? wheelRoll - 2*M_PI : wheelRoll));

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

/*
        if (proposedPos != ScooterPos) {
            print = true;
        }
*/
        //Verifica collisione con x e z;
        if (!checkCollision(proposedPos, ang, 0.7f)) {
            // Solo se non c'è collisione, aggiorna la posizione
            ScooterPos = proposedPos;
        }

        glm::mat4 Mv;

        glm::mat4 Wm = glm::translate(glm::mat4(1), ScooterPos) * glm::rotate(glm::mat4(1), ang + Yaw, glm::vec3(0,1,0))
                                                         * glm::rotate(glm::mat4(1), ang - SteeringAng, glm::vec3(0,0,1));

        glm::mat4 FWm = glm::translate(glm::mat4(1), ScooterPos) * glm::rotate(glm::mat4(1), ang + Yaw*3.0f, glm::vec3(0,1,0));

        if(glfwGetKey(window, GLFW_KEY_SPACE)){
            if(!debounce) {
				debounce = true;
				curDebounce = GLFW_KEY_SPACE;
				if(currScene != 1) {
					currScene = 1;
				}else if(currScene == 1) {
					currScene = 0;
				}

				RebuildPipeline();
			}
        }else {
			if((curDebounce == GLFW_KEY_SPACE) && debounce) {
				debounce = false;
				curDebounce = 0;
			}
		}

        if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

        //Perspective camera
        if(currScene == 0) {
			CamYaw += ROT_SPEED * deltaT * r.y;
			CamPitch -= ROT_SPEED * deltaT * r.x;
			CamRoll -= ROT_SPEED * deltaT * r.z;
			CamDist -= MOVE_SPEED * deltaT * m.y;
		
			CamYaw = (CamYaw < 0.0f ? 0.0f : (CamYaw > 2*M_PI ? 2*M_PI : CamYaw));
			CamPitch = (CamPitch < 0.0f ? 0.0f : (CamPitch > M_PI_2-0.01f ? M_PI_2-0.01f : CamPitch));
			CamRoll = (CamRoll < -M_PI ? -M_PI : (CamRoll > M_PI ? M_PI : CamRoll));
			CamDist = 5.0f;
				
			glm::vec3 CamTarget = ScooterPos + glm::vec3(0,2,-5);
			CamPos = CamTarget + glm::vec3(glm::rotate(glm::mat4(1), Yaw + CamYaw, glm::vec3(0,1,0)) * glm::rotate(glm::mat4(1), -CamPitch, glm::vec3(1,0,0)) * 
							 glm::vec4(0, 0, CamDist,1));

			const float lambdaCam = 10.0f;
			dampedCamPos = CamPos * (1 - exp(-lambdaCam * deltaT)) +
						 dampedCamPos * exp(-lambdaCam * deltaT); 
			M = MakeViewProjectionLookAt(dampedCamPos, CamTarget, glm::vec3(0,1,0), CamRoll, glm::radians(90.0f), Ar, 0.1f, 500.0f);
            Mv = ViewMatrix;
		}else if(currScene == 1){ //Isometric camera
            M = glm::mat4(1.0f / 10.0f, 0, 0, 0,
                  0, -4.0f / 30.0f, 0, 0,
                  0, 0, 1.0f/(-500.0f-500.0f), 0,
                  0, 0, -500.0f/(-500.0f-500.0f), 1)*
                glm::mat4(1, 0, 0, 0,
                  0, glm::cos(glm::radians(-35.26f)), -glm::sin(glm::radians(-35.26f)), 0,
                  0, glm::sin(glm::radians(-35.26f)), glm::cos(glm::radians(-35.26f)), 0,
                  0, 0, 0, 1) *
                glm::mat4(glm::cos(glm::radians(-45.0f)), 0, -glm::sin(glm::radians(-45.0f)), 0,
                  0, 1, 0, 0,
                  glm::sin(glm::radians(-45.0f)), 0, glm::cos(glm::radians(-45.0f)), 0,
                  0, 0, 0, 1);
            Mv =  glm::inverse(
							glm::translate(glm::mat4(1), ScooterPos) *
							glm::rotate(glm::mat4(1), DlookAng, glm::vec3(0,1,0))
						);
        }
        // Here is where you actually update your uniforms

        

        glm::mat4 ViewPrj =  M * Mv;

        // updates global uniforms
        // Global
        GlobalUniformBufferObject gubo{};

        gubo.lightDir = glm::vec3(0.0f, 1.0f, 0.0f);
        gubo.lightColor = glm::vec4(1.0f);
        gubo.eyePos = glm::vec3(glm::inverse(ViewMatrix) * glm::vec4(0, 3, 0, 1));
        DSGlobal.map(currentImage, &gubo, 0);


// **A10** Add to compute the uniforms and pass them to the shaders. You need two uniforms: one for the matrices, and the other for the material parameters.
        ScooterMatricesUniformBufferObject ScooterUbo{};
        ScooterParametersUniformBufferObject ScooterParUbo{};

        CityMatricesUniformBufferObject CityUbo{};
        CityParametersUniformBufferObject CityParUbo{};

        SkyScraperMatricesUniformBufferObject SkyScraperUbo{};
        SkyScraperParametersUniformBufferObject SkyScraperParUbo{};

        LampPostMatricesUniformBufferObject LampPostUbo{};
        LampPostParametersUniformBufferObject LampPostParUbo{};

        skyBoxUniformBufferObject SkyBoxUbo{};

        // World and normal matrix should be the identiy. The World-View-Projection should be variable ViewPrj
        
        ScooterUbo.mMat = Wm * glm::mat4(1.0f);
		ScooterUbo.mvpMat = ViewPrj * ScooterUbo.mMat;
		ScooterUbo.nMat = glm::inverse(glm::transpose(ScooterUbo.mMat));

        CityUbo.mMat = glm::mat4(1.0f);
        CityUbo.nMat = glm::mat4(1.0f);
        CityUbo.mvpMat = ViewPrj;

        for (int i = 0; i < NSKYSCRAPER; i++) {
            // Aggiorna la matrice del modello per il grattacielo corrente

            SkyScraperUbo.mMat[i] = glm::translate(glm::mat4(1.0f), glm::vec3(84.0 - (24.0 * (i % 8)),0.0,84 - (24 * (i / 8))));

            // Calcola la matrice MVP per il grattacielo corrente
            SkyScraperUbo.mvpMat[i] = ViewPrj * SkyScraperUbo.mMat[i];

            // Calcola la matrice normale per il grattacielo corrente
            SkyScraperUbo.nMat[i] = glm::inverse(glm::transpose(SkyScraperUbo.mMat[i]));
        }

        for(int i = 0; i < NLAMPPOST/4; i++) {
            LampPostUbo.mMat[i * 4] =
                    glm::translate(glm::mat4(1.0f), glm::vec3(75 - (24 * (i % 8)), 0.0, 85 - (24 * (i / 8)))) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            LampPostUbo.mvpMat[i * 4] = ViewPrj * LampPostUbo.mMat[i * 4];
            LampPostUbo.nMat[i * 4] = glm::inverse(glm::transpose(LampPostUbo.mMat[i * 4]));
            LampPostUbo.mMat[i * 4 + 1] =
                    glm::translate(glm::mat4(1.0f), glm::vec3(84 - (24 * (i % 8)), 0.0, 75 - (24 * (i / 8)))) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(-180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            LampPostUbo.mvpMat[i * 4 + 1] = ViewPrj * LampPostUbo.mMat[i * 4 + 1];
            LampPostUbo.nMat[i * 4 + 1] = glm::inverse(glm::transpose(LampPostUbo.mMat[i * 4 + 1]));
            LampPostUbo.mMat[i * 4 + 2] =
                    glm::translate(glm::mat4(1.0f), glm::vec3(93 - (24 * (i % 8)), 0.0, 85 - (24 * (i / 8)))) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(-270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            LampPostUbo.mvpMat[i * 4 + 2] = ViewPrj * LampPostUbo.mMat[i * 4 + 2];
            LampPostUbo.nMat[i * 4 + 2] = glm::inverse(glm::transpose(LampPostUbo.mMat[i * 4 + 2]));
            LampPostUbo.mMat[i * 4 + 3] = glm::translate(glm::mat4(1.0f), glm::vec3(84 - (24 * (i % 8)), 0.0, 93 - (24 * (i / 8))));
            LampPostUbo.mvpMat[i * 4 + 3] = ViewPrj * LampPostUbo.mMat[i * 4 + 3];
            LampPostUbo.nMat[i * 4 + 3] = glm::inverse(glm::transpose(LampPostUbo.mMat[i * 4 + 3]));
        }

        /*
        LampPostUbo.mMat[0] = glm::translate(glm::mat4(1.0f), glm::vec3(3.0,0.0,8.0)) * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        LampPostUbo.mvpMat[0] = ViewPrj * LampPostUbo.mMat[0];
        LampPostUbo.nMat[0] = glm::inverse(glm::transpose(LampPostUbo.mMat[0]));
        LampPostUbo.mMat[1] = glm::translate(glm::mat4(1.0f), glm::vec3(3.0,0.0,18.0)) * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        LampPostUbo.mvpMat[1] = ViewPrj * LampPostUbo.mMat[1];
        LampPostUbo.nMat[1] = glm::inverse(glm::transpose(LampPostUbo.mMat[1]));
        LampPostUbo.mMat[2] = glm::translate(glm::mat4(1.0f), glm::vec3(7.0,0.0,3.0)) * glm::rotate(glm::mat4(1.0f), glm::radians(-180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        LampPostUbo.mvpMat[2] = ViewPrj * LampPostUbo.mMat[2];
        LampPostUbo.nMat[2] = glm::inverse(glm::transpose(LampPostUbo.mMat[2]));
        LampPostUbo.mMat[3] = glm::translate(glm::mat4(1.0f), glm::vec3(17.0,0.0,3.0)) * glm::rotate(glm::mat4(1.0f), glm::radians(-180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        LampPostUbo.mvpMat[3] = ViewPrj * LampPostUbo.mMat[3];
        LampPostUbo.nMat[3] = glm::inverse(glm::transpose(LampPostUbo.mMat[3]));
        LampPostUbo.mMat[4] = glm::translate(glm::mat4(1.0f), glm::vec3(21.0,0.0,8.0)) * glm::rotate(glm::mat4(1.0f), glm::radians(-270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        LampPostUbo.mvpMat[4] = ViewPrj * LampPostUbo.mMat[4];
        LampPostUbo.nMat[4] = glm::inverse(glm::transpose(LampPostUbo.mMat[4]));
        LampPostUbo.mMat[5] = glm::translate(glm::mat4(1.0f), glm::vec3(21.0,0.0,18.0)) * glm::rotate(glm::mat4(1.0f), glm::radians(-270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        LampPostUbo.mvpMat[5] = ViewPrj * LampPostUbo.mMat[5];
        LampPostUbo.nMat[5] = glm::inverse(glm::transpose(LampPostUbo.mMat[5]));
        LampPostUbo.mMat[6] = glm::translate(glm::mat4(1.0f), glm::vec3(7.0,0.0,21.0));
        LampPostUbo.mvpMat[6] = ViewPrj * LampPostUbo.mMat[6];
        LampPostUbo.nMat[6] = glm::inverse(glm::transpose(LampPostUbo.mMat[6]));
        LampPostUbo.mMat[7] = glm::translate(glm::mat4(1.0f), glm::vec3(17.0,0.0,21.0));
        LampPostUbo.mvpMat[7] = ViewPrj * LampPostUbo.mMat[7];
        LampPostUbo.nMat[7] = glm::inverse(glm::transpose(LampPostUbo.mMat[7]));
         */

        float SkyBox_scale_factor = 98.0f;

// Traslazione verso l'origine per centrare la SkyBox
        glm::vec3 skybox_center_offset = glm::vec3(0, 3, -5); // Sostituisci con i valori corretti per centrare la SkyBox

        SkyBoxUbo.mvpMat = M * glm::mat4(glm::mat3(ViewMatrix))
                           * glm::translate(glm::mat4(1.0f), skybox_center_offset) // Traslazione
                           * glm::scale(glm::mat4(1.0f), glm::vec3(SkyBox_scale_factor, SkyBox_scale_factor, SkyBox_scale_factor)); // Scala

        // These informations should be used to fill the Uniform Buffer Object in Binding 0 of your DSL
        DSScooter.map(currentImage, &ScooterUbo, 0);

        DSCity.map(currentImage, &CityUbo, 0);

        DSSkyScraper.map(currentImage, &SkyScraperUbo, 0);

        DSLampPost.map(currentImage, &LampPostUbo, 0);

        DSskyBox.map(currentImage, &SkyBoxUbo, 0);

        // The specular power of the uniform buffer containing the material parameters of the new object should be set to:
        // XXX.Power = 200.0
        // Where you replace XXX.Power with the field of the local variable corresponding to the uniform buffer object
        ScooterParUbo.Pow = 200.0f;

        CityParUbo.Pow = 200.0f;

        SkyScraperParUbo.Pow = 200.0f;

        LampPostParUbo.Pow = 200.0f;

        // The textre angle parameter of the uniform buffer containing the material parameters of the new object shoud be set to: tTime * TangTurnTimeFact
        // XXX.Ang = tTime * TangTurnTimeFact;
        // Where you replace XXX.Ang with the local field of the variable corresponding to the uniform buffer object
        ScooterParUbo.Ang = 0.0f;

        CityParUbo.Ang = 0.0f;

        SkyScraperParUbo.Ang = 0.0f;

        LampPostParUbo.Ang = 0.0f;

        // These informations should be used to fill the Uniform Buffer Object in Binding 6 of your DSL
        DSScooter.map(currentImage, &ScooterParUbo, 7);

        DSCity.map(currentImage, &CityParUbo, 2);

        DSSkyScraper.map(currentImage, &SkyScraperParUbo, 2);

        DSLampPost.map(currentImage, &LampPostParUbo, 2);

    }



    bool checkCollision(glm::vec3 centerPos, float angle, float halfLength) {
        // Direzione della moto basata sul suo orientamento
/*        glm::vec2 direction(cos(angle), sin(angle));
        glm::vec2 perpendicularDirection(-direction.y, direction.x);*/
        glm::vec2 direction(-sin(angle), cos(angle));


        // Calcola i punti davanti e dietro al baricentro
        glm::vec2 frontPoint = glm::vec2(centerPos.x, centerPos.z) + halfLength * direction;
        glm::vec2 backPoint = glm::vec2(centerPos.x, centerPos.z) - halfLength * direction;

/*
        if (print) {
            std::cout << "Front point: \t" << frontPoint.x << ", \t" << frontPoint.y << std::endl;
            std::cout << "Center: \t" << centerPos.x << ", \t" << centerPos.z << std::endl;
            std::cout << "Back point: \t" << backPoint.x << ", \t" << backPoint.y << std::endl;
            std::cout << "Angle: \t" << angle << ", \t" <<angle*180/PI << std::endl;
            std::cout << "---------------------------------------" << std::endl;
        }
*/
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
            return true;
        }

        // Controlla la collisione sia per il punto frontale che per il punto posteriore
        for (const auto& barrier : barriers) {
            if ((frontPoint.x >= barrier.min.x && frontPoint.x <= barrier.max.x &&
                frontPoint.y >= barrier.min.y && frontPoint.y <= barrier.max.y) || // check if front point is in barrier
                (backPoint.x >= barrier.min.x && backPoint.x <= barrier.max.x &&
                backPoint.y >= barrier.min.y && backPoint.y <= barrier.max.y))     // check if back point is in barrier
            {
//                std::cout << "-------- COLLISIONE ------------" << std::endl;
                return true;  // Collisione rilevata
            }
        }
        return false;
    }

    std::vector<glm::vec2> generateCenters(int coord, int step) {
        std::vector<glm::vec2> centers;

        for (int i = -coord; i <= coord; i+=step) {
            for (int j = -coord; j <= coord; j+=step) {
                centers.push_back(glm::vec2(static_cast<float>(i), static_cast<float>(j)));
            }
        }

        return centers;
    }

    std::vector<BoundingBox> getBarriers(const std::vector<glm::vec2>& centers, float sideLength) {
        std::vector<BoundingBox> barriers;
        float halfSide = sideLength / 2.0f;

        for (const auto& center : centers) {
            glm::vec2 min = {center.x - halfSide, center.y - halfSide};
            glm::vec2 max = {center.x + halfSide, center.y + halfSide};
            barriers.push_back({min, max});
        }

        return barriers;
    }


};

// This is the main: probably you do not need to touch this!
int main() {
    A10 app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


