
#ifndef OPENGL_SAMPLES_SHADER_H
#define OPENGL_SAMPLES_SHADER_H

#include "pch.h"

namespace OpenGL {

    class Shader {
        public:
            typedef std::pair<std::string, GLenum> ShaderComponent;

            // Compiles all shader components and returns a linked program.
            // Throws std::runtime_error on compilation error.

            // Takes explicit pairings of shader -> shader type.
            Shader(std::string name, std::initializer_list<ShaderComponent> shaderComponents);

            // Deduces shader type from file extension.
            // .vert - Vertex
            // .frag - Fragment
            // .geom - Geometry
            Shader(std::string name, std::initializer_list<std::string> shaderComponents);

            ~Shader();

            void Bind() const;
            void Unbind() const;

            template <typename DataType>
            void SetUniform(const std::string& uniformName, DataType data);

        private:
            template <typename DataType>
            void SetUniformData(GLuint uniformLocation, DataType data) const;

            void CreateShader(const std::vector<ShaderComponent>& components);

            [[nodiscard]] std::string ReadShaderFile(const std::string& filepath) const;
            [[nodiscard]] GLenum ShaderTypeFromExtension(const std::string& extension) const;

            [[nodiscard]] std::string ShaderTypeToString(GLenum shaderType) const;
            [[nodiscard]] GLuint CompileShaderComponent(const std::pair<std::string, GLenum>& shaderComponent) const;

            std::string name_;
            GLuint program_;
            std::unordered_map<std::string, GLint> uniformLocations_;
    };

}

#include "shader.tpp"

#endif //OPENGL_SAMPLES_SHADER_H
