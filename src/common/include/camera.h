
#ifndef OPENGL_EXAMPLES_CAMERA_H
#define OPENGL_EXAMPLES_CAMERA_H

#include "pch.h"

namespace OpenGL {

    class Camera {
        public:
            Camera(int width, int height);
            ~Camera();

            void SetPosition(const glm::vec3& position);

            // Look-at vector.
            void SetTargetPosition(const glm::vec3& position);
            void SetLookAtDirection(const glm::vec3& direction);

            void SetNearPlaneDistance(float distance);
            void SetFarPlaneDistance(float distance);

            void SetFOVAngle(float fov);

            // View * Perspective.
            [[nodiscard]] const glm::mat4& GetCameraTransform();
            [[nodiscard]] const glm::mat4& GetPerspectiveTransform();
            [[nodiscard]] const glm::mat4& GetViewTransform();

            [[nodiscard]] const glm::vec3& GetForwardVector() const;
            [[nodiscard]] const glm::vec3& GetUpVector() const;

            [[nodiscard]] float GetNearPlaneDistance() const;
            [[nodiscard]] float GetFarPlaneDistance() const;

        private:
            void CalculateMatrix();

            glm::mat4 cameraTransform_;
            glm::mat4 viewTransform_;
            glm::mat4 perspectiveTransform_;

            glm::vec3 position_;
            glm::vec3 lookAtDirection_;
            glm::vec3 up_;

            float fov_;
            float aspectRatio_;

            float nearPlaneDistance_;
            float farPlaneDistance_;

            bool isDirty_;
    };

}

#endif //OPENGL_EXAMPLES_CAMERA_H
