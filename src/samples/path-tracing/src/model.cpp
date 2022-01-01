
#include "model.h"

namespace OpenGL {

    Model::Model(Mesh m) : mesh(std::move(m)) {
        // Break model up into individual triangles.
        std::size_t numTriangles = mesh.indices.size() / 3;

        objectSpaceTriangles.reserve(numTriangles);

        for (unsigned i = 0; i < numTriangles; ++i) {
            objectSpaceTriangles.emplace_back(OpenGL::Triangle(mesh.vertices[mesh.indices[i * 3 + 0]],
                                                               mesh.vertices[mesh.indices[i * 3 + 1]],
                                                               mesh.vertices[mesh.indices[i * 3 + 2]]));
        }

        triangles.reserve(numTriangles); // Number of triangles does not change.
    }

    void Model::Recalculate(const glm::mat4& cameraTransform) {
        const glm::mat4& modelTransform = transform.GetTransform();

        glm::mat4 vertexTransform = cameraTransform * modelTransform;
//        glm::mat4 normalTransform = cameraTransform * glm::inverse(modelTransform);

        for (const Triangle& original : objectSpaceTriangles) {
            // Apply transformation to vertices.
            // Computes normal automatically, no need to apply normal transformation.
            triangles.emplace_back(vertexTransform * original.v1,
                                   vertexTransform * original.v2,
                                   vertexTransform * original.v3);
        }
    }

    bool Model::IsDirty() const {
        return transform.IsDirty();
    }

}
