
#include "pch.h"
#include "material.h"

namespace OpenGL {

    bool Material::OnImGui() {
        bool updated = false;

        ImGui::PushStyleColor(ImGuiCol_Text, 0xff999999);

        ImGui::Text("Albedo:");
        glm::vec3 tempAlbedo = albedo;
        if (ImGui::ColorEdit3("##albedo", &tempAlbedo.x)) {
            // Manual input can go outside the valid range.
            tempAlbedo = glm::clamp(tempAlbedo, glm::vec3(0.0f), glm::vec3(1.0f));

            // At least one needs to be different for GPU data to get updated.
            glm::vec3 delta = glm::abs(tempAlbedo - albedo);
            if (delta.x > std::numeric_limits<float>::epsilon() || delta.y > std::numeric_limits<float>::epsilon() || delta.z > std::numeric_limits<float>::epsilon()) {
                albedo = tempAlbedo; // ImGui automatically converts color back to [0.0, 1.0] range.
                updated = true;
            }
        }

        ImGui::Text("Index of Refraction:");
        float tempIOR = ior;
        if (ImGui::SliderFloat("##ior", &tempIOR, 1.0f, 5.0f)) {
            // Manual input can go outside the valid range.
            tempIOR = glm::clamp(tempIOR, 1.0f, 5.0f);

            if (glm::abs(tempIOR - ior) > std::numeric_limits<float>::epsilon()) {
                ior = tempIOR;
                updated = true;
            }
        }

        ImGui::Text("Emissive Color:");
        glm::vec3 tempEmissive = emissive;
        if (ImGui::ColorEdit3("##emissive", &tempEmissive.x)) {
            // Manual input can go outside the valid range.
            tempEmissive = glm::clamp(tempEmissive, glm::vec3(0.0f), glm::vec3(1.0f));

            // At least one needs to be different for GPU data to get updated.
            glm::vec3 delta = glm::abs(tempEmissive - emissive);
            if (delta.x > std::numeric_limits<float>::epsilon() || delta.y > std::numeric_limits<float>::epsilon() || delta.z > std::numeric_limits<float>::epsilon()) {
                emissive = tempEmissive; // ImGui automatically converts color back to [0.0, 1.0] range.
                updated = true;
            }
        }

        ImGui::Text("Emissive Strength:");
        float tempEmissiveStrength = emissiveStrength;
        if (ImGui::SliderFloat("##emissiveStrength", &tempEmissiveStrength, 0.01f, 20.0f)) {
            // Manual input can go outside the valid range.
            tempEmissiveStrength = glm::clamp(tempEmissiveStrength, 0.01f, 20.0f);

            if (glm::abs(tempEmissiveStrength - emissiveStrength) > std::numeric_limits<float>::epsilon()) {
                emissiveStrength = tempEmissiveStrength;
                updated = true;
            }
        }

        ImGui::Text("Reflection Probability:");
        float tempReflectionProbability = reflectionProbability;
        if (ImGui::SliderFloat("##reflectionProbability", &tempReflectionProbability, 0.0f, 1.0f)) {
            // Manual input can go outside the valid range.
            tempReflectionProbability = glm::clamp(tempReflectionProbability, 0.0f, 1.0f);

            if (glm::abs(tempReflectionProbability - reflectionProbability) > std::numeric_limits<float>::epsilon()) {
                reflectionProbability = tempReflectionProbability;

                // Clamp sum of probabilities to 1.0f.
                if (refractionProbability + reflectionProbability > 1.0f) {
                    refractionProbability = 1.0f - reflectionProbability;
                }

                updated = true;
            }
        }

        ImGui::Text("Reflection Roughness:");
        float tempReflectionRoughness = reflectionRoughness;
        if (ImGui::SliderFloat("##reflectionRoughness", &tempReflectionRoughness, 0.0f, 1.0f)) {
            // Manual input can go outside the valid range.
            tempReflectionRoughness = glm::clamp(tempReflectionRoughness, 0.0f, 1.0f);

            if (glm::abs(tempReflectionRoughness - reflectionRoughness) > std::numeric_limits<float>::epsilon()) {
                reflectionRoughness = tempReflectionRoughness;
                updated = true;
            }
        }

        ImGui::Text("Absorbance:");
        glm::vec3 tempAbsorbance = absorbance;
        if (ImGui::ColorEdit3("##absorbance", &tempAbsorbance.x)) {
            // Manual input can go outside the valid range.
            tempAbsorbance = glm::clamp(tempAbsorbance, glm::vec3(0.0f), glm::vec3(1.0f));

            // At least one needs to be different for GPU data to get updated.
            glm::vec3 delta = glm::abs(tempAbsorbance - absorbance);
            if (delta.x > std::numeric_limits<float>::epsilon() || delta.y > std::numeric_limits<float>::epsilon() || delta.z > std::numeric_limits<float>::epsilon()) {
                absorbance = tempAbsorbance; // ImGui automatically converts color back to [0.0, 1.0] range.
                updated = true;
            }
        }

        ImGui::Text("Refraction Probability:");
        float tempRefractionProbability = refractionProbability;
        if (ImGui::SliderFloat("##refractionProbability", &tempRefractionProbability, 0.0f, 1.0f)) {
            // Manual input can go outside the valid range.
            tempRefractionProbability = glm::clamp(tempRefractionProbability, 0.0f, 1.0f);

            if (glm::abs(tempRefractionProbability - refractionProbability) > std::numeric_limits<float>::epsilon()) {
                refractionProbability = tempRefractionProbability;

                // Clamp sum of probabilities to 1.0f.
                if (refractionProbability + reflectionProbability > 1.0f) {
                    reflectionProbability = 1.0f - refractionProbability;
                }

                updated = true;
            }
        }

        ImGui::Text("Refraction Roughness:");
        float tempRefractionRoughness = refractionProbability;
        if (ImGui::SliderFloat("##refractionRoughness", &tempRefractionRoughness, 0.0f, 1.0f)) {
            // Manual input can go outside the valid range.
            tempRefractionRoughness = glm::clamp(tempRefractionRoughness, 0.0f, 1.0f);

            if (glm::abs(tempRefractionRoughness - refractionProbability) > std::numeric_limits<float>::epsilon()) {
                refractionRoughness = tempRefractionRoughness;
                updated = true;
            }
        }

        ImGui::PopStyleColor();

        return updated;
    }

}