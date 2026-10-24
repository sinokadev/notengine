#include <knot/renderer.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <filesystem>
#include <unordered_set>
#include <random>
#include <limits>
#include <knot/mesh.h>

#ifdef _WIN32
#define STBIW_WINDOWS_UTF8
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#define DISABLE_SKYMAP false
#define DISABLE_SHADOW false

namespace knot {
namespace {

bool writePng(const std::string& path, int width, int height, std::vector<unsigned char>& pixels) {
    const std::size_t rowBytes = static_cast<std::size_t>(width) * 4u;

    if (rowBytes > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return false;
    }

    for (int row = 0; row < height / 2; ++row) {
        auto* topRow = pixels.data() + static_cast<std::size_t>(row) * rowBytes;
        auto* bottomRow = pixels.data() + static_cast<std::size_t>(height - row - 1) * rowBytes;
        std::swap_ranges(topRow, topRow + rowBytes, bottomRow);
    }

    return stbi_write_png(path.c_str(), width, height, 4, pixels.data(), static_cast<int>(rowBytes)) != 0;
}

} // namespace

Renderer& Renderer::get() {
    static Renderer instance;
    return instance;
}

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::init(GLADloadfunc loadProc) {
    std::cout << "[Info] Not Engine Renderer Init" << std::endl;

    // Load GL
    if (!gladLoadGL(loadProc)) {
        std::cerr << "[Error] Failed to load OpenGL functions" << std::endl;
        return false;
    }

    // GL Config
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);

    glGenBuffers(1, &lightSSBO);
    glGenBuffers(1, &instanceVBO);

    // Sky Map and IBL
    skyboxMesh = createCube();
    auto skyboxSource = std::make_shared<ShaderSource>(getAssetRoot() + "shaders/skybox.vert", getAssetRoot() + "shaders/skybox.frag");
    skyboxShader = std::make_shared<Shader>(skyboxSource, SKYBOX_SHADER_ID);

    generateBRDFLUT();

    // Shadow Map
    glGenFramebuffers(1, &depthMapFBO);

    // depth map texture gen
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_RESOLUTION, SHADOW_RESOLUTION, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // depth map bind
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[Error] Shadow framebuffer is not complete!\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Shadow Shader
    auto dirShadowSource = std::make_shared<ShaderSource>(getAssetRoot() + "shaders/dir_shadow.vert", getAssetRoot() + "shaders/dir_shadow.frag");

    dirShadowShader = std::make_shared<Shader>(dirShadowSource, SHADOW_SHADER_ID);

    auto pointShadowSource =
        std::make_shared<ShaderSource>(getAssetRoot() + "shaders/point_shadow.vert", getAssetRoot() + "shaders/point_shadow.frag");

    pointShadowShader = std::make_shared<Shader>(pointShadowSource, SHADOW_SHADER_ID - 1);

    auto prePassSource = std::make_shared<ShaderSource>(getAssetRoot() + "shaders/prepass.vert", getAssetRoot() + "shaders/prepass.frag");
    prePassShader = std::make_shared<Shader>(prePassSource, SHADOW_SHADER_ID - 2);

    auto ssaoSource = std::make_shared<ShaderSource>(getAssetRoot() + "shaders/ssao.vert", getAssetRoot() + "shaders/ssao.frag");
    ssaoShader = std::make_shared<Shader>(ssaoSource, SHADOW_SHADER_ID - 3);

    auto ssaoBlurSource = std::make_shared<ShaderSource>(getAssetRoot() + "shaders/ssao.vert", getAssetRoot() + "shaders/ssao_blur.frag");
    ssaoBlurShader = std::make_shared<Shader>(ssaoBlurSource, SHADOW_SHADER_ID - 4);

    // Create the view-space normal and depth target.
    glGenFramebuffers(1, &prePassFBO);
    glGenTextures(1, &prePassTexture);
    glGenTextures(1, &prePassDepth);

    glBindFramebuffer(GL_FRAMEBUFFER, prePassFBO);

    glBindTexture(GL_TEXTURE_2D, prePassTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 1, 1, 0, GL_RGBA, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, prePassTexture, 0);

    glBindTexture(GL_TEXTURE_2D, prePassDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 1, 1, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, prePassDepth, 0);

    const GLenum ssaoDrawBuffer = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &ssaoDrawBuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[Error] SSAO pre-pass framebuffer is not complete!\n";
    }

    // Create the raw and blurred ambient-occlusion targets.
    glGenFramebuffers(1, &ssaoFBO);
    glGenTextures(1, &ssaoTexture);

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

    glBindTexture(GL_TEXTURE_2D, ssaoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, 1, 1, 0, GL_RED, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTexture, 0);
    glDrawBuffers(1, &ssaoDrawBuffer);

    glGenFramebuffers(1, &ssaoBlurFBO);
    glGenTextures(1, &ssaoBlurTexture);

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);

    glBindTexture(GL_TEXTURE_2D, ssaoBlurTexture);
    const float fullyLit = 1.0f;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, 1, 1, 0, GL_RED, GL_FLOAT, &fullyLit);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlurTexture, 0);
    glDrawBuffers(1, &ssaoDrawBuffer);

    // Build a deterministic hemisphere kernel and 4x4 rotation-noise tile.
    std::mt19937 randomGenerator(0x5A17u);
    std::uniform_real_distribution<float> randomUnit(0.0f, 1.0f);

    for (std::size_t i = 0; i < ssaoKernel.size(); ++i) {
        glm::vec3 sample(randomUnit(randomGenerator) * 2.0f - 1.0f, randomUnit(randomGenerator) * 2.0f - 1.0f, randomUnit(randomGenerator));
        sample = glm::normalize(sample) * randomUnit(randomGenerator);

        const float t = static_cast<float>(i) / static_cast<float>(ssaoKernel.size());
        const float scale = glm::mix(0.1f, 1.0f, t * t);

        ssaoKernel[i] = sample * scale;
    }

    std::array<glm::vec3, 16> noise{};

    for (auto& value : noise) {
        value = glm::vec3(randomUnit(randomGenerator) * 2.0f - 1.0f, randomUnit(randomGenerator) * 2.0f - 1.0f, 0.0f);
    }

    glGenTextures(1, &ssaoNoiseTexture);
    glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, noise.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    resizeSSAOTargets(framebufferWidth, framebufferHeight);

    // Set texture units and upload the kernel once during initialization.
    ssaoShader->use();
    ssaoShader->set("gNormalDepth", 0);
    ssaoShader->set("noiseTexture", 1);
    ssaoShader->set("sampleCount", static_cast<int>(ssaoKernel.size()));
    for (std::size_t i = 0; i < ssaoKernel.size(); ++i) {
        ssaoShader->set("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
    }

    ssaoBlurShader->use();
    ssaoBlurShader->set("ssaoInput", 0);
    ssaoBlurShader->set("gNormalDepth", 1);

    glGenFramebuffers(1, &pointDepthFBO);
    glGenTextures(1, &pointDepthMap);

    initialized = true;
    return true;
}

void Renderer::shutdown() {
    if (!initialized) {
        return;
    }

    const bool hasContext = (glfwGetCurrentContext() != nullptr);
    if (lightSSBO != 0) {
        if (hasContext) {
            glDeleteBuffers(1, &lightSSBO);
        }
        lightSSBO = 0;
    }

    if (instanceVBO != 0) {
        if (hasContext) {
            glDeleteBuffers(1, &instanceVBO);
        }
        instanceVBO = 0;
    }

    if (brdfLUTTexture != 0) {
        if (hasContext) {
            glDeleteTextures(1, &brdfLUTTexture);
        }
        brdfLUTTexture = 0;
    }

    if (hasContext) {
        glDeleteFramebuffers(1, &prePassFBO);
        glDeleteFramebuffers(1, &ssaoFBO);
        glDeleteFramebuffers(1, &ssaoBlurFBO);
        glDeleteTextures(1, &prePassTexture);
        glDeleteTextures(1, &prePassDepth);
        glDeleteTextures(1, &ssaoTexture);
        glDeleteTextures(1, &ssaoBlurTexture);
        glDeleteTextures(1, &ssaoNoiseTexture);
    }
    prePassFBO = prePassTexture = prePassDepth = ssaoFBO = ssaoTexture = ssaoBlurFBO = ssaoBlurTexture = ssaoNoiseTexture = 0;
    ssaoWidth = ssaoHeight = 0;

    if (quadVAO != 0) {
        if (hasContext) {
            glDeleteVertexArrays(1, &quadVAO);
            glDeleteBuffers(1, &quadVBO);
        }
        quadVAO = 0;
        quadVBO = 0;
    }

    if (hasContext) {
        glDeleteFramebuffers(1, &depthMapFBO);
        glDeleteFramebuffers(1, &pointDepthFBO);
        glDeleteTextures(1, &depthMap);
        glDeleteTextures(1, &pointDepthMap);
    }
    depthMapFBO = depthMap = pointDepthFBO = pointDepthMap = 0;
    pointShadowCount = 0;
    pointShadowCapacity = 0;
    shadowedPointLights.clear();
    pointShadowLayerLookup.clear();
    gpuPointLights.clear();
    culledPointShadowGroups.clear();

    dirShadowShader.reset();
    pointShadowShader.reset();
    prePassShader.reset();
    ssaoShader.reset();
    ssaoBlurShader.reset();

    skyboxMesh.reset();
    skyboxShader.reset();

    initialized = false;
}

void Renderer::renderQuad() {
    if (quadVAO == 0) {
        float quadVertices[] = {
            // positions   // texture Coords
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void Renderer::generateBRDFLUT() {
    // gen texture
    glGenTextures(1, &brdfLUTTexture);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // attach texture to framebuffer
    GLuint captureFBO;
    glGenFramebuffers(1, &captureFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

    // get shader
    auto brdfSource = std::make_shared<ShaderSource>(getAssetRoot() + "shaders/brdf.vert", getAssetRoot() + "shaders/brdf.frag");
    std::shared_ptr<Shader> brdfShader = std::make_shared<Shader>(brdfSource, BRDF_SHADER_ID);

    // viewport backup
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // render
    // The BRDF shader writes only RG, so source-alpha blending is undefined here.
    const GLboolean wasBlendEnabled = glIsEnabled(GL_BLEND);
    glDisable(GL_BLEND);
    glViewport(0, 0, 512, 512);
    brdfShader->use();
    glClear(GL_COLOR_BUFFER_BIT);
    renderQuad();

    // clean
    if (wasBlendEnabled) {
        glEnable(GL_BLEND);
    }
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &captureFBO);
}

void Renderer::beginFrame(int fbWidth, int fbHeight) {
    if (fbWidth <= 0 || fbHeight <= 0)
        return;

    framebufferWidth = fbWidth;
    framebufferHeight = fbHeight;

    resizeSSAOTargets(fbWidth, fbHeight);

    glViewport(0, 0, framebufferWidth, framebufferHeight);
}

bool Renderer::saveScreenshot(const std::string& path, int width, int height) {
    if (!initialized || path.empty() || width <= 0 || height <= 0 || framebufferWidth <= 0 || framebufferHeight <= 0) {
        return false;
    }

    GLint maxTextureSize = 0;
    GLint maxRenderbufferSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbufferSize);
    const GLint maxCaptureSize = std::min(maxTextureSize, maxRenderbufferSize);
    if (width > maxCaptureSize || height > maxCaptureSize) {
        std::cerr << "[Error] Screenshot dimensions exceed the GPU capture limit (" << maxCaptureSize << "): " << width << "x" << height << '\n';
        return false;
    }

    const bool rerenderAtTargetResolution = width != framebufferWidth || height != framebufferHeight;
    if (rerenderAtTargetResolution && !activeScene) {
        std::cerr << "[Error] Cannot render a high-resolution screenshot without an active scene.\n";
        return false;
    }

    const std::filesystem::path outputPath(path);
    if (!outputPath.parent_path().empty()) {
        std::error_code error;
        std::filesystem::create_directories(outputPath.parent_path(), error);
        if (error) {
            std::cerr << "[Error] Failed to create screenshot directory: " << outputPath.parent_path().string() << ": " << error.message()
                      << '\n';
            return false;
        }
    }

    const std::size_t captureWidth = static_cast<std::size_t>(width);
    const std::size_t captureHeight = static_cast<std::size_t>(height);
    if (captureWidth > std::numeric_limits<std::size_t>::max() / 4u / captureHeight) {
        return false;
    }

    GLint previousReadFramebuffer = 0;
    GLint previousDrawFramebuffer = 0;
    GLint previousReadBuffer = 0;
    GLint previousDrawBuffer = 0;
    GLint previousTexture = 0;
    GLint previousRenderbuffer = 0;
    GLint previousPixelPackBuffer = 0;
    GLint previousPackAlignment = 0;
    GLint previousActiveTexture = 0;
    GLint previousViewport[4] = {};
    GLfloat clearColor[4] = {};

    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);
    glGetIntegerv(GL_READ_BUFFER, &previousReadBuffer);
    glGetIntegerv(GL_DRAW_BUFFER0, &previousDrawBuffer);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &previousRenderbuffer);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPixelPackBuffer);
    glGetIntegerv(GL_PACK_ALIGNMENT, &previousPackAlignment);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);

    std::vector<unsigned char> pixels(captureWidth * captureHeight * 4u);
    bool captureSucceeded = true;
    GLuint captureFBO = 0;
    GLuint captureTexture = 0;
    GLuint captureDepth = 0;
    const int oldFramebufferWidth = framebufferWidth;
    const int oldFramebufferHeight = framebufferHeight;

    if (rerenderAtTargetResolution) {
        glGenFramebuffers(1, &captureFBO);
        glGenTextures(1, &captureTexture);
        glGenRenderbuffers(1, &captureDepth);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindTexture(GL_TEXTURE_2D, captureTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, captureTexture, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, captureDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureDepth);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        captureSucceeded = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

        if (captureSucceeded) {
            framebufferWidth = width;
            framebufferHeight = height;
            resizeSSAOTargets(width, height);
            glViewport(0, 0, width, height);
            glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            captureSucceeded = renderScene(*activeScene, static_cast<float>(width) / static_cast<float>(height));
        }

        if (captureSucceeded) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, captureFBO);
            glReadBuffer(GL_COLOR_ATTACHMENT0);
        }
    } else {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glReadBuffer(GL_BACK);
    }

    if (captureSucceeded) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    }

    if (captureFBO != 0)
        glDeleteFramebuffers(1, &captureFBO);
    if (captureTexture != 0)
        glDeleteTextures(1, &captureTexture);
    if (captureDepth != 0)
        glDeleteRenderbuffers(1, &captureDepth);

    if (rerenderAtTargetResolution) {
        framebufferWidth = oldFramebufferWidth;
        framebufferHeight = oldFramebufferHeight;
        resizeSSAOTargets(oldFramebufferWidth, oldFramebufferHeight);
    }

    glActiveTexture(previousActiveTexture);
    glBindTexture(GL_TEXTURE_2D, previousTexture);
    glBindRenderbuffer(GL_RENDERBUFFER, previousRenderbuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFramebuffer);
    glReadBuffer(previousReadBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFramebuffer);
    glDrawBuffer(previousDrawBuffer);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, previousPixelPackBuffer);
    glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);

    if (!captureSucceeded) {
        std::cerr << "[Error] Failed to render screenshot framebuffer.\n";
        return false;
    }

    if (!writePng(path, width, height, pixels)) {
        std::cerr << "[Error] Failed to write screenshot: " << path << '\n';
        return false;
    }

    return true;
}

void Renderer::resizeSSAOTargets(int width, int height) {
    if (width <= 0 || height <= 0 || (width == ssaoWidth && height == ssaoHeight)) {
        return;
    }

    ssaoWidth = width;
    ssaoHeight = height;

    // Resize the pre-pass attachments to match the active framebuffer.
    glBindTexture(GL_TEXTURE_2D, prePassTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, prePassDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);

    // Keep a fully-lit fallback until the next SSAO pass completes.
    glBindTexture(GL_TEXTURE_2D, ssaoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurTexture);
    std::vector<float> fullyLit(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 1.0f);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, fullyLit.data());
}

void Renderer::processDirLights(const std::shared_ptr<Shader>& shader, const std::vector<const DirLight*>& dirLights) {
    if (!shader)
        return;

    if (!dirLights.empty()) {
        const auto* dirLight = dirLights.front();

        shader->set("dirLight.direction", dirLight->getDirection());
        shader->set("dirLight.ambient", dirLight->ambient);
        shader->set("dirLight.diffuse", dirLight->diffuse);
        shader->set("dirLight.specular", dirLight->specular);
    } else {
        shader->set("dirLight.direction", glm::vec3(0.0f, -1.0f, 0.0f));

        shader->set("dirLight.ambient", glm::vec3(0.0f));
        shader->set("dirLight.diffuse", glm::vec3(0.0f));
        shader->set("dirLight.specular", glm::vec3(0.0f));
    }
}

void Renderer::processPointLights(const std::vector<const PbrPointLight*>& pointLights) {
    gpuPointLights.clear();
    gpuPointLights.reserve(pointLights.size());

    // Map shadow-casting lights to their cube-array layer.
    pointShadowLayerLookup.clear();
    pointShadowLayerLookup.reserve(shadowedPointLights.size());

    for (std::size_t i = 0; i < shadowedPointLights.size(); ++i) {
        pointShadowLayerLookup.emplace(shadowedPointLights[i], static_cast<int>(i));
    }

    // Pack scene lights into the GPU buffer layout.
    for (const auto* light : pointLights) {
        GPUMovingPointLight gpuLight;

        const auto layer = pointShadowLayerLookup.find(light);
        const int shadowLayer = light->castsShadow && layer != pointShadowLayerLookup.end() ? layer->second : -1;
        gpuLight.position = glm::vec4(light->position, static_cast<float>(shadowLayer));

        gpuLight.color = glm::vec4(light->color, light->intensity);

        gpuLight.radius = 5.0f * std::sqrt(light->intensity);

        gpuLight.constant = 1.0f;
        gpuLight.linear = 0.09f;
        gpuLight.quadratic = 0.032f;

        gpuPointLights.push_back(gpuLight);
    }

    // Upload the packed light list to the shader storage buffer.
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO);

    if (!gpuPointLights.empty()) {
        glBufferData(GL_SHADER_STORAGE_BUFFER, gpuPointLights.size() * sizeof(GPUMovingPointLight), gpuPointLights.data(), GL_DYNAMIC_DRAW);
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind buffer
}

void Renderer::renderInstanced(const std::shared_ptr<Model>& model, const std::vector<VisibleInstance>& instances, const Camera& camera,
                               float aspectRatio) {
    if (!model || model->subMeshes.empty() || instances.empty())
        return;

    std::vector<InstanceData> instanceData;
    instanceData.reserve(instances.size());

    for (const auto& inst : instances) {
        instanceData.push_back({inst.worldMatrix});
    }

    // Upload transforms once, then draw each submesh as one instanced batch.
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(InstanceData), instanceData.data(), GL_STREAM_DRAW);

    for (const auto& subMesh : model->subMeshes) {
        if (!subMesh.mesh || !subMesh.material || !subMesh.mesh->isReady())
            continue;

        auto shader = subMesh.material->getShader();

        if (!shader || !shader->isValid())
            continue;

        shader->use();
        subMesh.material->bind();
        shader->set("isInstanced", true);

        // Set the camera uniforms used by the material shader.
        shader->set("view", camera.getViewMatrix());
        shader->set("projection", camera.getProjectionMatrix(aspectRatio));
        shader->set("cameraPos", camera.position);

        subMesh.mesh->setupInstanceAttributes(instanceVBO);

        // Draw this submesh for all instances in the group.
        glBindVertexArray(subMesh.mesh->vao);

        glDrawElementsInstanced(GL_TRIANGLES, subMesh.mesh->indexCount, GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(instanceData.size()));

        glBindVertexArray(0);
    }
}

void Renderer::renderSingle(const std::shared_ptr<Model>& model, const glm::mat4& worldMatrix, const Camera& camera, float aspectRatio) {
    if (!model || model->subMeshes.empty())
        return;

    for (const auto& subMesh : model->subMeshes) {
        if (!subMesh.mesh || !subMesh.material || !subMesh.mesh->isReady())
            continue;

        auto shader = subMesh.material->getShader();
        if (!shader || !shader->isValid())
            continue;

        shader->use();
        subMesh.material->bind();

        // Provide the transform and camera state for a single object draw.
        shader->set("isInstanced", false);

        shader->set("model", worldMatrix);
        shader->set("view", camera.getViewMatrix());
        shader->set("projection", camera.getProjectionMatrix(aspectRatio));
        shader->set("cameraPos", camera.position);

        // Draw the mesh with its model transform.
        glBindVertexArray(subMesh.mesh->vao);

        glDrawElements(GL_TRIANGLES, subMesh.mesh->indexCount, GL_UNSIGNED_INT, nullptr);

        glBindVertexArray(0);
    }
}

bool Renderer::renderObject(const VisibleInstance& instance, const Camera& camera, float aspectRatio) {
    if (!initialized || !instance.object)
        return false;

    const auto& object = *instance.object;

    if (!object.model)
        return true;

    if (object.model->subMeshes.empty())
        return false;

    renderSingle(object.model, instance.worldMatrix, camera, aspectRatio);
    return true;
}

bool Renderer::renderObject(const Object& object, const Camera& camera, float aspectRatio) {
    return renderObject(VisibleInstance{&object, object.getWorldMatrix()}, camera, aspectRatio);
}

void Renderer::renderSkybox(unsigned int cubemapID, const Camera& camera, float aspectRatio) {
    if (DISABLE_SKYMAP)
        return;

    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);

    // Remove camera translation so the skybox stays centered around the view.
    skyboxShader->use();

    glm::mat4 view = glm::mat4(glm::mat3(camera.getViewMatrix()));
    glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);

    skyboxShader->set("view", view);
    skyboxShader->set("projection", projection);
    skyboxShader->set("exposure", AMBIENT_INTENSITY);

    // Bind the cubemap and draw the skybox cube.
    glBindVertexArray(skyboxMesh->vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);
    skyboxShader->set("skybox", 0);

    glDrawElements(GL_TRIANGLES, skyboxMesh->indexCount, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

void Renderer::renderObjects(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups, float aspectRatio) {
    const auto& camera = scene.getCamera();
    const auto dirLights = scene.getLightManager().getDirLights();
    const auto pointLights = scene.getLightManager().getPointLights();

    auto setupSceneUniforms = [&](const std::shared_ptr<Shader>& shader) {
        if (!shader || !shader->isValid())
            return;

        shader->use();

        processDirLights(shader, dirLights);

        shader->set("activePointLightCount", static_cast<int>(pointLights.size()));

        // Bind image-based lighting resources.
        // Irradiance Map
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_CUBE_MAP, scene.getIrradianceMap());
        shader->set("irradianceMap", 8);

        // Prefilter Map
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_CUBE_MAP, scene.getPrefilterMap());
        shader->set("prefilterMap", 9);

        // BRDF LUT
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
        shader->set("brdfLUT", 10);

        // Bind the directional and point shadow maps.
        // Shadow map
        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        shader->set("shadowMap", 11);

        glActiveTexture(GL_TEXTURE12);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, pointDepthMap);
        shader->set("pointShadowMap", 12);
        shader->set("pointShadowCount", pointShadowCount);

        glActiveTexture(GL_TEXTURE13);
        glBindTexture(GL_TEXTURE_2D, ssaoBlurTexture);
        shader->set("ssaoMap", 13);
        shader->set("ssaoEnabled", ssaoEnabled);

        // Set the remaining global lighting uniforms.
        shader->set("maxReflectionLOD", 4.0f);

        shader->set("ambientIntensity", AMBIENT_INTENSITY);

        shader->set("lightSpaceMatrix", lightSpaceMatrix);
    };

    struct TranslucentDraw {
        const SubMesh* subMesh;
        glm::mat4 worldMatrix;
        float depth;
    };
    std::vector<TranslucentDraw> translucentDraws;
    std::unordered_map<unsigned int, bool> alphaShaders;
    const auto view = camera.getViewMatrix();

    // Establish the opaque depth buffer before blending any translucent pixels.
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // Draw opaque fragments first and collect translucent submesh instances.
    for (const auto& [modelKey, instances] : instanceGroups) {
        // validation
        if (instances.empty())
            continue;

        const auto& model = instances.front().object->model;
        if (!model || model->subMeshes.empty())
            continue;

        // Prepare shader state and find submeshes that support the alpha pass.
        std::unordered_set<unsigned int> preparedShaders;

        for (const auto& subMesh : model->subMeshes) {
            if (!subMesh.material)
                continue;

            auto shader = subMesh.material->getShader();
            if (!shader || !shader->isValid())
                continue;

            if (preparedShaders.insert(shader->getShaderProgram()).second) {
                setupSceneUniforms(shader);
                const bool supportsAlphaPass = glGetUniformLocation(shader->getShaderProgram(), "alphaPass") >= 0;
                alphaShaders[shader->getShaderProgram()] = supportsAlphaPass;
                shader->set("alphaPass", 1);
            }

            // Custom shaders without the pass uniform are drawn only once.
            if (subMesh.mesh && subMesh.mesh->isReady() && alphaShaders[shader->getShaderProgram()]) {
                for (const auto& inst : instances) {
                    const auto center = view * inst.worldMatrix * glm::vec4(subMesh.mesh->boundsCenter, 1.0f);
                    translucentDraws.push_back({&subMesh, inst.worldMatrix, -center.z});
                }
            }
        }

        // Use instancing for larger groups; draw smaller groups individually.
        if (instances.size() >= INSTANCE_THRESHOLD) { // instanced objects render
            renderInstanced(model, instances, camera, aspectRatio);
        } else { // just render
            for (const auto& inst : instances) {
                renderObject(inst, camera, aspectRatio);
            }
        }
    }

    // Sort globally across models and instances. Depth testing remains enabled,
    // but blended fragments must not prevent farther surfaces from contributing.
    std::stable_sort(translucentDraws.begin(), translucentDraws.end(), [](const auto& a, const auto& b) { return a.depth > b.depth; });

    // Blend translucent fragments from far to near without writing depth.
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    for (const auto& draw : translucentDraws) {
        const auto& subMesh = *draw.subMesh;

        auto shader = subMesh.material->getShader();
        subMesh.material->bind();

        shader->set("alphaPass", 2);
        shader->set("isInstanced", false);
        shader->set("model", draw.worldMatrix);
        shader->set("view", view);
        shader->set("projection", camera.getProjectionMatrix(aspectRatio));
        shader->set("cameraPos", camera.position);

        glBindVertexArray(subMesh.mesh->vao);
        glDrawElements(GL_TRIANGLES, subMesh.mesh->indexCount, GL_UNSIGNED_INT, nullptr);

        // Preserve the default behavior of subsequent direct renderSingle calls.
        shader->set("alphaPass", 0);
    }

    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}

void Renderer::renderSSAO(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups, float aspectRatio) {
    if (!ssaoEnabled || !prePassShader || !prePassShader->isValid() || !ssaoShader || !ssaoShader->isValid() || !ssaoBlurShader ||
        !ssaoBlurShader->isValid()) {
        return;
    }

    const auto& camera = scene.getCamera();
    const glm::mat4 view = camera.getViewMatrix();
    const glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);

    // Render opaque geometry into the view-space normal/depth buffer.
    glViewport(0, 0, ssaoWidth, ssaoHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, prePassFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    prePassShader->use();
    prePassShader->set("view", view);
    prePassShader->set("projection", projection);
    prePassShader->set("alphaPass", 1);

    // Draw each visible model with the same instancing threshold as the main pass.
    for (const auto& [modelKey, instances] : instanceGroups) {
        (void)modelKey;
        if (instances.empty() || !instances.front().object || !instances.front().object->model) {
            continue;
        }

        const auto& model = instances.front().object->model;
        const bool useInstancing = instances.size() >= INSTANCE_THRESHOLD;

        if (useInstancing) {
            std::vector<InstanceData> instanceData;
            instanceData.reserve(instances.size());
            for (const auto& instance : instances) {
                instanceData.push_back({instance.worldMatrix});
            }

            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
            glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(InstanceData), instanceData.data(), GL_STREAM_DRAW);
        }

        prePassShader->set("isInstanced", useInstancing);

        for (const auto& subMesh : model->subMeshes) {
            if (!subMesh.mesh || !subMesh.mesh->isReady() || !subMesh.material) {
                continue;
            }

            // Material binding selects its own shader, so restore the pre-pass shader.
            subMesh.material->bind();
            prePassShader->use();
            prePassShader->set("material.albedoMap", 0);

            if (useInstancing) {
                subMesh.mesh->setupInstanceAttributes(instanceVBO);
                glBindVertexArray(subMesh.mesh->vao);
                glDrawElementsInstanced(GL_TRIANGLES, subMesh.mesh->indexCount, GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(instances.size()));
            } else {
                for (const auto& instance : instances) {
                    prePassShader->set("model", instance.worldMatrix);

                    glBindVertexArray(subMesh.mesh->vao);
                    glDrawElements(GL_TRIANGLES, subMesh.mesh->indexCount, GL_UNSIGNED_INT, nullptr);
                }
            }
        }
    }

    // Estimate ambient occlusion from the pre-pass data.
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, prePassTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture);
    ssaoShader->use();
    ssaoShader->set("projection", projection);
    ssaoShader->set("inverseProjection", glm::inverse(projection));
    ssaoShader->set("screenSize", glm::vec2(static_cast<float>(ssaoWidth), static_cast<float>(ssaoHeight)));
    renderQuad();

    // Blur the raw estimate while preserving depth and normal edges.
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, prePassTexture);
    ssaoBlurShader->use();
    ssaoBlurShader->set("screenSize", glm::vec2(static_cast<float>(ssaoWidth), static_cast<float>(ssaoHeight)));
    renderQuad();

    // Restore the main forward-rendering target and state.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
}

bool Renderer::renderScene(Scene& scene, float aspectRatio) {
    if (!initialized)
        return false;

    activeScene = &scene;
    GLint outputFramebuffer = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &outputFramebuffer);

    // var
    const auto& camera = scene.getCamera();
    auto& objectManager = scene.getObjectManager();
    auto& lightManager = scene.getLightManager();

    // Cache the scene lights for both shadowing and the main pass.
    const auto dirLights = lightManager.getDirLights();
    const auto pointLights = lightManager.getPointLights();

    // Group visible instances for the forward and shadow passes.
    std::unordered_map<const Model*, std::vector<VisibleInstance>> instanceGroups;
    std::unordered_map<const Model*, std::vector<VisibleInstance>> shadowGroups;

    const Frustum& frustum = camera.getFrustum(aspectRatio);

    // Build the shadow list and cull the main-pass list against the camera.
    for (const auto& object : objectManager.getObjects()) {
        if (!object->model || object->model->subMeshes.empty())
            continue;

        glm::mat4 worldMatrix = object->getWorldMatrix();

        shadowGroups[object->model.get()].push_back(VisibleInstance{object.get(), worldMatrix});

        // Only camera-visible objects contribute to the forward and SSAO passes.
        if (!object->isVisible(frustum, worldMatrix))
            continue;

        instanceGroups[object->model.get()].push_back(VisibleInstance{object.get(), worldMatrix});
    }

    // Run shadow and SSAO pre-passes before the forward pass.
    renderShadow(scene, shadowGroups);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(outputFramebuffer));
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    // Upload the layer mapping selected for this frame, including unshadowed lights.
    processPointLights(pointLights);

    renderSSAO(scene, instanceGroups, aspectRatio);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(outputFramebuffer));
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    // Draw the sky and forward-lit geometry.
    renderSkybox(scene.getCubeMap(), camera, aspectRatio);

    renderObjects(scene, instanceGroups, aspectRatio);

    return true;
}

void Renderer::renderDirShadow(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups) {
    if (DISABLE_SHADOW)
        return;
    if (!dirShadowShader || !dirShadowShader->isValid())
        return;

    const auto dirLights = scene.getLightManager().getDirLights();
    glm::vec3 lightDir(0.0f, -1.0f, 0.0f);

    // Use the first directional light, or a stable default when none is present.
    if (!dirLights.empty()) {
        lightDir = dirLights.front()->getDirection();
    }

    // Normalize the direction and choose a non-parallel up vector.
    if (glm::length(lightDir) < 0.0001f) { // is 0 vector?
        lightDir = glm::vec3(0.0f, -1.0f, 0.0f);
    } else {
        lightDir = glm::normalize(lightDir);
    }

    // Build the directional light's orthographic view-projection matrix.
    glm::mat4 lightProjection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, 0.1f, 30.0f);

    glm::vec3 lightPos = -lightDir * 10.0f;
    glm::vec3 target = glm::vec3(0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(lightDir, up)) > 0.99f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::mat4 lightView = glm::lookAt(lightPos, target, up);

    lightSpaceMatrix = lightProjection * lightView;

    // Render the directional shadow map.
    glViewport(0, 0, SHADOW_RESOLUTION, SHADOW_RESOLUTION);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_CLAMP);

    dirShadowShader->use();

    dirShadowShader->set("lightSpaceMatrix", lightSpaceMatrix);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.1f, 1.0f);

    renderShadowObjects(instanceGroups, dirShadowShader);

    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_DEPTH_CLAMP);

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
}

void Renderer::renderShadowObjects(const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups,
                                   const std::shared_ptr<Shader>& shader) {
    for (const auto& [model, instances] : instanceGroups) {
        if (!model || model->subMeshes.empty() || instances.empty())
            continue;

        const bool useInstancing = instances.size() >= INSTANCE_THRESHOLD;

        if (useInstancing) {
            // Upload model transforms once for the instanced shadow draw.
            std::vector<InstanceData> instanceData;
            instanceData.reserve(instances.size());
            for (const auto& inst : instances) {
                instanceData.push_back({inst.worldMatrix});
            }

            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
            glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(InstanceData), instanceData.data(), GL_STREAM_DRAW);

            shader->set("isInstanced", true);

            // Draw all submeshes with the shared instance transform buffer.
            for (const auto& subMesh : model->subMeshes) {
                if (!subMesh.mesh || !subMesh.mesh->isReady())
                    continue;

                subMesh.mesh->setupInstanceAttributes(instanceVBO);
                glBindVertexArray(subMesh.mesh->vao);
                glDrawElementsInstanced(GL_TRIANGLES, subMesh.mesh->indexCount, GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(instanceData.size()));
            }
        } else {
            shader->set("isInstanced", false);

            // Draw each instance with its own model matrix.
            for (const auto& instance : instances) {
                shader->set("model", instance.worldMatrix);
                for (const auto& subMesh : model->subMeshes) {
                    if (!subMesh.mesh || !subMesh.mesh->isReady())
                        continue;

                    glBindVertexArray(subMesh.mesh->vao);
                    glDrawElements(GL_TRIANGLES, subMesh.mesh->indexCount, GL_UNSIGNED_INT, nullptr);
                }
            }
        }
    }
    glBindVertexArray(0);
}

void Renderer::renderShadow(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups) {
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    renderDirShadow(scene, instanceGroups);
    renderPointShadow(scene, instanceGroups);
}

void Renderer::renderPointShadow(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups) {
    shadowedPointLights.clear();

    if (DISABLE_SHADOW || !pointShadowShader || !pointShadowShader->isValid()) {
        pointShadowCount = 0;
        return;
    }

    const auto lights = scene.getLightManager().getPointLights();
    GLint maxLayers = 0;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxLayers);

    for (const auto* light : lights) {
        if (light->castsShadow && shadowedPointLights.size() < static_cast<std::size_t>(maxLayers / 6)) {
            shadowedPointLights.push_back(light);
        }
    }
    const int count = static_cast<int>(shadowedPointLights.size());

    constexpr int POINT_SHADOW_RES = POINT_SHADOW_RESOLUTION;

    glActiveTexture(GL_TEXTURE12);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, pointDepthMap);

    if (count > pointShadowCapacity) {
        pointShadowCapacity = std::max(count, std::max(1, pointShadowCapacity * 2));
        glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_DEPTH_COMPONENT32F, POINT_SHADOW_RES, POINT_SHADOW_RES, pointShadowCapacity * 6, 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        // Configure sampling once when the texture storage grows.
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    pointShadowCount = count;
    if (count == 0)
        return;

    const glm::vec3 directions[] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    const glm::vec3 up[] = {{0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}};

    // Prepare the layered framebuffer and point-shadow shader.
    glViewport(0, 0, POINT_SHADOW_RES, POINT_SHADOW_RES);
    glBindFramebuffer(GL_FRAMEBUFFER, pointDepthFBO);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    pointShadowShader->use();

    const float pointShadowNear = std::max(kNearPlane, 0.15f);

    // Render all six cube faces for each shadow-casting light.
    for (int light = 0; light < count; ++light) {
        const auto& position = shadowedPointLights[light]->position;
        const float farPlane = std::max(pointShadowNear * 2.0f, 5.0f * std::sqrt(std::max(0.0f, shadowedPointLights[light]->intensity)));

        const auto projection = glm::perspective(glm::radians(90.0f), 1.0f, pointShadowNear, farPlane);

        // Reuse the same grouping storage while culling for each light.
        for (auto& [model, groupedInstances] : culledPointShadowGroups) {
            (void)model;
            groupedInstances.clear();
        }

        for (const auto& [model, instances] : instanceGroups) {
            for (const auto& inst : instances) {
                glm::vec3 objPos = glm::vec3(inst.worldMatrix[3]);

                const float dist = glm::distance(position, objPos);
                if (dist <= farPlane + 2.0f) {
                    culledPointShadowGroups[model].push_back(inst);
                }
            }
        }

        // Clear all six layers when this light has no nearby geometry.
        bool hasVisibleInstances = false;
        for (const auto& [model, groupedInstances] : culledPointShadowGroups) {
            (void)model;
            hasVisibleInstances |= !groupedInstances.empty();
        }
        if (!hasVisibleInstances) {
            for (int face = 0; face < 6; ++face) {
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointDepthMap, 0, light * 6 + face);
                glClear(GL_DEPTH_BUFFER_BIT);
            }
            continue;
        }

        pointShadowShader->set("lightPos", position);
        pointShadowShader->set("farPlane", farPlane);

        // Attach and render each cubemap face in turn.
        for (int face = 0; face < 6; ++face) {
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointDepthMap, 0, light * 6 + face);
            glClear(GL_DEPTH_BUFFER_BIT);
            pointShadowShader->set("lightSpaceMatrix", projection * glm::lookAt(position, position + directions[face], up[face]));

            renderShadowObjects(culledPointShadowGroups, pointShadowShader);
        }
    }

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
}
} // namespace knot
