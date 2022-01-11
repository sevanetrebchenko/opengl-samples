
#include "pch.h"
#include "utility.h"
#include "shader.h"
#include "transform.h"
#include "camera.h"
#include "object_loader.h"
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

    if (!std::filesystem::exists("src/samples/path-tracing/data")) {
        std::filesystem::create_directory("src/samples/path-tracing/data");
    }

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
    camera.SetPosition(glm::vec3(0.0f, 0.0f, 25.0f));

    float exposure = 1.0f;
    float apertureRadius = 0.2f;
    float focusDistance = glm::max(glm::distance(camera.GetPosition(), glm::vec3(0.0f)), 10.0f);
    bool focusOnClick = true;

    glViewport(0, 0, width, height);

    // Initialize skybox.
    // https://learnopengl.com/Advanced-OpenGL/Cubemaps
    std::vector<std::string> textureFaces = {
        "src/samples/path-tracing/assets/textures/skybox/water/pos_x.jpg",
        "src/samples/path-tracing/assets/textures/skybox/water/neg_x.jpg",
        "src/samples/path-tracing/assets/textures/skybox/water/pos_y.jpg",
        "src/samples/path-tracing/assets/textures/skybox/water/neg_y.jpg",
        "src/samples/path-tracing/assets/textures/skybox/water/pos_z.jpg",
        "src/samples/path-tracing/assets/textures/skybox/water/neg_z.jpg"
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

    // RGBA.
    std::vector<float> blankTexture;
    blankTexture.resize(width * height * 4, 0.0f);

    GLuint frame1;
    glGenTextures(1, &frame1);
    glBindTexture(GL_TEXTURE_2D, frame1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, blankTexture.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint frame2;
    glGenTextures(1, &frame2);
    glBindTexture(GL_TEXTURE_2D, frame2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, blankTexture.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint postProcessingFrame;
    glGenTextures(1, &postProcessingFrame);
    glBindTexture(GL_TEXTURE_2D, postProcessingFrame);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, blankTexture.data());
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
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frame1, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, frame2, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, postProcessingFrame, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Failed to initialize custom framebuffer on startup." << std::endl;
        return 1;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::vector<GLenum> drawBuffers(1);

    // Initialize global data UBO.
    GLuint ubo;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo); // Binding 0.

    // Inverse camera transforms (2x mat4), camera position (vec3, vec4 with padding).
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2 + sizeof(glm::vec4), nullptr, GL_STATIC_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Initialize scene objects.
    const int numSpheres = 256;
    std::vector<OpenGL::Sphere> spheres(numSpheres);
    int numActiveSpheres = 0;

    const int numAABBs = 256;
    std::vector<OpenGL::AABB> aabbs(numAABBs);
    int numActiveAABBs = 0;

    int index = 0;

    // 6x6 grid of spheres to showcase varying levels of both reflective materials and reflection roughness properties.
    {
        int side = 6;
        float radius = 2.0f;
        float gap = 1.0f;

        float length = (radius * 2.0f) * static_cast<float>(side) + gap * static_cast<float>(side);
        float offset = length / 2.0f;
        float delta = length / static_cast<float>(side - 1);

        for (int y = 0; y < side; ++y) {
            for (int x = 0; x < side; ++x) {
                OpenGL::Sphere& sphere = spheres[index++];
                sphere.radius = radius;
                sphere.position = glm::vec3(15.0f, static_cast<float>(y) * delta - offset, static_cast<float>(x) * delta - offset);

                // Configure material properties.
                OpenGL::Material& material = sphere.material;
                material.albedo = glm::vec3(1.0f);
                material.reflectionProbability = static_cast<float>(side - 1 - x) / (static_cast<float>(side - 1));
                material.reflectionRoughness = static_cast<float>(y) / (static_cast<float>(side - 1));

                ++numActiveSpheres;
            }
        }
    }

    // 1x6 grid of spheres to showcase refractive materials with varying levels of absorbance (Beer's Law).
    {
        int side = 6;
        float radius = 2.0f;
        float gap = 1.0f;

        float length = (radius * 2.0f) * static_cast<float>(side) + gap * static_cast<float>(side);
        float offset = length / 2.0f;
        float delta = length / static_cast<float>(side - 1);

        for (int i = 0; i < side; ++i) {
            OpenGL::Sphere& sphere = spheres[index++];
            sphere.radius = radius;
            sphere.position = glm::vec3(-15.0f, length / 4.0f, static_cast<float>(i) * delta - offset);

            // Configure material properties.
            OpenGL::Material& material = sphere.material;
            material.albedo = glm::vec3(0.90f, 0.25f, 0.25f);
            material.ior = 1.05f;
            material.refractionProbability = 0.98f;
            material.absorbance = glm::vec3(1.0f, 2.0f, 3.0f) * (static_cast<float>(i) / static_cast<float>(side));
            material.reflectionProbability = 0.02f;

            ++numActiveSpheres;
        }
    }

    // 1x6 grid of spheres to showcase refractive materials with varying levels of refraction roughness.
    {
        int side = 6;
        float radius = 2.0f;
        float gap = 1.0f;

        float length = (radius * 2.0f) * static_cast<float>(side) + gap * static_cast<float>(side);
        float offset = length / 2.0f;
        float delta = length / static_cast<float>(side - 1);

        for (int i = 0; i < side; ++i) {
            OpenGL::Sphere& sphere = spheres[index++];
            sphere.radius = radius;
            sphere.position = glm::vec3(-15.0f, -length / 4.0f, static_cast<float>(i) * delta - offset);

            // Configure material properties.
            OpenGL::Material& material = sphere.material;
            material.ior = 1.1f;
            material.refractionProbability = 0.98f;
            material.refractionRoughness = (static_cast<float>(side - 1 - i) / static_cast<float>(side));
            material.reflectionProbability = 0.02f;
            material.reflectionRoughness = (static_cast<float>(i) / static_cast<float>(side));

            ++numActiveSpheres;
        }
    }

    index = 0;

    // Box consisting of 6 thin AABB slabs around the demo scene.
    {
        float epsilon = 0.01f;
        float boxHeight = 40.0f;
        float boxWidth = 40.0f;
        float boxDepth = 56.0f;

        // Right wall (green).
        {
            OpenGL::AABB& wall = aabbs[index++];
            wall.position = glm::vec4(boxWidth / 2.0f, 0.0f, 0.0f, 1.0f);
            wall.dimensions = glm::vec4(epsilon, boxHeight / 2.0f + epsilon, boxDepth / 2.0f + epsilon, 0.0f);

            OpenGL::Material& material = wall.material;
            material.albedo = glm::vec3(0.37f, 0.67f, 0.37f);
            material.reflectionProbability = 1.0f;
            material.reflectionRoughness = 0.6f;

            ++numActiveAABBs;
        }

        // Left wall (transparent).
        {
            OpenGL::AABB& wall = aabbs[index++];
            wall.position = glm::vec4(-boxWidth / 2.0f, 0.0f, 0.0f, 1.0f);
            wall.dimensions = glm::vec4(epsilon, boxHeight / 2.0f + epsilon, boxDepth / 2.0f + epsilon, 0.0f);

            OpenGL::Material& material = wall.material;
            material.albedo = glm::vec3(1.0f);
            material.ior = 1.52f; // Glass.
            material.refractionProbability = 1.0f;
            material.absorbance = glm::vec3(0.1f);

            ++numActiveAABBs;
        }

        // Back wall (blue).
        {
            OpenGL::AABB& wall = aabbs[index++];
            wall.position = glm::vec4(0.0f, 0.0f, boxDepth / 2.0f, 1.0f);
            wall.dimensions = glm::vec4(boxWidth / 2.0f + epsilon, boxHeight / 2.0f + epsilon, epsilon, 0.0f);

            OpenGL::Material& material = wall.material;
            material.albedo = glm::vec3(0.07f, 0.25f, 0.45f);
            material.reflectionProbability = 1.0f;
            material.reflectionRoughness = 0.6f;

            ++numActiveAABBs;
        }

        // Front wall (reflective).
        {
            OpenGL::AABB& wall = aabbs[index++];
            wall.position = glm::vec4(0.0f, 0.0f, -boxDepth / 2.0f, 1.0f);
            wall.dimensions = glm::vec4(boxWidth / 2.0f + epsilon, boxHeight / 2.0f + epsilon, epsilon, 0.0f);

            OpenGL::Material& material = wall.material;
            material.albedo = glm::vec3(0.95f, 0.75f, 0.30f);
            material.reflectionProbability = 1.0f;
            material.reflectionRoughness = 0.25f;

            ++numActiveAABBs;
        }

        // Floor (red).
        {
            OpenGL::AABB& wall = aabbs[index++];

            wall.position = glm::vec4(0.0f, -boxHeight / 2.0f, 0.0f, 1.0f);
            wall.dimensions = glm::vec4(boxWidth / 2.0f + epsilon, epsilon, boxDepth / 2.0f + epsilon, 0.0f);

            OpenGL::Material& material = wall.material;
            material.albedo = glm::vec3(0.2f, 0.04f, 0.04f);
            material.reflectionProbability = 1.0f;
            material.reflectionRoughness = 0.6f;

            ++numActiveAABBs;
        }

        // Ceiling (transparent).
        {
            OpenGL::AABB& wall = aabbs[index++];
            wall.position = glm::vec4(0.0f, boxHeight / 2.0f, 0.0f, 1.0f);
            wall.dimensions = glm::vec4(boxWidth / 2.0f + epsilon, epsilon, boxDepth / 2.0f + epsilon, 0.0f);

            OpenGL::Material& material = wall.material;
            material.ior = 1.52f; // Glass.
            material.refractionProbability = 1.0f;
            material.absorbance = glm::vec3(0.1f);

            ++numActiveAABBs;
        }

        // Light.
        {
            OpenGL::AABB& light = aabbs[index++];
            light.position = glm::vec4(0.0f, boxHeight / 2.0f - 2.0f, 0.0f, 1.0f);
            light.dimensions = glm::vec4(boxWidth / 6.0f, epsilon, boxDepth / 6.0f, 0.0f);

            OpenGL::Material& material = light.material;
            material.emissive = glm::vec3(1.0f);
            material.emissiveStrength = 15.0f;
            material.reflectionProbability = 1.0f;

            ++numActiveAABBs;
        }
    }

    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo); // Binding 1.

    // Number of active spheres (int, vec4 with padding), array of 256 spheres.
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) + numSpheres * sizeof(OpenGL::Sphere) + sizeof(glm::vec4) + numAABBs * sizeof(OpenGL::AABB), nullptr, GL_STATIC_DRAW);

    {
        std::size_t offset = 0;

        // Set sphere object data.
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(int), &numActiveSpheres);
        offset += sizeof(glm::vec4);

        for (int i = 0; i < numActiveSpheres; ++i) {
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(OpenGL::Sphere), &spheres[i]);
            offset += sizeof(OpenGL::Sphere);
        }
        offset += (numSpheres - numActiveSpheres) * sizeof(OpenGL::Sphere);

        // Set AABB object data.
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(int), &numActiveAABBs);
        offset += sizeof(glm::vec4);

        for (int i = 0; i < numActiveAABBs; ++i) {
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(OpenGL::AABB), &aabbs[i]);
            offset += sizeof(OpenGL::AABB);
        }
        offset += (numAABBs - numActiveAABBs) * sizeof(OpenGL::AABB);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Initialize necessary buffers for a full-screen quad.
    std::vector<glm::vec3> vertices = {
        { -1.0f, 1.0f, 0.0f },
        { -1.0f, -1.0f, 0.0f },
        { 1.0f, -1.0f, 0.0f },
        { 1.0f, 1.0f, 0.0f }
    };
    std::vector<glm::vec2> uv = {
        { 0.0f, 1.0f },
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
    };
    std::vector<unsigned> indices = { 0, 1, 2, 0, 2, 3 };

    GLuint verticesVBO;
    glGenBuffers(1, &verticesVBO);
    glBindBuffer(GL_ARRAY_BUFFER, verticesVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint uvVBO;
    glGenBuffers(1, &uvVBO);
    glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
    glBufferData(GL_ARRAY_BUFFER, uv.size() * sizeof(uv[0]), uv.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Attach VBOs.
    glBindBuffer(GL_ARRAY_BUFFER, verticesVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);

    // Attach EBO.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glBindVertexArray(0);

    // Timestep.
    float current;
    float previous = 0.0f;
    float dt = 0.0f;

    // Compile shaders.
    OpenGL::Shader pathTracingShader { "Path Tracing", { "src/samples/path-tracing/assets/shaders/fsq.vert",
                                                         "src/samples/path-tracing/assets/shaders/path_tracing.frag" } };
    OpenGL::Shader postProcessingShader { "Post Processing", { "src/samples/path-tracing/assets/shaders/fsq.vert",
                                                               "src/samples/path-tracing/assets/shaders/post_processing.frag" } };

    int frameCounter = 0;
    int samplesPerPixel = 1;
    int numRayBounces = 16;

    glBindVertexArray(vao);

    while ((glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) && (glfwWindowShouldClose(window) == 0)) {
        glfwPollEvents();

        // Start the Dear ImGui frame.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        bool refreshRenderTargets = false;

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
            glBindTexture(GL_TEXTURE_2D, frame1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, blankTexture.data());
            glBindTexture(GL_TEXTURE_2D, 0);

            glBindTexture(GL_TEXTURE_2D, frame2);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, blankTexture.data());
            glBindTexture(GL_TEXTURE_2D, 0);

            glBindTexture(GL_TEXTURE_2D, postProcessingFrame);
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
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frame1, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, frame2, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, postProcessingFrame, 0);
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
        const float cameraSpeed = 10.0f;
        const glm::vec3& cameraPosition = camera.GetPosition();
        const glm::vec3& cameraForwardVector = camera.GetForwardVector();
        const glm::vec3& cameraUpVector = camera.GetUpVector();

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && !io.WantCaptureKeyboard) {
            camera.SetPosition(cameraPosition + cameraSpeed * cameraForwardVector * dt);
            refreshRenderTargets = true;
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && !io.WantCaptureKeyboard) {
            camera.SetPosition(cameraPosition - cameraSpeed * cameraForwardVector * dt);
            refreshRenderTargets = true;
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && !io.WantCaptureKeyboard) {
            camera.SetPosition(cameraPosition - glm::normalize(glm::cross(cameraForwardVector, cameraUpVector)) * cameraSpeed * dt);
            refreshRenderTargets = true;
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && !io.WantCaptureKeyboard) {
            camera.SetPosition(cameraPosition + glm::normalize(glm::cross(cameraForwardVector, cameraUpVector)) * cameraSpeed * dt);
            refreshRenderTargets = true;
        }

        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && !io.WantCaptureKeyboard) {
            camera.SetPosition(cameraPosition + cameraSpeed * cameraUpVector * dt);
            refreshRenderTargets = true;
        }

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS && !io.WantCaptureKeyboard) {
            camera.SetPosition(cameraPosition - cameraSpeed * cameraUpVector * dt);
            refreshRenderTargets = true;
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

            if (glm::abs(dx) > std::numeric_limits<float>::epsilon() || glm::abs(dy) > std::numeric_limits<float>::epsilon()) {
                refreshRenderTargets = true;
            }

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

        bool isCameraDirty = camera.IsDirty();
        glm::mat4 inverseProjectionMatrix = glm::inverse(camera.GetPerspectiveTransform());
        glm::mat4 inverseViewMatrix = glm::inverse(camera.GetViewTransform());

        static bool sphereSelected = false;
        static bool aabbSelected = false;
        static int currentSelectedObjectIndex = -1;

        // Mouse picking.
        static int previousState = GLFW_RELEASE;
        int newState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        if ((newState == GLFW_RELEASE && previousState == GLFW_PRESS) && !io.WantCaptureMouse) {
            sphereSelected = false;
            aabbSelected = false;

            int previouslySelectedObjectIndex = currentSelectedObjectIndex;
            bool selectedObject = false;

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
            rayDirection = inverseProjectionMatrix * glm::vec4(ndc, -1.0f, 1.0f);
            rayDirection = glm::vec4(glm::vec2(rayDirection), -1.0f, 0.0f);
            rayDirection = glm::normalize(glm::vec3(inverseViewMatrix * glm::vec4(rayDirection, 0.0f)));

            // Intersect with all scene objects, get the closest intersection.
            float tMin = 0.001f;
            float tMax = std::numeric_limits<float>::max();

            // Intersect with all active spheres.
            for (int i = 0; i < numActiveSpheres; ++i) {
                OpenGL::Sphere& temp = spheres[i];

                glm::vec3 sphereToRayOrigin = rayOrigin - temp.position;

                // https://antongerdelan.net/opengl/raycasting.html
                float b = dot(rayDirection, sphereToRayOrigin);
                float c = dot(sphereToRayOrigin, sphereToRayOrigin) - (temp.radius * temp.radius);

                float discriminant = b * b - c;
                if (discriminant < 0.0) {
                    // No real roots, no intersection.
                    continue;
                }

                float sqrtDiscriminant = sqrt(discriminant);

                float t1 = -b - sqrtDiscriminant;
                float t2 = -b + sqrtDiscriminant;

                if (t2 < 0.0) {
                    // Ray exited behind the origin (sphere is behind the camera).
                    continue;
                }

                // Bounds check.
                float t = t1 < 0.0 ? t2 : t1;
                if (t < tMin || t > tMax) {
                    // Closer intersection has already been found.
                    continue;
                }

                // Update bounds.
                tMax = t;

                sphereSelected = true;
                aabbSelected = false;

                currentSelectedObjectIndex = i;
                selectedObject = true;
            }

            // Intersect with all active AABBs.
            for (int i = 0; i < numActiveAABBs; ++i) {
                OpenGL::AABB& temp = aabbs[i];

                glm::vec3 minimum = temp.position - temp.dimensions;
                glm::vec3 maximum = temp.position + temp.dimensions;

                // Taken from Real Time Collision Detection, Chapter 5.
                float currentTMin = 0.0f;
                float currentTMax = std::numeric_limits<float>::max();

                bool invalid = false;

                for (int axis = 0; axis < 3; ++axis) {
                    if (glm::abs(rayDirection[axis]) < std::numeric_limits<float>::epsilon()) {
                        // Ray is parallel to the slab, no intersection can happen unless the origin is within the bounds of the slab.
                        if (rayOrigin[axis] < minimum[axis] || rayOrigin[axis] > maximum[axis]) {
                            invalid = true;
                            break;
                        }
                    }
                    else {
                        // Compute times at which ray enters and leaves the slab.
                        float inverseDirection = 1.0f / rayDirection[axis];
                        float t1 = (minimum[axis] - rayOrigin[axis]) * inverseDirection;
                        float t2 = (maximum[axis] - rayOrigin[axis]) * inverseDirection;

                        // Make t1 be the intersection time with the near plane, t2 be the intersection time with the far plane.
                        if (t1 > t2) {
                            std::swap(t1, t2);
                        }

                        currentTMin = glm::max(currentTMin, t1);
                        currentTMax = glm::min(currentTMax, t2);

                        if (currentTMin > currentTMax) {
                            invalid = true;
                            break;
                        }
                    }
                }

                if (invalid) {
                    continue;
                }

                // Intersection time is currentTMin.
                // Bounds check.
                if (currentTMin < tMin || currentTMin > tMax) {
                    // Closer intersection has already been found.
                    continue;
                }

                // Update bounds.
                tMax = currentTMin;

                aabbSelected = true;
                sphereSelected = false;

                currentSelectedObjectIndex = i;
                selectedObject = true;
            }

            if (selectedObject) {
                if (previouslySelectedObjectIndex != currentSelectedObjectIndex) {
                    refreshRenderTargets = true;
                }
            }
            else {
                currentSelectedObjectIndex = -1;
            }

            previouslySelectedObjectIndex = currentSelectedObjectIndex;
        }

        previousState = newState;

        if (ImGui::Begin("Scene Objects")) {
            if (sphereSelected) {
                OpenGL::Sphere& object = spheres[currentSelectedObjectIndex];

                if (focusOnClick) {
                    focusDistance = glm::distance(object.position, cameraPosition);
                }

                bool updateGPUData = object.OnImGui();
                if (updateGPUData) {
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
                    glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) + (currentSelectedObjectIndex) * sizeof(OpenGL::Sphere), sizeof(OpenGL::Sphere), &object);
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

                    refreshRenderTargets = true;
                }
            }
            else if (aabbSelected) {
                OpenGL::AABB& object = aabbs[currentSelectedObjectIndex];

                if (focusOnClick) {
                    focusDistance = glm::distance(glm::vec3(object.position), cameraPosition);
                }

                bool updateGPUData = object.OnImGui();
                if (updateGPUData) {
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
                    glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) + numSpheres * sizeof(OpenGL::Sphere) + sizeof(glm::vec4) + currentSelectedObjectIndex * sizeof(OpenGL::AABB), sizeof(OpenGL::AABB), &object);
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

                    refreshRenderTargets = true;
                }
            }
            else {
                // No object currently selected.
                ImGui::PushStyleColor(ImGuiCol_Text, 0xff999999);
                ImGui::Text("No object selected.");
                ImGui::PopStyleColor();
            }
        }
        ImGui::End();

        // Update camera transformation matrices.
        if (isCameraDirty) {
            int offset = 0;

            glBindBuffer(GL_UNIFORM_BUFFER, ubo);

            // Inverse projection matrix.
            glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(glm::mat4), glm::value_ptr(inverseProjectionMatrix));
            offset += sizeof(glm::mat4);

            // Inverse view matrix.
            glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(glm::mat4), glm::value_ptr(inverseViewMatrix));
            offset += sizeof(glm::mat4);

            glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(glm::vec3), glm::value_ptr(camera.GetPosition()));

            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }

        // Sample overview and statistics.
        if (ImGui::Begin("Sample Overview")) {
            ImGui::PushStyleColor(ImGuiCol_Text, 0xff999999);

            ImGui::Text("Render time:");
            ImGui::Text("%.3f ms/frame (%.1f FPS)", dt * 1000.0f, 1.0f / dt);

            static char outputFilename[256] = { "result" };

            if (ImGui::Button("Take Screenshot")) {
                static std::string outputDirectory = "src/samples/path-tracing/data/screenshots/";

                if (!std::filesystem::exists(outputDirectory)) {
                    // Create output directory if it doesn't exist.
                    std::filesystem::create_directory(outputDirectory);
                }

                // Save final output image.
                int channels = 4;
                std::vector<unsigned char> pixels(width * height * channels);

                // Read in OpenGL texture data.
                glBindTexture(GL_TEXTURE_2D, postProcessingFrame);
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

        // Configuration of path tracing options.
        if (ImGui::Begin("Path Tracing Options")) {
            ImGui::PushStyleColor(ImGuiCol_Text, 0xff999999);

            ImGui::Text("Samples per pixel:");
            int tempSamplesPerPixel = samplesPerPixel;
            if (ImGui::SliderInt("##spp", &tempSamplesPerPixel, 1, 64)) {
                // Manual input can go outside the valid range.
                tempSamplesPerPixel = glm::clamp(tempSamplesPerPixel, 1, 64);

                if (tempSamplesPerPixel != samplesPerPixel) {
                    samplesPerPixel = tempSamplesPerPixel;
                    refreshRenderTargets = true;
                }
            }

            ImGui::Text("Number of ray bounces:");
            int tempNumRayBounces = numRayBounces;
            if (ImGui::SliderInt("##numRayBounces", &tempNumRayBounces, 2, 64)) {
                // Manual input can go outside the valid range.
                tempNumRayBounces = glm::clamp(tempNumRayBounces, 2, 64);

                if (tempNumRayBounces != numRayBounces) {
                    numRayBounces = tempNumRayBounces;
                    refreshRenderTargets = true;
                }
            }

            ImGui::PopStyleColor();
        }
        ImGui::End();

        // Configuration of scene camera.
        if (ImGui::Begin("Camera Options")) {
            ImGui::PushStyleColor(ImGuiCol_Text, 0xff999999);

            ImGui::Text("Exposure:");
            float tempExposure = exposure;
            if (ImGui::SliderFloat("##exposure", &tempExposure, 0.1f, 5.0f)) {
                // Manual input can go outside the valid range.
                tempExposure = glm::clamp(tempExposure, 0.1f, 5.0f);

                if (glm::abs(tempExposure - exposure) > std::numeric_limits<float>::epsilon()) {
                    exposure = tempExposure;
                    refreshRenderTargets = true;
                }
            }

            ImGui::Text("Aperture Diameter:");
            float apertureDiameter = apertureRadius * 2.0f;
            if (ImGui::SliderFloat("##apertureDiameter", &apertureDiameter, 0.0f, 10.0f)) {
                // Manual input can go outside the valid range.
                apertureDiameter = glm::clamp(apertureDiameter, 0.0f, 10.0f);

                if (glm::abs(apertureDiameter - (apertureRadius * 2.0f)) > std::numeric_limits<float>::epsilon()) {
                    apertureRadius = apertureDiameter / 2.0f;
                    refreshRenderTargets = true;
                }
            }

            ImGui::Text("Focus Distance:");
            float tempFocusDistance = focusDistance;
            if (ImGui::SliderFloat("##focusDistance", &tempFocusDistance, 2.0f, 100.0f)) {
                // Manual input can go outside the valid range.
                tempFocusDistance = glm::clamp(tempFocusDistance, 2.0f, 100.0f);

                if (glm::abs(tempFocusDistance - focusDistance) > std::numeric_limits<float>::epsilon()) {
                    focusDistance = tempFocusDistance;
                    refreshRenderTargets = true;
                }
            }

            ImGui::Checkbox("Focus Selected Object?", &focusOnClick);

            ImGui::PopStyleColor();
        }
        ImGui::End();

        pathTracingShader.Bind();

        pathTracingShader.SetUniform("frameCounter", frameCounter);
        pathTracingShader.SetUniform("samplesPerPixel", samplesPerPixel);
        pathTracingShader.SetUniform("numRayBounces", numRayBounces);
        pathTracingShader.SetUniform("focusDistance", focusDistance);
        pathTracingShader.SetUniform("apertureRadius", apertureRadius);

        int previousFrameIndex = (frameCounter + 1) % 2;
        int currentFrameIndex = (frameCounter % 2);

        GLuint previousFrameImage = previousFrameIndex == 0 ? frame1 : frame2;
        GLuint currentFrameImage = currentFrameIndex == 0 ? frame1 : frame2;

        // For the best visual clarity, de-noising textures need to be reset when anything in the scene configuration changes.
        if (refreshRenderTargets) {
            glBindTexture(GL_TEXTURE_2D, previousFrameImage);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, blankTexture.data());
            glBindTexture(GL_TEXTURE_2D, 0);

            frameCounter = 0;
        }



        // Render to intermediate textures.
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // Determine which texture is the previous frame and which texture is the current frame.
        // Previous frame is readonly for de-noising, current frame will be copied into the default framebuffer for rendering.
        glActiveTexture(GL_TEXTURE0);
        glBindImageTexture(0, previousFrameImage, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        pathTracingShader.SetUniform("previousFrameImage", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);
        pathTracingShader.SetUniform("skyboxTexture", 1);

        drawBuffers[0] = GL_COLOR_ATTACHMENT0 + currentFrameIndex;
        glDrawBuffers(1, drawBuffers.data());

        // Render to FBO attachment.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
        pathTracingShader.Unbind();



        // Render to final texture.
        postProcessingShader.Bind();

        glActiveTexture(GL_TEXTURE0);
        glBindImageTexture(0, currentFrameImage, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        postProcessingShader.SetUniform("finalImage", 0);

        postProcessingShader.SetUniform("exposure", exposure);

        drawBuffers[0] = GL_COLOR_ATTACHMENT2; // Color attachment 2 is always the render target for final output.
        glDrawBuffers(1, drawBuffers.data());

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

        postProcessingShader.Unbind();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);



        // Render final output to screen.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glNamedFramebufferReadBuffer(fbo, GL_COLOR_ATTACHMENT2); // Set the read buffer to be the render attachment of the final output.

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glBlitFramebuffer(0, 0, width, height,
                          0, 0, width, height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);



        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Save ImGui .ini settings.
        if (io.WantSaveIniSettings) {
            ImGui::SaveIniSettingsToDisk(imGuiIni.c_str());

            // Manually change flag.
            io.WantSaveIniSettings = false;
        }

        glfwSwapBuffers(window);

        ++frameCounter %= INT_MAX;

        current = (float)glfwGetTime();
        dt = current - previous;
        previous = current;
    }

    // Shutdown.
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &uvVBO);
    glDeleteBuffers(1, &verticesVBO);
    glDeleteBuffers(1, &ssbo);
    glDeleteBuffers(1, &ubo);
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteTextures(1, &postProcessingFrame);
    glDeleteTextures(1, &frame2);
    glDeleteTextures(1, &frame1);
    glDeleteTextures(1, &skybox);

    ImGui::SaveIniSettingsToDisk(imGuiIni.c_str());
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}
