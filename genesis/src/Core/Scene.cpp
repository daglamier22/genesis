#include "Scene.h"

namespace Genesis {
    Scene::Scene() {
        positions.insert({meshTypes::GROUND, {}});
        positions.insert({meshTypes::GIRL, {}});
        positions.insert({meshTypes::SKULL, {}});

        positions[meshTypes::GROUND].push_back(glm::vec3(10.0f, 0.0f, 0.0f));
        positions[meshTypes::GIRL].push_back(glm::vec3(17.0f, 0.0f, 0.0f));
        positions[meshTypes::SKULL].push_back(glm::vec3(15.0f, -5.0f, 0.0f));
        positions[meshTypes::SKULL].push_back(glm::vec3(15.0f, 5.0f, 0.0f));
    }

}  // namespace Genesis
