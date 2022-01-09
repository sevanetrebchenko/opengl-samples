
#include "pch.h"
#include "primitives.h"

namespace OpenGL {

    bool Sphere::OnImGui() {
        bool updated = false;

        ImGui::PushStyleColor(ImGuiCol_Text, 0xff999999);

        ImGui::Text("Position:");
        glm::vec3 tempPosition = position;
        if (ImGui::DragFloat3("##position", &tempPosition.x, 0.1f, -30.0f, 30.0f)) {
            // Manual input can go outside the valid range.
            tempPosition = glm::clamp(tempPosition, glm::vec3(-30.0f), glm::vec3(30.0f));

            // At least one needs to be different for GPU data to get updated.
            glm::vec3 delta = glm::abs(tempPosition - position);
            if (delta.x > std::numeric_limits<float>::epsilon() || delta.y > std::numeric_limits<float>::epsilon() || delta.z > std::numeric_limits<float>::epsilon()) {
                position = tempPosition;
                updated = true;
            }
        }

        ImGui::Text("Radius:");
        float tempRadius = radius;
        if (ImGui::SliderFloat("##radius", &tempRadius, 0.001f, 20.0f)) {
            // Manual input can go outside the valid range.
            tempRadius = glm::clamp(tempRadius, 0.001f, 20.0f);

            if (glm::abs(tempRadius - radius) > std::numeric_limits<float>::epsilon()) {
                radius = tempRadius;
                updated = true;
            }
        }

        ImGui::Separator();

        ImGui::PopStyleColor();

        return updated | material.OnImGui();
    }

    bool AABB::OnImGui() {
        bool updated = false;

        ImGui::PushStyleColor(ImGuiCol_Text, 0xff999999);

        ImGui::Text("Position:");
        glm::vec3 tempPosition = position;
        if (ImGui::DragFloat3("##position", &tempPosition.x, 0.1f, -30.0f, 30.0f)) {
            // Manual input can go outside the valid range.
            tempPosition = glm::clamp(tempPosition, glm::vec3(-30.0f), glm::vec3(30.0f));

            // At least one needs to be different for GPU data to get updated.
            glm::vec3 delta = glm::abs(tempPosition - glm::vec3(position));
            if (delta.x > std::numeric_limits<float>::epsilon() || delta.y > std::numeric_limits<float>::epsilon() || delta.z > std::numeric_limits<float>::epsilon()) {
                position = glm::vec4(tempPosition, 0.0f);
                updated = true;
            }
        }

        ImGui::Text("Dimensions:");
        glm::vec3 tempDimensions = dimensions;
        if (ImGui::SliderFloat3("##dimensions", &tempDimensions.x, 0.001f, 20.0f)) {
            // Manual input can go outside the valid range.
            tempDimensions = glm::clamp(tempDimensions, glm::vec3(0.001f), glm::vec3(20.0f));

            glm::vec3 delta = glm::abs(tempDimensions - glm::vec3(dimensions));
            if (delta.x > std::numeric_limits<float>::epsilon() || delta.y > std::numeric_limits<float>::epsilon() || delta.z > std::numeric_limits<float>::epsilon()) {
                dimensions = glm::vec4(tempDimensions, 0.0f);
                updated = true;
            }
        }

        ImGui::Separator();

        ImGui::PopStyleColor();

        return updated | material.OnImGui();
    }
}