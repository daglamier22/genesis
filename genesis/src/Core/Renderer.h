#pragma once

namespace Genesis {
    class Renderer {
        public:
            virtual ~Renderer() {}
            virtual void init() = 0;
            virtual void shutdown() = 0;
    };
}  // namespace Genesis
