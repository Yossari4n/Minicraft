#ifndef Renderer_h
#define Renderer_h

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>

#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <cstring>
#include <algorithm>
#include <optional>
#include <array>

constexpr unsigned int WIDTH = 800;
constexpr unsigned int HEIGHT = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
    constexpr bool EnableValidationLayers = false;
#else
    constexpr bool EnableValidationLayers = true;
#endif

const std::vector<const char*> ValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class Renderer {
    // Contains hooks to all queue families required by renderer
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
    
    static void FramebufferResizeCallback(GLFWwindow *window, int width, int height) {
        auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
        app->m_FramebufferResized = true;
    }
    
    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
    };
    
public:
    struct Vertex {
        glm::vec2 Position;
        glm::vec3 Color;
        glm::vec2 TextureCoordinate;
        
        static VkVertexInputBindingDescription BingindDescription() {
            VkVertexInputBindingDescription description{};
            description.binding = 0;
            description.stride = sizeof(Vertex);
            description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return description;
        }
        
        static std::array<VkVertexInputAttributeDescription, 3> AttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 3> descriptions;
            descriptions[0].binding = 0;
            descriptions[0].location = 0;
            descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            descriptions[0].offset = offsetof(Vertex, Position);
            
            descriptions[1].binding = 0;
            descriptions[1].location = 1;
            descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            descriptions[1].offset = offsetof(Vertex, Color);
            
            descriptions[2].binding = 0;
            descriptions[2].location = 2;
            descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            descriptions[2].offset = offsetof(Vertex, TextureCoordinate);
            
            return descriptions;
        }
    };
    
    [[nodiscard]] GLFWwindow* Initialize();
    void DrawFrame();
    void Destroy();
    
private:
    GLFWwindow* m_Window;
    
    // Setup
    VkInstance m_Instance;
    VkSurfaceKHR m_Surface;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device;
    
    VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;
    VkSwapchainKHR m_Swapchain;
    std::vector<VkImage> m_SwapchainImages;
    VkFormat m_SwapchainImageFormat;
    VkExtent2D m_SwapchainExtent;
    std::vector<VkImageView> m_SwapchainImageViews;
    VkRenderPass m_RenderPass;
    VkDescriptorSetLayout m_DescriptorSetLayout;
    VkPipelineLayout m_PipelineLayout;
    VkPipeline m_GraphicsPipeline;
    std::vector<VkFramebuffer> m_SwapchainFramebuffers;
    VkCommandPool m_CommandPool;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    //TODO std::array
    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;
    std::vector<VkFence> m_ImagesInFlight;
    size_t m_CurrentFrame = 0;
    VkBuffer m_VertexBuffer;
    VkDeviceMemory m_VertexBufferMemory;
    VkBuffer m_IndexBuffer;
    VkDeviceMemory m_IndexBufferMemory;
    std::vector<VkBuffer> m_UniformBuffers;
    std::vector<VkDeviceMemory> m_UniformBuffersMemory;
    VkDescriptorPool m_DescriptorPool;
    std::vector<VkDescriptorSet> m_DescriptorSets;
    VkImage m_TextureImage;
    VkDeviceMemory m_TextureImageMemory;
    VkImageView m_TextureImageView;
    VkSampler m_Sampler;
    
    bool m_FramebufferResized = false;
    
    void DestroySwapchain();
    void RecreateSwapchain();
    void UpdateUniformBuffer(uint32_t index);
    
    void CreateWindow();
    bool CreateInstance();
    bool CreateSurface();
    bool PickPhysicalDevice();
    bool CreateLogiaclDevice();
    bool CreateSwapchain();
    bool CreateImageViews();
    bool CreateRenderPass();
    bool CreateDescriptorSetLayout();
    bool CreateGraphicPipeline();
    bool CreateFramebuffers();
    bool CreateCommandPool();
    bool CreateTextureImage();
    bool CreateTextureImageView();
    bool CreateTextureSampler();
    bool CreateVertexBuffer();
    bool CreateIndexBuffer();
    bool CreateUniformBuffers();
    bool CreateDescriptorPool();
    bool CreateDescriptorSet();
    bool CreateCommandBuffers();
    bool CreateSyncObjects();
    
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
    uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) const;
    bool CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties, VkBuffer* buffer, VkDeviceMemory* buffer_memory) const;
    void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) const;
    void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_properties, VkImage* image, VkDeviceMemory* image_memory) const;
    VkCommandBuffer BeginSingleTimeCommands() const;
    void EndSingleTimeCommands(VkCommandBuffer buffer) const;
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout) const;
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
    VkImageView CreateImageView(VkImage image, VkFormat format) const;
};

const std::vector<Renderer::Vertex> Vertices = {
    // Position         Color                   Texture Coordinates
    {{-0.5f, -0.5f},    {1.0f, 0.0f, 0.0f},     {0.0f, 0.0f}},
    {{0.5f, -0.5f},     {0.0f, 1.0f, 0.0f},     {1.0f, 0.0f}},
    {{0.5f, 0.5f},      {0.0f, 0.0f, 1.0f},     {1.0f, 1.0f}},
    {{-0.5f, 0.5f},     {1.0f, 1.0f, 1.0f},     {0.0f, 1.0f}}
};

const std::vector<uint16_t> Indices = {
    0, 1, 2, 2, 3, 0
};

#endif
