#pragma once

namespace Genesis {
    class Application {
        public:
            Application();
            virtual ~Application();

            void run();
    };

    Application* createApp();
}
