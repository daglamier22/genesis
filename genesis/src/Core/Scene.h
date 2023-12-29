#pragma once

#include <glm/glm.hpp>

namespace Genesis {
    enum class meshTypes {
        GROUND,
        GIRL,
        SKULL
    };

    class Scene {
        public:
            Scene();

            std::unordered_map<meshTypes, std::vector<glm::vec3>> positions;
    };
}  // namespace Genesis
