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
const float G = 0.1f;

// Generates vertices and indices for a 3D UV Sphere
void CreateSphere(float radius, unsigned int sectors, unsigned int stacks,
                  std::vector<float> &vertices, std::vector<unsigned int> &indices)
{
    vertices.clear();
    indices.clear();

    float x, y, z, xy; // vertex position
    float sectorStep = 2 * M_PI / sectors;
    float stackStep = M_PI / stacks;
    float sectorAngle, stackAngle;

    for (unsigned int i = 0; i <= stacks; ++i)
    {
        stackAngle = M_PI / 2 - i * stackStep; // starting from pi/2 to -pi/2
        xy = radius * cosf(stackAngle);        // r * cos(u)
        z = radius * sinf(stackAngle);         // r * sin(u)

        // add (sectors+1) vertices per stack
        for (unsigned int j = 0; j <= sectors; ++j)
        {
            sectorAngle = j * sectorStep; // starting from 0 to 2pi

            // vertex position (x, y, z)
            x = xy * cosf(sectorAngle); // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle); // r * cos(u) * sin(v)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    // generate CCW index list of triangles
    unsigned int k1, k2;
    for (unsigned int i = 0; i < stacks; ++i)
    {
        k1 = i * (sectors + 1); // beginning of current stack
        k2 = k1 + sectors + 1;  // beginning of next stack

        for (unsigned int j = 0; j < sectors; ++j, ++k1, ++k2)
        {
            // 2 triangles per sector excluding first and last stacks
            if (i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (stacks - 1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

class Object
{
public:
    inline static std::vector<float> vertices;
    inline static std::vector<unsigned int> indices;
    inline static unsigned int VBO, VAO, EBO;
    glm::vec3 position;
    glm::vec3 velocity;
    float radius;
    float mass;
    float lastTime = glfwGetTime();

    Object(glm::vec3 position, glm::vec3 velocity, float radius, float mass)
    {
        this->position = position;
        this->velocity = velocity;
        this->radius = radius;
        this->mass = mass;
    }

    void accelerate(glm::vec3 acceleration, float deltaTime)
    {
        velocity += acceleration * deltaTime;
        position += velocity * deltaTime;

        float bounceEff = 0.8f;

        // X-axis bounds
        if (position.x - radius < -1.0f)
        {
            position.x = -1.0f + radius;
            velocity.x = -velocity.x * bounceEff;
        }
        else if (position.x + radius > 1.0f)
        {
            position.x = 1.0f - radius;
            velocity.x = -velocity.x * bounceEff;
        }

        // Y-axis bounds
        if (position.y - radius < -1.0f)
        {
            position.y = -1.0f + radius;
            velocity.y = -velocity.y * bounceEff;
        }
        else if (position.y + radius > 1.0f)
        {
            position.y = 1.0f - radius;
            velocity.y = -velocity.y * bounceEff;
        }

        // Z-axis bounds
        if (position.z - radius < -1.0f)
        {
            position.z = -1.0f + radius;
            velocity.z = -velocity.z * bounceEff;
        }
        else if (position.z + radius > 1.0f)
        {
            position.z = 1.0f - radius;
            velocity.z = -velocity.z * bounceEff;
        }
    };

    void Draw(Shader &shader)
    {
        glBindVertexArray(VAO);
        float scaleVal = radius / 0.25f;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::scale(model, glm::vec3(scaleVal, scaleVal, scaleVal));
        // model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        shader.setMat4("model", model);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    static void setupMesh()
    {
        CreateSphere(0.25f, 36, 18, vertices, indices);

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // Bind and allocate Vertex data
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Bind and allocate Index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        int stride = 3 * sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }
};

std::vector<glm::vec3> calculateGravity(const std::vector<Object> &objects)
{
    // ============================================================================
    // PHYSICS DECOUPLING (SIMULTANEOUS STATE UPDATE)
    // ============================================================================
    // We separate the calculation of gravity from the application of acceleration.
    //
    // 1. "calculateGravity" reads the current positions of all objects to find forces.
    // 2. We store these net accelerations in a temporary vector array.
    // 3. Only AFTER all forces are known do we loop through and update positions.
    //
    // This prevents a "mutation lag" bug: if we updated Object A's position instantly,
    // Object B would calculate its gravity using Object A's *new* position instead
    // of its *current* position for this frame, violating laws of physics symmetry.
    // ============================================================================
    std::vector<glm::vec3> accelerations(objects.size(), glm::vec3(0.0f));

    for (size_t i = 0; i < objects.size(); ++i)
    {
        for (size_t j = i + 1; j < objects.size(); ++j)
        {
            glm::vec3 direction = objects[j].position - objects[i].position;
            float distance = glm::length(direction);

            // Avoid division by zero when objects overlap closely
            float minDistance = objects[i].radius + objects[j].radius;
            if (distance < minDistance)
                distance = minDistance;

            if (distance > 0.001f)
            {
                glm::vec3 normDir = direction / distance;

                // Newton's law: F = (G * m1 * m2) / r^2
                float forceMagnitude = (G * objects[i].mass * objects[j].mass) / (distance * distance);

                // Newton's Second Law: a = F / m
                accelerations[i] += normDir * (forceMagnitude / objects[i].mass);
                accelerations[j] -= normDir * (forceMagnitude / objects[j].mass);
            }
        }
    }

    return accelerations;
}

int main()
{

    std::cout << "Hello, World!" << std::endl;

    GLFWwindow *window = startGLFW(SCR_WIDTH, SCR_HEIGHT, "CGravity");

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // build and compile the shader program
    Shader ourShader("../src/shaders/shader.vs", "../src/shaders/shader.fs");

    std::vector<Object> objects = {
        Object(glm::vec3(-0.4f, 0.0f,  0.0f), glm::vec3(0.0f, 0.4f,  0.2f), 0.25f, 50.0f),
        Object(glm::vec3( 0.4f, 0.0f,  0.0f), glm::vec3(0.0f, -0.4f, -0.2f), 0.10f, 10.0f)
    };

    Object::setupMesh();

    glEnable(GL_DEPTH_TEST);

    float lastFrame = glfwGetTime();

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        // input
        processInput(window);

        // rendering commands here

        // Whenever we call glClear and clear the color buffer,
        // the entire color buffer will be filled with the color asconfigured by glClearColor.
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ourShader.use();
        // create transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        // note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best practice to set it outside the main loop only once.
        ourShader.setMat4("projection", projection);
        // camera
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("view", view);

        std::vector<glm::vec3> acc = calculateGravity(objects);
        for (size_t i = 0; i < objects.size(); ++i)
        {
            objects[i].accelerate(acc[i], deltaTime);
            objects[i].Draw(ourShader);
        }
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
