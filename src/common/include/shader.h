
#ifndef OPENGL_SAMPLES_SHADER_H
#define OPENGL_SAMPLES_SHADER_H

#include "pch.h"

namespace OpenGL {

    class Shader {
        public:
            // Compiles all shader components and returns a linked program.
            // Throws std::runtime_error on compilation error.
            Shader(std::initializer_list<std::pair<std::string, GLenum>> shaderComponents);
            ~Shader();

            void Bind() const;
            void Unbind() const;

            template <typename DataType>
            void SetUniform(const std::string& uniformName, DataType data);

        private:
            template <typename DataType>
            void SetUniformData(GLuint uniformLocation, DataType data) const;

            [[nodiscard]] std::string ShaderTypeToString(GLenum shaderType) const;
            [[nodiscard]] GLuint CompileShaderComponent(const std::pair<std::string, GLenum>& shaderComponent) const;

            GLuint program_;
            std::unordered_map<std::string, GLint> uniformLocations_;
    };

}

#include "shader.tpp"

#endif //OPENGL_SAMPLES_SHADER_H
