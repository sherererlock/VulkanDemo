
#include <iostream>

#include "HelloVulkan.h"

int main()
{
    HelloVulkan vulkanapp;
    vulkanapp.Init();

    vulkanapp.Run();

    vulkanapp.Cleanup();

    return 0;
}