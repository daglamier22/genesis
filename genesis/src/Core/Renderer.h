#pragma once

#include "Window.h"

namespace Genesis {
    class Renderer {
        public:
            Renderer(std::shared_ptr<Window> window) {}
            virtual ~Renderer() {}
            virtual void init(std::shared_ptr<Window> window) = 0;
            virtual void shutdown() = 0;
    };
}  // namespace Genesis
