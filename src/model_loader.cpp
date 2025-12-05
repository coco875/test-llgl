#include "model_loader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stb_image.h>
#include <LLGL/Utils/VertexFormat.h>

namespace {

constexpr unsigned int ASSIMP_LOAD_FLAGS = aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs |
                                           aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices;

std::string extractDirectory(const std::string& path) {
    size_t lastSlash = path.find_last_of("/\\");
    return (lastSlash != std::string::npos) ? path.substr(0, lastSlash + 1) : "";
}

LLGL::Texture* loadTextureFromFile(const std::string& path, LLGL::RenderSystemPtr& renderer) {
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);

    if (!data) {
        LLGL::Log::Errorf("Failed to load texture: %s\n", path.c_str());
        return nullptr;
    }

    LLGL::ImageView imageView(LLGL::ImageFormat::RGBA, LLGL::DataType::UInt8, data,
                              static_cast<size_t>(width * height * 4));

    LLGL::TextureDescriptor texDesc;
    texDesc.type = LLGL::TextureType::Texture2D;
    texDesc.format = LLGL::Format::RGBA8UNorm;
    texDesc.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    texDesc.miscFlags = LLGL::MiscFlags::GenerateMips;

    LLGL::Texture* texture = renderer->CreateTexture(texDesc, &imageView);

    stbi_image_free(data);
    LLGL::Log::Printf("Loaded texture: %s (%dx%d)\n", path.c_str(), width, height);

    return texture;
}

} // anonymous namespace

bool Model::load(const std::string& path, LLGL::RenderSystemPtr& renderer) {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path, ASSIMP_LOAD_FLAGS);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        LLGL::Log::Errorf("Assimp error: %s\n", importer.GetErrorString());
        return false;
    }

    directory_ = extractDirectory(path);

    // Process scene hierarchy
    processNode(scene->mRootNode, scene);

    // Load materials and textures
    loadMaterials(scene, renderer);

    // Calculate bounding box
    calculateBounds();

    LLGL::Log::Printf("Model loaded: %zu meshes, %zu materials\n", meshes_.size(), materials_.size());
    LLGL::Log::Printf("Bounds: (%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f)\n", bounds_.minPoint.x, bounds_.minPoint.y,
                      bounds_.minPoint.z, bounds_.maxPoint.x, bounds_.maxPoint.y, bounds_.maxPoint.z);
    LLGL::Log::Printf("Center: (%.2f, %.2f, %.2f), Radius: %.2f\n", getCenter().x, getCenter().y, getCenter().z,
                      getRadius());

    return true;
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    // Process meshes in this node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes_.push_back(processMesh(mesh, scene));
    }

    // Recurse into children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    Mesh result;
    result.vertices.reserve(mesh->mNumVertices);

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        ModelVertex vertex;

        vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

        if (mesh->HasNormals()) {
            vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        } else {
            vertex.normal = { 0.0f, 1.0f, 0.0f };
        }

        if (mesh->mTextureCoords[0]) {
            vertex.texCoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        } else {
            vertex.texCoord = { 0.0f, 0.0f };
        }

        result.vertices.push_back(vertex);
    }

    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            result.indices.push_back(face.mIndices[j]);
        }
    }

    result.materialIndex = mesh->mMaterialIndex;

    return result;
}

void Model::loadMaterials(const aiScene* scene, LLGL::RenderSystemPtr& renderer) {
    materials_.resize(scene->mNumMaterials);

    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* mat = scene->mMaterials[i];
        // print the shader model
        aiString shaderModel;
        if (mat->Get(AI_MATKEY_SHADER_FRAGMENT, shaderModel) == AI_SUCCESS) {
            LLGL::Log::Printf("Shader Model: %s\n", shaderModel.C_Str());
        }

        Material& material = materials_[i];

        // Get diffuse color
        aiColor3D color(0.8f, 0.8f, 0.8f);
        mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        material.diffuseColor = { color.r, color.g, color.b };

        // Load diffuse texture if available
        if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString texPath;
            mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);

            std::string fullPath = directory_ + texPath.C_Str();
            material.diffuseTexturePath = fullPath;
            material.diffuseTexture = loadTextureFromFile(fullPath, renderer);
            material.hasTexture = (material.diffuseTexture != nullptr);
        }
    }
}

void Model::calculateBounds() {
    bounds_ = Math::AABB{};

    for (const auto& mesh : meshes_) {
        for (const auto& vertex : mesh.vertices) {
            bounds_.expand(vertex.position);
        }
    }
}

void Model::createBuffers(LLGL::RenderSystemPtr& renderer) {
    vertexFormat_.AppendAttribute({ "position", LLGL::Format::RGB32Float });
    vertexFormat_.AppendAttribute({ "normal", LLGL::Format::RGB32Float });
    vertexFormat_.AppendAttribute({ "texCoord", LLGL::Format::RG32Float });
    vertexFormat_.SetStride(sizeof(ModelVertex));
    for (auto& mesh : meshes_) {
        // Vertex buffer
        LLGL::BufferDescriptor vbDesc;
        vbDesc.size = mesh.vertices.size() * sizeof(ModelVertex);
        vbDesc.bindFlags = LLGL::BindFlags::VertexBuffer;
        vbDesc.vertexAttribs = vertexFormat_.attributes;
        vbDesc.debugName = "ModelVertexBuffer";

        mesh.vertexBuffer = renderer->CreateBuffer(vbDesc, mesh.vertices.data());

        // Index buffer
        LLGL::BufferDescriptor ibDesc;
        ibDesc.size = mesh.indices.size() * sizeof(uint32_t);
        ibDesc.bindFlags = LLGL::BindFlags::IndexBuffer;
        ibDesc.format = LLGL::Format::R32UInt;
        ibDesc.debugName = "ModelIndexBuffer";

        mesh.indexBuffer = renderer->CreateBuffer(ibDesc, mesh.indices.data());
    }
}

void Model::release(LLGL::RenderSystemPtr& renderer) {
    for (auto& mesh : meshes_) {
        if (mesh.vertexBuffer) {
            renderer->Release(*mesh.vertexBuffer);
            mesh.vertexBuffer = nullptr;
        }
        if (mesh.indexBuffer) {
            renderer->Release(*mesh.indexBuffer);
            mesh.indexBuffer = nullptr;
        }
    }

    for (auto& material : materials_) {
        if (material.diffuseTexture) {
            renderer->Release(*material.diffuseTexture);
            material.diffuseTexture = nullptr;
        }
    }

    meshes_.clear();
    materials_.clear();
}
