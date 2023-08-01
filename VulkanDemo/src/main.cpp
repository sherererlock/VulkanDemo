
#include <iostream>

#include "HelloVulkan.h"

int main()
{
    std::string command = "Shaders\\GLSL\\compile.bat";

    int result = std::system(command.c_str());

    HelloVulkan vulkanapp;
    vulkanapp.Init();

    vulkanapp.Run();

    vulkanapp.Cleanup();

    getchar();
    return 0;
}