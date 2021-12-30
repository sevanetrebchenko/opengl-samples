
#ifndef OPENGL_SAMPLES_OBJECT_LOADER_H
#define OPENGL_SAMPLES_OBJECT_LOADER_H

#include "pch.h"

namespace OpenGL {

    struct Mesh {
        void RecalculateNormals();

        std::vector<glm::vec3> vertices;
        std::vector<unsigned> indices;

        std::vector<glm::vec2> uv;

        // Uses vertex normals.
        std::vector<glm::vec3> normals;
    };

    class ObjectLoader {
        public:
            static ObjectLoader& Instance();

            [[nodiscard]] Mesh LoadFromFile(const std::string& filename);

        private:
            ObjectLoader();
            ~ObjectLoader();

            void MinMaxVertex(const glm::vec3& vertex, glm::vec3& minimum, glm::vec3& maximum) const;
            void TransformToOrigin(std::vector<glm::vec3>& vertices, const glm::vec3& minimum, const glm::vec3& maximum) const;
            void ScaleToUniform(std::vector<glm::vec3>& vertices, const glm::vec3& minimum, const glm::vec3& maximum) const;

            std::unordered_map<std::string, Mesh> loadedMeshes_;
    };

}


#endif //OPENGL_SAMPLES_OBJECT_LOADER_H
