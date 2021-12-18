
#include "transform.h"

namespace OpenGL {


    Transform::Transform() : transform_(),
                             position_(),
                             scale_(1.0f),
                             rotation_(),
                             isDirty_(true) {

    }

    Transform::~Transform() {
    }

    void Transform::SetPosition(const glm::vec3 &position) {
        position_ = position;
        isDirty_ = true;
    }

    void Transform::SetPosition(float x, float y, float z) {
        SetPosition(glm::vec3(x, y, z));
    }

    void Transform::SetScale(const glm::vec3 &scale) {
        scale_ = scale;
        isDirty_ = true;
    }

    void Transform::SetScale(float x, float y, float z) {
        SetScale(glm::vec3(x, y, z));
    }

    void Transform::SetRotation(const glm::vec3 &rotation) {
        rotation_ = rotation;
        isDirty_ = true;
    }

    void Transform::SetRotation(float x, float y, float z) {
        SetRotation(glm::vec3(x, y, z));
    }

    const glm::mat4 &Transform::GetTransform() {
        return transform_;
    }

    const glm::vec3 &Transform::GetPosition() const {
        return position_;
    }

    const glm::vec3 &Transform::GetScale() const {
        return scale_;
    }

    const glm::vec3 &Transform::GetRotation() const {
        return rotation_;
    }

    void Transform::CalculateMatrix() {
        if (isDirty_) {
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), position_);
            glm::mat4 scale = glm::scale(scale_);
            glm::mat4 rotationX = glm::rotate(glm::radians(rotation_.x), glm::vec3(1.0f, 0.0f, 0.0f));
            glm::mat4 rotationY = glm::rotate(glm::radians(rotation_.y), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 rotationZ = glm::rotate(glm::radians(rotation_.z), glm::vec3(0.0f, 0.0f, 1.0f));

            transform_ = translation * rotationX * rotationY * rotationZ * scale;
            isDirty_ = false;
        }
    }

}