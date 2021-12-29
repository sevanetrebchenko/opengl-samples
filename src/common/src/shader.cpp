
#include "shader.h"
#include "utility.h"

namespace OpenGL {

    Shader::Shader(std::string name, std::initializer_list<ShaderComponent> shaderComponents) : name_(std::move(name)) {
        CreateShader(shaderComponents);
    }

    Shader::Shader(std::string name, std::initializer_list<std::string> shaderComponents) : name_(std::move(name)) {
        std::vector<ShaderComponent> components;

        for (const std::string& component : shaderComponents) {
            GLenum shaderType = ShaderTypeFromExtension(GetAssetExtension(component));
            components.emplace_back(ShaderComponent(component, shaderType));
        }

        CreateShader(components);
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
        std::string fileContents = ReadShaderFile(shaderFilePath);
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

    std::string Shader::ReadShaderFile(const std::string& filepath) const {
        static std::stringstream fileBuffer;
        std::ifstream fileReader;

        // Open the file.
        fileReader.open(filepath);
        if (fileReader.is_open()) {
            fileBuffer << fileReader.rdbuf();
        }
        else {
            // Could not open file
            throw std::runtime_error("Could not open shader file: \"" + filepath + "\"");
        }

        std::string fileContents = fileBuffer.str();
        fileBuffer.str(std::string()); // Clear buffer contents.
        fileBuffer.clear(); // Reset fail and eof bits.

        return std::move(fileContents);
    }

    GLenum Shader::ShaderTypeFromExtension(const std::string &extension) const {
        if (extension == "vert") {
            return GL_VERTEX_SHADER;
        }
        if (extension == "frag") {
            return GL_FRAGMENT_SHADER;
        }
        if (extension == "geom") {
            return GL_GEOMETRY_SHADER;
        }

        return GL_INVALID_VALUE;
    }

    void Shader::CreateShader(const std::vector<ShaderComponent>& components) {
        if (components.empty()) {
            throw std::runtime_error("CreateShader called with no shader components.");
        }

        program_ = glCreateProgram();
        unsigned numShaderComponents = components.size();
        std::vector<GLuint> shaders(numShaderComponents);
        unsigned currentShaderIndex = 0;

        //--------------------------------------------------------------------------------------------------------------
        // SHADER COMPONENT COMPILING
        //--------------------------------------------------------------------------------------------------------------
        for (const std::pair<std::string, GLenum>& component : components) {
            GLuint shader = CompileShaderComponent(component);

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
            for (unsigned i = 0; i < numShaderComponents; ++i) {
                glDeleteShader(shaders[i]);
            }

            throw std::runtime_error("Shader failed to link. Provided error information: " + errorMessage);
        }

        // Shader types are no longer necessary.
        for (GLuint shaderComponentID : shaders) {
            glDetachShader(program_, shaderComponentID);
            glDeleteShader(shaderComponentID);
        }
    }

}

