#include <genesis.h>

class Testbed : public Genesis::Application {
    public:
        Testbed(std::string applicationName)
            : Application(applicationName) {
            GN_CLIENT_IFNO("Application created.");
        }

        ~Testbed() {
        }
};

Genesis::Application* Genesis::createApp() {
    return new Testbed("TestBed");
}
