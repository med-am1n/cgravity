#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "shader.h"
#include "camera.h"
#include <stb_image.h>
#include <filesystem.h>

#include <vector>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

std::vector<float> CreateCircle(float centerX, float centerY, float radius, int res);
GLFWwindow *startGLFW(float width, float hight, const char *title);

float posY = 1.0f;
float velocity = 0.0f;
const float gravity = 9.81f; // meters/sec²
float lastTime = glfwGetTime();

class Object
{
public:
    std::vector<float> position;
    std::vector<float> vertices;
    Shader shader;
    float velocity;
    unsigned int VBO, VAO;
        Object(const std::vector<float>& position_,
           const std::vector<float>& vertices_,
           const Shader& shader_,
           float velocity_)
        : position(position_), vertices(vertices_), shader(shader_), velocity(velocity_)
    {

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        int stride = 3 * sizeof(float);
        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
        glEnableVertexAttribArray(0);
    }

    void Draw(const glm::mat4& view, const glm::mat4& projection)
    {
        shader.use();
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        glBindVertexArray(VAO);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position[0], position[1], position[1]));
        // model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLE_FAN, 0, vertices.size() / 3);
    }
};

int main()
{

    std::cout << "Hello, World!" << std::endl;

    GLFWwindow *window = startGLFW(SCR_WIDTH, SCR_HEIGHT, "CGravity");

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // build and compile the shader program
    Shader ourShader("../src/shaders/shader.vs", "../src/shaders/shader.fs");

    std::vector<float> vertices1 = CreateCircle(0.0f, 0.0f, 0.25f, 100);
    std::vector<float> vertices2 = CreateCircle(0.0f, 0.0f, 0.10f, 100);

    Object circle1({0.0f, 0.5f, 0.0f}, vertices1, ourShader,0.0f);
    Object circle2({0.0f, -0.5f, 0.0f}, vertices2, ourShader ,0.0f);

    glEnable(GL_DEPTH_TEST);
    // render loop
    while (!glfwWindowShouldClose(window))
    {
        // input
        processInput(window);

        // rendering commands here

        // Whenever we call glClear and clear the color buffer,
        // the entire color buffer will be filled with the color asconfigured by glClearColor.
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // create transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        // camera
        glm::mat4 view = camera.GetViewMatrix();
        
        circle1.Draw(view, projection);
        circle2.Draw(view, projection);
        // Double buffering:
        // Rendering is done in a hidden back buffer while the front buffer is displayed on screen.
        // Once rendering is complete, buffers are swapped (glfwSwapBuffers) to avoid flickering
        // and ensure the user only sees fully rendered frames -no flickering issues.
        glfwSwapBuffers(window);

        // check and call events
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

std::vector<float> CreateCircle(float centerX, float centerY, float radius, int res)
{

    std::vector<float> vertices;

    // Center vertex
    vertices.push_back(centerX);
    vertices.push_back(centerY);
    vertices.push_back(0.0f);

    for (int i = 0; i <= res; i++)
    {
        float angle = 2.0f * M_PI * i / res;

        vertices.push_back(centerX + radius * cos(angle));
        vertices.push_back(centerY + radius * sin(angle));
        vertices.push_back(0.0f);
    }

    return vertices;
}

GLFWwindow *startGLFW(float width, float hight, const char *title)
{

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize glfw" << std::endl;
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, title, nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);

    // initialize GLAD
    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return nullptr;
    }

    return window;
}
