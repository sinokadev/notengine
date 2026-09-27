#pragma once
#include <volk/volk.h>
#include <knot/camera.h>
#include <knot/scene.h>
#include <functional>
#include <memory>

struct GLFWwindow;
namespace knot {
struct InstanceData {
    glm::mat4 model;
};
struct VisibleInstance {
    const Object* object = nullptr;
    glm::mat4 worldMatrix{1.f};
};
struct GPUMovingPointLight {
    glm::vec4 position, color;
    float radius, constant, linear, quadratic;
};
/** @brief Vulkan 1.1 renderer. All GPU operations run on the window's thread. */
class Renderer {
public:
    static Renderer& get();
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    static constexpr float kNearPlane = .1f, kFarPlane = 100.f;
    /** @brief Initialize with a GLFW_NO_API window. The window must outlive shutdown(). */
    bool init(GLFWwindow* window);
    void shutdown();
    bool isInitialized() const;
    /** @brief Wait for previous work, acquire a swapchain image. False while minimized/out of date. */
    bool beginFrame(int width, int height);
    /** @brief Submit scene jobs and optional UI commands, then present. */
    void endFrame(const std::function<void(VkCommandBuffer)>& overlay = {});
    void setClearColor(const glm::vec4& color);
    void setVsync(bool enabled);
    bool renderScene(Scene& scene, float aspectRatio);
    void renderSingle(const std::shared_ptr<Model>& model, const glm::mat4& world, const Camera& camera, float aspectRatio);
    bool renderObject(const Object& object, const Camera& camera, float aspectRatio);
    bool renderObject(const VisibleInstance& instance, const Camera& camera, float aspectRatio);
    void renderInstanced(const std::shared_ptr<Model>& model, const std::vector<VisibleInstance>& instances, const Camera& camera, float aspectRatio);
    void renderSkybox(unsigned int cubemap, const Camera& camera, float aspectRatio);
    /** @brief Queue an offscreen scene. Image view stays valid until the next size change/shutdown. */
    VkDescriptorImageInfo renderSceneToTexture(Scene* scene, int width, int height);
    /** @brief Save the next completed frame as a PPM image. */
    void requestScreenshot(const std::string& path);
    void waitIdle();
    VkInstance getInstance() const;
    VkPhysicalDevice getPhysicalDevice() const;
    VkDevice getDevice() const;
    VkQueue getQueue() const;
    uint32_t getQueueFamily() const;
    VkRenderPass getRenderPass() const;
    uint32_t getImageCount() const;
    VkSampleCountFlagBits getSampleCount() const;
    uint32_t getValidationErrorCount() const;

private:
    Renderer();
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace knot
