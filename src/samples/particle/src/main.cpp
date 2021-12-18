
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
    std::string name = "Particle";

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

    // Compile shaders.
    std::string workingDirectory = std::filesystem::current_path().string();
    GLuint shader = OpenGL::LoadShader({ std::make_pair("src/samples/particle/assets/shaders/particle.vert", GL_VERTEX_SHADER), std::make_pair("src/samples/particle/assets/shaders/particle.frag", GL_FRAGMENT_SHADER) });

    // Allocate particles.
    int numParticles = 10000000;
    float range = 100.0f;

    std::vector<OpenGL::Particle> particles;
    particles.resize(numParticles);

    for (OpenGL::Particle& particle : particles) {
        glm::vec3 position (glm::linearRand(-range, range), glm::linearRand(-range, range), glm::linearRand(-range, range));
        particle.position = glm::vec4(position, 1.0f);
    }

    // Construct necessary buffers.
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
    camera.SetPosition(glm::vec3(0.0f, 0.0f, 5.0f));
    camera.SetTargetPosition(glm::vec3(0.0f));

    // Timestep.
    float current = 0.0f;
    float previous = 0.0f;
    float dt = 0.0f;

    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glPointSize(1.0f);

    // Bind shader uniforms.
    glUseProgram(shader);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBindVertexArray(vao);

    while ((glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) && (glfwWindowShouldClose(window) == 0)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUniform1f(glGetUniformLocation(shader, "dt"), dt);
        glUniform3fv(glGetUniformLocation(shader, "centerOfGravity"), 1, glm::value_ptr(glm::vec3(0.0f)));
        glUniformMatrix4fv(glGetUniformLocation(shader, "cameraTransform"), 1, GL_FALSE, glm::value_ptr(camera.GetCameraTransform()));

        glDrawArrays(GL_POINTS, 0, numParticles);

        current = (float)glfwGetTime();
        dt = current - previous;
        previous = current;

        glfwSwapBuffers(window);
    }

    glUseProgram(0);
    glDeleteBuffers(1, &ssbo);

    // Shutdown.
    glfwDestroyWindow(window);
    glfwTerminate();
}