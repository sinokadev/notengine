#pragma once
#include <volk/volk.h>
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>

namespace knot::vk {
void check(VkResult result, const char* operation);
struct Buffer {
    VkBuffer handle{};
    VkDeviceMemory memory{};
    VkDeviceSize size{};
};
struct Image {
    VkImage handle{};
    VkDeviceMemory memory{};
    VkImageView view{};
    VkSampler sampler{};
    uint32_t width{}, height{}, layers{1}, levels{1};
    VkFormat format{};
};
struct Texture {
    Image image;
    std::vector<float> pixels;
}; // Linear RGBA, retained for environment convolution.
struct Geometry {
    Buffer vertices, indices;
};
struct Context {
    VkInstance instance{};
    VkDebugUtilsMessengerEXT debug{};
    VkSurfaceKHR surface{};
    VkPhysicalDevice physical{};
    VkDevice device{};
    VkQueue queue{};
    uint32_t family{};
    VkCommandPool pool{};
    uint32_t validationErrors{};
    std::unordered_map<unsigned, Texture> textures;
    std::unordered_map<unsigned, Geometry> meshes;
    unsigned nextId{1};
    void init(GLFWwindow* window);
    void shutdown();
    uint32_t memoryType(uint32_t bits, VkMemoryPropertyFlags flags);
    Buffer buffer(VkDeviceSize size, VkBufferUsageFlags usage, const void* data = nullptr);
    void destroy(Buffer& buffer);
    Image image(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT,
                uint32_t layers = 1, uint32_t levels = 1, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
    void destroy(Image& image);
    void immediate(const std::function<void(VkCommandBuffer)>& record);
    void barrier(VkCommandBuffer cmd, const Image& image, VkImageLayout from, VkImageLayout to, VkPipelineStageFlags srcStage,
                 VkPipelineStageFlags dstStage, VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                 VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
    unsigned upload(const std::vector<float>& pixels, int width, int height, int layers = 1, int levels = 1);
};
Context& context();
} // namespace knot::vk
