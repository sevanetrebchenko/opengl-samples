
#include "object_loader.h"

namespace OpenGL {

//    void Mesh::RecalculateNormals() {
//        std::size_t numIndices = indices.size();
//        assert(numIndices % 3 == 0);
//
//        std::vector<std::vector<glm::vec3>> vertexAdjacentFaceNormals;
//        vertexAdjacentFaceNormals.resize(vertices.size());
//
//        // Traverse all triples of indices and compute face normals from each.
//        // Attempt to add each normal to the involved vertex if it hasn't been already.
//        for (unsigned i = 0; i < numIndices; i += 3) {
//            unsigned& index1 = indices[i + 0];
//            unsigned& index2 = indices[i + 1];
//            unsigned& index3 = indices[i + 2];
//            glm::vec3& vertex1 = vertices[index1];
//            glm::vec3& vertex2 = vertices[index2];
//            glm::vec3& vertex3 = vertices[index3];
//
//            // Calculate face normal.
//            glm::vec3 faceNormal = glm::cross(vertex3 - vertex2, vertex1 - vertex2);
//
//            bool duplicateNormal = false;
//            // Attempt to add each normal to the involved vertices.
//            for (unsigned j = 0; j < 3; ++j) {
//                unsigned& index = indices[i + j];
//                // Check if normal was already added to this face's vertices.
//                for (const auto &normal : vertexAdjacentFaceNormals[index]) {
//                    if ((glm::dot(faceNormal, normal) - glm::dot(faceNormal, faceNormal)) > std::numeric_limits<float>::epsilon()) {
//                        duplicateNormal = true;
//                        break;
//                    }
//                }
//
//                if (!duplicateNormal) {
//                    vertexAdjacentFaceNormals[index].emplace_back(faceNormal);
//                }
//            }
//        }
//
//        // Compute normals from precomputed adjacent normal list.
//        int numNormals = vertexAdjacentFaceNormals.size();
//        normals.resize(numNormals);
//
//        // Fill mesh data normal data buffer.
//        for (int i = 0; i < numNormals; ++i) {
//            glm::vec3& vertexNormal = normals[i];
//
//            // Sum all adjacent face normals (without duplicates).
//            for (const glm::vec3& normal : vertexAdjacentFaceNormals[i]) {
//                vertexNormal += normal;
//            }
//
//            vertexNormal = glm::normalize(vertexNormal);
//        }
//    }

    ObjectLoader &ObjectLoader::Instance() {
        static ObjectLoader loader;
        return loader;
    }

    Mesh ObjectLoader::LoadFromFile(const std::string &filename) {
        if (loadedMeshes_.find(filename) != loadedMeshes_.end()) {
            return loadedMeshes_[filename]; // Make copy.
        }

        // Loading new mesh.
        Mesh mesh;
        std::vector<glm::vec3> vertices;
        std::vector<unsigned> indices;
        std::unordered_map<glm::vec3, unsigned> uniqueVertices;
        std::vector<glm::vec2> uv;

        glm::vec3 minimum(std::numeric_limits<float>::max());
        glm::vec3 maximum(std::numeric_limits<float>::lowest());

        // Prepare tinyobj loading parameters.
        tinyobj::attrib_t attributes; // Holds all positions, normals, and texture coordinates.
        std::vector<tinyobj::shape_t> shapeData; // Holds all separate objects and their faces.
        std::vector<tinyobj::material_t> materialData;
        std::string warning;
        std::string error;

        // Triangulation enabled by default.
        if (!tinyobj::LoadObj(&attributes, &shapeData, &materialData, &warning, &error, filename.c_str())) {
            throw std::runtime_error("Failed to load OBJ file: " + filename + ". Provided information: " + warning + " (WARNING) " + error + "(ERROR)");
        }

        bool hasVertexNormals = !attributes.normals.empty();
        bool hasTextureCoordinates = !attributes.texcoords.empty();
        bool hasVertexColors = !attributes.colors.empty();

        // Push shape data.
        for (const tinyobj::shape_t& shape : shapeData) {
            for (const tinyobj::index_t& index : shape.mesh.indices) {
                int vertexBase = 3 * index.vertex_index;
                glm::vec3 vertex(attributes.vertices[vertexBase + 0], attributes.vertices[vertexBase + 1], attributes.vertices[vertexBase + 2]);

                int vertexNormalBase = 3 * index.normal_index;
                glm::vec3 vertexNormal;
                if (hasVertexNormals) {
                    vertexNormal = glm::vec3(attributes.normals[vertexNormalBase + 0], attributes.normals[vertexNormalBase + 1], attributes.normals[vertexNormalBase + 2]);
                }

                int textureCoordinateBase = 2 * index.texcoord_index;
                glm::vec2 textureCoordinate;
                if (hasTextureCoordinates) {
                    textureCoordinate = glm::vec2(attributes.texcoords[textureCoordinateBase + 0], attributes.texcoords[textureCoordinateBase + 1]);
                }

                int vertexColorBase = vertexBase;
                glm::vec3 vertexColor;
                if (hasVertexColors) {
                    vertexColor = glm::vec3(attributes.colors[vertexColorBase + 0], attributes.colors[vertexColorBase + 1], attributes.colors[vertexColorBase + 2]);
                }

                // Found unique vertex.
                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = vertices.size();
                    vertices.push_back(vertex);
                    MinMaxVertex(vertex, minimum, maximum);

                    if (hasTextureCoordinates) {
                        uv.push_back(textureCoordinate);
                    }
                }

                // In case of duplicate vertex, push back index instead.
                indices.push_back(uniqueVertices[vertex]);
            }
        }

        // Normalize mesh.
        TransformToOrigin(vertices, minimum, maximum);
        ScaleToUniform(vertices, minimum, maximum);

        mesh.vertices = vertices;
        mesh.indices = indices;
        mesh.uv = uv;

//        mesh.RecalculateNormals();

        loadedMeshes_[filename] = mesh;
        return mesh;
    }

    ObjectLoader::ObjectLoader() {

    }

    ObjectLoader::~ObjectLoader() {

    }

    void ObjectLoader::MinMaxVertex(const glm::vec3 &vertex, glm::vec3 &minimum, glm::vec3 &maximum) const {
        // Find mesh extrema.
        if (vertex.x < minimum.x) {
            minimum.x = vertex.x;
        }
        else if (vertex.x > maximum.x) {
            maximum.x = vertex.x;
        }

        if (vertex.y < minimum.y) {
            minimum.y = vertex.y;
        }
        else if (vertex.y > maximum.y) {
            maximum.y = vertex.y;
        }

        if (vertex.z < minimum.z) {
            minimum.z = vertex.z;
        }
        else if (vertex.z > maximum.z) {
            maximum.z = vertex.z;
        }
    }

    void ObjectLoader::TransformToOrigin(std::vector<glm::vec3> &vertices, const glm::vec3 &minimum, const glm::vec3 &maximum) const {
        // Center model at (0, 0, 0)
        glm::vec3 centerPosition = glm::vec3((minimum + maximum) / 2.0f);
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), -centerPosition);
        for (glm::vec3& vertex : vertices) {
            vertex = transform * glm::vec4(vertex, 1.0f);
        }
    }

    void ObjectLoader::ScaleToUniform(std::vector<glm::vec3> &vertices, const glm::vec3 &minimum, const glm::vec3 &maximum) const {
        glm::vec3 boundingBoxDimensions = maximum - minimum;

        // Scale the mesh to range [-1 1] on all axes.
        float maxDimension = std::max(boundingBoxDimensions.x, std::max(boundingBoxDimensions.y, boundingBoxDimensions.z));
        glm::mat4 uniformScale = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f / maxDimension));

        for (glm::vec3& vertex : vertices) {
            vertex = uniformScale * glm::vec4(vertex, 1.0f);
        }
    }

}