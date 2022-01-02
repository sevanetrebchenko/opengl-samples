
#include "pch.h"
#include "utility.h"
#include "shader.h"
#include "transform.h"
#include "camera.h"
#include "object_loader.h"
#include "triangle.h"
#include "model.h"
#include "primitives.h"

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

    // Attempt to load ImGui .ini configuration.
    // Find imgui_layout.ini file inside 'data' subdirectory for this sample.
    bool foundIni = false;

    for (const std::string& file : Utilities::GetFiles("src/samples/path-tracing/data")) {
        if (Utilities::GetAssetName(file) == "imgui_layout" && Utilities::GetAssetExtension(file) == "ini") {
            foundIni = true;
            break;
        }
    }

    std::string imGuiIni = "src/samples/path-tracing/data/imgui_layout.ini";

    if (foundIni) {
        ImGui::LoadIniSettingsFromDisk(imGuiIni.c_str());
    }

    io.IniFilename = nullptr; // Enable manual saving.

    // Setup Platform/Renderer backend.
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Initialize camera.
    OpenGL::Camera camera { width, height };
    camera.SetPosition(glm::vec3(0.0f, 0.0f, 10.0f));
    glViewport(0, 0, width, height);

    // Initialize skybox.
    // https://learnopengl.com/Advanced-OpenGL/Cubemaps
    // Cubemap order:
    // GL_TEXTURE_CUBE_MAP_POSITIVE_X - Right
    // GL_TEXTURE_CUBE_MAP_NEGATIVE_X - Left
    // GL_TEXTURE_CUBE_MAP_POSITIVE_Y - Top
    // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y - Bottom
    // GL_TEXTURE_CUBE_MAP_POSITIVE_Z - Back
    // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z - Front
    std::vector<std::string> textureFaces = {
        "src/samples/path-tracing/assets/textures/skybox/pos_x.png",
        "src/samples/path-tracing/assets/textures/skybox/neg_x.png",
        "src/samples/path-tracing/assets/textures/skybox/pos_y.png",
        "src/samples/path-tracing/assets/textures/skybox/neg_y.png",
        "src/samples/path-tracing/assets/textures/skybox/pos_z.png",
        "src/samples/path-tracing/assets/textures/skybox/neg_z.png"
    };

    GLuint skybox;
    glGenTextures(1, &skybox);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);

    // Load skybox faces into texture.
    unsigned char* textureData;
    int textureWidth;
    int textureHeight;
    int textureChannels;

    for (unsigned i = 0; i < textureFaces.size(); ++i) {
        textureData = stbi_load(textureFaces[i].c_str(), &textureWidth, &textureHeight, &textureChannels, 0);

        if (textureData) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);
            stbi_image_free(textureData);
        }
        else {
            std::cerr << "Failed to load skybox texture: " << textureFaces[i] << std::endl;
            stbi_image_free(textureData);
            return 1;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // Initialize models.
//    std::vector<OpenGL::Model> models;
//    models.emplace_back(OpenGL::ObjectLoader::Instance().LoadFromFile("src/common/assets/models/bunny.obj"));

    std::vector<OpenGL::Sphere> spheres(256);

    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo); // Binding 1.
    glBufferData(GL_SHADER_STORAGE_BUFFER, 256 * sizeof(OpenGL::Sphere), nullptr, GL_STATIC_DRAW);

    std::size_t offset = 0;

    // Set data.
    // numSpheres.
    int numSpheres = 1;
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(int), &numSpheres);
    offset += sizeof(glm::vec4);

    for (OpenGL::Sphere& sphere : spheres) {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(OpenGL::Sphere), &sphere);

//        // Each sphere needs 3 floats worth of padding.
//        offset += sizeof(OpenGL::Sphere) + sizeof(float) * 3;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Necessary buffers for a full-screen quad.
    std::vector<glm::vec3> vertices = {
        { -1.0f, 1.0f, 0.0f },
        { -1.0f, -1.0f, 0.0f },
        { 1.0f, -1.0f, 0.0f },
        { 1.0f, 1.0f, 0.0f }
    };
    std::vector<unsigned> indices = { 0, 1, 2, 0, 2, 3 };

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Attach VBO.
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    // Attach EBO.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glBindVertexArray(0);

    // Timestep.
    float current;
    float previous = 0.0f;
    float dt = 0.0f;

    // Compile shaders.
    OpenGL::Shader shader { "Path Tracing Shader", { "src/samples/path-tracing/assets/shaders/fsq.vert",
                                                     "src/samples/path-tracing/assets/shaders/path_tracing.frag" } };
    shader.Bind();

    glBindVertexArray(vao);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);
    shader.SetUniform("skybox", 0);

    unsigned frame = 0;

    while ((glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) && (glfwWindowShouldClose(window) == 0)) {
        glfwPollEvents();

        // Moving camera.
        static const float cameraSpeed = 10.0f;
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

        // Update models.
        std::size_t bufferOffset = 0;
        glm::mat4 cameraTransform = camera.GetCameraTransform();

//        for (OpenGL::Model& model : models) {
//            std::size_t numBytes = static_cast<long>(model.triangles.size() * sizeof(OpenGL::Triangle));
//
//            if (model.IsDirty()) {
//                model.Recalculate(cameraTransform);
//                glBufferSubData(GL_SHADER_STORAGE_BUFFER, bufferOffset, numBytes, model.triangles.data());
//            }
//
//            bufferOffset += numBytes;
//        }

        shader.SetUniform("cameraPosition", camera.GetPosition());
        shader.SetUniform("inverseProjectionMatrix", glm::inverse(camera.GetPerspectiveTransform()));
        shader.SetUniform("inverseViewMatrix", glm::inverse(camera.GetViewTransform()));
        shader.SetUniform("imageResolution", glm::vec2(width, height));
        shader.SetUniform("frame", ++frame);
        shader.SetUniform("samplesPerPixel", 16);
        shader.SetUniform("numRayBounces", 16);

        // Rendering.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

        // Start the Dear ImGui frame.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        current = (float)glfwGetTime();
        dt = current - previous;
        previous = current;

        // Render ImGui on top of everything.
        // Framework overview.
        if (ImGui::Begin("Overview")) {
            ImGui::Text("Render time:");
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Save ImGui .ini settings.
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantSaveIniSettings) {
            ImGui::SaveIniSettingsToDisk(imGuiIni.c_str());

            // Manually change flag.
            io.WantSaveIniSettings = false;
        }

        glfwSwapBuffers(window);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindVertexArray(0);
    shader.Unbind();

    // Shutdown.
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ssbo);

    ImGui::SaveIniSettingsToDisk(imGuiIni.c_str());
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}
