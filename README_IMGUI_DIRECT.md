# ImGui Direct LLGL Implementation

Ce projet fournit une nouvelle implémentation d'ImGui directement basée sur LLGL (Lightweight Graphics Library), répondant aux exigences de performance et d'intégration simplifiée.

## Aperçu

Cette implémentation directe offre une alternative performante aux backends ImGui traditionnels (OpenGL, Vulkan, etc.) en utilisant directement les API LLGL pour la gestion des ressources et le rendu.

## Fonctionnalités Principales

### ✅ 1. Initialisation d'ImGui avec LLGL
- Intégration native avec le système de rendu LLGL
- Initialisation simplifiée avec configuration personnalisable
- Support de tous les backends LLGL (OpenGL, Vulkan, Metal, DirectX)

### ✅ 2. Gestion des textures et buffers via LLGL
- Gestion dynamique des buffers vertex/index avec redimensionnement automatique
- Système de textures optimisé pour ImGui (atlas de polices, textures utilisateur)
- Mapping efficace de la mémoire pour les mises à jour fréquentes

### ✅ 3. Rendu des widgets ImGui dans LLGL
- Pipeline de rendu personnalisé avec shaders GLSL optimisés
- Support complet de l'alpha blending et du scissor testing
- Rendu batché pour des performances optimales

### ✅ 4. Structure de code propre et modulable
- Classe `DirectRenderer` encapsulant toute la logique de rendu
- API globale simple pour une intégration facile
- Configuration flexible via la structure `Config`
- Séparation claire entre logique de rendu et application

### ✅ 5. Documentation complète
- Guide d'utilisation détaillé dans `docs/imgui_llgl_direct.md`
- Exemples d'implémentation fonctionnels
- Documentation des API et des meilleures pratiques
- Guide de migration depuis les backends traditionnels

## Fichiers Principaux

- **`src/imgui_llgl_direct.h`** - Interface et définitions de classes
- **`src/imgui_llgl_direct.cpp`** - Implémentation complète du renderer
- **`src/main_simple.cpp`** - Exemple d'utilisation simple
- **`src/main_direct_example.cpp`** - Exemple complet avec démonstration
- **`docs/imgui_llgl_direct.md`** - Documentation détaillée

## Utilisation Rapide

```cpp
#include "imgui_llgl_direct.h"

// Configuration du renderer
ImGuiLLGL::Config config;
config.maxVertices = 65536;
config.maxIndices = 65536;
config.enableAlphaBlending = true;

// Initialisation
if (!ImGuiLLGL::Init(renderSystem, swapChain, config)) {
    // Gestion d'erreur
}

// Boucle de rendu
while (running) {
    ImGuiLLGL::NewFrame();
    ImGui::NewFrame();
    
    // Votre interface ImGui ici
    ImGui::ShowDemoWindow();
    
    // Rendu
    ImGui::Render();
    ImGuiLLGL::RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

// Nettoyage
ImGuiLLGL::Shutdown();
```

## Avantages par rapport aux backends traditionnels

1. **Performance améliorée** - Pas de surcharge des backends intermédiaires
2. **Intégration simplifiée** - Utilisation directe des ressources LLGL
3. **Contrôle précis** - Gestion fine des états de rendu et des ressources
4. **Compatibilité étendue** - Fonctionne avec tous les backends LLGL
5. **Maintenance réduite** - Moins de dépendances externes

## Compilation

Le projet se compile avec les dépendances suivantes :
- LLGL (automatiquement téléchargé via CMake)
- ImGui (automatiquement téléchargé via CMake)
- SDL2 pour la gestion des entrées
- Bibliothèques système (OpenGL, X11 sur Linux)

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make Test-LLGL
```

## Limitations et Considérations

1. **Dépendance LLGL** - Nécessite un système de rendu LLGL initialisé
2. **Compatibilité des shaders** - Les shaders doivent être compatibles avec l'API graphique cible
3. **Gestion des entrées** - Nécessite encore un backend d'entrée séparé (SDL2, Win32, etc.)

## Support et Maintenance

Cette implémentation fournit une base solide pour l'intégration d'ImGui avec LLGL. Elle est conçue pour être facilement extensible et maintenue, avec une architecture modulaire permettant l'ajout de nouvelles fonctionnalités.

La documentation complète et les exemples fournis permettent une adoption rapide et une intégration efficace dans les projets existants utilisant LLGL.