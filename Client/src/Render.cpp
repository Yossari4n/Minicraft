#include "Renderer.h"

bool operator==(const Renderer::Vertex left, const Renderer::Vertex& right) {
    return
        left.Position == right.Position
        && left.Color == right.Color
        && left.TextureCoordinate == right.TextureCoordinate;
}

GLFWwindow* Renderer::Initialize() {
    CreateWindow();
    
    bool vulkan_available = CreateInstance();
    if (vulkan_available) vulkan_available = CreateSurface();
    if (vulkan_available) vulkan_available = PickPhysicalDevice();
    if (vulkan_available) vulkan_available = CreateLogiaclDevice();
    if (vulkan_available) vulkan_available = CreateSwapchain();
    if (vulkan_available) vulkan_available = CreateImageViews();
    if (vulkan_available) vulkan_available = CreateRenderPass();
    if (vulkan_available) vulkan_available = CreateDescriptorSetLayout();
    if (vulkan_available) vulkan_available = CreateGraphicPipeline();
    if (vulkan_available) vulkan_available = CreateCommandPool();
    if (vulkan_available) vulkan_available = CreateDepthResources();
    if (vulkan_available) vulkan_available = CreateFramebuffers();
    if (vulkan_available) vulkan_available = CreateTextureImage();
    if (vulkan_available) vulkan_available = CreateTextureImageView();
    if (vulkan_available) vulkan_available = CreateTextureSampler();
    if (vulkan_available) vulkan_available = LoadModel();
    if (vulkan_available) vulkan_available = CreateVertexBuffer();
    if (vulkan_available) vulkan_available = CreateIndexBuffer();
    if (vulkan_available) vulkan_available = CreateUniformBuffers();
    if (vulkan_available) vulkan_available = CreateDescriptorPool();
    if (vulkan_available) vulkan_available = CreateDescriptorSet();
    if (vulkan_available) vulkan_available = CreateCommandBuffers();
    if (vulkan_available) vulkan_available = CreateSyncObjects();
    
    if (!vulkan_available) {
        glfwSetWindowTitle(m_Window, "Failed to initialize Vulkan");
    }
    
    return m_Window;
}

void Renderer::DrawFrame() {
    vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
    
    uint32_t image_index = -1;
    VkResult swapchain_status = vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &image_index);
    
    if (swapchain_status == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return;
    } else if (swapchain_status != VK_SUCCESS && swapchain_status != VK_SUBOPTIMAL_KHR) {
        std::cerr << "Failed to acquire next image\n";
    }
    
    if (m_ImagesInFlight[image_index] != VK_NULL_HANDLE) {
        vkWaitForFences(m_Device, 1, &m_ImagesInFlight[image_index], VK_TRUE, UINT64_MAX);
    }
    m_ImagesInFlight[image_index] = m_InFlightFences[m_CurrentFrame];
    
    VkSemaphore wait_semaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
    VkSemaphore signal_semaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    
    UpdateUniformBuffer(image_index);
    
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_CommandBuffers[image_index];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    
    vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);
    
    if (vkQueueSubmit(m_GraphicsQueue, 1, &submit_info, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) {
        std::cerr << "Failed to submit draw command buffer\n";
    }
    
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &m_Swapchain;
    present_info.pImageIndices = &image_index;
    
    swapchain_status = vkQueuePresentKHR(m_PresentQueue, &present_info);
    if (swapchain_status == VK_ERROR_OUT_OF_DATE_KHR || swapchain_status == VK_SUBOPTIMAL_KHR || m_FramebufferResized) {
        m_FramebufferResized = false;
        RecreateSwapchain();
    } else if (swapchain_status != VK_SUCCESS) {
        std::cerr << "Failed to present swap chain image\n";
    }
    
    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::Destroy() {
    vkDeviceWaitIdle(m_Device);
    
    DestroySwapchain();
    vkDestroySampler(m_Device, m_Sampler, nullptr);
    vkDestroyImageView(m_Device, m_TextureImageView, nullptr);
    vkDestroyImage(m_Device, m_TextureImage, nullptr);
    vkFreeMemory(m_Device, m_TextureImageMemory, nullptr);
    vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
    }
    vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
    vkFreeMemory(m_Device, m_IndexBufferMemory, nullptr);
    vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
    vkFreeMemory(m_Device, m_VertexBufferMemory, nullptr);
    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
    vkDestroyDevice(m_Device, nullptr);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyInstance(m_Instance, nullptr);
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Renderer::DestroySwapchain() {
    vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
    vkDestroyImage(m_Device, m_DepthImage, nullptr);
    vkFreeMemory(m_Device, m_DepthImageMemory, nullptr);
    
    for (auto framebuffer : m_SwapchainFramebuffers) {
        vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
    }
    
    for (size_t i = 0; i < m_SwapchainImages.size(); i++) {
        vkDestroyBuffer(m_Device, m_UniformBuffers[i], nullptr);
        vkFreeMemory(m_Device, m_UniformBuffersMemory[i], nullptr);
    }
    
    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
    vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
    for (auto view : m_SwapchainImageViews) {
        vkDestroyImageView(m_Device, view, nullptr);
    }
    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
}

void Renderer::RecreateSwapchain() {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_Window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_Window, &width, &height);
        glfwWaitEvents();
    }
    
    vkDeviceWaitIdle(m_Device);
    
    CreateSwapchain();
    CreateImageViews();
    CreateRenderPass();
    CreateGraphicPipeline();
    CreateDepthResources();
    CreateFramebuffers();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSet();
    CreateCommandBuffers();
}

void Renderer::UpdateUniformBuffer(uint32_t index) {
    static auto start = std::chrono::high_resolution_clock::now();
    
    auto current = std::chrono::high_resolution_clock::now();
    float delta = std::chrono::duration<float, std::chrono::seconds::period>(current - start).count();
    
    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f); //glm::rotate(glm::mat4(1.0f), delta * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.projection = glm::perspective(glm::radians(45.0f), m_SwapchainExtent.width / (float)m_SwapchainExtent.height, 0.1f, 10.0f);
    ubo.projection[1][1] *= -1;
    
    void* data;
    vkMapMemory(m_Device, m_UniformBuffersMemory[index], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(m_Device, m_UniformBuffersMemory[index]);
}

void Renderer::CreateWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    
    m_Window = glfwCreateWindow(WIDTH, HEIGHT, "Minicraft", nullptr, nullptr);
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, Renderer::FramebufferResizeCallback);
}

bool Renderer::CreateInstance() {
    if (EnableValidationLayers && !CheckValidationLayers()) {
        std::cerr << "Validation layers requested, but not available\n";
        return false;
    }
    
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Minicraft";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo instance{};
    instance.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance.pApplicationInfo = &app_info;
    instance.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&instance.enabledExtensionCount);;
    
    if (EnableValidationLayers) {
        instance.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
        instance.ppEnabledLayerNames = ValidationLayers.data();
    } else {
        instance.enabledLayerCount = 0;
    }
    
    if (vkCreateInstance(&instance, nullptr, &m_Instance) != VK_SUCCESS) {
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
    
    float queue_priority = 1.0f;
    for (auto family : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        
        queue_create_infos.push_back(queue_create_info);
    }
    
    VkPhysicalDeviceFeatures device_features{};
    device_features.samplerAnisotropy = VK_TRUE;
    
    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
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
    
    if (indices.GraphicsFamily != indices.PresentFamily) {
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
        m_SwapchainImageViews[i] = CreateImageView(m_SwapchainImages[i], m_SwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        
        if (m_SwapchainImageViews[i] == VK_NULL_HANDLE) {
            return false;
        }
    }
    
    return true;
}

bool Renderer::CreateRenderPass() {
    VkAttachmentDescription color_attachment{};
    color_attachment.format = m_SwapchainImageFormat;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = FindDepthFormat();
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;
    
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };
    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_create_info.pAttachments = attachments.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;
    
    if (vkCreateRenderPass(m_Device, &render_pass_create_info, nullptr, &m_RenderPass) != VK_SUCCESS) {
        std::cerr << "Failed to create render pass\n";
        return false;
    }
    
    return true;
}

bool Renderer::CreateDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding ubo_binding{};
    ubo_binding.binding = 0;
    ubo_binding.descriptorCount = 1;
    ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    VkDescriptorSetLayoutBinding sampler_binding{};
    sampler_binding.binding = 1;
    sampler_binding.descriptorCount = 1;
    sampler_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_binding.pImmutableSamplers = nullptr;
    sampler_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {ubo_binding, sampler_binding};
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout{};
    descriptor_set_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout.bindingCount = static_cast<uint32_t>(bindings.size());
    descriptor_set_layout.pBindings = bindings.data();
    
    if (vkCreateDescriptorSetLayout(m_Device, &descriptor_set_layout, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor set layout\n";
        return false;
    }
    
    return true;
}

bool Renderer::CreateGraphicPipeline() {
    auto vert_shader_code = ReadFile("resources/vert.spv");
    auto frag_shader_code = ReadFile("resources/frag.spv");
    
    VkShaderModule vert_shader_module = CreateShaderModule(vert_shader_code);
    VkShaderModule frag_shader_module = CreateShaderModule(frag_shader_code);
    
    VkPipelineShaderStageCreateInfo vert_create_info{};
    vert_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_create_info.module = vert_shader_module;
    vert_create_info.pName = "main";
    
    VkPipelineShaderStageCreateInfo frag_create_info{};
    frag_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_create_info.module = frag_shader_module;
    frag_create_info.pName = "main";
    
    VkPipelineShaderStageCreateInfo shader_stages[] = { vert_create_info, frag_create_info };
    
    auto binding_description = Vertex::BingindDescription();
    auto attribute_descriptions = Vertex::AttributeDescriptions();
    
    VkPipelineVertexInputStateCreateInfo vertex_input_create_info{};
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_create_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_create_info.pVertexAttributeDescriptions = attribute_descriptions.data();
    
    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info{};
    input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_create_info.primitiveRestartEnable = VK_FALSE;
    
    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(m_SwapchainExtent.width);
    viewport.height = static_cast<float>(m_SwapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissors{};
    scissors.offset = {0, 0};
    scissors.extent = m_SwapchainExtent;
    
    VkPipelineViewportStateCreateInfo viewport_state_create_info{};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissors;
    
    VkPipelineRasterizationStateCreateInfo rasterization_create_info{};
    rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_create_info.depthClampEnable = VK_FALSE;
    rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_create_info.lineWidth = 1.0f;
    rasterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_create_info.depthBiasEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo multisampling_create_info{};
    multisampling_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_create_info.sampleShadingEnable = VK_FALSE;
    multisampling_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info{};
    depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_create_info.stencilTestEnable = VK_FALSE;
    
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo color_blend_create_info{};
    color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_create_info.logicOpEnable = VK_FALSE;
    color_blend_create_info.attachmentCount = 1;
    color_blend_create_info.pAttachments = &color_blend_attachment;
    
    VkPipelineLayoutCreateInfo layout_create_info{};
    layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.setLayoutCount = 1;
    layout_create_info.pSetLayouts = &m_DescriptorSetLayout;
    layout_create_info.pushConstantRangeCount = 0;
    layout_create_info.pPushConstantRanges = nullptr;
    
    if (vkCreatePipelineLayout(m_Device, &layout_create_info, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create pipeline layout\n";
        return false;
    }
    
    VkGraphicsPipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stages;
    pipeline_create_info.pVertexInputState = &vertex_input_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
    pipeline_create_info.pViewportState = &viewport_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterization_create_info;
    pipeline_create_info.pMultisampleState = &multisampling_create_info;
    pipeline_create_info.pColorBlendState = &color_blend_create_info;
    pipeline_create_info.layout = m_PipelineLayout;
    pipeline_create_info.renderPass = m_RenderPass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
    
    if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &m_GraphicsPipeline) != VK_SUCCESS) {
        std::cerr << "Failed to create graphics pipeline\n";
        return false;
    }
    
    vkDestroyShaderModule(m_Device, vert_shader_module, nullptr);
    vkDestroyShaderModule(m_Device, frag_shader_module, nullptr);
    
    return true;
}

bool Renderer::CreateFramebuffers() {
    m_SwapchainFramebuffers.resize(m_SwapchainImageViews.size());
    
    for (size_t i = 0; i < m_SwapchainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = { m_SwapchainImageViews[i], m_DepthImageView };
        
        VkFramebufferCreateInfo framebuffer_create_info{};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass = m_RenderPass;
        framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebuffer_create_info.pAttachments = attachments.data();
        framebuffer_create_info.width = m_SwapchainExtent.width;
        framebuffer_create_info.height = m_SwapchainExtent.height;
        framebuffer_create_info.layers = 1;
        
        if (vkCreateFramebuffer(m_Device, &framebuffer_create_info, nullptr, &m_SwapchainFramebuffers[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create framebuffer\n";
            return false;
        }
    }
    
    return true;
}

bool Renderer::CreateCommandPool() {
    QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
    
    VkCommandPoolCreateInfo command_pool_create_info{};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.queueFamilyIndex = indices.GraphicsFamily.value();
    
    if (vkCreateCommandPool(m_Device, &command_pool_create_info, nullptr, &m_CommandPool) != VK_SUCCESS) {
        std::cerr << "Failed to create command pool\n";
        return false;
    }
    
    return true;
}

bool Renderer::CreateDepthResources() {
    auto format = FindDepthFormat();
    
    CreateImage(m_SwapchainExtent.width, m_SwapchainExtent.height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_DepthImage, &m_DepthImageMemory);
    m_DepthImageView = CreateImageView(m_DepthImage, format, VK_IMAGE_ASPECT_DEPTH_BIT);
    
    return true;
}

bool Renderer::CreateTextureImage() {
    int width, height, channels;
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    
    VkDeviceSize size = width * height * 4;
    
    if (!pixels) {
        std::cerr << "Failed to load texture\n";
        return false;
    }
    
    VkBuffer staging;
    VkDeviceMemory staging_memory;
    CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging, &staging_memory);
    
    void* data;
    vkMapMemory(m_Device, staging_memory, 0, size, 0, &data);
    memcpy(data, pixels, static_cast<uint32_t>(size));
    vkUnmapMemory(m_Device, staging_memory);
    
    CreateImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_TextureImage, &m_TextureImageMemory);
    TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage(staging, m_TextureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    
    TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    vkDestroyBuffer(m_Device, staging, nullptr);
    vkFreeMemory(m_Device, staging_memory, nullptr);
    
    return true;
}

bool Renderer::CreateTextureImageView() {
    m_TextureImageView = CreateImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    
    if (m_TextureImageView == VK_NULL_HANDLE) {
        return false;
    }
    
    return true;
}

bool Renderer::CreateTextureSampler() {
    VkSamplerCreateInfo sampler{};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.anisotropyEnable = VK_TRUE;
    sampler.maxAnisotropy = 16.0f;
    sampler.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler.unnormalizedCoordinates = VK_FALSE;
    sampler.compareEnable = VK_FALSE;
    sampler.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.mipLodBias = 0.0f;
    sampler.minLod = 0.0f;
    sampler.maxLod = 0.0f;
    
    if (vkCreateSampler(m_Device, &sampler, nullptr, &m_Sampler) != VK_SUCCESS) {
        std::cerr << "Failed to create texture sampler\n";
        return false;
    }
    
    return true;
}

bool Renderer::LoadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;
    
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        std::cerr << "Failed to load object\n";
        return false;
    }
    
    std::unordered_map<Vertex, uint32_t> unique_vertices;
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            
            vertex.Position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            
            vertex.TextureCoordinate = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };
            
            if (unique_vertices.count(vertex) == 0) {
                unique_vertices[vertex] = static_cast<uint32_t>(m_Vertices.size());
                m_Vertices.push_back(vertex);
            }
            
            m_Indices.push_back(unique_vertices[vertex]);
        }
    }
    
    return true;
}

bool Renderer::CreateVertexBuffer() {
    VkDeviceSize size = sizeof(m_Vertices[0]) * m_Vertices.size();
    
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    if (!CreateBuffer(size,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      &staging_buffer,
                      &staging_buffer_memory)) {
        std::cerr << "Failed to create staging buffer for vertex buffer\n";
        return false;
    }
    
    void* data;
    vkMapMemory(m_Device, staging_buffer_memory, 0, size, 0, &data);
    memcpy(data, m_Vertices.data(), size);
    vkUnmapMemory(m_Device, staging_buffer_memory);
    
    if (!CreateBuffer(size,
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      &m_VertexBuffer,
                      &m_VertexBufferMemory)) {
        std::cerr << "Failed to create vertex buffer\n";
        return false;
    }
    
    CopyBuffer(staging_buffer, m_VertexBuffer, size);
    
    vkDestroyBuffer(m_Device, staging_buffer, nullptr);
    vkFreeMemory(m_Device, staging_buffer_memory, nullptr);
    
    return true;
}

bool Renderer::CreateIndexBuffer() {
    VkDeviceSize size = sizeof(m_Indices[0]) * m_Indices.size();
    
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    if (!CreateBuffer(size,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      &staging_buffer,
                      &staging_buffer_memory)) {
        std::cerr << "Failed to create staging buffer for index buffer";
        return false;
    }
    
    void* data;
    vkMapMemory(m_Device, staging_buffer_memory, 0, size, 0, &data);
    memcpy(data, m_Indices.data(), size);
    vkUnmapMemory(m_Device, staging_buffer_memory);
    
    if (!CreateBuffer(size,
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      &m_IndexBuffer,
                      &m_IndexBufferMemory)) {
        std::cerr << "Failed to create index buffer";
        return false;
    }
    
    CopyBuffer(staging_buffer, m_IndexBuffer, size);
    
    vkDestroyBuffer(m_Device, staging_buffer, nullptr);
    vkFreeMemory(m_Device, staging_buffer_memory, nullptr);
    
    return true;
}

bool Renderer::CreateUniformBuffers() {
    VkDeviceSize size = sizeof(UniformBufferObject);
    
    m_UniformBuffers.resize(m_SwapchainImages.size());
    m_UniformBuffersMemory.resize(m_SwapchainImages.size());
    
    for (size_t i = 0; i < m_SwapchainImages.size(); i++) {
        CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_UniformBuffers[i], &m_UniformBuffersMemory[i]);
    }
    
    return true;
}

bool Renderer::CreateDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> sizes;
    sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sizes[0].descriptorCount = static_cast<uint32_t>(m_SwapchainImages.size());
    sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sizes[1].descriptorCount = static_cast<uint32_t>(m_SwapchainImages.size());
    
    VkDescriptorPoolCreateInfo descriptor_pool{};
    descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool.poolSizeCount = static_cast<uint32_t>(sizes.size());
    descriptor_pool.pPoolSizes = sizes.data();
    descriptor_pool.maxSets = static_cast<uint32_t>(m_SwapchainImages.size());
    
    if (vkCreateDescriptorPool(m_Device, &descriptor_pool, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor pool\n";
        return false;
    }
    
    return true;
}

bool Renderer::CreateDescriptorSet() {
    std::vector<VkDescriptorSetLayout> layouts(m_SwapchainImages.size(), m_DescriptorSetLayout);
    
    VkDescriptorSetAllocateInfo descriptor_set{};
    descriptor_set.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set.descriptorPool = m_DescriptorPool;
    descriptor_set.descriptorSetCount = static_cast<uint32_t>(m_SwapchainImages.size());
    descriptor_set.pSetLayouts = layouts.data();
    
    m_DescriptorSets.resize(m_SwapchainImages.size());
    if (vkAllocateDescriptorSets(m_Device, &descriptor_set, m_DescriptorSets.data()) != VK_SUCCESS) {
        std::cerr << "Failed to allocate descriptor sets\n";
        return false;
    }
    
    for (size_t i = 0; i < m_SwapchainImages.size(); i++) {
        VkDescriptorBufferInfo descriptor_buffer{};
        descriptor_buffer.buffer = m_UniformBuffers[i];
        descriptor_buffer.offset = 0;
        descriptor_buffer.range = sizeof(UniformBufferObject);
        
        VkDescriptorImageInfo descriptor_image{};
        descriptor_image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptor_image.imageView = m_TextureImageView;
        descriptor_image.sampler = m_Sampler;
        
        std::array<VkWriteDescriptorSet, 2> write_descriptors{};
        write_descriptors[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptors[0].dstSet = m_DescriptorSets[i];
        write_descriptors[0].dstBinding = 0;
        write_descriptors[0].dstArrayElement = 0;
        write_descriptors[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_descriptors[0].descriptorCount = 1;
        write_descriptors[0].pBufferInfo = &descriptor_buffer;
        
        write_descriptors[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptors[1].dstSet = m_DescriptorSets[i];
        write_descriptors[1].dstBinding = 1;
        write_descriptors[1].dstArrayElement = 0;
        write_descriptors[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_descriptors[1].descriptorCount = 1;
        write_descriptors[1].pImageInfo = &descriptor_image;
        
        vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(write_descriptors.size()), write_descriptors.data(), 0, nullptr);
    }
    
    return true;
}

bool Renderer::CreateCommandBuffers() {
    m_CommandBuffers.resize(m_SwapchainFramebuffers.size());
    
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = m_CommandPool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());
    
    if (vkAllocateCommandBuffers(m_Device, &allocate_info, m_CommandBuffers.data()) != VK_SUCCESS) {
        std::cerr << "Failed to allocate command buffers\n";
        return false;
    }
    
    for (size_t i = 0; i < m_CommandBuffers.size(); i++) {
        VkCommandBufferBeginInfo command_buffer_begin_info{};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        
        if (vkBeginCommandBuffer(m_CommandBuffers[i], &command_buffer_begin_info) != VK_SUCCESS) {
            std::cerr << "Failed to begin command buffer\n";
            return false;
        }
        
        std::array<VkClearValue, 2> clear_values{};
        clear_values[0] = {0.0f, 0.0f, 0.0f, 1.0f};
        clear_values[1] = {1.0f, 0};
        
        VkRenderPassBeginInfo render_pass_begin_info{};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.renderPass = m_RenderPass;
        render_pass_begin_info.framebuffer = m_SwapchainFramebuffers[i];
        render_pass_begin_info.renderArea.offset = {0, 0};
        render_pass_begin_info.renderArea.extent = m_SwapchainExtent;
        render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        render_pass_begin_info.pClearValues = clear_values.data();
        
        vkCmdBeginRenderPass(m_CommandBuffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
        
        VkBuffer vertex_buffers[] = { m_VertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(m_CommandBuffers[i], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[i], 0, nullptr);
        vkCmdDrawIndexed(m_CommandBuffers[i], static_cast<uint32_t>(m_Indices.size()), 1, 0, 0, 0);
        vkCmdEndRenderPass(m_CommandBuffers[i]);
        
        if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS) {
            std::cerr << "Failed to record command buffer\n";
            return false;
        }
    }
    
    return true;
}

bool Renderer::CreateSyncObjects() {
    m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_ImagesInFlight.resize(m_SwapchainImages.size(), VK_NULL_HANDLE);
    
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_Device, &semaphore_create_info, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS
            || vkCreateSemaphore(m_Device, &semaphore_create_info, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS
            || vkCreateFence(m_Device, &fence_create_info, nullptr, &m_InFlightFences[i])) {
            std::cerr << "Failed to create synchronization objects\n";
            return false;
        }
    }
    
    return true;
}

bool Renderer::CheckValidationLayers() const {
    // Get vector of all available layers
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
    
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);
    
    return queue_families_indices.IsComplete()
        && extensions_supported
        && swap_chain_adequate
        && features.samplerAnisotropy;
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
    
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_Window, &width, &height);
    
    VkExtent2D extend;
    extend.width = std::clamp(static_cast<unsigned int>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extend.height = std::clamp(static_cast<unsigned int>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    
    return extend;
}

std::vector<char> Renderer::ReadFile(const std::string &filename) const {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    
    auto file_size = static_cast<size_t>(file.tellg());
    
    std::vector<char> buffer(file_size);
    file.seekg(0);
    file.read(buffer.data(), file_size);
    
    return buffer;
}

VkShaderModule Renderer::CreateShaderModule(const std::vector<char> &code) const {
    VkShaderModuleCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    VkShaderModule shader_module;
    if (vkCreateShaderModule(m_Device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
        std::cerr << "Failed to create shader module\n";
        return VK_NULL_HANDLE;
    }
    
    return shader_module;
}

uint32_t Renderer::FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memory_properties{};
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memory_properties);
    
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i))
            && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    std::cerr << "Failed to find suitable memory type\n";
    return -1;
}

bool Renderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties, VkBuffer* buffer, VkDeviceMemory* buffer_memory) const {
    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(m_Device, &create_info, nullptr, buffer) != VK_SUCCESS) {
        std::cerr << "Failed to create buffer\n";
        return false;
    }
    
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(m_Device, *buffer, &requirements);
    
    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, memory_properties);
    
    if (vkAllocateMemory(m_Device, &allocate_info, nullptr, buffer_memory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate buffer memory\n";
        return false;
    }
    
    vkBindBufferMemory(m_Device, *buffer, *buffer_memory, 0);
    return true;
}

void Renderer::CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) const {
    VkCommandBuffer command_buffer = BeginSingleTimeCommands();
    
    VkBufferCopy region{};
    region.size = size;
    vkCmdCopyBuffer(command_buffer, src, dst, 1, &region);
    
    EndSingleTimeCommands(command_buffer);
}

void Renderer::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_properties, VkImage *image, VkDeviceMemory *image_memory) const {
    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width = static_cast<uint32_t>(width);
    image_create_info.extent.height = static_cast<uint32_t>(height);
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.format = format;
    image_create_info.tiling = tiling;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = usage;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    
    if (vkCreateImage(m_Device, &image_create_info, nullptr, image) != VK_SUCCESS) {
        std::cerr << "Failed to create image\n";
    }
    
    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(m_Device, *image, &requirements);
    
    VkMemoryAllocateInfo memory{};
    memory.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory.allocationSize = requirements.size;
    memory.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, memory_properties);
    
    if (vkAllocateMemory(m_Device, &memory, nullptr, image_memory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate image memory\n";
    }
    
    vkBindImageMemory(m_Device, *image, *image_memory, 0);
}

VkCommandBuffer Renderer::BeginSingleTimeCommands() const {
    VkCommandBufferAllocateInfo buffer{};
    buffer.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    buffer.commandPool = m_CommandPool;
    buffer.commandBufferCount = 1;
    
    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(m_Device, &buffer, &command_buffer);
    
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(command_buffer, &begin_info);
    return command_buffer;
}

void Renderer::EndSingleTimeCommands(VkCommandBuffer command_buffer) const {
    vkEndCommandBuffer(command_buffer);
    
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    
    vkQueueSubmit(m_GraphicsQueue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_GraphicsQueue);
    
    vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &command_buffer);
}

void Renderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout) const {
    VkCommandBuffer command_buffer = BeginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;
    
    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        std::cerr << "Unsupported layout transition\n";
    }
    
    vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    EndSingleTimeCommands(command_buffer);
}

void Renderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const {
    VkCommandBuffer command_buffer = BeginSingleTimeCommands();
    
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    
    vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    EndSingleTimeCommands(command_buffer);
}

VkImageView Renderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags) const {
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = format;
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.subresourceRange.aspectMask = aspect_flags;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;
    
    VkImageView view;
    if (vkCreateImageView(m_Device, &create_info, nullptr, &view) != VK_SUCCESS) {
        std::cerr << "Failed to create image view\n";
        return VK_NULL_HANDLE;
    }
    
    return view;
}

VkFormat Renderer::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const {
    for (auto format : candidates) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &properties);
        
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    
    std::cerr << "Failed to find supported format\n";
    return VkFormat{};
}

VkFormat Renderer::FindDepthFormat() const {
    return FindSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

bool Renderer::HasStencilComponent(VkFormat format) const {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}


