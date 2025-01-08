#include "headers.h"
#include "trianglemesh.h"
#include "camera.h"
#include "shaderprog.h"
#include "light.h"
#include "imagetexture.h"
#include "skybox.h"


// Global variables.
int screenWidth = 600;
int screenHeight = 600;
// Triangle mesh.
TriangleMesh* mesh = nullptr;
// Lights.
DirectionalLight* dirLight = nullptr;
PointLight* pointLight = nullptr;
SpotLight* spotLight = nullptr;
glm::vec3 dirLightDirection = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 dirLightRadiance = glm::vec3(0.6f, 0.6f, 0.6f);
glm::vec3 pointLightPosition = glm::vec3(0.8f, 0.0f, 0.8f);
glm::vec3 pointLightIntensity = glm::vec3(0.5f, 0.1f, 0.1f);
glm::vec3 spotLightPosition = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 spotLightDirection = glm::vec3(0.0f, -1.0f, 0.0f);
glm::vec3 spotLightIntensity = glm::vec3(0.25f, 0.25f, 0.1f);
float spotLightCutoffStartInDegree = 30.0f;
float spotLightTotalWidthInDegree = 45.0f;
glm::vec3 ambientLight = glm::vec3(0.2f, 0.2f, 0.2f);
// Camera.
Camera* camera = nullptr;
glm::vec3 cameraPos = glm::vec3(0.0f, 1.0f, 5.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float fovy = 30.0f;
float zNear = 0.1f;
float zFar = 1000.0f;
// Shader.
FillColorShaderProg* fillColorShader = nullptr;
PhongShadingDemoShaderProg* phongShadingShader = nullptr;
SkyboxShaderProg* skyboxShader = nullptr;
// UI.
const float lightMoveSpeed = 0.2f;
// Shading Mode
int lightingMode = 0;
// Skybox.
Skybox* skybox = nullptr;
float rotationSpeed = 1.0f;


std::string modelFilePath = "../TestModels_HW3/TexCube";
std::string skyFilePath = "../TestTextures_HW3/photostudio_02_2k.png";

// SceneObject.
struct SceneObject
{
    SceneObject() {
        mesh = nullptr;
        worldMatrix = glm::mat4x4(1.0f);
    }
    TriangleMesh* mesh;
    glm::mat4x4 worldMatrix;
};
SceneObject sceneObj;

// ScenePointLight (for visualization of a point light).
struct ScenePointLight
{
    ScenePointLight() {
        light = nullptr;
        worldMatrix = glm::mat4x4(1.0f);
        visColor = glm::vec3(1.0f, 1.0f, 1.0f);
    }
    PointLight* light;
    glm::mat4x4 worldMatrix;
    glm::vec3 visColor;
};
ScenePointLight pointLightObj;
ScenePointLight spotLightObj;

// Function prototypes.
void ReleaseResources();
// Callback functions.
void RenderSceneCB();
void ReshapeCB(int, int);
void ProcessSpecialKeysCB(int, int, int);
void ProcessKeysCB(unsigned char, int, int);
void SetupRenderState();
void SetupScene(const std::string& modelFilePath);
void LoadObjects(const std::string&);
void CreateCamera();
void CreateSkybox(const std::string);
void CreateShaderLib();



void ReleaseResources()
{
    std::cout << "Resource Releasing..." << std::endl;
    // Delete scene objects and lights.
    if (mesh != nullptr) {
        delete mesh;
        mesh = nullptr;
    }
    if (pointLight != nullptr) {
        delete pointLight;
        pointLight = nullptr;
    }
    if (dirLight != nullptr) {
        delete dirLight;
        dirLight = nullptr;
    }
    if (spotLight != nullptr) {
        delete spotLight;
        spotLight = nullptr;
    }
    // Delete camera.
    if (camera != nullptr) {
        delete camera;
        camera = nullptr;
    }
    // Delete shaders.
    if (fillColorShader != nullptr) {
        delete fillColorShader;
        fillColorShader = nullptr;
    }
    if (phongShadingShader != nullptr) {
        delete phongShadingShader;
        phongShadingShader = nullptr;
    }
    if (skyboxShader != nullptr) {
        delete skyboxShader;
        skyboxShader = nullptr;
    }
    std::cout << "Resource Releasing Finished" << std::endl;
}

static float curObjRotationY = 30.0f;
const float rotStep = 0.02f;
void RenderSceneCB()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    TriangleMesh* pMesh = sceneObj.mesh;
    if (pMesh != nullptr) {
        // Update transform.
        //curObjRotationY += rotStep;
        glm::mat4x4 S = glm::scale(glm::mat4x4(1.0f), glm::vec3(1.5f, 1.5f, 1.5f));
        glm::mat4x4 R = glm::rotate(glm::mat4x4(1.0f), glm::radians(curObjRotationY), glm::vec3(0, 1, 0));
        sceneObj.worldMatrix = S * R;
        // -------------------------------------------------------
		// Note: if you want to compute lighting in the View Space, 
        //       you might need to change the code below.
		// -------------------------------------------------------
        glm::mat4x4 normalMatrix = glm::transpose(glm::inverse(camera->GetViewMatrix() * sceneObj.worldMatrix));
        glm::mat4x4 MVP = camera->GetProjMatrix() * camera->GetViewMatrix() * sceneObj.worldMatrix;
        
        phongShadingShader->Bind();

        // Transformation Matrix
        glUniformMatrix4fv(phongShadingShader->GetLocM(), 1, GL_FALSE, glm::value_ptr(sceneObj.worldMatrix));
        glUniformMatrix4fv(phongShadingShader->GetLocNM(), 1, GL_FALSE, glm::value_ptr(normalMatrix));
        glUniformMatrix4fv(phongShadingShader->GetLocMVP(), 1, GL_FALSE, glm::value_ptr(MVP));

        // Set Camera Position
        glUniform3fv(phongShadingShader->GetLocCameraPos(), 1, glm::value_ptr(camera->GetCameraPos()));

        // Set Light data.
        // Directional Light
        if (dirLight != nullptr) {
            glUniform3fv(phongShadingShader->GetLocDirLightDir(), 1, glm::value_ptr(dirLight->GetDirection()));
            glUniform3fv(phongShadingShader->GetLocDirLightRadiance(), 1, glm::value_ptr(dirLight->GetRadiance()));
        }
        // Point Light
        if (pointLight != nullptr) {
            glUniform3fv(phongShadingShader->GetLocPointLightPos(), 1, glm::value_ptr(pointLight->GetPosition()));
            glUniform3fv(phongShadingShader->GetLocPointLightIntensity(), 1, glm::value_ptr(pointLight->GetIntensity()));
        }
        // Spot Light
        if (spotLight != nullptr) {
            glUniform3fv(phongShadingShader->GetLocSpotLightPos(), 1, glm::value_ptr(spotLight->GetPosition()));
            glUniform3fv(phongShadingShader->GetLocSpotLightIntensity(), 1, glm::value_ptr(spotLight->GetIntensity()));
            glUniform3fv(phongShadingShader->GetLocSpotLightDir(), 1, glm::value_ptr(spotLight->GetDirection()));
            glUniform1f(phongShadingShader->GetLocSpotLightTotalWidth(), spotLight->GetTotalWidthDegree());
            glUniform1f(phongShadingShader->GetLocSpotLightFoS(), spotLight->GetFallofStartDegree());
            glUniform1f(phongShadingShader->GetLocCosSpotLightTotalWidth(), spotLight->GetCosTotalWidthDegree());
            glUniform1f(phongShadingShader->GetLocCosSpotLightFoS(), spotLight->GetCosFallofStartDegree());
        }

        // Ambient Light
        glUniform3fv(phongShadingShader->GetLocAmbientLight(), 1, glm::value_ptr(ambientLight));

        // Lighting Mode
        glUniform1i(phongShadingShader->GetLocLightingMode(), lightingMode);

        for (auto& subMesh : mesh->GetSubMeshes()) {
            
            ImageTexture* imageData = subMesh.material->GetMapKd();
            // Bind Texture Data
            if (imageData != nullptr) {
                imageData->Bind(GL_TEXTURE0);
                glUniform1i(phongShadingShader->GetLocMapKd(), 0);
                glUniform1i(phongShadingShader->GetLocUseMapKd(), 1);
            }
            else glUniform1i(phongShadingShader->GetLocUseMapKd(), 0);


            // Set SubMesh Material Data
            // Material properties.
            glUniform3fv(phongShadingShader->GetLocKa(), 1, glm::value_ptr(subMesh.material->GetKa()));
            glUniform3fv(phongShadingShader->GetLocKd(), 1, glm::value_ptr(subMesh.material->GetKd()));
            glUniform3fv(phongShadingShader->GetLocKs(), 1, glm::value_ptr(subMesh.material->GetKs()));
            glUniform1f(phongShadingShader->GetLocNs(), subMesh.material->GetNs());


            mesh->Render(subMesh);
        }

        phongShadingShader->UnBind();
    }
    // -------------------------------------------------------------------------------------------

    // Visualize the light with fill color. ------------------------------------------------------
    PointLight* pointLight = pointLightObj.light;
    if (pointLight != nullptr) {
        glm::mat4x4 T = glm::translate(glm::mat4x4(1.0f), pointLight->GetPosition());
        pointLightObj.worldMatrix = T;
        glm::mat4x4 MVP = camera->GetProjMatrix() * camera->GetViewMatrix() * pointLightObj.worldMatrix;
        fillColorShader->Bind();
        glUniformMatrix4fv(fillColorShader->GetLocMVP(), 1, GL_FALSE, glm::value_ptr(MVP));
        glUniform3fv(fillColorShader->GetLocFillColor(), 1, glm::value_ptr(pointLightObj.visColor));
        // Render the point light.
        pointLight->Draw();
        fillColorShader->UnBind();
    }
    SpotLight* spotLight = (SpotLight*)(spotLightObj.light);
    if (spotLight != nullptr) {
        glm::mat4x4 T = glm::translate(glm::mat4x4(1.0f), spotLight->GetPosition());
        spotLightObj.worldMatrix = T;
        glm::mat4x4 MVP = camera->GetProjMatrix() * camera->GetViewMatrix() * spotLightObj.worldMatrix;
        fillColorShader->Bind();
        glUniformMatrix4fv(fillColorShader->GetLocMVP(), 1, GL_FALSE, glm::value_ptr(MVP));
        glUniform3fv(fillColorShader->GetLocFillColor(), 1, glm::value_ptr(spotLightObj.visColor));
        // Render the spot light.
        spotLight->Draw();
        fillColorShader->UnBind();
    }
    // -------------------------------------------------------------------------------------------

    // Render skybox. ----------------------------------------------------------------------------
    if (skybox != nullptr) {
        skybox->Render(camera, skyboxShader);
    }
    // -------------------------------------------------------------------------------------------

    glutSwapBuffers();
}

void ReshapeCB(int w, int h)
{
    // Update viewport.
    screenWidth = w;
    screenHeight = h;
    glViewport(0, 0, screenWidth, screenHeight);
    // Adjust camera and projection.
    float aspectRatio = (float)screenWidth / (float)screenHeight;
    camera->UpdateProjection(fovy, aspectRatio, zNear, zFar);
}

void ProcessSpecialKeysCB(int key, int x, int y)
{
    // Handle special (functional) keyboard inputs such as F1, spacebar, page up, etc. 
    switch (key) {
    // Rendering mode.
    case GLUT_KEY_F1:
        // Render with point mode.
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        break;
    case GLUT_KEY_F2:
        // Render with line mode.
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;
    case GLUT_KEY_F3:
        // Render with fill mode.
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    case GLUT_KEY_F4:
        // Chang Shading Mode
        lightingMode++;
        lightingMode = lightingMode % 4;
        break;
    
    // Light control.
    case GLUT_KEY_LEFT:
        if (pointLight != nullptr)
            pointLight->MoveLeft(lightMoveSpeed);
        break;
    case GLUT_KEY_RIGHT:
        if (pointLight != nullptr)
            pointLight->MoveRight(lightMoveSpeed);
        break;
    case GLUT_KEY_UP:
        if (pointLight != nullptr)
            pointLight->MoveUp(lightMoveSpeed);
        break;
    case GLUT_KEY_DOWN:
        if (pointLight != nullptr)
            pointLight->MoveDown(lightMoveSpeed);
        break;
    default:
        break;
    }
}

void ProcessKeysCB(unsigned char key, int x, int y)
{
    // Handle other keyboard inputs those are not defined as special keys.
    if (key == 27) {
        // Release memory allocation if needed.
        ReleaseResources();
        exit(0);
    }
    // Spot light control.
    if (spotLight != nullptr) {
        if (key == 'a')
            spotLight->MoveLeft(lightMoveSpeed);
        if (key == 'd')
            spotLight->MoveRight(lightMoveSpeed);
        if (key == 'w')
            spotLight->MoveUp(lightMoveSpeed);
        if (key == 's')
            spotLight->MoveDown(lightMoveSpeed);
    }

    // Skybox rotate control
    if (skybox != nullptr) {
        if (key == 'z') {
            skybox->RotateLeft(rotationSpeed);
        }
        if (key == 'c') {
            skybox->RotateRight(rotationSpeed);

        }
    }
}

void SetupRenderState()
{
    glEnable(GL_DEPTH_TEST);

    glm::vec4 clearColor = glm::vec4(0.44f, 0.57f, 0.75f, 1.00f);
    glClearColor(
        (GLclampf)(clearColor.r), 
        (GLclampf)(clearColor.g), 
        (GLclampf)(clearColor.b), 
        (GLclampf)(clearColor.a)
    );
}

void LoadObjects(const std::string& modelPath)
{
    mesh = new TriangleMesh();
    mesh->LoadFromFile(modelPath, true);
    mesh->ShowInfo();
    sceneObj.mesh = mesh;    
    mesh->CreateBuffer();
}

void CreateLights()
{
    // Create a directional light.
    dirLight = new DirectionalLight(dirLightDirection, dirLightRadiance);
    // Create a point light.
    pointLight = new PointLight(pointLightPosition, pointLightIntensity);
    pointLightObj.light = pointLight;
    pointLightObj.visColor = glm::normalize((pointLightObj.light)->GetIntensity());
    // Create a spot light.
    spotLight = new SpotLight(spotLightPosition, spotLightIntensity, spotLightDirection, 
            spotLightCutoffStartInDegree, spotLightTotalWidthInDegree);
    spotLightObj.light = spotLight;
    spotLightObj.visColor = glm::normalize((spotLightObj.light)->GetIntensity());
}

void CreateCamera()
{
    // Create a camera and update view and proj matrices.
    camera = new Camera((float)screenWidth / (float)screenHeight);
    camera->UpdateView(cameraPos, cameraTarget, cameraUp);
    float aspectRatio = (float)screenWidth / (float)screenHeight;
    camera->UpdateProjection(fovy, aspectRatio, zNear, zFar);
}

void CreateSkybox(const std::string texFilePath)
{
    const int numSlices = 36;
    const int numStacks = 18;
    const float radius = 50.0f;
    skybox = new Skybox(texFilePath, numSlices, numStacks, radius);
}

void CreateShaderLib()
{
    fillColorShader = new FillColorShaderProg();
    if (!fillColorShader->LoadFromFiles("shaders/fixed_color.vs", "shaders/fixed_color.fs"))
        exit(1);

    phongShadingShader = new PhongShadingDemoShaderProg();
    if (!phongShadingShader->LoadFromFiles("shaders/phong_shading_demo.vs", "shaders/phong_shading_demo.fs"))
        exit(1);

    skyboxShader = new SkyboxShaderProg();
    if (!skyboxShader->LoadFromFiles("shaders/skybox.vs", "shaders/skybox.fs"))
        exit(1);
}

//Obcjet Path Menu Dealing Function
void PathMenu(int index) {
    //ReleasResource First
    ReleaseResources();

    //Change File Path
    switch (index) {
    case 1:
        modelFilePath = "../TestModels_HW3/Ferrari";
        std::cout << "Select: Bunny Object" << std::endl;
        break;
    case 2:
        modelFilePath = "../TestModels_HW3/Forklift";
        std::cout << "Select: ColorCube Object" << std::endl;
        break;
    case 3:
        modelFilePath = "../TestModels_HW3/Gengar";
        std::cout << "Select: Forklift Object" << std::endl;
        break;
    case 4:
        modelFilePath = "../TestModels_HW3/Ivysaur";
        std::cout << "Select: Gengar Object" << std::endl;
        break;
    case 5:
        modelFilePath = "../TestModels_HW3/Koffing";
        std::cout << "Select: Koffing Object" << std::endl;
        break;
    case 6:
        modelFilePath = "../TestModels_HW3/MagikarpF";
        std::cout << "Select: Ivysaur Object" << std::endl;
        break;
    case 7:
        modelFilePath = "../TestModels_HW3/Rose";
        std::cout << "Select: Pillows Object" << std::endl;
        break;
    case 8:
        modelFilePath = "../TestModels_HW3/Slowbro";
        std::cout << "Select: Rose Object" << std::endl;
        break;
    case 9:
        modelFilePath = "../TestModels_HW3/TexCube";
        std::cout << "Select: Soccer Object" << std::endl;
    default:
        break;
    }

    //Buile Up Selected Object Scene
    SetupRenderState();
    SetupScene(modelFilePath);
}

// Direction Menu Dealing Function
void DirectionMenu(int index) {
    // Chnage Direction
    switch (index) {
    case 1:
        dirLight->SetDirection(glm::vec3(0, 0, -1));
        break;
    case 2:
        dirLight->SetDirection(glm::vec3(-1, 0, -1));
        break;
    case 3:
        dirLight->SetDirection(glm::vec3(-1, 0, 0));
        break;
    case 4:
        dirLight->SetDirection(glm::vec3(-1, 0, 1));
        break;
    case 5:
        dirLight->SetDirection(glm::vec3(0, 0, 1));
        break;
    case 6:
        dirLight->SetDirection(glm::vec3(1, 0, 1));
        break;
    case 7:
        dirLight->SetDirection(glm::vec3(1, 0, 0));
        break;
    case 8:
        dirLight->SetDirection(glm::vec3(1, 0, -1));
        break;
    case 9:
        dirLight->SetDirection(glm::vec3(0, 1, 0));
        break;
    case 10:
        dirLight->SetDirection(glm::vec3(0, -1, 0));
        break;
    }
}

// Skybox Path Menu Dealing Function
void SkyboxPathMenu(int index) {
    if (skybox != nullptr) {
        delete skybox;
        skybox = nullptr;
    }

    switch (index) {
    case 1:
        skyFilePath = "../TestTextures_HW3/photostudio_02_2k.png";
        break;
    case 2:
        skyFilePath = "../TestTextures_HW3/sunflowers_2k.png";
        break;
    case 3:
        skyFilePath = "../TestTextures_HW3/veranda_2k.png";
        break;
    }
    CreateSkybox(skyFilePath);
}


//Create Path Menu Function
void createPathMenu() {
    int mainMenu = glutCreateMenu(PathMenu);

    glutAddMenuEntry("Ferrari Object", 1);
    glutAddMenuEntry("Forklift Object", 2);
    glutAddMenuEntry("Gengar Object", 3);
    glutAddMenuEntry("Ivysaur Object", 4);
    glutAddMenuEntry("Koffing Object", 5);
    glutAddMenuEntry("MagikarpF Object", 6);
    glutAddMenuEntry("Rose Object", 7);
    glutAddMenuEntry("Slowbro Object", 8);
    glutAddMenuEntry("TexCube Object", 9);

    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

// Create Dirtional Light Direction Menu Funciton
void createDirectionMenu() {
    int mainMenu = glutCreateMenu(DirectionMenu);

    glutAddMenuEntry("From Front", 1);
    glutAddMenuEntry("From Front Right", 2);
    glutAddMenuEntry("From Right", 3);
    glutAddMenuEntry("From Back Right", 4);
    glutAddMenuEntry("From Back", 5);
    glutAddMenuEntry("From Back Left", 6);
    glutAddMenuEntry("From Left", 7);
    glutAddMenuEntry("From Front Left", 8);
    glutAddMenuEntry("From Bottom", 9);
    glutAddMenuEntry("From Top", 10);


    glutAttachMenu(GLUT_MIDDLE_BUTTON);
}

void createSkyBoxPathMenu() {
    int mainMenu = glutCreateMenu(SkyboxPathMenu);

    glutAddMenuEntry("Photos Studio", 1);
    glutAddMenuEntry("Sunflowers", 2);
    glutAddMenuEntry("Veranda", 3);

    glutAttachMenu(GLUT_LEFT_BUTTON);
}

void SetupScene(const std::string& modelFilePath) {
    LoadObjects(modelFilePath);
    CreateLights();
    CreateCamera();
    CreateShaderLib();
}

int main(int argc, char** argv)
{
    // Setting window properties.
    glutInit(&argc, argv);
    glutSetOption(GLUT_MULTISAMPLE, 4);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(screenWidth, screenHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Texture Mapping");

    // Initialize GLEW.
    // Must be done after glut is initialized!
    GLenum res = glewInit();
    if (res != GLEW_OK) {
        std::cerr << "GLEW initialization error: " 
                  << glewGetErrorString(res) << std::endl;
        return 1;
    }

    // Initialization.
    SetupRenderState();
    SetupScene(modelFilePath);
    CreateSkybox(skyFilePath);

    // Initialize Path Menu
    createPathMenu();

    // Initialize Direction Menu
    createDirectionMenu();

    // Initialize Skybox Path Menu
    createSkyBoxPathMenu();

    // Register callback functions.
    glutDisplayFunc(RenderSceneCB);
    glutIdleFunc(RenderSceneCB);
    glutReshapeFunc(ReshapeCB);
    glutSpecialFunc(ProcessSpecialKeysCB);
    glutKeyboardFunc(ProcessKeysCB);

    // Start rendering loop.
    glutMainLoop();

    return 0;
}
