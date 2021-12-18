
#ifndef OPENGL_EXAMPLES_TRANSFORM_H
#define OPENGL_EXAMPLES_TRANSFORM_H

#include "pch.h"

namespace OpenGL {

    class Transform {
        public:
            Transform();
            ~Transform();

            void SetPosition(const glm::vec3& position);
            void SetPosition(float x, float y, float z);

            void SetScale(const glm::vec3& scale);
            void SetScale(float x, float y, float z);

            void SetRotation(const glm::vec3& rotation);
            void SetRotation(float x, float y, float z);

            [[nodiscard]] const glm::mat4& GetTransform();

            [[nodiscard]] const glm::vec3& GetPosition() const;
            [[nodiscard]] const glm::vec3& GetScale() const;
            [[nodiscard]] const glm::vec3& GetRotation() const;

        private:
            void CalculateMatrix();

            glm::mat4 transform_;

            glm::vec3 position_;
            glm::vec3 scale_;
            glm::vec3 rotation_;

            bool isDirty_;
    };

}

#endif //OPENGL_EXAMPLES_TRANSFORM_H
