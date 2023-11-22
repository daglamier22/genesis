#include <Genesis.h>

class Testbed : public Genesis::Application {
    public:
        Testbed(std::string applicationName)
            : Application(applicationName) {
            GN_CLIENT_INFO("Application created.");
        }

        ~Testbed() {
        }
};

Genesis::Application* Genesis::createApp() {
    return new Testbed("TestBed");
}
