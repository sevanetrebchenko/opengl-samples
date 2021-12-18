
#include "pch.h"
#include "particle.h"
#include "shader.h"
#include "camera.h"

int main() {
    // Initialize GLFW.
    int initializationCode = glfwInit();
    if (!initializationCode) {
        throw std::runtime_error("Failed to initialize GLFW.");
    }

    // Setting up OpenGL properties.
    glfwWindowHint(GLFW_SAMPLES, 1); // change for anti-aliasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int width = 1280;
    int height = 720;

    GLFWwindow* window = glfwCreateWindow(width, height, "GPU-Driven Particles", nullptr, nullptr);
    if (!window) {
        throw std::runtime_error("Failed to create GLFW window.");
    }

    // Initialize OpenGL.
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize Glad (OpenGL).");
    }

    std::cout << "Sample: GPU-Driven Particles" << std::endl;
    std::cout << "Vendor: " << (const char*)(glGetString(GL_VENDOR)) << std::endl;
    std::cout << "Renderer: " << (const char*)(glGetString(GL_RENDERER)) << std::endl;
    std::cout << "OpenGL Version: " << (const char*)(glGetString(GL_VERSION)) << std::endl;

    GLint ssboBindings;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &ssboBindings);
    std::cout << "Maximum shader storage buffer (SSBO) bindings: " << ssboBindings << std::endl;

    GLint ssboBlockSize;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &ssboBlockSize);
    std::cout << "Maximum shader storage block size: " << ssboBlockSize << std::endl;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glPointSize(1.0f);

    // Initialize camera.
    OpenGL::Camera camera { width, height };
    camera.SetPosition(glm::vec3(0.0f, 0.0f, 25.0f));
    glViewport(0, 0, width, height);

    // Initialize particle data.
    int numParticles = 1000000;

    std::vector<OpenGL::Particle> particles;
    particles.resize(numParticles);

    for (OpenGL::Particle& particle : particles) {
        particle.position = glm::vec4(glm::ballRand(100.0f), 1.0f);
    }

    // Construct necessary buffers for sample.
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

    // Timestep.
    float current;
    float previous = 0.0f;
    float dt = 0.0f;

    // Compile shaders.
    OpenGL::Shader shader { "Particle Shader", { "particle/assets/shaders/particle.vert", "particle/assets/shaders/particle.frag" } };

    shader.Bind();
    glBindVertexArray(vao);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

    while ((glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) && (glfwWindowShouldClose(window) == 0)) {
        glfwPollEvents();

        // Moving camera.
        static const float cameraSpeed = 100.0f;
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

        // Handle pausing the simulation on ONE key press.
        static int previousPauseKeyState = GLFW_RELEASE;
        int currentPauseKeyState = glfwGetKey(window, GLFW_KEY_SPACE);
        static bool isRunning = true;

        if (previousPauseKeyState == GLFW_PRESS && currentPauseKeyState == GLFW_RELEASE) {
            isRunning = !isRunning;
        }

        shader.SetUniform("isRunning", isRunning ? 1.0f : 0.0f);

        previousPauseKeyState = currentPauseKeyState;

        // Mouse input.
        static glm::vec2 previousCursorPosition;
        static bool initialInput = true;

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            glm::dvec2 cursorPosition;
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
            glm::vec3 centerOfGravity = cameraPosition + cameraForwardVector * 25.0f;
            shader.SetUniform("centerOfGravity", centerOfGravity);
            shader.SetUniform("isActive", 1.0f);
        }
        else {
            shader.SetUniform("isActive", 0.0f);
        }

        shader.SetUniform("dt", dt);
        shader.SetUniform("cameraTransform", camera.GetCameraTransform());

        // Rendering.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_POINTS, 0, numParticles);

        // Necessary if particle data is being read from the SSBO on the CPU after rendering.
        // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        current = (float)glfwGetTime();
        dt = current - previous;
        previous = current;

        glfwSwapBuffers(window);
    }

    shader.Unbind();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindVertexArray(0);

    glDeleteBuffers(1, &ssbo);
    glDeleteVertexArrays(1, &vao);

    // Shutdown.
    glfwDestroyWindow(window);
    glfwTerminate();
}