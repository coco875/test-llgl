#include "primitives.h"
#include <cmath>

namespace Primitives {

Mesh createCube(float size) {
    Mesh mesh;
    float s = size * 0.5f;

    // 24 vertices (4 per face, pour avoir des normales correctes)
    ModelVertex vertices[] = {
        // Front face (+Z)
        { { -s, -s, s }, { 0, 0, 1 }, { 0, 0 } },
        { { s, -s, s }, { 0, 0, 1 }, { 1, 0 } },
        { { s, s, s }, { 0, 0, 1 }, { 1, 1 } },
        { { -s, s, s }, { 0, 0, 1 }, { 0, 1 } },
        // Back face (-Z)
        { { -s, -s, -s }, { 0, 0, -1 }, { 1, 0 } },
        { { -s, s, -s }, { 0, 0, -1 }, { 1, 1 } },
        { { s, s, -s }, { 0, 0, -1 }, { 0, 1 } },
        { { s, -s, -s }, { 0, 0, -1 }, { 0, 0 } },
        // Top face (+Y)
        { { -s, s, -s }, { 0, 1, 0 }, { 0, 1 } },
        { { -s, s, s }, { 0, 1, 0 }, { 0, 0 } },
        { { s, s, s }, { 0, 1, 0 }, { 1, 0 } },
        { { s, s, -s }, { 0, 1, 0 }, { 1, 1 } },
        // Bottom face (-Y)
        { { -s, -s, -s }, { 0, -1, 0 }, { 0, 0 } },
        { { s, -s, -s }, { 0, -1, 0 }, { 1, 0 } },
        { { s, -s, s }, { 0, -1, 0 }, { 1, 1 } },
        { { -s, -s, s }, { 0, -1, 0 }, { 0, 1 } },
        // Right face (+X)
        { { s, -s, -s }, { 1, 0, 0 }, { 0, 0 } },
        { { s, s, -s }, { 1, 0, 0 }, { 0, 1 } },
        { { s, s, s }, { 1, 0, 0 }, { 1, 1 } },
        { { s, -s, s }, { 1, 0, 0 }, { 1, 0 } },
        // Left face (-X)
        { { -s, -s, -s }, { -1, 0, 0 }, { 1, 0 } },
        { { -s, -s, s }, { -1, 0, 0 }, { 0, 0 } },
        { { -s, s, s }, { -1, 0, 0 }, { 0, 1 } },
        { { -s, s, -s }, { -1, 0, 0 }, { 1, 1 } },
    };

    mesh.vertices.assign(std::begin(vertices), std::end(vertices));

    // Indices (2 triangles par face, 6 faces)
    uint32_t indices[] = {
        0,  1,  2,  2,  3,  0,  // Front
        4,  5,  6,  6,  7,  4,  // Back
        8,  9,  10, 10, 11, 8,  // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Right
        20, 21, 22, 22, 23, 20, // Left
    };

    mesh.indices.assign(std::begin(indices), std::end(indices));
    mesh.materialIndex = 0;

    return mesh;
}

Mesh createSphere(float radius, int segments, int rings) {
    Mesh mesh;

    // Générer les vertices
    for (int ring = 0; ring <= rings; ++ring) {
        float phi = Math::PI * static_cast<float>(ring) / static_cast<float>(rings);
        float y = std::cos(phi) * radius;
        float ringRadius = std::sin(phi) * radius;

        for (int seg = 0; seg <= segments; ++seg) {
            float theta = 2.0f * Math::PI * static_cast<float>(seg) / static_cast<float>(segments);
            float x = std::cos(theta) * ringRadius;
            float z = std::sin(theta) * ringRadius;

            ModelVertex v;
            v.position = { x, y, z };
            v.normal = Math::Vec3(x, y, z).normalized();
            v.texCoord = { static_cast<float>(seg) / static_cast<float>(segments),
                           static_cast<float>(ring) / static_cast<float>(rings) };
            mesh.vertices.push_back(v);
        }
    }

    // Générer les indices
    for (int ring = 0; ring < rings; ++ring) {
        for (int seg = 0; seg < segments; ++seg) {
            uint32_t curr = ring * (segments + 1) + seg;
            uint32_t next = curr + segments + 1;

            mesh.indices.push_back(curr);
            mesh.indices.push_back(next);
            mesh.indices.push_back(curr + 1);

            mesh.indices.push_back(curr + 1);
            mesh.indices.push_back(next);
            mesh.indices.push_back(next + 1);
        }
    }

    mesh.materialIndex = 0;
    return mesh;
}

Mesh createPlane(float width, float height, int subdivisions) {
    Mesh mesh;

    float halfW = width * 0.5f;
    float halfH = height * 0.5f;
    int divisions = subdivisions + 1;

    // Vertices
    for (int z = 0; z <= divisions; ++z) {
        for (int x = 0; x <= divisions; ++x) {
            float fx = static_cast<float>(x) / static_cast<float>(divisions);
            float fz = static_cast<float>(z) / static_cast<float>(divisions);

            ModelVertex v;
            v.position = { -halfW + fx * width, 0.0f, -halfH + fz * height };
            v.normal = { 0.0f, 1.0f, 0.0f };
            v.texCoord = { fx, fz };
            mesh.vertices.push_back(v);
        }
    }

    // Indices
    for (int z = 0; z < divisions; ++z) {
        for (int x = 0; x < divisions; ++x) {
            uint32_t topLeft = z * (divisions + 1) + x;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = topLeft + (divisions + 1);
            uint32_t bottomRight = bottomLeft + 1;

            mesh.indices.push_back(topLeft);
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(topRight);

            mesh.indices.push_back(topRight);
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(bottomRight);
        }
    }

    mesh.materialIndex = 0;
    return mesh;
}

Mesh createCylinder(float radius, float height, int segments) {
    Mesh mesh;
    float halfH = height * 0.5f;

    // Side vertices
    for (int i = 0; i <= segments; ++i) {
        float theta = 2.0f * Math::PI * static_cast<float>(i) / static_cast<float>(segments);
        float x = std::cos(theta) * radius;
        float z = std::sin(theta) * radius;
        float u = static_cast<float>(i) / static_cast<float>(segments);

        // Bottom vertex
        ModelVertex vBottom;
        vBottom.position = { x, -halfH, z };
        vBottom.normal = { std::cos(theta), 0.0f, std::sin(theta) };
        vBottom.texCoord = { u, 0.0f };
        mesh.vertices.push_back(vBottom);

        // Top vertex
        ModelVertex vTop;
        vTop.position = { x, halfH, z };
        vTop.normal = { std::cos(theta), 0.0f, std::sin(theta) };
        vTop.texCoord = { u, 1.0f };
        mesh.vertices.push_back(vTop);
    }

    // Side indices
    for (int i = 0; i < segments; ++i) {
        uint32_t curr = i * 2;
        uint32_t next = curr + 2;

        mesh.indices.push_back(curr);
        mesh.indices.push_back(next);
        mesh.indices.push_back(curr + 1);

        mesh.indices.push_back(curr + 1);
        mesh.indices.push_back(next);
        mesh.indices.push_back(next + 1);
    }

    // Top cap center
    uint32_t topCenterIdx = static_cast<uint32_t>(mesh.vertices.size());
    ModelVertex topCenter;
    topCenter.position = { 0.0f, halfH, 0.0f };
    topCenter.normal = { 0.0f, 1.0f, 0.0f };
    topCenter.texCoord = { 0.5f, 0.5f };
    mesh.vertices.push_back(topCenter);

    // Bottom cap center
    uint32_t bottomCenterIdx = static_cast<uint32_t>(mesh.vertices.size());
    ModelVertex bottomCenter;
    bottomCenter.position = { 0.0f, -halfH, 0.0f };
    bottomCenter.normal = { 0.0f, -1.0f, 0.0f };
    bottomCenter.texCoord = { 0.5f, 0.5f };
    mesh.vertices.push_back(bottomCenter);

    // Cap vertices
    uint32_t capStartIdx = static_cast<uint32_t>(mesh.vertices.size());
    for (int i = 0; i <= segments; ++i) {
        float theta = 2.0f * Math::PI * static_cast<float>(i) / static_cast<float>(segments);
        float x = std::cos(theta) * radius;
        float z = std::sin(theta) * radius;

        // Top cap vertex
        ModelVertex vTop;
        vTop.position = { x, halfH, z };
        vTop.normal = { 0.0f, 1.0f, 0.0f };
        vTop.texCoord = { 0.5f + std::cos(theta) * 0.5f, 0.5f + std::sin(theta) * 0.5f };
        mesh.vertices.push_back(vTop);

        // Bottom cap vertex
        ModelVertex vBottom;
        vBottom.position = { x, -halfH, z };
        vBottom.normal = { 0.0f, -1.0f, 0.0f };
        vBottom.texCoord = { 0.5f + std::cos(theta) * 0.5f, 0.5f - std::sin(theta) * 0.5f };
        mesh.vertices.push_back(vBottom);
    }

    // Cap indices
    for (int i = 0; i < segments; ++i) {
        uint32_t curr = capStartIdx + i * 2;
        uint32_t next = curr + 2;

        // Top cap
        mesh.indices.push_back(topCenterIdx);
        mesh.indices.push_back(curr);
        mesh.indices.push_back(next);

        // Bottom cap
        mesh.indices.push_back(bottomCenterIdx);
        mesh.indices.push_back(next + 1);
        mesh.indices.push_back(curr + 1);
    }

    mesh.materialIndex = 0;
    return mesh;
}

Mesh createCone(float radius, float height, int segments) {
    Mesh mesh;
    float halfH = height * 0.5f;

    // Apex vertex
    uint32_t apexIdx = 0;
    ModelVertex apex;
    apex.position = { 0.0f, halfH, 0.0f };
    apex.normal = { 0.0f, 1.0f, 0.0f };
    apex.texCoord = { 0.5f, 1.0f };
    mesh.vertices.push_back(apex);

    // Base vertices (pour les côtés)
    for (int i = 0; i <= segments; ++i) {
        float theta = 2.0f * Math::PI * static_cast<float>(i) / static_cast<float>(segments);
        float x = std::cos(theta) * radius;
        float z = std::sin(theta) * radius;

        // Normale inclinée
        float ny = radius / height;
        Math::Vec3 normal = Math::Vec3(std::cos(theta), ny, std::sin(theta)).normalized();

        ModelVertex v;
        v.position = { x, -halfH, z };
        v.normal = normal;
        v.texCoord = { static_cast<float>(i) / static_cast<float>(segments), 0.0f };
        mesh.vertices.push_back(v);
    }

    // Side indices
    for (int i = 0; i < segments; ++i) {
        mesh.indices.push_back(apexIdx);
        mesh.indices.push_back(1 + i + 1);
        mesh.indices.push_back(1 + i);
    }

    // Base cap center
    uint32_t baseCenterIdx = static_cast<uint32_t>(mesh.vertices.size());
    ModelVertex baseCenter;
    baseCenter.position = { 0.0f, -halfH, 0.0f };
    baseCenter.normal = { 0.0f, -1.0f, 0.0f };
    baseCenter.texCoord = { 0.5f, 0.5f };
    mesh.vertices.push_back(baseCenter);

    // Base cap vertices
    uint32_t baseStartIdx = static_cast<uint32_t>(mesh.vertices.size());
    for (int i = 0; i <= segments; ++i) {
        float theta = 2.0f * Math::PI * static_cast<float>(i) / static_cast<float>(segments);
        float x = std::cos(theta) * radius;
        float z = std::sin(theta) * radius;

        ModelVertex v;
        v.position = { x, -halfH, z };
        v.normal = { 0.0f, -1.0f, 0.0f };
        v.texCoord = { 0.5f + std::cos(theta) * 0.5f, 0.5f - std::sin(theta) * 0.5f };
        mesh.vertices.push_back(v);
    }

    // Base cap indices
    for (int i = 0; i < segments; ++i) {
        mesh.indices.push_back(baseCenterIdx);
        mesh.indices.push_back(baseStartIdx + i + 1);
        mesh.indices.push_back(baseStartIdx + i);
    }

    mesh.materialIndex = 0;
    return mesh;
}

Model createDefaultModel() {
    Model model;
    model.getMeshes().push_back(createCube(1.0f));
    return model;
}

} // namespace Primitives
