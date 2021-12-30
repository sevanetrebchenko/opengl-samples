
#include "pch.h"
#include "shader.h"
#include "camera.h"

int main() {
    // Initialize GLFW.
    int initializationCode = glfwInit();
    if (!initializationCode) {
        throw std::runtime_error("Failed to initialize GLFW.");
    }

    // Setting up OpenGL properties.
    glfwWindowHint(GLFW_SAMPLES, 1);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int width = 1280;
    int height = 720;

    GLFWwindow* window = glfwCreateWindow(width, height, "GLSL Path Tracing", nullptr, nullptr);
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

    std::cout << "Sample: GLSL Path Tracing" << std::endl;
    std::cout << "Vendor: " << (const char*)(glGetString(GL_VENDOR)) << std::endl;
    std::cout << "Renderer: " << (const char*)(glGetString(GL_RENDERER)) << std::endl;
    std::cout << "OpenGL Version: " << (const char*)(glGetString(GL_VERSION)) << std::endl;

    // Initialize ImGui.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::StyleColorsDark();

    // Initialize ImGui Flags.
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Platform/Renderer backend.
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Initialize camera.
    OpenGL::Camera camera { width, height };
    camera.SetPosition(glm::vec3(0.0f, 0.0f, 25.0f));
    glViewport(0, 0, width, height);

    // Default GL_NEAREST texture filtering option is sufficient.
    // Position texture.
    GLuint positionTexture;
    glGenTextures(1, &positionTexture);
    glBindTexture(GL_TEXTURE_2D, positionTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Normal texture.
    GLuint normalTexture;
    glGenTextures(1, &normalTexture);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Final output texture.
    GLuint outputTexture;
    glGenTextures(1, &outputTexture);
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Depth buffer.
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Initialize custom framebuffer for deferred rendering.
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, positionTexture, 0); // Position - 0.
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalTexture, 0);   // Normals  - 1.
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, outputTexture, 0);   // Output  - 2.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Failed to initialize custom framebuffer." << std::endl;
        return 1;
    }

    // Timestep.
    float current;
    float previous = 0.0f;
    float dt = 0.0f;

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

        // Start the Dear ImGui frame.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        current = (float)glfwGetTime();
        dt = current - previous;
        previous = current;

        // Rendering.

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // Geometry pass.

        // Do not write depth from future passes (preserve depth of actual scene, not FSQ).
        glDisable(GL_DEPTH_TEST);

        // Output pass.

        // Return depth-writing back to normal.
        glEnable(GL_DEPTH_TEST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Render ImGui on top of everything.
        // Enable window docking.
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        // Framework overview.
        if (ImGui::Begin("Overview")) {
            ImGui::Text("Render time:");
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        }
        ImGui::End();

        // Deferred rendering output.
        if (ImGui::Begin("Framebuffer")) {
            // Ensure proper scene image scaling.
            float maxWidth = ImGui::GetWindowContentRegionWidth();
            float aspect = static_cast<float>(width) / static_cast<float>(height);
            ImVec2 imageSize = ImVec2(maxWidth, maxWidth / aspect);

            ImGui::Image(reinterpret_cast<ImTextureID>(outputTexture), imageSize, ImVec2(0, 1), ImVec2(1, 0));
        }

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Shutdown.
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}
