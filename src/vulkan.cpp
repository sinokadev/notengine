#include "vulkan_internal.h"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace knot::vk {
void check(VkResult r, const char* op) {
    if (r != VK_SUCCESS)
        throw std::runtime_error(std::string(op) + " failed (VkResult " + std::to_string(r) + ")");
}
Context& context() {
    static Context c;
    return c;
}
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* data, void* user) {
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        ++static_cast<Context*>(user)->validationErrors;
    std::cerr << "[Vulkan validation] " << data->pMessage << '\n';
    return VK_FALSE;
}
void Context::init(GLFWwindow* window) {
    check(volkInitialize(), "volkInitialize");
    uint32_t count = 0;
    const char** required = glfwGetRequiredInstanceExtensions(&count);
    if (!required)
        throw std::runtime_error("GLFW cannot find Vulkan surface extensions");
    std::vector<const char*> extensions(required, required + count);
    bool validation = std::getenv("KNOT_VULKAN_VALIDATION") != nullptr;
    const char* layer = "VK_LAYER_KHRONOS_validation";
    if (validation)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    uint32_t extCount = 0;
    check(vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr), "instance extensions");
    std::vector<VkExtensionProperties> exts(extCount);
    check(vkEnumerateInstanceExtensionProperties(nullptr, &extCount, exts.data()), "instance extensions");
    bool portability =
        std::any_of(exts.begin(), exts.end(), [](auto& e) { return std::strcmp(e.extensionName, "VK_KHR_portability_enumeration") == 0; });
    if (portability)
        extensions.push_back("VK_KHR_portability_enumeration");
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "Knot";
    app.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &app;
    ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    ci.ppEnabledExtensionNames = extensions.data();
    if (portability)
        ci.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    if (validation) {
        ci.enabledLayerCount = 1;
        ci.ppEnabledLayerNames = &layer;
    }
    check(vkCreateInstance(&ci, nullptr, &instance), "vkCreateInstance");
    volkLoadInstance(instance);
    validationErrors = 0;
    if (validation) {
        VkDebugUtilsMessengerCreateInfoEXT di{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        di.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        di.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        di.pfnUserCallback = debugCallback;
        di.pUserData = this;
        check(vkCreateDebugUtilsMessengerEXT(instance, &di, nullptr, &debug), "debug messenger");
    }
    check(glfwCreateWindowSurface(instance, window, nullptr, &surface), "glfwCreateWindowSurface");
    uint32_t n = 0;
    check(vkEnumeratePhysicalDevices(instance, &n, nullptr), "physical devices");
    std::vector<VkPhysicalDevice> devices(n);
    check(vkEnumeratePhysicalDevices(instance, &n, devices.data()), "physical devices");
    for (auto candidate : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(candidate, &props);
        if (props.apiVersion < VK_API_VERSION_1_1)
            continue;
        uint32_t ec = 0;
        check(vkEnumerateDeviceExtensionProperties(candidate, nullptr, &ec, nullptr), "device extensions");
        std::vector<VkExtensionProperties> de(ec);
        check(vkEnumerateDeviceExtensionProperties(candidate, nullptr, &ec, de.data()), "device extensions");
        if (!std::any_of(de.begin(), de.end(), [](auto& e) { return std::strcmp(e.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0; }))
            continue;
        uint32_t qc = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &qc, nullptr);
        std::vector<VkQueueFamilyProperties> qs(qc);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &qc, qs.data());
        for (uint32_t i = 0; i < qc; ++i) {
            VkBool32 present = false;
            check(vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface, &present), "surface support");
            if ((qs[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
                physical = candidate;
                family = i;
                break;
            }
        }
        if (physical)
            break;
    }
    if (!physical)
        throw std::runtime_error("No Vulkan 1.1 graphics/present device available");
    float priority = 1;
    VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qi.queueFamilyIndex = family;
    qi.queueCount = 1;
    qi.pQueuePriorities = &priority;
    std::vector<const char*> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    uint32_t ec = 0;
    check(vkEnumerateDeviceExtensionProperties(physical, nullptr, &ec, nullptr), "device extensions");
    std::vector<VkExtensionProperties> de(ec);
    check(vkEnumerateDeviceExtensionProperties(physical, nullptr, &ec, de.data()), "device extensions");
    for (auto& e : de)
        if (std::strcmp(e.extensionName, "VK_KHR_portability_subset") == 0)
            deviceExtensions.push_back("VK_KHR_portability_subset");
    VkDeviceCreateInfo dc{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dc.queueCreateInfoCount = 1;
    dc.pQueueCreateInfos = &qi;
    dc.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    dc.ppEnabledExtensionNames = deviceExtensions.data();
    check(vkCreateDevice(physical, &dc, nullptr, &device), "vkCreateDevice");
    volkLoadDevice(device);
    vkGetDeviceQueue(device, family, 0, &queue);
    VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pi.queueFamilyIndex = family;
    pi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    check(vkCreateCommandPool(device, &pi, nullptr, &pool), "command pool");
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physical, &props);
    std::cout << "[Info] Vulkan: " << props.deviceName << '\n';
}
uint32_t Context::memoryType(uint32_t bits, VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties p;
    vkGetPhysicalDeviceMemoryProperties(physical, &p);
    for (uint32_t i = 0; i < p.memoryTypeCount; ++i)
        if ((bits & (1u << i)) && (p.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    throw std::runtime_error("No compatible Vulkan memory type");
}
Buffer Context::buffer(VkDeviceSize size, VkBufferUsageFlags usage, const void* data) {
    Buffer b;
    b.size = std::max<VkDeviceSize>(size, 16);
    try {
        VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        ci.size = b.size;
        ci.usage = usage;
        ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        check(vkCreateBuffer(device, &ci, nullptr, &b.handle), "create buffer");
        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(device, b.handle, &req);
        VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        ai.allocationSize = req.size;
        ai.memoryTypeIndex = memoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkAllocateMemory(device, &ai, nullptr, &b.memory), "buffer memory");
        check(vkBindBufferMemory(device, b.handle, b.memory, 0), "bind buffer");
        if (data && size) {
            void* dst;
            check(vkMapMemory(device, b.memory, 0, size, 0, &dst), "map buffer");
            std::memcpy(dst, data, size);
            vkUnmapMemory(device, b.memory);
        }
    } catch (...) {
        destroy(b);
        throw;
    }
    return b;
}
void Context::destroy(Buffer& b) {
    if (b.handle)
        vkDestroyBuffer(device, b.handle, nullptr);
    if (b.memory)
        vkFreeMemory(device, b.memory, nullptr);
    b = {};
}
Image Context::image(uint32_t w, uint32_t h, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect, uint32_t layers, uint32_t levels,
                     VkSampleCountFlagBits samples) {
    Image im;
    im.width = w;
    im.height = h;
    im.format = format;
    im.layers = layers;
    im.levels = levels;
    try {
        VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        ci.imageType = VK_IMAGE_TYPE_2D;
        ci.extent = {w, h, 1};
        ci.mipLevels = levels;
        ci.arrayLayers = layers;
        ci.format = format;
        ci.tiling = VK_IMAGE_TILING_OPTIMAL;
        ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ci.usage = usage;
        ci.samples = samples;
        if (layers == 6)
            ci.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        check(vkCreateImage(device, &ci, nullptr, &im.handle), "create image");
        VkMemoryRequirements req;
        vkGetImageMemoryRequirements(device, im.handle, &req);
        VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        ai.allocationSize = req.size;
        ai.memoryTypeIndex = memoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(device, &ai, nullptr, &im.memory), "image memory");
        check(vkBindImageMemory(device, im.handle, im.memory, 0), "bind image");
        VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vi.image = im.handle;
        vi.viewType = layers == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
        vi.format = format;
        vi.subresourceRange = {aspect, 0, levels, 0, layers};
        check(vkCreateImageView(device, &vi, nullptr, &im.view), "image view");
        if (usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
            VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(physical, format, &properties);
            const bool linear =
                aspect != VK_IMAGE_ASPECT_DEPTH_BIT && (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
            si.magFilter = si.minFilter = linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
            si.mipmapMode = linear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
            si.addressModeU = si.addressModeV = si.addressModeW =
                layers == 6 || aspect == VK_IMAGE_ASPECT_DEPTH_BIT ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            si.maxLod = float(levels - 1);
            check(vkCreateSampler(device, &si, nullptr, &im.sampler), "sampler");
        }
    } catch (...) {
        destroy(im);
        throw;
    }
    return im;
}
void Context::destroy(Image& im) {
    if (im.sampler)
        vkDestroySampler(device, im.sampler, nullptr);
    if (im.view)
        vkDestroyImageView(device, im.view, nullptr);
    if (im.handle)
        vkDestroyImage(device, im.handle, nullptr);
    if (im.memory)
        vkFreeMemory(device, im.memory, nullptr);
    im = {};
}
void Context::immediate(const std::function<void(VkCommandBuffer)>& record) {
    VkCommandBuffer cmd{};
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool = pool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    check(vkAllocateCommandBuffers(device, &ai, &cmd), "allocate upload command");
    try {
        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(vkBeginCommandBuffer(cmd, &bi), "begin upload");
        record(cmd);
        check(vkEndCommandBuffer(cmd), "end upload");
        VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        si.commandBufferCount = 1;
        si.pCommandBuffers = &cmd;
        check(vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE), "submit upload");
        check(vkQueueWaitIdle(queue), "wait upload");
    } catch (...) {
        vkFreeCommandBuffers(device, pool, 1, &cmd);
        throw;
    }
    vkFreeCommandBuffers(device, pool, 1, &cmd);
}
void Context::barrier(VkCommandBuffer cmd, const Image& im, VkImageLayout from, VkImageLayout to, VkPipelineStageFlags ss, VkPipelineStageFlags ds,
                      VkAccessFlags sa, VkAccessFlags da, VkImageAspectFlags aspect) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.oldLayout = from;
    b.newLayout = to;
    b.srcAccessMask = sa;
    b.dstAccessMask = da;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = im.handle;
    b.subresourceRange = {aspect, 0, im.levels, 0, im.layers};
    vkCmdPipelineBarrier(cmd, ss, ds, 0, 0, nullptr, 0, nullptr, 1, &b);
}
unsigned Context::upload(const std::vector<float>& pixels, int width, int height, int layers, int levels) {
    if (!device)
        throw std::runtime_error("Initialize Renderer before creating textures");
    if (width <= 0 || height <= 0 || levels <= 0 || (layers != 1 && layers != 6))
        throw std::invalid_argument("Invalid texture dimensions");
    size_t expected = 0;
    for (int level = 0; level < levels; ++level)
        expected += size_t(std::max(1, width >> level)) * std::max(1, height >> level) * layers * 4;
    if (pixels.size() != expected)
        throw std::invalid_argument("Invalid texture pixel count");
    Texture t;
    t.pixels = pixels;
    Buffer staging{};
    try {
        t.image = image(width, height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_IMAGE_ASPECT_COLOR_BIT, layers, levels);
        staging = buffer(pixels.size() * sizeof(float), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, pixels.data());
        immediate([&](VkCommandBuffer cmd) {
            barrier(cmd, t.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
            std::vector<VkBufferImageCopy> copies;
            VkDeviceSize offset = 0;
            for (int level = 0; level < levels; ++level) {
                uint32_t w = std::max(1, width >> level), h = std::max(1, height >> level);
                VkBufferImageCopy copy{};
                copy.bufferOffset = offset;
                copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, uint32_t(level), 0, uint32_t(layers)};
                copy.imageExtent = {w, h, 1};
                copies.push_back(copy);
                offset += VkDeviceSize(w) * h * layers * 4 * sizeof(float);
            }
            vkCmdCopyBufferToImage(cmd, staging.handle, t.image.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, uint32_t(copies.size()), copies.data());
            barrier(cmd, t.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
        });
        destroy(staging);
        unsigned id = nextId++;
        textures.emplace(id, std::move(t));
        return id;
    } catch (...) {
        destroy(staging);
        destroy(t.image);
        throw;
    }
}
void Context::shutdown() {
    if (device) {
        vkDeviceWaitIdle(device);
        for (auto& [id, t] : textures)
            destroy(t.image);
        textures.clear();
        for (auto& [id, m] : meshes) {
            destroy(m.vertices);
            destroy(m.indices);
        }
        meshes.clear();
        if (pool)
            vkDestroyCommandPool(device, pool, nullptr);
        vkDestroyDevice(device, nullptr);
    }
    pool = {};
    device = {};
    queue = {};
    physical = {};
    if (surface)
        vkDestroySurfaceKHR(instance, surface, nullptr);
    surface = {};
    if (debug)
        vkDestroyDebugUtilsMessengerEXT(instance, debug, nullptr);
    debug = {};
    if (instance)
        vkDestroyInstance(instance, nullptr);
    instance = {};
}
} // namespace knot::vk
