#pragma once

#include "model_loader.h"

namespace Primitives {

// Crée un mesh cube unitaire centré à l'origine
Mesh createCube(float size = 1.0f);

// Crée un mesh sphère
Mesh createSphere(float radius = 0.5f, int segments = 32, int rings = 16);

// Crée un mesh plan
Mesh createPlane(float width = 1.0f, float height = 1.0f, int subdivisions = 1);

// Crée un mesh cylindre
Mesh createCylinder(float radius = 0.5f, float height = 1.0f, int segments = 32);

// Crée un mesh cone
Mesh createCone(float radius = 0.5f, float height = 1.0f, int segments = 32);

// Crée un modèle par défaut (cube) si le chargement échoue
Model createDefaultModel();

} // namespace Primitives
