// This has been adapted from the Vulkan tutorial

#include "headers/tiny_obj_loader.h"
#include "modules/Starter.hpp"
#include "WVP.hpp"

#include "modules/TextMaker.hpp"

#include <thread>
#include <chrono>

#define PI 3.14159265359


TextMaker txt;

std::vector<SingleText> outText = {{2, {"test", "", "", ""}, 0, 0}};

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

struct SingleObjectMatricesUniformBufferObject {
        alignas(16) glm::mat4 mvpMat;
        alignas(16) glm::mat4 mMat;
        alignas(16) glm::mat4 nMat;
};

#define NSKYSCRAPER 16
int indicesSkyScraper1[NSKYSCRAPER] = {3, 15, 8, 30, 12, 5, 26, 1, 9, 16, 22, 37, 40, 49, 61};
int indicesSkyScraper2[NSKYSCRAPER] = {2, 6, 10, 18, 20, 25, 33, 41, 45, 50, 52, 53, 54, 56, 58, 60};
int indicesSkyScraper3[NSKYSCRAPER] = {4, 7, 11, 14, 19, 21, 23, 29, 31, 32, 34, 39, 42, 44, 47, 48};
int indicesSkyScraper4[NSKYSCRAPER] = {0, 9, 13, 17, 24, 27, 35, 36, 38, 43, 46, 51, 55, 57, 59, 62};

struct SkyScraperMatricesUniformBufferObject {
    alignas(16) glm::mat4 mvpMat[NSKYSCRAPER];  // Model-View-Projection matrix
    alignas(16) glm::mat4 mMat[NSKYSCRAPER];    // Model matrix
    alignas(16) glm::mat4 nMat[NSKYSCRAPER];    // Normal matrix
};

#define NTREE 64
int indicesTree1[16] = {3, 15, 8, 30, 12, 5, 26, 1, 9, 16, 22, 28, 37, 40, 49, 61};
int indicesTree2[16] = {2, 6, 10, 18, 20, 25, 33, 41, 45, 50, 52, 53, 54, 56, 58, 60};
int indicesTree3[16] = {4, 7, 11, 14, 19, 21, 23, 29, 31, 32, 34, 39, 42, 44, 47, 48};
int indicesTree4[16] = {0, 9, 13, 17, 24, 27, 35, 36, 38, 43, 46, 51, 55, 57, 59, 62};

struct TreeMatricesUniformBufferObject {
    alignas(16) glm::mat4 mvpMat[NTREE];  // Model-View-Projection matrix
    alignas(16) glm::mat4 mMat[NTREE];    // Model matrix
    alignas(16) glm::mat4 nMat[NTREE];    // Normal matrix
};

// Funzione per aggiornare le matrici di un albero
void updateTreeMatrices(TreeMatricesUniformBufferObject* treeUbo, int& counter, int i, int offset, const glm::mat4& ViewPrj, bool applyScale) {
    for (int k = 0; k < 4; ++k) {
        glm::vec3 translation;
        switch (k) {
            case 0: translation = glm::vec3(75 - (24 * (i % 8)), 0.0, 85 - offset - (24 * (i / 8))); break;
            case 1: translation = glm::vec3(84 + offset - (24 * (i % 8)), 0.0, 75 - (24 * (i / 8))); break;
            case 2: translation = glm::vec3(93 - (24 * (i % 8)), 0.0, 85 + offset - (24 * (i / 8))); break;
            case 3: translation = glm::vec3(84 - offset - (24 * (i % 8)), 0.0, 93 - (24 * (i / 8))); break;
        }

        int idx = counter * 4 + k;
        treeUbo->mMat[idx] = glm::translate(glm::mat4(1.0f), translation);

        // Applica la scala solo se richiesto
        if (applyScale) {
            treeUbo->mMat[idx] = glm::scale(treeUbo->mMat[idx], glm::vec3(0.5f, 0.5f, 0.5f));
        }

        treeUbo->mvpMat[idx] = ViewPrj * treeUbo->mMat[idx];
        treeUbo->nMat[idx] = glm::inverse(glm::transpose(treeUbo->mMat[idx]));
    }

    ++counter;
}

#define NLAMPPOST 256
struct LampPostMatricesUniformBufferObject {
    alignas(16) glm::mat4 mvpMat[NLAMPPOST];  // Model-View-Projection matrix
    alignas(16) glm::mat4 mMat[NLAMPPOST];    // Model matrix
    alignas(16) glm::mat4 nMat[NLAMPPOST];    // Normal matrix
};

struct skyBoxUniformBufferObject {
    alignas(16) glm::mat4 mvpMat;           //usata per il rendering della skybox (la scatola che rappresenta il cielo o lo sfondo)
};

struct EmissionUniformBufferObject {
    alignas(16) glm::mat4 mvpMat;           //utilizzato per oggetti con emissione di luce
};

// **A10** Place here the CPP struct for the uniform buffer for the parameters
struct ObjectParametersUniformBufferObject {
    alignas(4) float Pow;
    alignas(4) float Ang;
};


// **A10** Place here the CPP struct for the vertex definition
struct ObjectVertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 UV;
    glm::vec4 tan;
};

struct skyBoxVertex {
    glm::vec3 pos;
};

struct EmissionVertex {
    glm::vec3 pos;
    glm::vec2 UV;
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
    DescriptorSetLayout DSLPizzeria;
    DescriptorSetLayout DSLSkyScraper[4];
    DescriptorSetLayout DSLTree[4];
    DescriptorSetLayout DSLLampPost;
    DescriptorSetLayout DSLskyBox;	// For skyBox
    DescriptorSetLayout DSLEmission;


// **A10** Place here the variable for the VertexDescriptor
    VertexDescriptor VDScooter;
    VertexDescriptor VDScooterFWheel;
    VertexDescriptor VDCity;
    VertexDescriptor VDPizzeria;
    VertexDescriptor VDSkyScraper[4];
    VertexDescriptor VDTree[4];
    VertexDescriptor VDLampPost;
    VertexDescriptor VDskyBox;
    VertexDescriptor VDEmission;

// **A10** Place here the variable for the Pipeline
    Pipeline PScooter;
    Pipeline PScooterFWheel;
    Pipeline PCity;
    Pipeline PPizzeria;
    Pipeline PSkyScraper[4];
    Pipeline PTree[4];
    Pipeline PLampPost;
    Pipeline PskyBox;
    Pipeline PEmission;

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
    Model MPizzeria;
    Model MSkyScraper[4];
    Model MTree[4];
    Model MLampPost;
    Model MskyBox;
    Model Msun;

    Texture TScooterBaseColor, TScooterNormal, TScooterHeight, TScooterMetallic, TScooterRoughness, TScooterAmbientOcclusion;
    Texture TCity;
    Texture TPizzeria;
    Texture TSkyScraper[4];
    Texture TTree[4];
    Texture TLampPost;
    Texture TskyBox;
    Texture Tsun;

    DescriptorSet DSScooter;
    DescriptorSet DSScooterFWheel;
    DescriptorSet DSCity;
    DescriptorSet DSPizzeria;
    DescriptorSet DSSkyScraper[4];
    DescriptorSet DSTree[4];
    DescriptorSet DSLampPost;
    DescriptorSet DSskyBox;
    DescriptorSet DSsun;

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
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(SingleObjectMatricesUniformBufferObject), 1},  // Matrice uniforme del modello scooter
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},  // Texture Base Color
                {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},  // Texture Normal
                {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1},  // Texture Height
                {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1},  // Texture Metallic
                {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1},  // Texture Roughness
                {6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5, 1},  // Texture Ambient Occlusion
                {7, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectParametersUniformBufferObject), 1},
        });
        DSLCity.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(SingleObjectMatricesUniformBufferObject), 1},  // Matrice uniforme del modello scooter
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},  // Texture Base Color
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectParametersUniformBufferObject), 1},
        });
        DSLPizzeria.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(SingleObjectMatricesUniformBufferObject), 1},  // Matrice uniforme del modello scooter
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},  // Texture Base Color
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectParametersUniformBufferObject), 1},
        });
        for(int i = 0; i < 4; i++)
        {
            DSLSkyScraper[i].init(this, {
                    {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(SkyScraperMatricesUniformBufferObject), 1},
                    {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
                    {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectParametersUniformBufferObject), 1},
            });
        }
        for(int i = 0; i < 4; i++)
        {
            DSLTree[i].init(this, {
                    {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(TreeMatricesUniformBufferObject), 1},  // Matrice uniforme del modello scooter
                    {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},  // Texture
                    {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectParametersUniformBufferObject), 1},
            });
        }
        DSLLampPost.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(LampPostMatricesUniformBufferObject), 1},  // Matrice uniforme del modello scooter
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},  // Texture Base Color
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectParametersUniformBufferObject), 1},
        });
        DSLskyBox.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(skyBoxUniformBufferObject), 1},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
        });
        DSLEmission.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(EmissionUniformBufferObject), 1},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
        });

// **A10** Place here the initialization for the VertexDescriptor
        VDScooter.init(this, {
                {0, sizeof(ObjectVertex), VK_VERTEX_INPUT_RATE_VERTEX}  // Descrive la dimensione del vertice e la frequenza di input
        }, {
                               // Descrizione degli attributi dei vertici
                               {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjectVertex, pos), sizeof(glm::vec3), POSITION},  // Posizione (3 componenti float)
                               {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjectVertex, norm), sizeof(glm::vec3), NORMAL},    // Normale (3 componenti float)
                               {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(ObjectVertex, UV), sizeof(glm::vec2), UV},             // UV (2 componenti float)
                               {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ObjectVertex, tan), sizeof(glm::vec4), TANGENT}  // Tangente (4 componenti float)
                       });

        VDCity.init(this, {
                {0, sizeof(ObjectVertex), VK_VERTEX_INPUT_RATE_VERTEX}  // Descrive la dimensione del vertice e la frequenza di input
        }, {
                            // Descrizione degli attributi dei vertici
                            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjectVertex, pos), sizeof(glm::vec3), POSITION},  // Posizione (3 componenti float)
                            {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjectVertex, norm), sizeof(glm::vec3), NORMAL},    // Normale (3 componenti float)
                            {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(ObjectVertex, UV), sizeof(glm::vec2), UV},             // UV (2 componenti float)
                            {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ObjectVertex, tan), sizeof(glm::vec4), TANGENT}  // Tangente (4 componenti float)
                    });
        VDPizzeria.init(this, {
                {0, sizeof(ObjectVertex), VK_VERTEX_INPUT_RATE_VERTEX}  // Descrive la dimensione del vertice e la frequenza di input
        }, {
                            // Descrizione degli attributi dei vertici
                            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjectVertex, pos), sizeof(glm::vec3), POSITION},  // Posizione (3 componenti float)
                            {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjectVertex, norm), sizeof(glm::vec3), NORMAL},    // Normale (3 componenti float)
                            {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(ObjectVertex, UV), sizeof(glm::vec2), UV},             // UV (2 componenti float)
                            {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ObjectVertex, tan), sizeof(glm::vec4), TANGENT}  // Tangente (4 componenti float)
                    });
        for(int i = 0; i < 4; i++)
        {
            VDSkyScraper[i].init(this, {
                    {0, sizeof(ObjectVertex), VK_VERTEX_INPUT_RATE_VERTEX}  // Descrive la dimensione del vertice e la frequenza di input
            }, {
                                       // Descrizione degli attributi dei vertici
                                       {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjectVertex, pos), sizeof(glm::vec3), POSITION},  // Posizione (3 componenti float)
                                       {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjectVertex, norm), sizeof(glm::vec3), NORMAL},    // Normale (3 componenti float)
                                       {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(ObjectVertex, UV), sizeof(glm::vec2), UV},             // UV (2 componenti float)
                                       {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ObjectVertex, tan), sizeof(glm::vec4), TANGENT}  // Tangente (4 componenti float)
                               });
        }
        for(int i = 0; i < 4; i++)
        {
            VDTree[i].init(this, {
                    {0, sizeof(ObjectVertex), VK_VERTEX_INPUT_RATE_VERTEX}  // Descrive la dimensione del vertice e la frequenza di input
            }, {
                                 // Descrizione degli attributi dei vertici
                                 {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjectVertex, pos), sizeof(glm::vec3), POSITION},  // Posizione (3 componenti float)
                                 {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjectVertex, norm), sizeof(glm::vec3), NORMAL},    // Normale (3 componenti float)
                                 {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(ObjectVertex, UV), sizeof(glm::vec2), UV},             // UV (2 componenti float)
                                 {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ObjectVertex, tan), sizeof(glm::vec4), TANGENT}  // Tangente (4 componenti float)
                         });
        }
        VDLampPost.init(this, {
                {0, sizeof(ObjectVertex), VK_VERTEX_INPUT_RATE_VERTEX}  // Descrive la dimensione del vertice e la frequenza di input
        }, {
                                  // Descrizione degli attributi dei vertici
                                  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjectVertex, pos), sizeof(glm::vec3), POSITION},  // Posizione (3 componenti float)
                                  {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjectVertex, norm), sizeof(glm::vec3), NORMAL},    // Normale (3 componenti float)
                                  {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(ObjectVertex, UV), sizeof(glm::vec2), UV},             // UV (2 componenti float)
                                  {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ObjectVertex, tan), sizeof(glm::vec4), TANGENT}  // Tangente (4 componenti float)
                          });
        VDskyBox.init(this, {
                {0, sizeof(skyBoxVertex), VK_VERTEX_INPUT_RATE_VERTEX}
        }, {
                              {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(skyBoxVertex, pos),
                               sizeof(glm::vec3), POSITION}
                      });
        VDEmission.init(this, {
                {0, sizeof(EmissionVertex), VK_VERTEX_INPUT_RATE_VERTEX}
        }, {
                                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(EmissionVertex, pos),
                                        sizeof(glm::vec3), POSITION},
                                {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(EmissionVertex, UV),
                                        sizeof(glm::vec2), UV}
                        });

// **A10** Place here the initialization of the pipeline. Remember that it should use shaders in files
//		"shaders/NormalMapVert.spv" and "shaders/NormalMapFrag.spv", it should receive the new VertexDescriptor you defined
//		And should receive two DescriptorSetLayout, the first should be DSLGlobal, while the other must be the one you defined
        PScooter.init(this, &VDScooter, "shaders/NormalMapVert.spv", "shaders/NormalMapFrag.spv", {&DSLGlobal, &DSLScooter});
        PCity.init(this, &VDCity, "shaders/NormalMapVert.spv", "shaders/CityFrag.spv", {&DSLGlobal, &DSLCity});
        PPizzeria.init(this, &VDPizzeria, "shaders/NormalMapVert.spv", "shaders/PizzeriaFrag.spv", {&DSLGlobal, &DSLPizzeria});
        for(int i = 0; i < 4; i++)
        {
            PSkyScraper[i].init(this, &VDSkyScraper[i], "shaders/SkyScraperVert.spv", "shaders/SkyScraperFrag.spv", {&DSLGlobal, &DSLSkyScraper[i]});
        }
        for(int i = 0; i < 4; i++)
        {
            PTree[i].init(this, &VDTree[i], "shaders/TreeVert.spv", "shaders/TreeFrag.spv", {&DSLGlobal, &DSLTree[i]});
        }
        PLampPost.init(this, &VDLampPost, "shaders/LampPostVert.spv", "shaders/LampPostFrag.spv", {&DSLGlobal, &DSLLampPost});
        PskyBox.init(this, &VDskyBox, "shaders/SkyBoxVert.spv", "shaders/SkyBoxFrag.spv", {&DSLskyBox});
        PskyBox.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
                                    VK_CULL_MODE_BACK_BIT, false);
        PEmission.init(this, &VDEmission,  "shaders/EmissionVert.spv",    "shaders/EmissionFrag.spv", {&DSLEmission});

// **A10** Place here the loading of the model. It should be contained in file "models/Sphere.gltf", it should use the
//		Vertex descriptor you defined, and be of GLTF format.
        MScooter.init(this, &VDScooter, "models/Scooter.obj", OBJ);
        MCity.init(this, &VDCity, "models/road.obj", OBJ);
        MPizzeria.init(this, &VDPizzeria, "models/Pizzeria/pizzeria.obj", OBJ);
        const char* modelPaths[4] = {
                "models/apartment_1.mgcg",
                "models/apartment_2.mgcg",
                "models/apartment_3.mgcg",
                "models/apartment_4.mgcg"
        };
        for (int i = 0; i < 4; ++i) {
            MSkyScraper[i].init(this, &VDSkyScraper[i], modelPaths[i], MGCG);
        }
        const char* treeModelPaths[4] = {
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

        TPizzeria.init(this, "textures/pizzeria/TPizzeria.jpeg");

        for(int i = 0; i < 4; i++)
        {
            TSkyScraper[i].init(this, "textures/Textures_Skyscrapers.png");
        }
        for(int i = 0; i < 4; i++)
        {
            TTree[i].init(this, "textures/Textures_Vegetation.png");
        }

        TLampPost.init(this, "textures/Textures_Skyscrapers.png");

        TskyBox.init(this, "textures/starMap/starmap_g4k.jpg");

        Tsun.init(this, "textures/sun/2k_sun.jpg");

        // Descriptor pool sizes
        // WARNING!!!!!!!!
        // Must be set before initializing the text and the scene
// **A10** Update the number of elements to correctly size the descriptor sets pool
        DPSZs.uniformBlocksInPool = 1 + 2 + 2 + 2 + 8 + 8 + 2 + 1 + 1; // ScooterMatrixUniformBufferObject and ScooterShaderParameterdsUniformBufferObject, same for City, same for Pizzeria, same for the four SkyScrapers, same for the four trees, same for LampPost, 1 for SkyBox, 1 for sun
        DPSZs.texturesInPool = 6 + 1 + 1 + 4 + 4 + 1 + 1 + 1; // Textures (TScooterBaseColor, TScooterNormal, TScooterHeight, TScooterMetallic, TScooterRoughness, TScooterAmbientOcclusion) + 1 for city + 1 for pizzeria + 4 for the skyscrapers + 4 for the trees + 1 for LampPost + 1 for Skybox + 1 for sun
        DPSZs.setsInPool = 1 + 1 + 1 + 4 + 4 + 1 + 1 + 1 + 1; // DSScooter and DSCity and DSPizzeria and the four DS for SkyScrapers and the four DS for Trees and DSLampPost and DSSkyBox and DSSun

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
        PPizzeria.create();
        for(int i = 0; i < 4; i++)
        {
            PSkyScraper[i].create();
        }
        for(int i = 0; i < 4; i++)
        {
            PTree[i].create();
        }
        PLampPost.create();
        PskyBox.create();
        PEmission.create();

// **A10** Add the descriptor set creation

//        Texture TScooterAmbientOcclusion, TScooterBaseColor, TScooterNormal, TScooterHeight, TScooterMetallic, TScooterRoughness;
        DSScooter.init(this,&DSLScooter,{&TScooterBaseColor, &TScooterNormal, &TScooterHeight, &TScooterMetallic, &TScooterRoughness, &TScooterAmbientOcclusion});
        DSCity.init(this,&DSLCity,{&TCity});
        DSPizzeria.init(this,&DSLPizzeria,{&TPizzeria});
        for(int i = 0; i < 4; i++)
        {
            DSSkyScraper[i].init(this,&DSLSkyScraper[i],{&TSkyScraper[i]});
        }
        for(int i = 0; i < 4; i++)
        {
            DSTree[i].init(this,&DSLTree[i],{&TTree[i]});
        }
        DSLampPost.init(this,&DSLLampPost,{&TLampPost});
        DSskyBox.init(this, &DSLskyBox, {&TskyBox});
        DSsun.init(this, &DSLEmission, {&Tsun});

        DSGlobal.init(this, &DSLGlobal, {});
    }

    // Here you destroy your pipelines and Descriptor Sets!
    // All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
    void pipelinesAndDescriptorSetsCleanup() {
// **A10** Add the pipeline cleanup
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

        DSGlobal.cleanup();

// **A10** Add the descriptor set cleanup
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

        TPizzeria.cleanup();
        MPizzeria.cleanup();

        for(int i = 0; i < 4; i++)
        {
            TSkyScraper[i].cleanup();
            MSkyScraper[i].cleanup();
        }

        for(int i = 0; i < 4; i++)
        {
            TTree[i].cleanup();
            MTree[i].cleanup();
        }

        TLampPost.cleanup();
        MLampPost.cleanup();

        TskyBox.cleanup();
        MskyBox.cleanup();

        Tsun.cleanup();
        Msun.cleanup();

        DSLGlobal.cleanup();

// **A10** Add the cleanup for the descriptor set layout
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

        // Cleanup of the pipelines
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

        for(int i = 0; i < 4; i++)
        {
            // 3. Binding del pipeline e del modello per gli SkyScraper
            PSkyScraper[i].bind(commandBuffer);           // Pipeline for the SkyScraper1
            MSkyScraper[i].bind(commandBuffer);           // Model for the SkyScraper1

        // Binding dei descriptor set globali e quelli specifici dello SkyScraper1
            DSGlobal.bind(commandBuffer, PSkyScraper[i], 0, currentImage);    // Descriptor Set globale per lo SkyScraper1
            DSSkyScraper[i].bind(commandBuffer, PSkyScraper[i], 1, currentImage);      // Descriptor Set per lo SkyScraper1

        // Comando di disegno per lo SkyScraper1
            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(MSkyScraper[i].indices.size()), NSKYSCRAPER, 0, 0, 0);
        }

        for(int i = 0; i < 4; i++)
        {
            // 4. Binding del pipeline e del modello per i tree
            PTree[i].bind(commandBuffer);           // Pipeline for the SkyScraper1
            MTree[i].bind(commandBuffer);           // Model for the SkyScraper1

            // Binding dei descriptor set globali e quelli specifici dello SkyScraper1
            DSGlobal.bind(commandBuffer, PTree[i], 0, currentImage);    // Descriptor Set globale per lo SkyScraper1
            DSTree[i].bind(commandBuffer, PTree[i], 1, currentImage);      // Descriptor Set per lo SkyScraper1

            // Comando di disegno per lo SkyScraper1
            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(MTree[i].indices.size()), NTREE, 0, 0, 0);
        }

// 11. Binding del pipeline e del modello per il LampPost
        PLampPost.bind(commandBuffer);                // Pipeline per la città
        MLampPost.bind(commandBuffer);                // Modello della città

// Binding dei descriptor set globali e quelli specifici del LampPost
        DSGlobal.bind(commandBuffer, PLampPost, 0, currentImage);    // Descriptor Set globale per il LampPost
        DSLampPost.bind(commandBuffer, PLampPost, 1, currentImage);      // Descriptor Set per il LampPost

// Comando di disegno per il LampPost
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MLampPost.indices.size()), NLAMPPOST, 0, 0, 0);

// 12. Binding del pipeline e del modello per lo SkyBox
        PskyBox.bind(commandBuffer);
        MskyBox.bind(commandBuffer);
        DSskyBox.bind(commandBuffer, PskyBox, 0, currentImage);
        // Disegna il cielo
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MskyBox.indices.size()), 1, 0, 0, 0);

// 13. Binding del pipeline e del modello per il sole
        PEmission.bind(commandBuffer);
        Msun.bind(commandBuffer);
        DSsun.bind(commandBuffer, PEmission, 0, currentImage);
        // Disegna il sole
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(Msun.indices.size()), 1, 0, 0, 0);

// 14. Binding del pipeline e del modello per la pizzeria
        PPizzeria.bind(commandBuffer);                // Pipeline per la città
        MPizzeria.bind(commandBuffer);                // Modello della città

// Binding dei descriptor set globali e quelli specifici della città
        DSGlobal.bind(commandBuffer, PPizzeria, 0, currentImage);    // Descriptor Set globale per la città
        DSPizzeria.bind(commandBuffer, PPizzeria, 1, currentImage);      // Descriptor Set per la città

// Comando di disegno per la città
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MPizzeria.indices.size()), 1, 0, 0, 0);
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

        glm::mat4 baseTr = glm::mat4(1.0f);

        // updates global uniforms
        // Global
        GlobalUniformBufferObject gubo{};

        gubo.lightDir = glm::vec3(-1.0f, 1.0f, 1.0f);
        gubo.lightColor = glm::vec4(1.0f);
        gubo.eyePos = glm::vec3(glm::inverse(ViewMatrix) * glm::vec4(0, 3, 0, 1));
        DSGlobal.map(currentImage, &gubo, 0);

        EmissionUniformBufferObject emissionUbo{};
        emissionUbo.mvpMat = ViewPrj * glm::translate(glm::mat4(1), gubo.lightDir * 90.0f) * baseTr;
        emissionUbo.mvpMat = glm::scale(emissionUbo.mvpMat, glm::vec3(3.0f, 3.0f, 3.0f));
        DSsun.map(currentImage, &emissionUbo, 0);

// **A10** Add to compute the uniforms and pass them to the shaders. You need two uniforms: one for the matrices, and the other for the material parameters.
        SingleObjectMatricesUniformBufferObject ScooterUbo{};
        ObjectParametersUniformBufferObject ScooterParUbo{};

        SingleObjectMatricesUniformBufferObject CityUbo{};
        ObjectParametersUniformBufferObject CityParUbo{};

        SingleObjectMatricesUniformBufferObject PizzeriaUbo{};
        ObjectParametersUniformBufferObject PizzeriaParUbo{};

        SkyScraperMatricesUniformBufferObject skyScraperUbos[4]{};
        ObjectParametersUniformBufferObject skyScraperParamUbos[4]{};

        TreeMatricesUniformBufferObject treeUbos[4]{};               // Array di UBO per gli alberi
        ObjectParametersUniformBufferObject treeParamUbos[4]{};     // Array di parametri per gli alberi

        LampPostMatricesUniformBufferObject LampPostUbo{};
        ObjectParametersUniformBufferObject LampPostParUbo{};

        skyBoxUniformBufferObject SkyBoxUbo{};

        // World and normal matrix should be the identiy. The World-View-Projection should be variable ViewPrj
        
        ScooterUbo.mMat = Wm * glm::mat4(1.0f);
		ScooterUbo.mvpMat = ViewPrj * ScooterUbo.mMat;
		ScooterUbo.nMat = glm::inverse(glm::transpose(ScooterUbo.mMat));

        CityUbo.mMat = glm::mat4(1.0f);
        CityUbo.nMat = glm::mat4(1.0f);
        CityUbo.mvpMat = ViewPrj;


        // Aggiorna la matrice del modello per la pizzeria con fattore di scala 1.8 e rotazione di 45 gradi
        PizzeriaUbo.mMat = glm::translate(glm::mat4(1.0f),
                                          glm::vec3(84.0 - (24.0 * (28 % 8)), 0.0, 84 - (24 * (28 / 8))));
        PizzeriaUbo.mMat = glm::rotate(PizzeriaUbo.mMat, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        PizzeriaUbo.mMat = glm::scale(PizzeriaUbo.mMat, glm::vec3(1.8f, 1.8f, 1.8f));

        // Calcola la matrice MVP per la pizzeria
        PizzeriaUbo.mvpMat = ViewPrj * PizzeriaUbo.mMat;

        // Calcola la matrice normale per la pizzeria
        PizzeriaUbo.nMat = glm::inverse(glm::transpose(PizzeriaUbo.mMat));




        int currentIndex = -1;
        int counterSkyScraper[4] = {0, 0, 0, 0}; // Array di contatori

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

            // Escludi la tile della pizzeria
            if (i == 28) {
                currentIndex = -1;
            }

            if (currentIndex != -1) {
                // Aggiorna la matrice del modello per il grattacielo corrente
                skyScraperUbos[currentIndex].mMat[counterSkyScraper[currentIndex]] = glm::translate(glm::mat4(1.0f),
                                                                                                    glm::vec3(84.0 - (24.0 * (i % 8)), 0.0, 84 - (24 * (i / 8))));

                skyScraperUbos[currentIndex].mMat[counterSkyScraper[currentIndex]] = glm::rotate(
                        skyScraperUbos[currentIndex].mMat[counterSkyScraper[currentIndex]],
                        glm::radians(90.0f * (float)(i % 4)), // Angolo in gradi
                        glm::vec3(0.0f, 1.0f, 0.0f));         // Asse Y

                // Calcola la matrice MVP per il grattacielo corrente
                skyScraperUbos[currentIndex].mvpMat[counterSkyScraper[currentIndex]] =
                        ViewPrj * skyScraperUbos[currentIndex].mMat[counterSkyScraper[currentIndex]];

                // Calcola la matrice normale per il grattacielo corrente
                skyScraperUbos[currentIndex].nMat[counterSkyScraper[currentIndex]] =
                        glm::inverse(glm::transpose(skyScraperUbos[currentIndex].mMat[counterSkyScraper[currentIndex]]));

                // Incrementa il contatore
                counterSkyScraper[currentIndex]++;
            }
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


        currentIndex = -1;
        int offset = 6;
        int counterTree[4] = {0, 0, 0, 0};                          // Contatori per ciascun albero
        int* indicesTree[4] = {indicesTree1, indicesTree2, indicesTree3, indicesTree4}; // Indici

        for (int i = 0; i < 64; ++i) {
            currentIndex = -1;

            // Determina l'indice dell'albero corrente
            for (int j = 0; j < 4; ++j) {
                for (int k = 0; k < 16; ++k) {
                    if (indicesTree[j][k] == i) {
                        currentIndex = j;
                        break;
                    }
                }
                if (currentIndex != -1) break;
            }

            // Escludi la tile della pizzeria
            if (i == 28) {
                currentIndex = -1;
            }

            // Aggiorna le matrici dell'albero corrente, applicando la scala solo per Tree1 e Tree4
            if (currentIndex != -1) {
                bool applyScale = (currentIndex == 0 || currentIndex == 3); // Scala solo per Tree1 e Tree4
                updateTreeMatrices(&treeUbos[currentIndex], counterTree[currentIndex], i, offset, ViewPrj, applyScale);
            }
        }


        float SkyBox_scale_factor = 98.0f;

        // Traslazione verso l'origine per centrare la SkyBox
        glm::vec3 skybox_center_offset = glm::vec3(0, 3, -5); // Sostituisci con i valori corretti per centrare la SkyBox

        SkyBoxUbo.mvpMat = M * glm::mat4(glm::mat3(ViewMatrix))
                           * glm::translate(glm::mat4(1.0f), skybox_center_offset) // Traslazione
                           * glm::scale(glm::mat4(1.0f), glm::vec3(SkyBox_scale_factor, SkyBox_scale_factor, SkyBox_scale_factor)); // Scala

        // These informations should be used to fill the Uniform Buffer Object in Binding 0 of your DSL
        DSScooter.map(currentImage, &ScooterUbo, 0);

        DSCity.map(currentImage, &CityUbo, 0);

        DSPizzeria.map(currentImage, &PizzeriaUbo, 0);

        for(int i = 0; i < 4; i++)
        {
            DSSkyScraper[i].map(currentImage, &skyScraperUbos[i], 0);
        }

        for(int i = 0; i < 4; i++)
        {
            DSTree[i].map(currentImage, &treeUbos[i], 0);
        }

        DSLampPost.map(currentImage, &LampPostUbo, 0);

        DSskyBox.map(currentImage, &SkyBoxUbo, 0);

        // The specular power of the uniform buffer containing the material parameters of the new object should be set to:
        // XXX.Power = 200.0
        // Where you replace XXX.Power with the field of the local variable corresponding to the uniform buffer object
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

        // The textre angle parameter of the uniform buffer containing the material parameters of the new object shoud be set to: tTime * TangTurnTimeFact
        // XXX.Ang = tTime * TangTurnTimeFact;
        // Where you replace XXX.Ang with the local field of the variable corresponding to the uniform buffer object
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

        // These informations should be used to fill the Uniform Buffer Object in Binding 6 of your DSL
        DSScooter.map(currentImage, &ScooterParUbo, 7);

        DSCity.map(currentImage, &CityParUbo, 2);

        DSPizzeria.map(currentImage, &PizzeriaParUbo, 2);

        for(int i = 0; i < 4; i++)
        {
            DSSkyScraper[i].map(currentImage, &skyScraperParamUbos[i], 2);
        }

        for(int i = 0; i < 4; i++)
        {
            DSTree[i].map(currentImage, &treeParamUbos[i], 2);
        }

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


