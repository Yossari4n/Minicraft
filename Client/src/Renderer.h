#ifndef Renderer_h
#define Renderer_h

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <optional>

constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

const std::vector<const char*> ValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
    const bool EnableValidationLayers = false;
#else
    const bool EnableValidationLayers = true;
#endif

class Renderer {
    struct QueueFamilyIndices {
        std::optional<uint32_t> GraphicsFamily;
        
        bool IsComplete() const { return GraphicsFamily.has_value(); }
    };
    
public:
    [[nodiscard]] GLFWwindow* Initialize();
    void DrawFrame();
    void Destroy();
    
private:
    GLFWwindow* m_Window;
    VkInstance m_Instance;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device;
    VkQueue m_GraphicsQueue;
    
    bool CreateInstance();
    bool PickPhysicalDevice();
    bool CreateLogiaclDevice();
    
    GLFWwindow* CreateWindow() const;
    bool CheckValidationLayers() const;
    bool IsDeviceSuitable(VkPhysicalDevice device) const;
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
};

#endif
