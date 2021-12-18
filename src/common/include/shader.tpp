
#ifndef OPENGL_SAMPLES_SHADER_TPP
#define OPENGL_SAMPLES_SHADER_TPP

namespace OpenGL {

    template <typename DataType>
    void Shader::SetUniform(const std::string& uniformName, DataType data) {
        auto uniformLocation = uniformLocations_.find(uniformName);

        // Location not found.
        if (uniformLocation == uniformLocations_.end()) {
            // Find location first.
            GLint location = glGetUniformLocation(program_, uniformName.c_str());
            uniformLocations_.emplace(uniformName, location);

            SetUniformData(location, data);
        }
        else {
            SetUniformData(uniformLocation->second, data);
        }
    }

    template<typename DataType>
    void Shader::SetUniformData(GLuint uniformLocation, DataType data) const {
        // BOOL, INT
        if constexpr (std::is_same_v<DataType, int> || std::is_same_v<DataType, bool>) {
            glUniform1i(uniformLocation, data);
        }
        // FLOAT
        else if constexpr (std::is_same_v<DataType, float>) {
            glUniform1f(uniformLocation, data);
        }
        // VEC2
        else if constexpr (std::is_same_v<DataType, glm::vec2>) {
            glUniform2fv(uniformLocation, 1, glm::value_ptr(data));
        }
        // VEC3
        else if constexpr (std::is_same_v<DataType, glm::vec3>) {
            glUniform3fv(uniformLocation, 1, glm::value_ptr(data));
        }
        // VEC4
        else if constexpr (std::is_same_v<DataType, glm::vec4>) {
            glUniform4fv(uniformLocation, 1, glm::value_ptr(data));
        }
        // MAT3
        else if constexpr (std::is_same_v<DataType, glm::mat3>) {
            glUniformMatrix3fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(data));
        }
        // MAT4
        else if constexpr (std::is_same_v<DataType, glm::mat4>) {
            glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(data));
        }
        // Texture sampler
        // TODO:
    }

}

#endif //OPENGL_SAMPLES_SHADER_TPP