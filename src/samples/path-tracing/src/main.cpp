
#include "pch.h"
#include "shader.h"
#include "camera.h"
#include "object_loader.h"
#include "triangle.h"

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
    std::string imGuiIni = "src/samples/path-tracing/data/imgui_layout.ini";
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

    // Initialize models.
    OpenGL::Mesh bunny = OpenGL::ObjectLoader::Instance().LoadFromFile("src/common/assets/models/bunny.obj");

    // Break model up into individual triangles.
    std::size_t numTriangles = bunny.indices.size() / 3;

    std::vector<OpenGL::Triangle> triangles;
    triangles.reserve(numTriangles);

    for (unsigned i = 0; i < numTriangles; ++i) {
        triangles.emplace_back(OpenGL::Triangle(bunny.vertices[bunny.indices[i * 3 + 0]],
                                                bunny.vertices[bunny.indices[i * 3 + 1]],
                                                bunny.vertices[bunny.indices[i * 3 + 2]]));
    }

    GLuint ubo;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, numTriangles * sizeof(OpenGL::Triangle), triangles.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

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
    OpenGL::Shader shader { "Path Tracing Shader", { "src/samples/path-tracing/assets/shaders/fsq.vert", "src/samples/path-tracing/assets/shaders/path_tracing.frag" } };
    shader.Bind();

    glBindVertexArray(vao);

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

    glBindVertexArray(0);
    shader.Unbind();

    // Shutdown.
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ubo);

    ImGui::SaveIniSettingsToDisk(imGuiIni.c_str());
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}
