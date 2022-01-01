
#pragma once

#include "triangle.h"
#include "transform.h"
#include "object_loader.h"

namespace OpenGL {

    class Model {
        public:
            explicit Model(Mesh m);
            ~Model() = default;

            void Recalculate(const glm::mat4& cameraTransform);

            // Returns whether model triangle positions need to be recalculated or not.
            [[nodiscard]] bool IsDirty() const;

            Mesh mesh;
            Transform transform;

            // Triangles (in world space) for path tracing.
            std::vector<Triangle> triangles;

        private:
            // For applying transformations.
            std::vector<Triangle> objectSpaceTriangles;
    };

}