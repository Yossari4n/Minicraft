#ifndef Renderer_h
#define Renderer_h

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <cstring>
#include <algorithm>
#include <optional>

constexpr unsigned int WIDTH = 800;
constexpr unsigned int HEIGHT = 600;

const std::vector<const char*> ValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
    const bool EnableValidationLayers = false;
#else
    const bool EnableValidationLayers = true;
#endif

class Renderer {
    struct QueueFamilyIndices {
        std::optional<uint32_t> GraphicsFamily;
        std::optional<uint32_t> PresentFamily;
        
        bool IsComplete() const {
            return GraphicsFamily.has_value() && PresentFamily.has_value();
        }
    };
    
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR Capabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
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
    VkSurfaceKHR m_Surface;
    VkQueue m_PresentQueue;
    VkSwapchainKHR m_Swapchain;
    std::vector<VkImage> m_SwapchainImages;
    VkFormat m_SwapchainImageFormat;
    VkExtent2D m_SwapchainExtent;
    std::vector<VkImageView> m_SwapchainImageViews;
    VkRenderPass m_RenderPass;
    VkPipelineLayout m_PipelineLayout;
    VkPipeline m_GraphicsPipeline;
    
    bool CreateInstance();
    bool CreateSurface();
    bool PickPhysicalDevice();
    bool CreateLogiaclDevice();
    bool CreateSwapchain();
    bool CreateImageViews();
    bool CreateRenderPass();
    bool CreateGraphicPipeline();
    
    GLFWwindow* CreateWindow() const;
    bool CheckValidationLayers() const;
    bool IsDeviceSuitable(VkPhysicalDevice device) const;
    bool CheckDeviceExtensionsSupport(VkPhysicalDevice device) const;
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes) const;
    VkExtent2D ChooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities) const;
    std::vector<char> ReadFile(const std::string& filename) const;
    VkShaderModule CreateShaderModule(const std::vector<char>& code) const;
};

#endif
