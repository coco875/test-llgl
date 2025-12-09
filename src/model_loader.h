#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include <LLGL/LLGL.h>
#include <LLGL/Utils/VertexFormat.h>
#include "math_types.h"

// Forward declarations
struct aiNode;
struct aiMesh;
struct aiScene;

// Vertex structure for 3D models
struct ModelVertex {
    Math::Vec3 position;
    Math::Vec3 normal;
    Math::Vec2 texCoord;
};

// GPU mesh data
struct Mesh {
    std::vector<ModelVertex> vertices;
    std::vector<uint32_t> indices;
    LLGL::Buffer* vertexBuffer = nullptr;
    LLGL::Buffer* indexBuffer = nullptr;
    uint32_t materialIndex = 0;

    uint32_t indexCount() const {
        return static_cast<uint32_t>(indices.size());
    }
};

// Material data
struct Material {
    std::string diffuseTexturePath;
    LLGL::Texture* diffuseTexture = nullptr;
    Math::Vec3 diffuseColor{ 0.8f, 0.8f, 0.8f };
    bool hasTexture = false;
};

// Complete 3D model
class Model {
  public:
    Model() = default;
    ~Model() = default;

    // Non-copyable
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    // Movable
    Model(Model&&) = default;
    Model& operator=(Model&&) = default;

    // Loading
    bool load(const std::string& path, LLGL::RenderSystemPtr& renderer);
    void createBuffers(LLGL::RenderSystemPtr& renderer);
    void release(LLGL::RenderSystemPtr& renderer);

    // Accessors
    const std::vector<Mesh>& getMeshes() const {
        return meshes_;
    }
    std::vector<Mesh>& getMeshes() {
        return meshes_;
    }

    const std::vector<Material>& getMaterials() const {
        return materials_;
    }
    const Material& getMaterial(uint32_t index) const {
        return materials_[index];
    }

    const LLGL::VertexFormat& getVertexFormat() const {
        return vertexFormat_;
    }

    const Math::AABB& getBounds() const {
        return bounds_;
    }
    Math::Vec3 getCenter() const {
        return bounds_.center();
    }
    float getRadius() const {
        return bounds_.radius();
    }

    const std::string& getDirectory() const {
        return directory_;
    }

    void calculateBounds();

  private:
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    void loadMaterials(const aiScene* scene, LLGL::RenderSystemPtr& renderer);

    std::vector<Mesh> meshes_;
    std::vector<Material> materials_;
    std::string directory_;
    Math::AABB bounds_;
    LLGL::VertexFormat vertexFormat_;
};

// Helper to create vertex format for ModelVertex
inline LLGL::VertexFormat createModelVertexFormat() {
    LLGL::VertexFormat format;
    format.AppendAttribute({ "position", LLGL::Format::RGB32Float });
    format.AppendAttribute({ "normal", LLGL::Format::RGB32Float });
    format.AppendAttribute({ "texCoord", LLGL::Format::RG32Float });
    format.SetStride(sizeof(ModelVertex));
    return format;
}
