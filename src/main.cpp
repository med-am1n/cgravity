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

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window, float deltaTime);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
// camera
Camera camera(glm::vec3(0.0f, 0.0f, 4.0f));

std::vector<float> CreateCircle(float centerX, float centerY, float radius, int res);
GLFWwindow *startGLFW(float width, float hight, const char *title);

float posY = 1.0f;
float velocity = 0.0f;
const float gravity = 9.81f; // meters/sec²
const float G = 0.1f;
float TIME_SCALE = 0.2f;

// Global variables to track mouse state metrics
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool cursorLocked = false; // Controls mode switching (true = Fly Mode, false = UI Mode)
bool isPaused = false;
float PREVIOUS_TIME_SCALE = 0.2f;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);

void CreateSphere(float radius, unsigned int sectors, unsigned int stacks,
                  std::vector<float> &vertices, std::vector<unsigned int> &indices);

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
    glm::vec3 color;

    Object(glm::vec3 position, glm::vec3 velocity, float radius, float mass, glm::vec3 color)
    {
        this->position = position;
        this->velocity = velocity;
        this->radius = radius;
        this->mass = mass;
        this->color = color;
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
        shader.setVec3("objectColor", color);
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

        int stride = 6 * sizeof(float);

        // Position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
        glEnableVertexAttribArray(0);

        // Normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
};

std::vector<glm::vec3> calculateGravity(const std::vector<Object> &objects);
void resolveCollisions(std::vector<Object> &objects);

// Generates a grid mesh forming a 3D Paraboloid well
void CreateParaboloidGrid(int gridWidth, int gridDepth, float spacing,
                          std::vector<float> &vertices, std::vector<unsigned int> &indices)
{
    vertices.clear();
    indices.clear();

    // Generate positions and structural normal vectors
    for (int z = 0; z <= gridDepth; ++z)
    {
        for (int x = 0; x <= gridWidth; ++x)
        {
            // Center the grid coordinates around origin (0, 0, 0)
            float posX = (x - gridWidth / 2.0f) * spacing;
            float posZ = (z - gridDepth / 2.0f) * spacing;

            // Base paraboloid equation: Y falls off relative to distance from center
            float posY = -0.5f * ((posX * posX) + (posZ * posZ));

            // Position Layout channels
            vertices.push_back(posX);
            vertices.push_back(posY);
            vertices.push_back(posZ);

            // Base temporary Normals (pointing upward along Y axis)
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
        }
    }

    // Triangle indices maps
    for (int z = 0; z < gridDepth; ++z)
    {
        for (int x = 0; x < gridWidth; ++x)
        {
            unsigned int row1 = z * (gridWidth + 1);
            unsigned int row2 = (z + 1) * (gridWidth + 1);

            // Triangle 1
            indices.push_back(row1 + x);
            indices.push_back(row2 + x);
            indices.push_back(row1 + x + 1);

            // Triangle 2
            indices.push_back(row1 + x + 1);
            indices.push_back(row2 + x);
            indices.push_back(row2 + x + 1);
        }
    }
}

class ParaboloidSpace
{
public:
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;

    int width = 40;
    int depth = 40;
    float spacing = 0.05f;

    void setupMesh()
    {
        CreateParaboloidGrid(width, depth, spacing, vertices, indices);

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // Using GL_DYNAMIC_DRAW because we will stream updated coordinates every single frame
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        int stride = 6 * sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    // Warps the grid fabric dynamically relative to planet locations and masses
    void UpdateFabric(const std::vector<Object> &planets)
    {
        int vertexCount = (width + 1) * (depth + 1);

        for (int i = 0; i < vertexCount; ++i)
        {
            float x = vertices[i * 6 + 0];
            float z = vertices[i * 6 + 2];

            // center the grid baseline
            float accumulatedDepth = 0.0f;

            for (const auto &planet : planets)
            {
                float dx = x - planet.position.x;
                float dz = z - planet.position.z;
                float distSq = (dx * dx) + (dz * dz) + 0.01f;

                // prevents the grid from stretching too far down now that it is higher up
                accumulatedDepth -= (0.002f * planet.mass) / sqrt(distSq);
            }

            // Apply the newly calculated Y coordinate
            vertices[i * 6 + 1] = accumulatedDepth;
        }

        // Stream the updated grid coordinates to your graphics card
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    }
    void Draw(Shader &shader)
    {
        glBindVertexArray(VAO);
        glm::mat4 model = glm::mat4(1.0f); // Fixed at origin space
        shader.setMat4("model", model);

        // Pass a constant grid color (Gridlines hue color)
        shader.setVec3("objectColor", glm::vec3(0.15f, 0.45f, 0.35f));

        // Render in Wireframe mode so you can see the paraboloid grid mesh deformation lines
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);

        // Restore standard solid drawing configuration for the planets loop
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindVertexArray(0);
    }
};
int main()
{

    std::cout << "Hello, World!" << std::endl;

    GLFWwindow *window = startGLFW(SCR_WIDTH, SCR_HEIGHT, "CGravity");

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Setup Dear ImGui Context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer Backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // build and compile the shader program
    Shader ourShader("../src/shaders/shader.vs", "../src/shaders/shader.fs");

    std::vector<Object> objects = {
        Object(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), 0.10f, 200.0f, glm::vec3(1.0f, 0.85f, 0.2f)),
        Object(glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(0.0f, 0.25f, 0.1f), 0.09f, 15.0f, glm::vec3(0.0f, 0.9f, 0.8f)),
        Object(glm::vec3(-0.6f, 0.2f, -0.1f), glm::vec3(0.1f, -0.22f, 0.0f), 0.12f, 25.0f, glm::vec3(0.85f, 0.25f, 0.2f))};

    Object::setupMesh();

    ParaboloidSpace spaceFabric;
    spaceFabric.setupMesh();

    glEnable(GL_DEPTH_TEST);

    float lastFrame = glfwGetTime();

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        // input
        processInput(window, deltaTime);

        // rendering commands here

        // Whenever we call glClear and clear the color buffer,
        // the entire color buffer will be filled with the color asconfigured by glClearColor.
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // START IMGUI FRAME
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // DRAW IMGUI INTERFACES
        ImGui::Begin("Simulation Controls");

        if (isPaused)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "STATUS: PAUSED");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "STATUS: RUNNING");
        }

        // Capture slider modifications directly using boolean gates
        bool speedChanged = ImGui::SliderFloat("Speed", &TIME_SCALE, 0.0f, 1.5f);
        bool fovChanged = ImGui::SliderFloat("FOV (Zoom)", &camera.Zoom, 45.0f, 1.0f);

        if (speedChanged || fovChanged)
        {
            if (isPaused)
            {
                isPaused = false;

                PREVIOUS_TIME_SCALE = TIME_SCALE;

                // Keep cursor visible (UI Mode)
                cursorLocked = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }

        ImGui::Text("Application Performance: %.1f FPS", io.Framerate);
        ImGui::End();

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
            objects[i].accelerate(acc[i], deltaTime * TIME_SCALE);
        }
        resolveCollisions(objects);


        spaceFabric.UpdateFabric(objects);
        
        spaceFabric.Draw(ourShader);
        
        for (size_t i = 0; i < objects.size(); ++i)
        {
            objects[i].Draw(ourShader);
        }

        // render command
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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

void processInput(GLFWwindow *window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (isPaused)
        return;
    // Keyboard controls for 3D exploration space
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    {
        TIME_SCALE += 0.5f * deltaTime;
        if (TIME_SCALE > 1.5f)
            TIME_SCALE = 1.5f;
    }

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        TIME_SCALE -= 0.5f * deltaTime;
        if (TIME_SCALE < 0.0f)
            TIME_SCALE = 0.0f;
    }
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

            // Normals (Normal = Position / Radius)
            vertices.push_back(x / radius);
            vertices.push_back(y / radius);
            vertices.push_back(z / radius);
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

void resolveCollisions(std::vector<Object> &objects)
{
    float restitution = 0.8f;

    for (size_t i = 0; i < objects.size(); ++i)
    {
        for (size_t j = i + 1; j < objects.size(); ++j)
        {
            glm::vec3 delta = objects[j].position - objects[i].position;
            float distance = glm::length(delta);
            float minDistance = objects[i].radius + objects[j].radius;

            if (distance < minDistance)
            {
                // Prevent division by zero
                glm::vec3 normal = (distance > 0.0001f) ? delta / distance : glm::vec3(1.0f, 0.0f, 0.0f);

                float overlap = minDistance - distance;
                float totalInverseMass = (1.0f / objects[i].mass) + (1.0f / objects[j].mass);

                glm::vec3 separationVector = normal * (overlap / totalInverseMass);

                objects[i].position -= separationVector * (1.0f / objects[i].mass);
                objects[j].position += separationVector * (1.0f / objects[j].mass);

                glm::vec3 relativeVelocity = objects[i].velocity - objects[j].velocity;
                float velAlongNormal = glm::dot(relativeVelocity, normal);
                if (velAlongNormal > 0.0f)
                {
                    float impulseScalar = -(1.0f + restitution) * velAlongNormal;
                    impulseScalar /= totalInverseMass;

                    glm::vec3 impulse = impulseScalar * normal;
                    objects[i].velocity += impulse / objects[i].mass;
                    objects[j].velocity -= impulse / objects[j].mass;
                }
            }
        }
    }
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

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    if (isPaused)
        return;
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

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    if (isPaused)
        return;
    // Let ImGui have priority on scrolling if hovering over UI windows
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        isPaused = !isPaused;

        if (isPaused)
        {
            PREVIOUS_TIME_SCALE = TIME_SCALE;
            TIME_SCALE = 0.0f;

            cursorLocked = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else
        {
            TIME_SCALE = PREVIOUS_TIME_SCALE;
            cursorLocked = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true; // Prevents camera camera snapping jumps
        }
    }
}
