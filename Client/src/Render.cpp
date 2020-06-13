#include "Renderer.h"

GLFWwindow* Renderer::Initialize() {
    m_Window = CreateWindow();
    
    bool vulkan_available = CreateInstance();
    if (vulkan_available) vulkan_available = PickPhysicalDevice();
    if (vulkan_available) vulkan_available = CreateLogiaclDevice();
    
    return m_Window;
}

void Renderer::DrawFrame() {
    
}

void Renderer::Destroy() {
    vkDestroyDevice(m_Device, nullptr);
    vkDestroyInstance(m_Instance, nullptr);
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

bool Renderer::CreateInstance() {
    if (EnableValidationLayers && !CheckValidationLayers()) {
        std::cout << "Validation layers requested, but not available\n";
        return false;
    }
    
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Minicraft\n";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&create_info.enabledExtensionCount);;
    
    if (EnableValidationLayers) {
        create_info.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
        create_info.ppEnabledLayerNames = ValidationLayers.data();
    } else {
        create_info.enabledLayerCount = 0;
    }
    
    if (vkCreateInstance(&create_info, nullptr, &m_Instance) != VK_SUCCESS) {
        std::cout << "Failed to create Vulkan instance\n";
        return false;
    }
    
    return true;
}

bool Renderer::PickPhysicalDevice() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(m_Instance, &device_count, nullptr);
    
    if (device_count == 0) {
        std:: cout << "Failed to find GPUs with Vulkan support\n";
        return false;
    }
    
    std::vector<VkPhysicalDevice> physical_devices(device_count);
    vkEnumeratePhysicalDevices(m_Instance, &device_count, physical_devices.data());
    
    // Pick first suitable physical device
    auto suitable = std::find_if(physical_devices.begin(),
                                 physical_devices.end(),
                                 [=](VkPhysicalDevice device) { return IsDeviceSuitable(device); } );
    
    if (suitable != physical_devices.end()) {
        m_PhysicalDevice = *suitable;
        return true;
    } else {
        std::cout << "Failed to find a suitable GPU\n";
        return false;
    }
}

bool Renderer::CreateLogiaclDevice() {
    QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
    
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = indices.GraphicsFamily.value();
    queue_create_info.queueCount = 1;
    float queue_priority = 1.0f;
    queue_create_info.pQueuePriorities = &queue_priority;
    
    VkPhysicalDeviceFeatures device_features{};
    
    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = 0;
    
    if (EnableValidationLayers) {
        device_create_info.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
        device_create_info.ppEnabledLayerNames = ValidationLayers.data();
    } else {
        device_create_info.enabledLayerCount = 0;
    }
    
    if (vkCreateDevice(m_PhysicalDevice, &device_create_info, nullptr, &m_Device) != VK_SUCCESS) {
        std::cout << "Failed to create logical device\n";
        return false;
    }
    
    vkGetDeviceQueue(m_Device, indices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
    
    return true;
}

GLFWwindow* Renderer::CreateWindow() const {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    return glfwCreateWindow(WIDTH, HEIGHT, "Minicraft", nullptr, nullptr);
}

bool Renderer::CheckValidationLayers() const {
    uint32_t layers_count = 0;
    vkEnumerateInstanceLayerProperties(&layers_count, nullptr);
    
    std::vector<VkLayerProperties> available_layers(layers_count);
    vkEnumerateInstanceLayerProperties(&layers_count, available_layers.data());
    
    // Check if all layers in ValidationLayers are presented in available_layers
    for (auto layer : ValidationLayers) {
        auto result = std::find_if(available_layers.begin(),
                                   available_layers.end(),
                                   [&](const VkLayerProperties& rhs){ return strcmp(layer, rhs.layerName) == 0; });
        
        if (result == available_layers.end()) {
            return false;
        }
    }
    
    return true;
}

bool Renderer::IsDeviceSuitable(VkPhysicalDevice device) const {
    auto queue_families_indices = FindQueueFamilies(device);
    
    return queue_families_indices.IsComplete();
}

Renderer::QueueFamilyIndices Renderer::FindQueueFamilies(VkPhysicalDevice device) const {
    uint32_t queue_families_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_families_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, queue_families.data());
    
    // Find index of first VkQueueFamilyProperties in queue_families supporting graphics operations
    QueueFamilyIndices indices;
    auto complete = std::find_if(queue_families.begin(),
                                 queue_families.end(),
                                 [=](VkQueueFamilyProperties queue_family) { return queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT; });
    
    if (complete != queue_families.end()) {
        indices.GraphicsFamily = std::distance(queue_families.begin(), complete);
    }
    
    return indices;
}
