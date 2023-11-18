#include <genesis.h>

class Testbed : public Genesis::Application {
    public:
        Testbed() {

        }

        ~Testbed() {

        }
};

Genesis::Application* Genesis::createApp() {
    return new Testbed();
}
