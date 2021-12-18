
#include "shader.h"
#include "utility.h"

namespace OpenGL {

    Shader::Shader(std::initializer_list<std::pair<std::string, GLenum>> shaderComponents) {
        if (empty(shaderComponents)) {
            throw std::runtime_error("LoadShader called with no shader components.");
        }

        program_ = glCreateProgram();
        unsigned numShaderComponents = shaderComponents.size();
        GLuint* shaders = new GLenum[numShaderComponents];
        unsigned currentShaderIndex = 0;

        //--------------------------------------------------------------------------------------------------------------
        // SHADER COMPONENT COMPILING
        //--------------------------------------------------------------------------------------------------------------
        for (const std::pair<std::string, GLenum>& shaderComponent : shaderComponents) {
            GLuint shader = CompileShaderComponent(shaderComponent);

            // Shader component successfully compiled.
            glAttachShader(program_, shader);
            shaders[currentShaderIndex++] = shader;
        }

        //--------------------------------------------------------------------------------------------------------------
        // SHADER PROGRAM LINKING
        //--------------------------------------------------------------------------------------------------------------
        glLinkProgram(program_);

        GLint isLinked = 0;
        glGetProgramiv(program_, GL_LINK_STATUS, &isLinked);
        if (!isLinked) {
            // Shader failed to link - get error information from OpenGL.
            GLint errorMessageLength = 0;
            glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &errorMessageLength);

            std::vector<GLchar> errorMessageBuffer;
            errorMessageBuffer.resize(errorMessageLength + 1);
            glGetProgramInfoLog(program_, errorMessageLength, nullptr, &errorMessageBuffer[0]);
            std::string errorMessage(errorMessageBuffer.begin(), errorMessageBuffer.end());

            // Program is unnecessary at this point.
            glDeleteProgram(program_);

            // Delete shader types.
            for (int i = 0; i < numShaderComponents; ++i) {
                glDeleteShader(shaders[i]);
            }

            throw std::runtime_error("Shader failed to link. Provided error information: " + errorMessage);
        }

        // Shader types are no longer necessary.
        for (int i = 0; i < numShaderComponents; ++i) {
            GLuint shaderComponentID = shaders[i];
            glDetachShader(program_, shaderComponentID);
            glDeleteShader(shaderComponentID);
        }
    }

    Shader::~Shader() {
        glDeleteProgram(program_);
    }

    void Shader::Bind() const {
        glUseProgram(program_);
    }

    void Shader::Unbind() const {
        glUseProgram(0);
    }

    std::string Shader::ShaderTypeToString(GLenum shaderType) const {
        switch(shaderType) {
            case GL_FRAGMENT_SHADER:
                return "FRAGMENT";
            case GL_VERTEX_SHADER:
                return "VERTEX";
            case GL_GEOMETRY_SHADER:
                return "GEOMETRY";
            default:
                return "";
        }
    }

    GLuint Shader::CompileShaderComponent(const std::pair<std::string, GLenum>& shaderComponent) const {
        std::string shaderFilePath = ConvertToNativeSeparators(shaderComponent.first);
        GLenum shaderType = shaderComponent.second;

        // Read in shader source.
        static std::stringstream fileBuffer;
        std::ifstream fileReader;

        // Open the file.
        fileReader.open(shaderFilePath);
        if (fileReader.is_open()) {
            fileBuffer << fileReader.rdbuf();
        }
        else {
            // Could not open file
            throw std::runtime_error("Could not open shader file: \"" + shaderFilePath + "\"");
        }

        std::string fileContents = fileBuffer.str();
        fileBuffer.str(std::string());
        fileBuffer.clear(); // Reset fail and eof bits.

        const GLchar* shaderSource = reinterpret_cast<const GLchar*>(fileContents.c_str());

        // Create shader from source.
        GLuint shader = glCreateShader(shaderType);
        glShaderSource(shader, 1, &shaderSource, nullptr); // If length is NULL, each string is assumed to be null terminated.
        glCompileShader(shader);

        // Compile shader source code.
        GLint isCompiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
        if (!isCompiled) {
            // Shader failed to compile - get error information from OpenGL.
            GLint errorMessageLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &errorMessageLength);

            std::vector<GLchar> errorMessageBuffer;
            errorMessageBuffer.resize(errorMessageLength + 1);
            glGetShaderInfoLog(shader, errorMessageLength, nullptr, &errorMessageBuffer[0]);
            std::string errorMessage(errorMessageBuffer.begin(), errorMessageBuffer.end());

            glDeleteShader(shader);
            throw std::runtime_error("Shader: " + shaderFilePath + " failed to compile " + ShaderTypeToString(shaderType) + " component (" + shaderFilePath + "). Provided error information: " + errorMessage);
        }

        return shader;
    }

}

