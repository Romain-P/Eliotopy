#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#include "Application.h"

int main() {
    Application app;
    app.run();
    return 0;
}