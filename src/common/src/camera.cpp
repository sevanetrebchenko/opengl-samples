
#include "camera.h"
#include "utility.h"

namespace OpenGL {

    Camera::Camera(int width, int height) : cameraTransform_(1.0f),
                                            viewTransform_(1.0f),
                                            perspectiveTransform_(1.0f),
                                            position_(0.0f, 0.0f, 0.0f),
                                            lookAtDirection_(0.0f, 0.0f, -1.0f),
                                            up_(0.0f, 1.0f, 0.0f),
                                            eulerAngles_(0.0f, -PI / 2.0f, 0.0f),
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
        lookAtDirection_ = glm::normalize(position - position_);
        isDirty_ = true;
    }

    void Camera::SetLookAtDirection(const glm::vec3 &direction) {
        lookAtDirection_ = glm::normalize(direction);
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

    void Camera::SetAspectRatio(float width, float height) {
        aspectRatio_ = width / height;
        isDirty_ = true;
    }

    void Camera::SetAspectRatio(float aspectRatio) {
        aspectRatio_ = aspectRatio;
        isDirty_ = true;
    }

    void Camera::SetEulerAngles(const glm::vec3 &eulerAngles) {
        eulerAngles_ = glm::radians(eulerAngles);

        float pitch = eulerAngles_.x;
        float yaw = eulerAngles_.y;
        float roll = eulerAngles_.z;

        glm::vec3 forwardVector;
        forwardVector.x = glm::cos(yaw) * glm::cos(pitch);
        forwardVector.y = glm::sin(pitch);
        forwardVector.z = glm::sin(yaw) * glm::cos(pitch);

        // Set forward vector.
        SetLookAtDirection(forwardVector);
    }

    void Camera::SetEulerAngles(float pitch, float yaw, float roll) {
        SetEulerAngles(glm::vec3(pitch, yaw, roll));
    }

    const glm::vec3 &Camera::GetPosition() const {
        return position_;
    }

    const glm::mat4 &Camera::GetCameraTransform() {
        if (isDirty_) {
            RecalculateMatrices();
        }
        return cameraTransform_;
    }

    const glm::mat4 &Camera::GetPerspectiveTransform() {
        if (isDirty_) {
            RecalculateMatrices();
        }
        return perspectiveTransform_;
    }

    const glm::mat4 &Camera::GetViewTransform() {
        if (isDirty_) {
            RecalculateMatrices();
        }
        return viewTransform_;
    }

    const glm::vec3 &Camera::GetForwardVector() const {
        return lookAtDirection_;
    }

    const glm::vec3 &Camera::GetUpVector() const {
        return up_;
    }

    const glm::vec3 &Camera::GetEulerAngles() const {
        return eulerAngles_;
    }

    float Camera::GetPitch() const {
        return eulerAngles_.x;
    }

    float Camera::GetYaw() const {
        return eulerAngles_.y;
    }

    float Camera::GetRoll() const {
        return eulerAngles_.z;
    }

    float Camera::GetNearPlaneDistance() const {
        return nearPlaneDistance_;
    }

    bool Camera::IsDirty() const {
        return isDirty_;
    }

    float Camera::GetAspectRatio() const {
        return aspectRatio_;
    }

    float Camera::GetFarPlaneDistance() const {
        return farPlaneDistance_;
    }

    void Camera::RecalculateMatrices() {
        viewTransform_ = glm::lookAt(position_, position_ + lookAtDirection_, up_);
        perspectiveTransform_ = glm::perspective(glm::radians(fov_), aspectRatio_, nearPlaneDistance_, farPlaneDistance_);
        cameraTransform_ = perspectiveTransform_ * viewTransform_;
        isDirty_ = false;
    }

}