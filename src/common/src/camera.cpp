
#include "camera.h"

namespace OpenGL {

    Camera::Camera(int width, int height) : cameraTransform_(1.0f),
                                            viewTransform_(1.0f),
                                            perspectiveTransform_(1.0f),
                                            position_(0.0f, 0.0f, 0.0f),
                                            lookAtDirection_(0.0f, 0.0f, -1.0f),
                                            up_(0.0f, 1.0f, 0.0f),
                                            fov_(75.0),
                                            aspectRatio_((float) width / (float) height),
                                            nearPlaneDistance_(0.01f),
                                            farPlaneDistance_(1000.0f),
                                            isDirty_(true) {

    }

    Camera::~Camera() {
    }

    void Camera::SetPosition(const glm::vec3 &position) {
        position_ = position;
        isDirty_ = true;
    }

    void Camera::SetTargetPosition(const glm::vec3 &position) {
        lookAtDirection_ = position - position_;
        isDirty_ = true;
    }

    void Camera::SetLookAtDirection(const glm::vec3 &direction) {
        lookAtDirection_ = direction;
        isDirty_ = true;
    }

    void Camera::SetNearPlaneDistance(float distance) {
        nearPlaneDistance_ = distance;
        isDirty_ = true;
    }

    void Camera::SetFarPlaneDistance(float distance) {
        farPlaneDistance_ = distance;
        isDirty_ = true;
    }

    void Camera::SetFOVAngle(float fov) {
        fov_ = fov;
        isDirty_ = true;
    }

    const glm::mat4 &Camera::GetCameraTransform() {
        CalculateMatrix();
        return cameraTransform_;
    }

    const glm::mat4 &Camera::GetPerspectiveTransform() {
        CalculateMatrix();
        return perspectiveTransform_;
    }

    const glm::mat4 &Camera::GetViewTransform() {
        CalculateMatrix();
        return viewTransform_;
    }

    const glm::vec3 &Camera::GetForwardVector() const {
        return lookAtDirection_;
    }

    const glm::vec3 &Camera::GetUpVector() const {
        return up_;
    }

    float Camera::GetNearPlaneDistance() const {
        return nearPlaneDistance_;
    }

    float Camera::GetFarPlaneDistance() const {
        return farPlaneDistance_;
    }

    void Camera::CalculateMatrix() {
        if (isDirty_) {
            viewTransform_ = glm::lookAt(position_, position_ + lookAtDirection_, up_);
            perspectiveTransform_ = glm::perspective(glm::radians(fov_), aspectRatio_, nearPlaneDistance_, farPlaneDistance_);
            cameraTransform_ = perspectiveTransform_ * viewTransform_;
            isDirty_ = false;
        }
    }

}