
#include "pch.h"
#include "particle.h"
#include "shader.h"
#include "camera.h"
#include "utility.h"

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

    GLFWwindow* window = glfwCreateWindow(width, height, "GPU-Driven Particles", nullptr, nullptr);
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

    // Initialize ImGui.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::StyleColorsDark();

    // Initialize ImGui Flags.
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Attempt to load ImGui .ini configuration.
    // Find imgui_layout.ini file inside 'data' subdirectory for this sample.
    bool foundIni = false;

    if (!std::filesystem::exists("src/samples/particles/data")) {
        std::filesystem::create_directory("src/samples/particles/data");
    }

    for (const std::string& file : Utilities::GetFiles("src/samples/particles/data")) {
        if (Utilities::GetAssetName(file) == "imgui_layout" && Utilities::GetAssetExtension(file) == "ini") {
            foundIni = true;
            break;
        }
    }

    std::string imGuiIni = "src/samples/particles/data/imgui_layout.ini";

    if (foundIni) {
        ImGui::LoadIniSettingsFromDisk(imGuiIni.c_str());
    }

    io.IniFilename = nullptr; // Enable manual saving.

    // Setup Platform/Renderer backend.
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glPointSize(1.0f);

    // Initialize camera.
    OpenGL::Camera camera { width, height };
    camera.SetPosition(glm::vec3(0.0f, 0.0f, 25.0f));
    glViewport(0, 0, width, height);

    // Initialize particles data.
    int numParticles = 2000000;

    std::vector<OpenGL::Particle> particles;
    particles.resize(numParticles);

    for (OpenGL::Particle& particle : particles) {
        particle.position = glm::vec4(glm::ballRand(200.0f), 1.0f);
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
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo); // Binding 0.
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // FBO for screenshot purposes.
    std::vector<float> blankTexture;
    blankTexture.resize(width * height * 4, 0.0f);

    GLuint outputTexture;
    glGenTextures(1, &outputTexture);
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, blankTexture.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Depth buffer.
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Initialize custom framebuffer.
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Failed to initialize custom framebuffer on startup." << std::endl;
        return 1;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    std::vector<GLenum> drawBuffers(1, GL_COLOR_ATTACHMENT0);

    // Timestep.
    float current;
    float previous = 0.0f;
    float dt = 0.0f;

    // Compile shaders.
    OpenGL::Shader shader { "Particle Shader", { "src/samples/particles/assets/shaders/particle.vert",
                                                 "src/samples/particles/assets/shaders/particle.frag" } };

    shader.Bind();
    glBindVertexArray(vao);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

    while ((glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) && (glfwWindowShouldClose(window) == 0)) {
        glfwPollEvents();

        // Start the Dear ImGui frame.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Handle resizing the window.
        int tempWidth;
        int tempHeight;
        glfwGetFramebufferSize(window, &tempWidth, &tempHeight);

        if (tempWidth != width || tempHeight != height) {
            // Window dimensions changed, resize content.
            width = tempWidth;
            height = tempHeight;

            blankTexture.resize(width * height * 4, 0.0f); // Inserts or deletes elements appropriately.

            // Reallocate FBO attachments with updated data storage size.
            glBindTexture(GL_TEXTURE_2D, outputTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, blankTexture.data());
            glBindTexture(GL_TEXTURE_2D, 0);

            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            // Reconstruct custom frame buffer.
            // Note: not sure if this fully necessary, resizing doesn't happen every frame so the performance overhead is negligible.
            glDeleteFramebuffers(1, &fbo);

            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "Failed to reinitialize custom framebuffer on window resize." << std::endl;
                break;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Update viewport.
            glViewport(0, 0, width, height);

            // Update camera.
            float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
            camera.SetAspectRatio(aspectRatio);
        }

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
            // Get ray that originates from the camera position in the rayDirection of the click.
            glm::dvec2 clickPosition;
            glfwGetCursorPos(window, &clickPosition.x, &clickPosition.y);

            // https://antongerdelan.net/opengl/raycasting.html
            // GLFW has (0, 0) in the top left, while OpenGL has (0, 0) in the bottom left.
            // Convert screen coordinates [0:WIDTH, HEIGHT:0] to normalized device coordinates [-1:1, -1:1].
            glm::vec2 ndc = glm::vec2(static_cast<float>(2.0f * clickPosition.x) / static_cast<float>(width) - 1.0f,
                                      1.0f - static_cast<float>(2.0f * clickPosition.y) / static_cast<float>(height));

            glm::vec3 rayOrigin = camera.GetPosition();

            glm::vec3 rayDirection;
            rayDirection = glm::inverse(camera.GetPerspectiveTransform()) * glm::vec4(ndc, -1.0f, 1.0f);
            rayDirection = glm::vec4(glm::vec2(rayDirection), -1.0f, 0.0f);
            rayDirection = glm::normalize(glm::vec3(glm::inverse(camera.GetViewTransform()) * glm::vec4(rayDirection, 0.0f)));

            glm::vec3 centerOfGravity = rayOrigin + rayDirection * 25.0f;
            shader.SetUniform("centerOfGravity", centerOfGravity);
            shader.SetUniform("isActive", 1.0f);
        }
        else {
            shader.SetUniform("isActive", 0.0f);
        }

        shader.SetUniform("dt", dt);
        shader.SetUniform("cameraTransform", camera.GetCameraTransform());

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glDrawBuffers(1, drawBuffers.data());

        // Rendering.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_POINTS, 0, numParticles);

        // Necessary if particles data is being read from the SSBO on the CPU after rendering.
        // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // Render final output to screen.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glNamedFramebufferReadBuffer(fbo, GL_COLOR_ATTACHMENT0); // Set the read buffer to be the render attachment of the final output.

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glBlitFramebuffer(0, 0, width, height,
                          0, 0, width, height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // Sample overview and statistics.
        if (ImGui::Begin("Sample Overview")) {
            ImGui::PushStyleColor(ImGuiCol_Text, 0xff999999);

            ImGui::Text("Render time:");
            ImGui::Text("%.3f ms/frame (%.1f FPS)", dt * 1000.0f, 1.0f / dt);

            static char outputFilename[256] = { "result" };

            if (ImGui::Button("Take Screenshot")) {
                static std::string outputDirectory = "src/samples/particles/data/screenshots/";

                if (!std::filesystem::exists(outputDirectory)) {
                    // Create output directory if it doesn't exist.
                    std::filesystem::create_directory(outputDirectory);
                }

                // Save final output image.
                int channels = 4;
                std::vector<unsigned char> pixels(width * height * channels);

                // Read in OpenGL texture data.
                glBindTexture(GL_TEXTURE_2D, outputTexture);
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
                glBindTexture(GL_TEXTURE_2D, 0);

                stbi_flip_vertically_on_write(true);
                stbi_write_png(std::string(outputDirectory + std::string(outputFilename) + ".png").c_str(), width, height, channels, pixels.data(), 0);
                stbi_write_jpg(std::string(outputDirectory + std::string(outputFilename) + ".jpg").c_str(), width, height, channels, pixels.data(), 100);
            }

            ImGui::Text("Filename:");
            ImGui::InputText("##outputFilename", outputFilename, 256);

            ImGui::PopStyleColor();
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Save ImGui .ini settings.
        if (io.WantSaveIniSettings) {
            ImGui::SaveIniSettingsToDisk(imGuiIni.c_str());

            // Manually change flag.
            io.WantSaveIniSettings = false;
        }

        current = (float)glfwGetTime();
        dt = current - previous;
        previous = current;

        glfwSwapBuffers(window);
    }

    shader.Unbind();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindVertexArray(0);

    // Shutdown.
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteTextures(1, &outputTexture);
    glDeleteBuffers(1, &ssbo);
    glDeleteVertexArrays(1, &vao);

    ImGui::SaveIniSettingsToDisk(imGuiIni.c_str());
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}