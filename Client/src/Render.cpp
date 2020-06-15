#include "Renderer.h"

GLFWwindow* Renderer::Initialize() {
    m_Window = CreateWindow();
    
    bool vulkan_available = CreateInstance();
    if (vulkan_available) vulkan_available = CreateSurface();
    if (vulkan_available) vulkan_available = PickPhysicalDevice();
    if (vulkan_available) vulkan_available = CreateLogiaclDevice();
    if (vulkan_available) vulkan_available = CreateSwapchain();
    if (vulkan_available) vulkan_available = CreateImageViews();
    
    if (!vulkan_available) {
        glfwSetWindowTitle(m_Window, "Failed to initialize Vulkan");
    }
    
    return m_Window;
}

void Renderer::DrawFrame() {
    
}

void Renderer::Destroy() {
    for (auto view : m_SwapchainImageViews) {
        vkDestroyImageView(m_Device, view, nullptr);
    }
    
    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
    vkDestroyDevice(m_Device, nullptr);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyInstance(m_Instance, nullptr);
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

bool Renderer::CreateInstance() {
    if (EnableValidationLayers && !CheckValidationLayers()) {
        std::cerr << "Validation layers requested, but not available\n";
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
        std::cerr << "Failed to create Vulkan instance\n";
        return false;
    }
    
    return true;
}

bool Renderer::CreateSurface() {
    if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface) != VK_SUCCESS) {
        std::cerr << "Failed to create window surface\n";
        return false;
    }
    
    return true;
}

bool Renderer::PickPhysicalDevice() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(m_Instance, &device_count, nullptr);
    
    if (device_count == 0) {
        std::cerr << "Failed to find GPUs with Vulkan support\n";
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
        std::cerr << "Failed to find a suitable GPU\n";
        return false;
    }
}

bool Renderer::CreateLogiaclDevice() {
    QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
    
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families{ indices.GraphicsFamily.value(), indices.PresentFamily.value() };
    
    for (auto family : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = family;
        queue_create_info.queueCount = 1;
        float queue_priority = 1.0f;
        queue_create_info.pQueuePriorities = &queue_priority;
        
        queue_create_infos.push_back(queue_create_info);
    }
    
    VkPhysicalDeviceFeatures device_features{};
    
    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
    device_create_info.ppEnabledExtensionNames = DeviceExtensions.data();
    
    if (EnableValidationLayers) {
        device_create_info.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
        device_create_info.ppEnabledLayerNames = ValidationLayers.data();
    } else {
        device_create_info.enabledLayerCount = 0;
    }
    
    if (vkCreateDevice(m_PhysicalDevice, &device_create_info, nullptr, &m_Device) != VK_SUCCESS) {
        std::cerr << "Failed to create logical device\n";
        return false;
    }
    
    vkGetDeviceQueue(m_Device, indices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, indices.PresentFamily.value(), 0, &m_PresentQueue);
    
    return true;
}

bool Renderer::CreateSwapchain() {
    auto swap_chain_support = QuerySwapChainSupport(m_PhysicalDevice);
    
    auto format = ChooseSwapSurfaceFormat(swap_chain_support.Formats);
    auto mode = ChooseSwapPresentMode(swap_chain_support.PresentModes);
    auto extend = ChooseSwapExtent(swap_chain_support.Capabilities);
    
    // Make sure images count is set to recommended +1 value but does not exceed maxImageExtent
    uint32_t images_count = swap_chain_support.Capabilities.minImageCount + 1;
    if (swap_chain_support.Capabilities.maxImageCount > 0 && images_count > swap_chain_support.Capabilities.maxImageCount) {
        images_count = swap_chain_support.Capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = m_Surface;
    create_info.minImageCount = images_count;
    create_info.imageFormat = format.format;
    create_info.imageColorSpace = format.colorSpace;
    create_info.presentMode = mode;
    create_info.imageExtent = extend;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    auto indices = FindQueueFamilies(m_PhysicalDevice);
    uint32_t queue_family_indices[] = { indices.GraphicsFamily.value(), indices.PresentFamily.value() };
    
    if (indices.GraphicsFamily.value() != indices.PresentFamily.value()) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    create_info.preTransform = swap_chain_support.Capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;
    
    if (vkCreateSwapchainKHR(m_Device, &create_info, nullptr, &m_Swapchain) != VK_SUCCESS) {
        std::cerr << "Failed to create swap chain\n";
        return false;
    }
    
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &images_count, nullptr);
    m_SwapchainImages.resize(images_count);
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &images_count, m_SwapchainImages.data());
    
    m_SwapchainImageFormat = format.format;
    m_SwapchainExtent = extend;
    
    return true;
}

bool Renderer::CreateImageViews() {
    m_SwapchainImageViews.resize(m_SwapchainImages.size());
    
    for (int i = 0; i < m_SwapchainImages.size(); i++) {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = m_SwapchainImages[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = m_SwapchainImageFormat;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(m_Device, &create_info, nullptr, &m_SwapchainImageViews[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create image view\n";
            return false;
        }
    }
    
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
                                   [=](const VkLayerProperties& rhs) { return strcmp(layer, rhs.layerName) == 0; });
        
        if (result == available_layers.end()) {
            return false;
        }
    }
    
    return true;
}

bool Renderer::IsDeviceSuitable(VkPhysicalDevice device) const {
    auto queue_families_indices = FindQueueFamilies(device);
    
    bool extensions_supported = CheckDeviceExtensionsSupport(device);
    
    bool swap_chain_adequate = false;
    if (extensions_supported) {
        auto swap_chain = QuerySwapChainSupport(device);
        swap_chain_adequate = !swap_chain.Formats.empty() && !swap_chain.PresentModes.empty();
    }
    
    return queue_families_indices.IsComplete()
        && extensions_supported
        && swap_chain_adequate;
}

bool Renderer::CheckDeviceExtensionsSupport(VkPhysicalDevice device) const {
    uint32_t extensions_count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensions_count, nullptr);
    std::vector<VkExtensionProperties> available_extensions(extensions_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensions_count, available_extensions.data());
    
    // Check if all extensions in DeviceExtensions are presented in available_extensions
    for (auto extension : DeviceExtensions) {
        auto result = std::find_if(available_extensions.begin(),
                                   available_extensions.end(),
                                   [=](const VkExtensionProperties& rhs) { return strcmp(extension, rhs.extensionName) == 0; });
        
        if (result == available_extensions.end()) {
            return false;
        }
    }
    
    return true;
}

Renderer::QueueFamilyIndices Renderer::FindQueueFamilies(VkPhysicalDevice device) const {
    uint32_t queue_families_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_families_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, queue_families.data());
    
    // Find indices of VkQueueFamilyProperties supporting graphics operations and present operations
    QueueFamilyIndices indices;
    for (int i = 0; i < queue_families.size() && !indices.IsComplete(); i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.GraphicsFamily = i;
        }
        
        VkBool32 present_supported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &present_supported);
        if (present_supported) {
            indices.PresentFamily = i;
        }
    }
    
    return indices;
}

Renderer::SwapChainSupportDetails Renderer::QuerySwapChainSupport(VkPhysicalDevice device) const {
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.Capabilities);
    
    uint32_t formats_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formats_count, nullptr);
    details.Formats.resize(formats_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formats_count, details.Formats.data());
    
    uint32_t presents_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presents_count, nullptr);
    details.PresentModes.resize(presents_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presents_count, details.PresentModes.data());
    
    return details;
}

VkSurfaceFormatKHR Renderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats) const {
    auto result = std::find_if(formats.begin(),
                               formats.end(),
                               [=](VkSurfaceFormatKHR availabe) { return availabe.format == VK_FORMAT_B8G8R8A8_SRGB && availabe.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; });
    
    // If no preferable format has been found return the first
    return result != formats.end() ? *result : formats[0];
}

VkPresentModeKHR Renderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &modes) const {
    auto result = std::find_if(modes.begin(),
                               modes.end(),
                               [=](VkPresentModeKHR available) { return available == VK_PRESENT_MODE_MAILBOX_KHR; });
    
    // If no preferable mode has been found return one guaranteed to ba available
    return result != modes.end() ? *result : VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::ChooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities) const {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    
    VkExtent2D extend;
    extend.width = std::clamp(WIDTH, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extend.height = std::clamp(HEIGHT, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    
    return extend;
}
