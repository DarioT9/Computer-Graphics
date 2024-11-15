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

        std::cout << "Model: " << offsets.name
                  << ", Vertex Offset: " << offsets.vertexOffset
                  << ", Index Offset: " << offsets.indexOffset
                  << ", Index Count: " << offsets.indexCount << std::endl;
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


// **A10** Place here the variable for the VertexDescriptor
    VertexDescriptor VDScooter;
    VertexDescriptor VDScooterFWheel;
    VertexDescriptor VDCity;

// **A10** Place here the variable for the Pipeline
    Pipeline PScooter;
    Pipeline PCity;

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
    Texture TScooterBaseColor, TScooterNormal, TScooterHeight, TScooterMetallic, TScooterRoughness, TScooterAmbientOcclusion;
    Texture TCity;
    DescriptorSet DSScooter;
    DescriptorSet DSScooterFWheel;
    DescriptorSet DSCity;

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
        DSLScooterFWheel.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(ScooterFWheelMatricesUniformBufferObject), 1},  // Matrice uniforme del modello scooter
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},  // Texture Base Color
                {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},  // Texture Normal
                {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1},  // Texture Height
                {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1},  // Texture Metallic
                {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1},  // Texture Roughness
                {6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5, 1},  // Texture Ambient Occlusion
                {7, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ScooterFWheelParametersUniformBufferObject), 1},
        });
        DSLCity.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(CityMatricesUniformBufferObject), 1},  // Matrice uniforme del modello scooter
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},  // Texture Base Color
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(CityParametersUniformBufferObject), 1},
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

        VDScooterFWheel.init(this, {
                {0, sizeof(ScooterFWheelVertex), VK_VERTEX_INPUT_RATE_VERTEX}  // Descrive la dimensione del vertice e la frequenza di input
        }, {
                               // Descrizione degli attributi dei vertici
                               {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ScooterFWheelVertex, pos), sizeof(glm::vec3), POSITION},  // Posizione (3 componenti float)
                               {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ScooterFWheelVertex, norm), sizeof(glm::vec3), NORMAL},    // Normale (3 componenti float)
                               {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(ScooterFWheelVertex, UV), sizeof(glm::vec2), UV},             // UV (2 componenti float)
                               {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ScooterFWheelVertex, tan), sizeof(glm::vec4), TANGENT}  // Tangente (4 componenti float)
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

// **A10** Place here the initialization of the pipeline. Remember that it should use shaders in files
//		"shaders/NormalMapVert.spv" and "shaders/NormalMapFrag.spv", it should receive the new VertexDescriptor you defined
//		And should receive two DescriptorSetLayout, the first should be DSLGlobal, while the other must be the one you defined
        PScooter.init(this, &VDScooter, "shaders/NormalMapVert.spv", "shaders/NormalMapFrag.spv", {&DSLGlobal, &DSLScooter, &DSLScooterFWheel});
        PCity.init(this, &VDCity, "shaders/NormalMapVert.spv", "shaders/CityFrag.spv", {&DSLGlobal, &DSLCity});

// **A10** Place here the loading of the model. It should be contained in file "models/Sphere.gltf", it should use the
//		Vertex descriptor you defined, and be of GLTF format.
        MScooter.init(this, &VDScooter, "models/Scooter.obj", OBJ);
        MFWheel.init(this, &VDScooterFWheel, "models/Scooter.obj",OBJ);
        MCity.init(this, &VDCity, "models/road.obj", OBJ);

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


        // Descriptor pool sizes
        // WARNING!!!!!!!!
        // Must be set before initializing the text and the scene
// **A10** Update the number of elements to correctly size the descriptor sets pool
        DPSZs.uniformBlocksInPool = 1 + 2 + 2 + 2; // ScooterMatrixUniformBufferObject and ScooterShaderParametersUniformBufferObject, same for City and front wheel
        DPSZs.texturesInPool = 6 + 1; // Textures (TScooterBaseColor, TScooterNormal, TScooterHeight, TScooterMetallic, TScooterRoughness, TScooterAmbientOcclusion) + 1 for city
        DPSZs.setsInPool = 1 + 1 + 1 + 1; // DSScooter and DSCity and DSScooterFWheel

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

// **A10** Add the descriptor set creation

//        Texture TScooterAmbientOcclusion, TScooterBaseColor, TScooterNormal, TScooterHeight, TScooterMetallic, TScooterRoughness;
        DSScooter.init(this,&DSLScooter,{&TScooterBaseColor, &TScooterNormal, &TScooterHeight, &TScooterMetallic, &TScooterRoughness, &TScooterAmbientOcclusion});
        DSScooterFWheel.init(this,&DSLScooterFWheel,{&TScooterBaseColor, &TScooterNormal, &TScooterHeight, &TScooterMetallic, &TScooterRoughness, &TScooterAmbientOcclusion});
        DSCity.init(this,&DSLCity,{&TCity});


        DSGlobal.init(this, &DSLGlobal, {});
    }

    // Here you destroy your pipelines and Descriptor Sets!
    // All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
    void pipelinesAndDescriptorSetsCleanup() {
// **A10** Add the pipeline cleanup
        PScooter.cleanup();
        PCity.cleanup();

        DSGlobal.cleanup();

// **A10** Add the descriptor set cleanup
        DSScooter.cleanup();
        DSScooterFWheel.cleanup();
        DSCity.cleanup();
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
        MFWheel.cleanup();

        TCity.cleanup();
        MCity.cleanup();

        DSLGlobal.cleanup();

// **A10** Add the cleanup for the descriptor set layout
        DSLScooter.cleanup();
        DSLScooterFWheel.cleanup();
        DSLCity.cleanup();

// **A10** Add the cleanup for the pipeline
        PScooter.destroy();
        PCity.destroy();

    }

    // Here it is the creation of the command buffer:
    // You send to the GPU all the objects you want to draw,
    // with their buffers and textures

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

// **A10** Add the commands to bind the pipeline, the mesh its two descriptor setes, and the draw call of the planet
        
        std::vector<ModelOffsets> ScooterOffsets = calculateOffsets("models/Scooter.obj");
        uint32_t FWheelSize = ScooterOffsets[3].indexCount;
        uint32_t FWheelIndexOffset = ScooterOffsets[3].indexOffset;
        uint32_t FWheelVertexOffset = ScooterOffsets[3].vertexOffset;

        std::cout << "Troubleshoot: " << ScooterOffsets[3].indexCount;

        PScooter.bind(commandBuffer);
        MScooter.bind(commandBuffer);
        
        DSGlobal.bind(commandBuffer, PScooter, 0, currentImage);
        DSScooter.bind(commandBuffer, PScooter, 1, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MScooter.indices.size()), 1, 0, 0, 0);
        
        MFWheel.bind(commandBuffer);
        DSScooterFWheel.bind(commandBuffer, PScooter, 2, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         FWheelSize, 1, FWheelIndexOffset, FWheelVertexOffset, 0);


// 2. Binding del pipeline e del modello per la città
        PCity.bind(commandBuffer);                // Pipeline per la città
        MCity.bind(commandBuffer);                // Modello della città

// Binding dei descriptor set globali e quelli specifici della città
        DSGlobal.bind(commandBuffer, PCity, 0, currentImage);    // Descriptor Set globale per la città
        DSCity.bind(commandBuffer, PCity, 1, currentImage);      // Descriptor Set per la città

// Comando di disegno per la città
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MCity.indices.size()), 1, 0, 0, 0);
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


        //tentativo
        const float STEERING_SPEED = glm::radians(30.0f);
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

        glm::mat4 Wm = glm::translate(glm::mat4(1), ScooterPos) * glm::rotate(glm::mat4(1), ang + Yaw, glm::vec3(0,1,0))
                                                         * glm::rotate(glm::mat4(1), ang - SteeringAng, glm::vec3(0,0,1));

        glm::mat4 FWm = glm::translate(glm::mat4(1), ScooterPos) * glm::rotate(glm::mat4(1), ang + Yaw*3.0f, glm::vec3(0,1,0));

        // The Fly model update proc.
        if((currScene == 0)) {
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
		}
        // Here is where you actually update your uniforms

        glm::mat4 Mv = ViewMatrix;

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

        ScooterFWheelMatricesUniformBufferObject FWUbo{};
        ScooterFWheelParametersUniformBufferObject FWParUbo{};

        CityMatricesUniformBufferObject CityUbo{};
        CityParametersUniformBufferObject CityParUbo{};

        // World and normal matrix should be the identiy. The World-View-Projection should be variable ViewPrj
        
        ScooterUbo.mMat = Wm * glm::mat4(1.0f);
		ScooterUbo.mvpMat = ViewPrj * ScooterUbo.mMat;
		ScooterUbo.nMat = glm::inverse(glm::transpose(ScooterUbo.mMat));

        FWUbo.mMat = FWm * glm::mat4(1.0f);
		FWUbo.mvpMat = ViewPrj * FWUbo.mMat;
		FWUbo.nMat = glm::inverse(glm::transpose(FWUbo.mMat));

        CityUbo.mMat = glm::mat4(1.0f);
        CityUbo.nMat = glm::mat4(1.0f);
        CityUbo.mvpMat = ViewPrj;

        // These informations should be used to fill the Uniform Buffer Object in Binding 0 of your DSL
        DSScooter.map(currentImage, &ScooterUbo, 0);

        DSScooterFWheel.map(currentImage, &FWUbo, 0);

        DSCity.map(currentImage, &CityUbo, 0);

        // The specular power of the uniform buffer containing the material parameters of the new object should be set to:
        // XXX.Power = 200.0
        // Where you replace XXX.Power with the field of the local variable corresponding to the uniform buffer object
        ScooterParUbo.Pow = 200.0f;

        FWParUbo.Pow = 200.0f;

        CityParUbo.Pow = 200.0f;

        // The textre angle parameter of the uniform buffer containing the material parameters of the new object shoud be set to: tTime * TangTurnTimeFact
        // XXX.Ang = tTime * TangTurnTimeFact;
        // Where you replace XXX.Ang with the local field of the variable corresponding to the uniform buffer object
        ScooterParUbo.Ang = 0.0f;

        FWParUbo.Ang = 0.0f;

        CityParUbo.Ang = 0.0f;

        // These informations should be used to fill the Uniform Buffer Object in Binding 6 of your DSL
        DSScooter.map(currentImage, &ScooterParUbo, 7);

        DSScooterFWheel.map(currentImage, &FWParUbo, 7);

        DSCity.map(currentImage, &CityParUbo, 2);

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

