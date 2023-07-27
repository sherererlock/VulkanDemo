#pragma once

#define CASCADED_COUNT 1
#define DEFAULT_FENCE_TIMEOUT 10000000000000000

#define Trans_Data_To_GPU \
void* data;\
vkMapMemory(device, uniformMemory, 0, bufferSize, 0, &data); \
memcpy(data, &ubo, bufferSize); \
vkUnmapMemory(device, uniformMemory); \

#pragma warning(disable:26812)
#pragma warning(disable:26495)