#pragma once

#include <vector>
#include <knot/camera.h>
#include <knot/resources.h>
#include <knot/scene.h>

namespace knot {
/** @brief Per-instance data uploaded for instanced mesh rendering. */
struct InstanceData {
    /** @brief Model-to-world matrix for one instance. */
    glm::mat4 model;
};
/** @brief An object paired with the world transform used for rendering. */
struct VisibleInstance {
    /** @brief Object to render. */
    const Object* object = nullptr;
    /** @brief Precomputed object-to-world matrix. */
    glm::mat4 worldMatrix{1.0f};
};
/** @brief GPU layout for a PBR point light stored in the light SSBO. */
struct GPUMovingPointLight {
    /** @brief Position in xyz; w is padding. */
    glm::vec4 position; // [x, y, z, w(Any data or Padding)]
    /** @brief RGB color and intensity in w. */
    glm::vec4 color; // [r, g, b, brightness]
    /** @brief Influence radius derived from light intensity. */
    float radius;
    /** @brief Constant attenuation coefficient. */
    float constant;
    /** @brief Linear attenuation coefficient. */
    float linear;
    /** @brief Quadratic attenuation coefficient. */
    float quadratic;
};
/** @brief OpenGL renderer for scenes, meshes, lights, and skyboxes. */
class Renderer {
public:
    /** @brief Get the singleton instance of the Renderer.
     * @return Reference to the single Renderer instance. */
    static Renderer& get();

    /**
     * @brief Destroys the renderer.
     *
     * Releases renderer-owned resources through shutdown().
     */
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    /** @brief Default near clipping distance exposed for renderer clients. */
    static constexpr float kNearPlane = 0.1f;
    /** @brief Default far clipping distance exposed for renderer clients. */
    static constexpr float kFarPlane = 100.0f;

    /** @brief Loads OpenGL functions and creates renderer-owned resources.
     *  @param loadProc GLAD-compatible OpenGL procedure loader.
     *  @return true when initialization succeeds. */
    bool init(GLADloadfunc loadProc);
    /** @brief Releases renderer-owned GPU resources. Safe to call repeatedly. */
    void shutdown();
    /** @brief Sets the OpenGL viewport for a new frame.
     *  Does nothing for non-positive framebuffer dimensions. */
    void beginFrame(int framebufferWidth, int framebufferHeight);
    /** @brief Renders one model using an explicit world transform. */
    void renderSingle(const std::shared_ptr<Model>& model, const glm::mat4& worldMatrix, const Camera& camera, float aspectRatio);
    /** @brief Renders an object using its current transform.
     *  @return false if the renderer or object model is invalid. */
    bool renderObject(const Object& object, const Camera& camera, float aspectRatio);
    /** @brief Renders an object using a precomputed world transform.
     *  @return false if the renderer or object model is invalid. */
    bool renderObject(const VisibleInstance& instance, const Camera& camera, float aspectRatio);
    /** @brief Draws a cubemap skybox centered on the camera. */
    void renderSkybox(unsigned int cubemapID, const Camera& camera, float aspectRatio);
    /** @brief Performs frustum culling, batching, and rendering for a scene.
     *  @return false when the renderer has not been initialized. */
    bool renderScene(Scene& scene, float aspectRatio);
    /** @brief Renders a Shadows. */
    void renderShadow(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups);

    /** @brief Writes the first directional light, or zero lighting, to a shader. */
    void processDirLights(const std::shared_ptr<Shader>& shader, const std::vector<const DirLight*>& dirLights);
    /** @brief Uploads point lights to shader-storage buffer binding 0. */
    void processPointLights(const std::vector<const PbrPointLight*>& pointLights);
    /** @brief Converts an HDR equirectangular texture to a cubemap. */
    unsigned int bakeHDRMapToCubemap(unsigned int hdrTexture2D, int size);
    /** @brief Renders several instances of one model in a single draw call. */
    void renderInstanced(const std::shared_ptr<Model>& model, const std::vector<VisibleInstance>& instances, const Camera& camera, float aspectRatio);

private:
    Renderer() = default;

    void renderObjects(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups, float aspectRatio);
    void renderShadowObjects(const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups,
                             const std::shared_ptr<Shader>& shader);
    void renderDirShadow(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups);
    void renderPointShadow(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups);

    bool initialized = false;

    GLuint lightSSBO = 0;

    unsigned int instanceVBO = 0;

    static constexpr std::size_t INSTANCE_THRESHOLD = 4;

    static constexpr unsigned int SKYBOX_SHADER_ID = 999999;

    std::shared_ptr<Mesh> skyboxMesh;

    std::shared_ptr<Shader> skyboxShader;

    static constexpr unsigned int BRDF_SHADER_ID = 999998;

    static constexpr float AMBIENT_INTENSITY = 1.0f;
    static constexpr unsigned int SHADOW_RESOLUTION = 2048;

    GLuint brdfLUTTexture = 0;

    void generateBRDFLUT();

    void renderQuad();

    GLuint quadVAO = 0;

    GLuint quadVBO = 0;

    unsigned int depthMapFBO = 0;
    unsigned int depthMap = 0;
    unsigned int pointDepthFBO = 0;
    unsigned int pointDepthMap = 0;
    int pointShadowCount = 0;

    static constexpr unsigned int SHADOW_SHADER_ID = 999997;

    std::shared_ptr<Shader> pointShadowShader;
    std::shared_ptr<Shader> dirShadowShader;
    glm::mat4 lightSpaceMatrix{1.0f};

    int framebufferWidth;
    int framebufferHeight;
};
} // namespace knot
