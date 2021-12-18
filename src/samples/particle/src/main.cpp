
#include "pch.h"
#include "particle.h"
#include "shader.h"
#include "camera.h"

int main() {
    // Initialize rendering context.
    int initializationCode = glfwInit();
    if (!initializationCode) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return 1;
    }

    // Setting up OpenGL properties
    glfwWindowHint(GLFW_SAMPLES, 1); // change for anti-aliasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int width = 1280;
    int height = 720;
    std::string name = "GPU-Driven Particles";

    GLFWwindow* window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window." << std::endl;
        return 1;
    }

    // Initialize OpenGL.
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize Glad (OpenGL)." << std::endl;
        return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    std::cout << "Sample: Particles" << std::endl;
    std::cout << "Vendor: " << (const char*)(glGetString(GL_VENDOR)) << std::endl;
    std::cout << "Renderer: " << (const char*)(glGetString(GL_RENDERER)) << std::endl;
    std::cout << "OpenGL Version: " << (const char*)(glGetString(GL_VERSION)) << std::endl;

    GLint ssboBindings;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &ssboBindings);
    std::cout << "Maximum shader storage buffer (SSBO) bindings: " << ssboBindings << std::endl;

    GLint ssboBlockSize;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &ssboBlockSize);
    std::cout << "Maximum shader storage block size: " << ssboBlockSize << std::endl;

    // Allocate particles.
    int numParticles = 10000000;
    float range = 50.0f;

    std::vector<OpenGL::Particle> particles;
    particles.resize(numParticles);

    for (OpenGL::Particle& particle : particles) {
        glm::vec3 position (glm::linearRand(-range, range), glm::linearRand(-range, range), glm::linearRand(-range, range));
        particle.position = glm::vec4(position, 1.0f);
    }

    // Construct necessary buffers.
    // SSBO needs an (empty) VAO to be present for rendering.
    GLuint vao;
    glGenBuffers(1, &vao);
    glGenVertexArrays(1, &vao);

    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, particles.size() * sizeof(OpenGL::Particle), particles.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo); // Binding 0.
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Setup camera.
    OpenGL::Camera camera(width, height);
    camera.SetPosition(glm::vec3(0.0f, 0.0f, 25.0f));
    camera.SetTargetPosition(glm::vec3(0.0f));

    glm::vec2 previousCursorPosition;
    glm::dvec2 cursorPosition;
    bool initialInput = true;

    int previousPauseKeyState = GLFW_RELEASE;

    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glPointSize(1.0f);

    // Timestep.
    float current = 0.0f;
    float previous = 0.0f;
    float dt = 0.0f;

    bool isActive = true;
    bool isRunning = true;

    // Compile shaders.
    OpenGL::Shader shader({ std::make_pair("particle/assets/shaders/particle.vert", GL_VERTEX_SHADER), std::make_pair("particle/assets/shaders/particle.frag", GL_FRAGMENT_SHADER) });
    shader.Bind();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBindVertexArray(vao);

    while ((glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) && (glfwWindowShouldClose(window) == 0)) {
        glfwPollEvents();

        // Moving camera.
        float cameraSpeed = 100.0f;
        const glm::vec3& cameraPosition = camera.GetPosition();
        const glm::vec3& cameraForwardVector = camera.GetForwardVector();
        const glm::vec3& cameraUpVector = camera.GetUpVector();
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camera.SetPosition(cameraPosition + cameraSpeed * cameraForwardVector * dt);
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            camera.SetPosition(cameraPosition - cameraSpeed * cameraForwardVector * dt);
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            camera.SetPosition(cameraPosition - glm::normalize(glm::cross(cameraForwardVector, cameraUpVector)) * cameraSpeed * dt);
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            camera.SetPosition(cameraPosition + glm::normalize(glm::cross(cameraForwardVector, cameraUpVector)) * cameraSpeed * dt);
        }

        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            camera.SetPosition(cameraPosition + cameraSpeed * cameraUpVector * dt);
        }

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            camera.SetPosition(cameraPosition - cameraSpeed * cameraUpVector * dt);
        }

        // Pausing simulation.
        int currentPauseKeyState = glfwGetKey(window, GLFW_KEY_SPACE);
        if (previousPauseKeyState == GLFW_PRESS && currentPauseKeyState == GLFW_RELEASE) {
            isRunning = !isRunning;
        }
        previousPauseKeyState = currentPauseKeyState;

        // Mouse input.
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            glfwGetCursorPos(window, &cursorPosition.x, &cursorPosition.y);

            // FPS camera.
            if (initialInput) {
                previousCursorPosition = cursorPosition;
                initialInput = false;
            }

            float mouseSensitivity = 0.1f;

            float dx = static_cast<float>(cursorPosition.x - previousCursorPosition.x) * mouseSensitivity;
            float dy = static_cast<float>(previousCursorPosition.y - cursorPosition.y) * mouseSensitivity; // Flipped.

            previousCursorPosition = cursorPosition;

            float pitch = glm::degrees(camera.GetPitch());
            float yaw = glm::degrees(camera.GetYaw());
            float roll = glm::degrees(camera.GetRoll());

            float limit = 89.0f;

            yaw += dx;
            pitch += dy;

            // Prevent camera forward vector to be parallel to camera up vector (0, 1, 0).
            if (pitch > limit) {
                pitch = limit;
            }
            if (pitch < -limit) {
                pitch = -limit;
            }

            camera.SetEulerAngles(pitch, yaw, roll);
        }
        else {
            initialInput = true;
        }

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            isActive = true;
        }
        else {
            isActive = false;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set shader uniforms.
        if (isActive) {
            glm::vec3 centerOfGravity = cameraPosition + cameraForwardVector * 25.0f;
            shader.SetUniform("centerOfGravity", centerOfGravity);
            shader.SetUniform("isActive", 1.0f);
        }
        else {
            shader.SetUniform("isActive", 0.0f);
        }

        if (isRunning) {
            shader.SetUniform("isRunning", 1.0f);
        }
        else {
            shader.SetUniform("isRunning", 0.0f);
        }

        shader.SetUniform("dt", dt);
        shader.SetUniform("cameraTransform", camera.GetCameraTransform());

        glDrawArrays(GL_POINTS, 0, numParticles);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        current = (float)glfwGetTime();
        dt = current - previous;
        previous = current;

        glfwSwapBuffers(window);
    }

    shader.Unbind();
    glDeleteBuffers(1, &ssbo);

    // Shutdown.
    glfwDestroyWindow(window);
    glfwTerminate();
}