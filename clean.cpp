// This has been adapted from the Vulkan tutorial

#include "modules/Starter.hpp"

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

// **A10** Place here the CPP struct for the uniform buffer for the parameters
struct ScooterParametersUniformBufferObject {
    alignas(4) float Pow;
    alignas(4) float Ang;
};

// **A10** Place here the CPP struct for the vertex definition
struct ScooterVertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 UV;
    glm::vec4 tan;
};


// MAIN !
class A10 : public BaseProject {
protected:


    // Descriptor Layouts ["classes" of what will be passed to the shaders]
    DescriptorSetLayout DSLGlobal;	// For Global values

// **A10** Place here the variable for the DescriptorSetLayout
    DescriptorSetLayout DSLScooter;

// **A10** Place here the variable for the VertexDescriptor
    VertexDescriptor VDScooter;

// **A10** Place here the variable for the Pipeline
    Pipeline PScooter;

/*
	// Scenes and texts
    TextMaker txt;
*/

    // Models, textures and Descriptor Sets (values assigned to the uniforms)
    DescriptorSet DSGlobal;

// **A10** Place here the variables for the Model, the five texture (diffuse, specular, normal map, emission and clouds) and the Descrptor Set
    Model MScooter;
    Texture TScooterBaseColor, TScooterNormal, TScooterHeight, TScooterMetallic, TScooterRoughness, TScooterAmbientOcclusion;
    DescriptorSet DSScooter;

    // Other application parameters
    int currScene = 0;
    int subpass = 0;

    glm::vec3 CamPos = glm::vec3(0.0, 0.1, 5.0);
    glm::mat4 ViewMatrix;

    float Ar;

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

// **A10** Place here the initialization of the pipeline. Remember that it should use shaders in files
//		"shaders/NormalMapVert.spv" and "shaders/NormalMapFrag.spv", it should receive the new VertexDescriptor you defined
//		And should receive two DescriptorSetLayout, the first should be DSLGlobal, while the other must be the one you defined
        PScooter.init(this, &VDScooter, "shaders/NormalMapVert.spv", "shaders/NormalMapFrag.spv", {&DSLGlobal, &DSLScooter});

// **A10** Place here the loading of the model. It should be contained in file "models/Sphere.gltf", it should use the
//		Vertex descriptor you defined, and be of GLTF format.
        MScooter.init(this, &VDScooter, "models/Scooter.obj", OBJ);

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


        // Descriptor pool sizes
        // WARNING!!!!!!!!
        // Must be set before initializing the text and the scene
// **A10** Update the number of elements to correctly size the descriptor sets pool
        DPSZs.uniformBlocksInPool = 1 + 2; // ScooterMatrixUniformBufferObject and ScooterShaderParametersUniformBufferObject
        DPSZs.texturesInPool = 6; // Textures (TScooterBaseColor, TScooterNormal, TScooterHeight, TScooterMetallic, TScooterRoughness, TScooterAmbientOcclusion)
        DPSZs.setsInPool = 1 + 1; // DSScooter

        std::cout << "Initialization completed!\n";
        std::cout << "Uniform Blocks in the Pool  : " << DPSZs.uniformBlocksInPool << "\n";
        std::cout << "Textures in the Pool        : " << DPSZs.texturesInPool << "\n";
        std::cout << "Descriptor Sets in the Pool : " << DPSZs.setsInPool << "\n";

        ViewMatrix = glm::translate(glm::mat4(1), -CamPos);
    }

    // Here you create your pipelines and Descriptor Sets!
    void pipelinesAndDescriptorSetsInit() {
// **A10** Add the pipeline creation
        PScooter.create();

// **A10** Add the descriptor set creation

//        Texture TScooterAmbientOcclusion, TScooterBaseColor, TScooterNormal, TScooterHeight, TScooterMetallic, TScooterRoughness;
        DSScooter.init(this,&DSLScooter,{&TScooterBaseColor, &TScooterNormal, &TScooterHeight, &TScooterMetallic, &TScooterRoughness, &TScooterAmbientOcclusion});


        DSGlobal.init(this, &DSLGlobal, {});
    }

    // Here you destroy your pipelines and Descriptor Sets!
    // All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
    void pipelinesAndDescriptorSetsCleanup() {
// **A10** Add the pipeline cleanup
        PScooter.cleanup();

        DSGlobal.cleanup();

// **A10** Add the descriptor set cleanup
        DSScooter.cleanup();
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
        DSLGlobal.cleanup();

// **A10** Add the cleanup for the descriptor set layout
        DSLScooter.cleanup();

// **A10** Add the cleanup for the pipeline
        PScooter.destroy();

    }

    // Here it is the creation of the command buffer:
    // You send to the GPU all the objects you want to draw,
    // with their buffers and textures

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

// **A10** Add the commands to bind the pipeline, the mesh its two descriptor setes, and the draw call of the planet
        PScooter.bind(commandBuffer);
        MScooter.bind(commandBuffer);
        DSGlobal.bind(commandBuffer, PScooter, 0, currentImage);
        DSScooter.bind(commandBuffer, PScooter, 1, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MScooter.indices.size()), 1, 0, 0, 0);


    }

    // Here is where you update the uniforms.
    // Very likely this will be where you will be writing the logic of your application.
    void updateUniformBuffer(uint32_t currentImage) {

        float deltaT;
        glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
        bool fire = false;
        getSixAxis(deltaT, m, r, fire);


        const float ROT_SPEED = glm::radians(120.0f);
        const float MOVE_SPEED = 2.0f;


        // The Fly model update proc.
        ViewMatrix = glm::rotate(glm::mat4(1), ROT_SPEED * r.x * deltaT,
                                 glm::vec3(1, 0, 0)) * ViewMatrix;
        ViewMatrix = glm::rotate(glm::mat4(1), ROT_SPEED * r.y * deltaT,
                                 glm::vec3(0, 1, 0)) * ViewMatrix;
        ViewMatrix = glm::rotate(glm::mat4(1), -ROT_SPEED * r.z * deltaT,
                                 glm::vec3(0, 0, 1)) * ViewMatrix;
        ViewMatrix = glm::translate(glm::mat4(1), -glm::vec3(
                MOVE_SPEED * m.x * deltaT, MOVE_SPEED * m.y * deltaT, MOVE_SPEED * m.z * deltaT))
                     * ViewMatrix;

        // Here is where you actually update your uniforms
        glm::mat4 M = glm::perspective(glm::radians(45.0f), Ar, 0.1f, 160.0f);
        M[1][1] *= -1;

        glm::mat4 Mv = ViewMatrix;

        glm::mat4 ViewPrj =  M * Mv;

        // updates global uniforms
        // Global
        GlobalUniformBufferObject gubo{};

        gubo.lightDir = glm::vec3(0.0f, 1.0f, 0.0f);
        gubo.lightColor = glm::vec4(1.0f);
        gubo.eyePos = glm::vec3(glm::inverse(ViewMatrix) * glm::vec4(0, 0, 0, 1));
        DSGlobal.map(currentImage, &gubo, 0);


// **A10** Add to compute the uniforms and pass them to the shaders. You need two uniforms: one for the matrices, and the other for the material parameters.
        ScooterMatricesUniformBufferObject ScooterUbo{};
        ScooterParametersUniformBufferObject ScooterParUbo{};

        // World and normal matrix should be the identiy. The World-View-Projection should be variable ViewPrj
        ScooterUbo.mMat = glm::mat4(1.0f);
        ScooterUbo.nMat = glm::mat4(1.0f);
        ScooterUbo.mvpMat = ViewPrj;

        // These informations should be used to fill the Uniform Buffer Object in Binding 0 of your DSL
        DSScooter.map(currentImage, &ScooterUbo, 0);

        // The specular power of the uniform buffer containing the material parameters of the new object should be set to:
        // XXX.Power = 200.0
        // Where you replace XXX.Power with the field of the local variable corresponding to the uniform buffer object
        ScooterParUbo.Pow = 200.0f;

        // The textre angle parameter of the uniform buffer containing the material parameters of the new object shoud be set to: tTime * TangTurnTimeFact
        // XXX.Ang = tTime * TangTurnTimeFact;
        // Where you replace XXX.Ang with the local field of the variable corresponding to the uniform buffer object
        ScooterParUbo.Ang = 0.0f;

        // These informations should be used to fill the Uniform Buffer Object in Binding 6 of your DSL
        DSScooter.map(currentImage, &ScooterParUbo, 7);
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
