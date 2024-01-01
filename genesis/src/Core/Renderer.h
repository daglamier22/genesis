#pragma once

#include "Scene.h"
#include "Window.h"

namespace Genesis {
    class Renderer {
        public:
            Renderer(std::shared_ptr<Window> window) : m_window(window) {}
            virtual ~Renderer() {}

            std::shared_ptr<Window> getWindow() const { return m_window; }

            virtual void init() = 0;
            virtual bool drawFrame(std::shared_ptr<Scene>) = 0;
            virtual void shutdown() = 0;
            virtual void waitForIdle() = 0;

        protected:
            std::shared_ptr<Window> m_window;
    };
}  // namespace Genesis
