#pragma once

#include <vector>
#include <array>
#include <functional>
#include <string>
#include <knot/camera.h>
#include <knot/resources.h>
#include <knot/scene.h>

namespace knot {
/** @brief Per-instance data uploaded for instanced mesh rendering. */
struct InstanceData {
    /** @brief Model-to-world matrix for one instance. */
    glm::mat4 model;
};
/** @brief Per-instance data uploaded for the instanced particle billboard pass.
 *
 * The layout is packed so the attribute offsets are stable: position and
 * velocity are three floats each, size and normalized age are single floats
 * and the color is the only 16-byte aligned member.
 */
struct ParticleInstance {
    /** @brief World-space particle center (attribute location 4). */
    glm::vec3 position;
    /** @brief World-space billboard width (attribute location 6). */
    float size;
    /** @brief World-space velocity used to orient and stretch the quad (attribute location 5). */
    glm::vec3 velocity;
    /** @brief Age divided by lifetime, 0 at spawn and 1 at expiry (attribute location 8). */
    float normalizedAge;
    /** @brief Base color with opacity in w (attribute location 7). */
    glm::vec4 color;
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
    /** @brief Position in xyz; w is the shadow layer, or -1 when unshadowed. */
    glm::vec4 position;
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
    static constexpr int kPostProcessInputTextureUnit = 0;
    static constexpr int kPostProcessSceneColorTextureUnit = 1;
    static constexpr int kPostProcessSceneDepthTextureUnit = 2;

    /** @brief Configures additional uniforms and textures for a post-processing shader.
     *  @param shader Shader bound for the pass.
     *  @param sceneColorTexture Original scene color texture before post-processing.
     *  @param sceneDepthTexture Original scene depth texture before post-processing.
     *  @param resolution Active framebuffer dimensions. */
    using PostProcessSetup =
        std::function<void(const Shader&, GLuint sceneColorTexture, GLuint sceneDepthTexture, const glm::ivec2& resolution)>;

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
    /** @brief Saves the current scene as a PNG, rerendering when the requested resolution differs.
     *  @return false if the renderer, scene, dimensions, or output path is invalid. */
    bool saveScreenshot(const std::string& path, int width, int height);
    /** @brief Renders one model using an explicit world transform. */
    void renderSingle(const std::shared_ptr<Model>& model, const glm::mat4& worldMatrix, const Camera& camera, float aspectRatio,
                      bool captureRadiance = false);
    /** @brief Renders an object using its current transform.
     *  @return false if the renderer or object model is invalid. */
    bool renderObject(const Object& object, const Camera& camera, float aspectRatio, bool captureRadiance = false);
    /** @brief Renders an object using a precomputed world transform.
     *  @return false if the renderer or object model is invalid. */
    bool renderObject(const VisibleInstance& instance, const Camera& camera, float aspectRatio, bool captureRadiance = false);
    /** @brief Draws a cubemap skybox centered on the camera. */
    void renderSkybox(unsigned int cubemapID, const Camera& camera, float aspectRatio, bool captureRadiance = false);
    /** @brief Performs frustum culling, batching, and rendering for a scene.
     *  Built-in shaders draw opaque fragments first, then translucent fragments
     *  sorted by sub-mesh center with depth writes disabled. Intersecting surfaces
     *  and triangles within a single sub-mesh are not individually sorted.
     *  The translucent pass adds one draw per visible sub-mesh instance, including
     *  those whose fragments turn out to be opaque. Custom shaders can opt in by
     *  implementing alphaPass (0 = all, 1 = opaque, 2 = translucent).
     *  @return false when the renderer has not been initialized. */
    bool renderScene(Scene& scene, float aspectRatio);
    /** @brief Adds a full-screen post-processing shader to the ordered render chain.
     *
     * Every shader receives u_InputTex, u_SceneColorTex, u_SceneDepthTex, u_Resolution,
     * and u_TexelSize uniforms. The input textures are bound to texture units 0, 1, and 2.
     * The optional setup callback can set custom uniforms and bind extra textures on units 3+.
     * @return false when the shader is null or invalid. */
    bool addPostProcessShader(std::shared_ptr<Shader> shader, PostProcessSetup setup = {});
    /** @brief Removes the first matching registered post-processing shader. */
    bool removePostProcessShader(const std::shared_ptr<Shader>& shader);
    /** @brief Removes all registered post-processing shaders. */
    void clearPostProcessShaders();
    /** @brief Captures and bakes the scene's configured local reflection probe.
     *  @return false if the renderer is not initialized, no probe is configured,
     *  or the cubemap capture framebuffer is incomplete. */
    bool captureReflectionProbe(Scene& scene);
    /** @brief Renders a Shadows. */
    void renderShadow(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups);

    /** @brief Writes the first directional light, or zero lighting, to a shader. */
    void processDirLights(const std::shared_ptr<Shader>& shader, const std::vector<const DirLight*>& dirLights);
    /** @brief Uploads point lights to shader-storage buffer binding 0. */
    void processPointLights(const std::vector<const PbrPointLight*>& pointLights);
    /** @brief Converts an HDR equirectangular texture to a cubemap. */
    unsigned int bakeHDRMapToCubemap(unsigned int hdrTexture2D, int size);
    /** @brief Renders several instances of one model in a single draw call. */
    void renderInstanced(const std::shared_ptr<Model>& model, const std::vector<VisibleInstance>& instances, const Camera& camera, float aspectRatio,
                         bool captureRadiance = false);

    /**
     * @brief Draws every live particle of the given systems as instanced billboards.
     *
     * All systems are uploaded into one dynamic instance buffer and drawn with a
     * single glDrawArraysInstanced call. The pass assumes the forward pass has
     * already written the scene: it keeps depth testing on, turns depth writes
     * off, disables back-face culling (billboards have no meaningful winding)
     * and blends with GL_SRC_ALPHA / GL_ONE_MINUS_SRC_ALPHA. Every piece of GL
     * state it changes is restored before it returns.
     *
     * @param systems Particle systems to draw; null entries are skipped.
     * @param camera View camera used to build the billboards.
     * @param aspectRatio Viewport width-to-height ratio.
     */
    void renderParticles(const std::vector<const ParticleSystem*>& systems, const Camera& camera, float aspectRatio);

    /** @brief Sets how strongly particle velocity lengthens the billboard (0 disables stretching). */
    void setParticleStretch(float stretch) {
        particleStretch = stretch > 0.0f ? stretch : 0.0f;
    }

    /** @brief Returns the current velocity-to-length factor of the particle pass. */
    float getParticleStretch() const {
        return particleStretch;
    }

private:
    Renderer() = default;

    struct PostProcessPass {
        std::shared_ptr<Shader> shader;
        PostProcessSetup setup;
    };

    void renderObjects(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups, const Camera& camera,
                       float aspectRatio, bool useReflectionProbe, bool useSSAO, bool captureRadiance, bool ssgiRadiancePass = false);
    void renderShadowObjects(const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups,
                             const std::shared_ptr<Shader>& shader);
    void renderDirShadow(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups);
    void renderPointShadow(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups);
    void renderSSAO(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups, float aspectRatio);
    void renderSSGI(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups, float aspectRatio);
    void resizeSSAOTargets(int width, int height);
    bool resizePostProcessTargets(int width, int height);
    bool renderPostProcessing(GLuint outputFramebuffer);

    bool initialized = false;

    GLuint lightSSBO = 0;

    unsigned int instanceVBO = 0;

    static constexpr std::size_t INSTANCE_THRESHOLD = 4;

    std::shared_ptr<Mesh> skyboxMesh;

    std::shared_ptr<Shader> skyboxShader;

    GLuint brdfLUTTexture = 0;

    GLuint prePassFBO = 0;
    GLuint prePassTexture = 0;
    GLuint prePassDepth = 0;
    GLuint ssaoFBO = 0;
    GLuint ssaoTexture = 0;
    GLuint ssaoBlurFBO = 0;
    GLuint ssaoBlurTexture = 0;
    GLuint ssaoNoiseTexture = 0;
    GLuint ssgiRadianceFBO = 0;
    GLuint ssgiRadianceTexture = 0;
    GLuint ssgiFBO = 0;
    GLuint ssgiTexture = 0;
    GLuint ssgiBlurFBO = 0;
    GLuint ssgiBlurTexture = 0;
    GLuint sceneColorFBO = 0;
    GLuint sceneColorTexture = 0;
    GLuint sceneDepthTexture = 0;
    std::array<GLuint, 2> postProcessFBOs{};
    std::array<GLuint, 2> postProcessTextures{};
    int ssaoWidth = 0;
    int ssaoHeight = 0;
    int ssgiWidth = 0;
    int ssgiHeight = 0;
    int postProcessWidth = 0;
    int postProcessHeight = 0;
    bool postProcessTargetsReady = false;
    std::array<glm::vec3, 32> ssaoKernel{};
    std::shared_ptr<Shader> prePassShader;
    std::shared_ptr<Shader> ssaoShader;
    std::shared_ptr<Shader> ssaoBlurShader;
    std::shared_ptr<Shader> ssgiShader;
    std::shared_ptr<Shader> ssgiBlurShader;
    std::shared_ptr<Shader> postProcessPassthroughShader;
    std::vector<PostProcessPass> postProcessPasses;
    bool ssgiTargetsReady = false;

    bool generateBRDFLUT();

    void renderQuad();

    GLuint quadVAO = 0;

    GLuint quadVBO = 0;

    unsigned int depthMapFBO = 0;
    unsigned int depthMap = 0;
    unsigned int pointDepthFBO = 0;
    unsigned int pointDepthMap = 0;
    int pointShadowCount = 0;
    int pointShadowCapacity = 0;
    static constexpr int MAX_POINT_SHADOWS = 4;
    static constexpr float MIN_POINT_SHADOW_CONTRIBUTION = 0.01f;
    std::vector<const PbrPointLight*> shadowedPointLights;
    std::unordered_map<const PbrPointLight*, int> pointShadowLayerLookup;
    std::vector<GPUMovingPointLight> gpuPointLights;
    std::unordered_map<const Model*, std::vector<VisibleInstance>> culledPointShadowGroups;

    std::shared_ptr<Shader> particleShader;

    GLuint particleVAO = 0;
    GLuint particleQuadVBO = 0;
    GLuint particleInstanceVBO = 0;

    /** @brief Scratch storage reused by renderParticles between frames. */
    std::vector<ParticleInstance> particleInstances;

    /** @brief Velocity-to-length factor applied by the particle vertex shader. */
    float particleStretch = 0.25f;

    /**
     * @brief Upper bound on the billboard length multiplier.
     *
     * A rain streak wants a length-to-width ratio well above 8, so the cap is
     * high enough that stretch and speed choose the ratio; it still stops a
     * bogus velocity or stretch from drawing a billboard across the screen.
     */
    static constexpr float PARTICLE_MAX_STRETCH = 24.0f;

    /** @brief Creates the billboard shader and the quad/instance buffers. */
    bool initParticlePass();

    std::shared_ptr<Shader> pointShadowShader;
    std::shared_ptr<Shader> dirShadowShader;
    glm::mat4 lightSpaceMatrix{1.0f};

    int framebufferWidth = 1;
    int framebufferHeight = 1;
    Scene* activeScene = nullptr;
};
} // namespace knot
