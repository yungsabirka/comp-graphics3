#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "ResourceManager.h"
#include <iostream>
#include "stb_image.h"
#include "Model.h"
#include "Camera.h"

const unsigned int SCR_WIDTH = 1400;
const unsigned int SCR_HEIGHT = 900;

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void processInput(GLFWwindow* window);

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

float toSpot = 1.0f;

unsigned int loadCubemap(const std::vector<std::string>& faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Christmas", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    stbi_set_flip_vertically_on_load(true);
    glEnable(GL_DEPTH_TEST);

    Shader ourShader = ResourceManager::LoadShader("vShader.vs", "fShader.fs", nullptr, "sh");
    Shader skyboxShader = ResourceManager::LoadShader("skybox.vs", "skybox.fs", nullptr, "skb");

    glfwSetWindowUserPointer(window, &ourShader);

    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    std::vector<std::string> faces
    {
        "skybox/px.png",
        "skybox/nx.png",
        "skybox/py.png",
        "skybox/ny.png",
        "skybox/pz.png",
        "skybox/nz.png"
    };
    stbi_set_flip_vertically_on_load(false);
    unsigned int cubemapTexture = loadCubemap(faces);
    stbi_set_flip_vertically_on_load(false);

    Model ufo("ufo/ufo.obj");
    float ufoScale = 0.05f;
    uint32_t ufosCount = 7;

    glm::vec3 ufoColors[] = { {1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {1, 1, 0}, {1, 0.5, 0.5}, {0.5, 0, 0.5}, {1, 0.5, 0} };

    glm::vec3 ufoPositions[] = { {10, 10, 10}, {16, 0, 7}, {13, 12, 0}, {0, 12, 7},
                                    {-14, 0, 8}, {-10, -7, 12}, {7, -14, 15} };

    glm::vec3 rotationVectors[] = { {500, -800, 300}, {-500, 100, 600}, {100, -100, 0}, {0, -1, 1},
                                    {-2, 0, 1}, {4, -5, 0}, {-4, 2, 4} };
    float angles[] = {0, 0, 0, 0, 0, 0, 0};
    float anglesVelocity[] = {0.3f, 0.15f, 0.3f, 0.15f , 0.3f, 0.15f , 0.2f};

    glm::mat4 ufoModel = glm::scale(glm::mat4(1.0f),  {ufoScale, ufoScale, ufoScale});

    Model tree("tree/christmas_tree.obj");
    glm::mat4 treeModel = glm::scale(glm::mat4(1.0f), { 0.3, 0.3, 0.3 });
    //backpackModel = glm::rotate(backpackModel, glm::radians(-90.0f), {1, 0, 0});

    ourShader.Use();

    ourShader.SetVector3f("dirLight.direction", -0.2f, -1.0f, -0.3f);
    ourShader.SetVector3f("dirLight.ambient", 0.1f, 0.1f, 0.1f);
    ourShader.SetVector3f("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
    ourShader.SetVector3f("dirLight.specular", 0.5f, 0.5f, 0.5f);

    ourShader.SetVector3f("spotLight.ambient", 0.0f, 0.0f, 0.0f);
    ourShader.SetVector3f("spotLight.diffuse", 0.0f, 1.0f, 0.0f);
    ourShader.SetVector3f("spotLight.specular", 1.0f, 0.0f, 0.0f);
    ourShader.SetFloat("spotLight.constant", 1.0f);
    ourShader.SetFloat("spotLight.linear", 0.01f);
    ourShader.SetFloat("spotLight.quadratic", 0.0f);
    ourShader.SetFloat("spotLight.cutOff", glm::cos(glm::radians(8.0f)));
    ourShader.SetFloat("spotLight.outerCutOff", glm::cos(glm::radians(10.0f)));
    ourShader.SetFloat("toSpot", 1.0f);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.Use();

        for (int i = 0; i < ufosCount; i++)
        {
            glm::vec3 ufoPos = glm::rotate(glm::mat4(1.0f), angles[i], rotationVectors[i]) * glm::vec4(ufoPositions[i], 0);
            std::string lightName = "pointLights[" + std::to_string(i) + "].";

            ourShader.SetVector3f((lightName + "position").c_str(), ufoPos);
            ourShader.SetFloat((lightName + "contant").c_str(), 4.0f);
            ourShader.SetFloat((lightName + "linear").c_str(), 0.1f);
            ourShader.SetFloat((lightName + "quadratic").c_str(), 0.0f);
            ourShader.SetVector3f((lightName + "ambient").c_str(), 0.1f, 0.1f, 0.1f);
            ourShader.SetVector3f((lightName + "diffuse").c_str(), 0.35f*ufoColors[i]);
            ourShader.SetVector3f((lightName + "specular").c_str(), ufoColors[i]);
        }
        
        ourShader.SetVector3f("spotLight.position", camera.Position);
        ourShader.SetVector3f("spotLight.direction", camera.Front);
        

        ourShader.SetVector3f("viewPos", camera.Position);

        // material properties
        ourShader.SetFloat("material.shininess", 128.0f);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        ourShader.SetMatrix4("projection", projection);

        // camera/view transformation
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.SetMatrix4("view", view);

        for (int i = 0; i < ufosCount; i++)
        {
            ourShader.SetFloat("emissionFactor", 0.45f * std::abs(std::sin(i * glfwGetTime())));
            ourShader.SetVector3f("emissionColor", ufoColors[i]);

            angles[i] += glm::radians(anglesVelocity[i]);
            glm::mat4 model = glm::rotate(glm::mat4(1.0f), angles[i], rotationVectors[i])  * glm::translate(ufoModel, ufoPositions[i]);
            model = glm::rotate(model, 7.0f * angles[6 - i], {1, 1, 1});
            ourShader.SetMatrix4("model", model);
            ufo.Draw(ourShader);
        }

        //backpackModel = glm::rotate(backpackModel, glm::radians(0.1f), {1, -1, 1});
        ourShader.SetFloat("emissionFactor", 0.0f);

        ourShader.SetMatrix4("model", treeModel);
        tree.Draw(ourShader);

        // draw skybox as last
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.Use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.SetMatrix4("view", view);
        skyboxShader.SetMatrix4("projection", projection);
        
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        Shader* sh = static_cast<Shader*>(glfwGetWindowUserPointer(window));
        sh->Use();
        
        if (toSpot == 0.0f)
        {
            toSpot = 1.0f;
        }
        else
        {
            toSpot = 0.0f;
        }

        sh->SetFloat("toSpot", toSpot);
    }
}