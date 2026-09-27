#include <knot/renderer.h>
#include "vulkan_internal.h"
#include "VulkanShaders.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace knot {
namespace {
struct SceneUniform {
    glm::mat4 viewProjection{1}, lightSpace{1}, skyViewProjection{1};
    glm::vec4 camera{}, direction{0, -1, 0, 0}, diffuse{}, ambient{};
    glm::ivec4 counts{};
};
struct MaterialUniform {
    glm::vec4 albedo{1};
    glm::vec4 factors{1};
};
struct Draw {
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;
    std::vector<glm::mat4> transforms;
};
struct Job {
    SceneUniform uniform;
    std::vector<GPUMovingPointLight> lights;
    std::vector<Draw> draws, casters;
    unsigned environment{}, irradiance{}, prefilter{};
    bool offscreen = false;
};
glm::mat4 clipCorrection() {
    glm::mat4 m(1);
    m[1][1] = -1;
    m[2][2] = .5f;
    m[3][2] = .5f;
    return m;
}
SceneUniform cameraUniform(const Camera& camera, float aspect) {
    SceneUniform u;
    u.viewProjection = clipCorrection() * camera.getProjectionMatrix(aspect) * camera.getViewMatrix();
    u.skyViewProjection = clipCorrection() * camera.getProjectionMatrix(aspect) * glm::mat4(glm::mat3(camera.getViewMatrix()));
    u.camera = glm::vec4(camera.position, 1);
    return u;
}
} // namespace
struct Renderer::Impl {
    vk::Context& c = vk::context();
    GLFWwindow* window{};
    VkSwapchainKHR swapchain{};
    VkFormat format{}, depthFormat{};
    VkExtent2D extent{};
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkSemaphore> finished;
    VkSemaphore acquired{};
    VkFence fence{};
    VkCommandBuffer cmd{};
    VkRenderPass mainPass{}, loadPass{}, offscreenPass{}, shadowPass{};
    VkDescriptorSetLayout sceneLayout{}, materialLayout{};
    VkPipelineLayout layout{};
    VkDescriptorPool descriptors{};
    VkPipeline alpha{}, pbr{}, sky{}, shadow{};
    vk::Image depth, colorMSAA, preview, previewDepth, previewMSAA, shadowMap, brdf;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkFramebuffer previewFB{}, shadowFB{};
    unsigned white{}, normal{}, blackCube{};
    std::shared_ptr<Mesh> cube;
    glm::vec4 clear{.055f, .065f, .085f, 1};
    bool vsync = true, recreate = false, active = false;
    uint32_t imageIndex{};
    std::vector<vk::Buffer> transient;
    std::vector<Job> jobs;
    std::string screenshot;
    bool canCapture = false;

    VkRenderPass renderPass(VkFormat color, bool withDepth, VkImageLayout finalLayout, bool load = false) {
        std::array<VkAttachmentDescription, 3> attachments{};
        const bool multisample = withDepth && color != VK_FORMAT_UNDEFINED && samples != VK_SAMPLE_COUNT_1_BIT;
        uint32_t count = 0;
        VkAttachmentReference colorRef{}, depthRef{}, resolveRef{};
        if (color != VK_FORMAT_UNDEFINED) {
            auto& a = attachments[count];
            a.format = color;
            a.samples = multisample ? samples : VK_SAMPLE_COUNT_1_BIT;
            a.loadOp = load ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
            a.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            a.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            a.initialLayout = load ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
            a.finalLayout = multisample ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : finalLayout;
            colorRef = {count++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        }
        if (withDepth) {
            auto& a = attachments[count];
            a.format = depthFormat;
            a.samples = multisample ? samples : VK_SAMPLE_COUNT_1_BIT;
            a.loadOp = load ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
            a.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            a.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            a.initialLayout = load ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
            a.finalLayout =
                color == VK_FORMAT_UNDEFINED ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthRef = {count++, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        }
        if (multisample) {
            auto& resolve = attachments[count];
            resolve.format = color;
            resolve.samples = VK_SAMPLE_COUNT_1_BIT;
            resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            resolve.finalLayout = finalLayout;
            resolveRef = {count++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        }
        VkSubpassDescription sub{};
        sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        if (color != VK_FORMAT_UNDEFINED) {
            sub.colorAttachmentCount = 1;
            sub.pColorAttachments = &colorRef;
            if (multisample)
                sub.pResolveAttachments = &resolveRef;
        }
        if (withDepth)
            sub.pDepthStencilAttachment = &depthRef;
        std::array<VkSubpassDependency, 2> deps{};
        deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        deps[0].dstSubpass = 0;
        deps[0].srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        deps[0].dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        deps[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        deps[1].srcSubpass = 0;
        deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        deps[1].srcStageMask = deps[0].dstStageMask;
        deps[1].dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        deps[1].srcAccessMask = deps[0].dstAccessMask;
        deps[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        VkRenderPassCreateInfo ci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        ci.attachmentCount = count;
        ci.pAttachments = attachments.data();
        ci.subpassCount = 1;
        ci.pSubpasses = &sub;
        ci.dependencyCount = 2;
        ci.pDependencies = deps.data();
        VkRenderPass pass{};
        vk::check(vkCreateRenderPass(c.device, &ci, nullptr, &pass), "render pass");
        return pass;
    }
    VkFramebuffer framebuffer(VkRenderPass pass, const std::vector<VkImageView>& attachments, uint32_t w, uint32_t h) {
        VkFramebufferCreateInfo ci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        ci.renderPass = pass;
        ci.attachmentCount = uint32_t(attachments.size());
        ci.pAttachments = attachments.data();
        ci.width = w;
        ci.height = h;
        ci.layers = 1;
        VkFramebuffer fb{};
        vk::check(vkCreateFramebuffer(c.device, &ci, nullptr, &fb), "framebuffer");
        return fb;
    }
    VkShaderModule module(const std::string& name) {
        auto data = VulkanShaders::get(name);
        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        ci.codeSize = data.second;
        ci.pCode = data.first;
        VkShaderModule m{};
        vk::check(vkCreateShaderModule(c.device, &ci, nullptr, &m), "shader module");
        return m;
    }
    VkPipeline pipeline(const char* vert, const char* frag, VkRenderPass pass, int kind) {
        // kind: 0 mesh, 1 sky, 2 shadow, 3 fullscreen integration.
        VkShaderModule vm = module(vert), fm{};
        VkPipeline result{};
        try {
            fm = module(frag);
            std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
            for (auto& s : stages) {
                s.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                s.pName = "main";
            }
            stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            stages[0].module = vm;
            stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            stages[1].module = fm;
            std::array<VkVertexInputBindingDescription, 2> bindings{
                {{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}, {1, sizeof(glm::mat4), VK_VERTEX_INPUT_RATE_INSTANCE}}};
            std::vector<VkVertexInputAttributeDescription> attrs{{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Position)}};
            if (kind == 0) {
                attrs.push_back({1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, TexCoords)});
                if (std::string(vert) != "unlit.vert") {
                    attrs.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Normal)});
                    attrs.push_back({3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Tangent)});
                }
            }
            if (kind == 0 || kind == 2)
                for (uint32_t i = 0; i < 4; ++i)
                    attrs.push_back({4 + i, 1, VK_FORMAT_R32G32B32A32_SFLOAT, i * uint32_t(sizeof(glm::vec4))});
            VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
            if (kind != 3) {
                vi.vertexBindingDescriptionCount = (kind == 1 ? 1 : 2);
                vi.pVertexBindingDescriptions = bindings.data();
                vi.vertexAttributeDescriptionCount = uint32_t(attrs.size());
                vi.pVertexAttributeDescriptions = attrs.data();
            }
            VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
            ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
            vp.viewportCount = vp.scissorCount = 1;
            VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
            rs.polygonMode = VK_POLYGON_MODE_FILL;
            rs.lineWidth = 1;
            rs.cullMode = kind == 0 ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
            rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
            ms.rasterizationSamples = kind < 2 ? samples : VK_SAMPLE_COUNT_1_BIT;
            VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
            ds.depthTestEnable = kind != 3;
            ds.depthWriteEnable = kind == 0 || kind == 2;
            ds.depthCompareOp = kind == 1 ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_LESS;
            VkPipelineColorBlendAttachmentState attachment{};
            attachment.colorWriteMask = 0xf;
            if (std::string(frag) == "alpha.frag") {
                attachment.blendEnable = VK_TRUE;
                attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                attachment.colorBlendOp = VK_BLEND_OP_ADD;
                attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                attachment.alphaBlendOp = VK_BLEND_OP_ADD;
            }
            VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
            if (kind != 2) {
                cb.attachmentCount = 1;
                cb.pAttachments = &attachment;
            }
            VkDynamicState states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo dyn{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
            dyn.dynamicStateCount = 2;
            dyn.pDynamicStates = states;
            VkGraphicsPipelineCreateInfo ci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
            ci.stageCount = 2;
            ci.pStages = stages.data();
            ci.pVertexInputState = &vi;
            ci.pInputAssemblyState = &ia;
            ci.pViewportState = &vp;
            ci.pRasterizationState = &rs;
            ci.pMultisampleState = &ms;
            ci.pDepthStencilState = &ds;
            ci.pColorBlendState = &cb;
            ci.pDynamicState = &dyn;
            ci.layout = layout;
            ci.renderPass = pass;
            vk::check(vkCreateGraphicsPipelines(c.device, VK_NULL_HANDLE, 1, &ci, nullptr, &result), "graphics pipeline");
        } catch (...) {
            if (fm)
                vkDestroyShaderModule(c.device, fm, nullptr);
            vkDestroyShaderModule(c.device, vm, nullptr);
            throw;
        }
        vkDestroyShaderModule(c.device, fm, nullptr);
        vkDestroyShaderModule(c.device, vm, nullptr);
        return result;
    }
    void destroySwapchain() {
        for (auto fb : framebuffers)
            vkDestroyFramebuffer(c.device, fb, nullptr);
        framebuffers.clear();
        for (auto v : views)
            vkDestroyImageView(c.device, v, nullptr);
        views.clear();
        for (auto s : finished)
            vkDestroySemaphore(c.device, s, nullptr);
        finished.clear();
        c.destroy(depth);
        c.destroy(colorMSAA);
        images.clear();
        if (swapchain)
            vkDestroySwapchainKHR(c.device, swapchain, nullptr);
        swapchain = {};
    }
    void createSwapchain() {
        vk::check(vkDeviceWaitIdle(c.device), "wait resize");
        VkSurfaceCapabilitiesKHR caps;
        vk::check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(c.physical, c.surface, &caps), "surface capabilities");
        uint32_t n = 0;
        vk::check(vkGetPhysicalDeviceSurfaceFormatsKHR(c.physical, c.surface, &n, nullptr), "surface formats");
        std::vector<VkSurfaceFormatKHR> formats(n);
        vk::check(vkGetPhysicalDeviceSurfaceFormatsKHR(c.physical, c.surface, &n, formats.data()), "surface formats");
        if (formats.empty())
            throw std::runtime_error("Surface has no Vulkan formats");
        VkSurfaceFormatKHR selected = formats.front();
        for (auto f : formats)
            if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                selected = f;
                break;
            }
        if (selected.format == VK_FORMAT_UNDEFINED)
            selected.format = VK_FORMAT_B8G8R8A8_UNORM;
        if (format && format != selected.format)
            throw std::runtime_error("Surface format changed; reinitialize Renderer");
        format = selected.format;
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        extent = caps.currentExtent;
        if (extent.width == UINT32_MAX) {
            extent.width = std::clamp(uint32_t(std::max(w, 1)), caps.minImageExtent.width, caps.maxImageExtent.width);
            extent.height = std::clamp(uint32_t(std::max(h, 1)), caps.minImageExtent.height, caps.maxImageExtent.height);
        }
        uint32_t count = std::max(2u, caps.minImageCount + 1);
        if (caps.maxImageCount)
            count = std::min(count, caps.maxImageCount);
        vk::check(vkGetPhysicalDeviceSurfacePresentModesKHR(c.physical, c.surface, &n, nullptr), "present modes");
        std::vector<VkPresentModeKHR> modes(n);
        vk::check(vkGetPhysicalDeviceSurfacePresentModesKHR(c.physical, c.surface, &n, modes.data()), "present modes");
        VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
        if (!vsync)
            for (auto wanted : {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR})
                if (std::find(modes.begin(), modes.end(), wanted) != modes.end()) {
                    mode = wanted;
                    break;
                }
        destroySwapchain();
        VkSwapchainCreateInfoKHR ci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        ci.surface = c.surface;
        ci.minImageCount = count;
        ci.imageFormat = format;
        ci.imageColorSpace = selected.colorSpace;
        ci.imageExtent = extent;
        ci.imageArrayLayers = 1;
        ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        canCapture = (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0;
        if (canCapture)
            ci.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ci.preTransform = caps.currentTransform;
        ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        for (auto flag : {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                          VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR})
            if (caps.supportedCompositeAlpha & flag) {
                ci.compositeAlpha = flag;
                break;
            }
        ci.presentMode = mode;
        ci.clipped = VK_TRUE;
        vk::check(vkCreateSwapchainKHR(c.device, &ci, nullptr, &swapchain), "swapchain");
        vk::check(vkGetSwapchainImagesKHR(c.device, swapchain, &n, nullptr), "swapchain images");
        images.resize(n);
        vk::check(vkGetSwapchainImagesKHR(c.device, swapchain, &n, images.data()), "swapchain images");
        if (!mainPass) {
            mainPass = renderPass(format, true, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            loadPass = renderPass(format, true, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true);
            offscreenPass = renderPass(format, true, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        depth =
            c.image(extent.width, extent.height, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, 1, 1, samples);
        if (samples != VK_SAMPLE_COUNT_1_BIT)
            colorMSAA = c.image(extent.width, extent.height, format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 1, 1, samples);
        for (auto im : images) {
            VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            vi.image = im;
            vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
            vi.format = format;
            vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            VkImageView view{};
            vk::check(vkCreateImageView(c.device, &vi, nullptr, &view), "swapchain view");
            views.push_back(view);
            const std::vector<VkImageView> attachments = samples != VK_SAMPLE_COUNT_1_BIT ? std::vector<VkImageView>{colorMSAA.view, depth.view, view}
                                                                                          : std::vector<VkImageView>{view, depth.view};
            framebuffers.push_back(framebuffer(mainPass, attachments, extent.width, extent.height));
            VkSemaphoreCreateInfo si{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            VkSemaphore semaphore{};
            vk::check(vkCreateSemaphore(c.device, &si, nullptr, &semaphore), "present semaphore");
            finished.push_back(semaphore);
        }
        recreate = false;
    }
    void startPass(VkCommandBuffer command, VkRenderPass pass, VkFramebuffer fb, uint32_t w, uint32_t h, bool depthOnly = false) {
        std::array<VkClearValue, 2> clears{};
        clears[0].color = {{clear.r, clear.g, clear.b, clear.a}};
        clears[1].depthStencil = {1, 0};
        if (depthOnly)
            clears[0].depthStencil = {1, 0};
        VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        bi.renderPass = pass;
        bi.framebuffer = fb;
        bi.renderArea.extent = {w, h};
        bi.clearValueCount = depthOnly ? 1 : 2;
        bi.pClearValues = clears.data();
        vkCmdBeginRenderPass(command, &bi, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport vp{0, 0, float(w), float(h), 0, 1};
        VkRect2D sc{{0, 0}, {w, h}};
        vkCmdSetViewport(command, 0, 1, &vp);
        vkCmdSetScissor(command, 0, 1, &sc);
    }
    void init(GLFWwindow* w) {
        window = w;
        c.init(w);
        for (auto f : {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM}) {
            VkFormatProperties p;
            vkGetPhysicalDeviceFormatProperties(c.physical, f, &p);
            if ((p.optimalTilingFeatures & (VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) ==
                (VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
                depthFormat = f;
                break;
            }
        }
        if (!depthFormat)
            throw std::runtime_error("No sampled depth attachment format");
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(c.physical, &properties);
        auto supported = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
        samples = (supported & VK_SAMPLE_COUNT_4_BIT)   ? VK_SAMPLE_COUNT_4_BIT
                  : (supported & VK_SAMPLE_COUNT_2_BIT) ? VK_SAMPLE_COUNT_2_BIT
                                                        : VK_SAMPLE_COUNT_1_BIT;
        createSwapchain();
        VkSemaphoreCreateInfo si{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vk::check(vkCreateSemaphore(c.device, &si, nullptr, &acquired), "acquire semaphore");
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vk::check(vkCreateFence(c.device, &fi, nullptr, &fence), "frame fence");
        VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        ai.commandPool = c.pool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = 1;
        vk::check(vkAllocateCommandBuffers(c.device, &ai, &cmd), "frame command");
        std::array<VkDescriptorSetLayoutBinding, 2> sb{
            {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
             {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}}};
        VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        li.bindingCount = 2;
        li.pBindings = sb.data();
        vk::check(vkCreateDescriptorSetLayout(c.device, &li, nullptr, &sceneLayout), "scene descriptor layout");
        std::array<VkDescriptorSetLayoutBinding, 9> mb{};
        for (uint32_t i = 0; i < 9; ++i)
            mb[i] = {i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
        li.bindingCount = 9;
        li.pBindings = mb.data();
        vk::check(vkCreateDescriptorSetLayout(c.device, &li, nullptr, &materialLayout), "material descriptor layout");
        VkDescriptorSetLayout layouts[] = {sceneLayout, materialLayout};
        VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MaterialUniform)};
        VkPipelineLayoutCreateInfo pi{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pi.setLayoutCount = 2;
        pi.pSetLayouts = layouts;
        pi.pushConstantRangeCount = 1;
        pi.pPushConstantRanges = &push;
        vk::check(vkCreatePipelineLayout(c.device, &pi, nullptr, &layout), "pipeline layout");
        VkDescriptorPoolSize sizes[] = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 90000}};
        VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        dp.maxSets = 11024;
        dp.poolSizeCount = 3;
        dp.pPoolSizes = sizes;
        vk::check(vkCreateDescriptorPool(c.device, &dp, nullptr, &descriptors), "frame descriptor pool");
        alpha = pipeline("unlit.vert", "alpha.frag", mainPass, 0);
        pbr = pipeline("alpha.vert", "pbr.frag", mainPass, 0);
        sky = pipeline("skybox.vert", "skybox.frag", mainPass, 1);
        shadowPass = renderPass(VK_FORMAT_UNDEFINED, true, VK_IMAGE_LAYOUT_UNDEFINED);
        shadow = pipeline("shadow.vert", "shadow.frag", shadowPass, 2);
        shadowMap =
            c.image(2048, 2048, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
        shadowFB = framebuffer(shadowPass, {shadowMap.view}, 2048, 2048);
        white = createSolidColorTexture({1, 1, 1});
        normal = createSolidColorTexture({.5f, .5f, 1});
        blackCube = c.upload(std::vector<float>(24, 0.f), 1, 1, 6);
        cube = createCube();
        // Integrate the split-sum BRDF once into a sampled floating-point attachment.
        brdf = c.image(256, 256, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        VkRenderPass pass{};
        VkFramebuffer fb{};
        VkPipeline bake{};
        try {
            pass = renderPass(brdf.format, false, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            fb = framebuffer(pass, {brdf.view}, 256, 256);
            bake = pipeline("brdf.vert", "brdf.frag", pass, 3);
            c.immediate([&](VkCommandBuffer command) {
                startPass(command, pass, fb, 256, 256);
                vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, bake);
                vkCmdDraw(command, 3, 1, 0, 0);
                vkCmdEndRenderPass(command);
            });
        } catch (...) {
            if (bake)
                vkDestroyPipeline(c.device, bake, nullptr);
            if (fb)
                vkDestroyFramebuffer(c.device, fb, nullptr);
            if (pass)
                vkDestroyRenderPass(c.device, pass, nullptr);
            throw;
        }
        vkDestroyPipeline(c.device, bake, nullptr);
        vkDestroyFramebuffer(c.device, fb, nullptr);
        vkDestroyRenderPass(c.device, pass, nullptr);
    }
    ~Impl() {
        if (c.device) {
            vkDeviceWaitIdle(c.device);
            jobs.clear();
            cube.reset();
            for (auto& b : transient)
                c.destroy(b);
            for (auto p : {alpha, pbr, sky, shadow})
                if (p)
                    vkDestroyPipeline(c.device, p, nullptr);
            if (previewFB)
                vkDestroyFramebuffer(c.device, previewFB, nullptr);
            if (shadowFB)
                vkDestroyFramebuffer(c.device, shadowFB, nullptr);
            c.destroy(preview);
            c.destroy(previewDepth);
            c.destroy(previewMSAA);
            c.destroy(shadowMap);
            c.destroy(brdf);
            destroySwapchain();
            for (auto p : {mainPass, loadPass, offscreenPass, shadowPass})
                if (p)
                    vkDestroyRenderPass(c.device, p, nullptr);
            if (descriptors)
                vkDestroyDescriptorPool(c.device, descriptors, nullptr);
            if (layout)
                vkDestroyPipelineLayout(c.device, layout, nullptr);
            if (sceneLayout)
                vkDestroyDescriptorSetLayout(c.device, sceneLayout, nullptr);
            if (materialLayout)
                vkDestroyDescriptorSetLayout(c.device, materialLayout, nullptr);
            if (fence)
                vkDestroyFence(c.device, fence, nullptr);
            if (acquired)
                vkDestroySemaphore(c.device, acquired, nullptr);
        }
        c.shutdown();
    }
    VkDescriptorSet allocate(VkDescriptorSetLayout setLayout) {
        VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        ai.descriptorPool = descriptors;
        ai.descriptorSetCount = 1;
        ai.pSetLayouts = &setLayout;
        VkDescriptorSet set{};
        vk::check(vkAllocateDescriptorSets(c.device, &ai, &set), "allocate descriptor set");
        return set;
    }
    vk::Buffer temporary(VkDeviceSize size, VkBufferUsageFlags usage, const void* data) {
        auto b = c.buffer(size, usage, data);
        transient.push_back(b);
        return b;
    }
    VkDescriptorSet sceneSet(const Job& job) {
        auto u = temporary(sizeof(SceneUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &job.uniform);
        GPUMovingPointLight empty{};
        auto l = temporary(std::max(size_t(1), job.lights.size()) * sizeof(empty), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                           job.lights.empty() ? &empty : job.lights.data());
        VkDescriptorBufferInfo infos[] = {{u.handle, 0, u.size}, {l.handle, 0, l.size}};
        VkDescriptorSet set = allocate(sceneLayout);
        VkWriteDescriptorSet writes[2]{};
        for (uint32_t i = 0; i < 2; ++i) {
            auto& wr = writes[i];
            wr.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            wr.dstSet = set;
            wr.dstBinding = i;
            wr.descriptorCount = 1;
            wr.descriptorType = i ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            wr.pBufferInfo = &infos[i];
        }
        vkUpdateDescriptorSets(c.device, 2, writes, 0, nullptr);
        return set;
    }
    VkDescriptorImageInfo textureInfo(unsigned id, unsigned fallback) {
        auto it = c.textures.find(id);
        if (it == c.textures.end())
            it = c.textures.find(fallback);
        if (it == c.textures.end())
            throw std::runtime_error("Texture belongs to a previous renderer lifetime");
        auto& im = it->second.image;
        return {im.sampler, im.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }
    VkDescriptorSet materialSet(const Job& job, const Material* material, bool skybox = false) {
        unsigned ids[] = {white, white, white, white, normal, job.irradiance, job.prefilter};
        if (auto* p = dynamic_cast<const PbrMaterial*>(material)) {
            ids[0] = p->albedoMap;
            ids[1] = p->metallicMap;
            ids[2] = p->roughnessMap;
            ids[3] = p->aoMap;
            ids[4] = p->normalMap;
        } else if (auto* t = dynamic_cast<const TextureMaterial*>(material))
            ids[0] = t->getTextureId();
        if (skybox)
            ids[5] = job.environment;
        std::array<VkDescriptorImageInfo, 9> infos{};
        for (int i = 0; i < 7; ++i)
            infos[i] = textureInfo(ids[i], i >= 5 ? blackCube : i == 4 ? normal : white);
        infos[7] = {brdf.sampler, brdf.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        infos[8] = {shadowMap.sampler, shadowMap.view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
        VkDescriptorSet set = allocate(materialLayout);
        std::array<VkWriteDescriptorSet, 9> writes{};
        for (uint32_t i = 0; i < 9; ++i) {
            auto& wr = writes[i];
            wr.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            wr.dstSet = set;
            wr.dstBinding = i;
            wr.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            wr.descriptorCount = 1;
            wr.pImageInfo = &infos[i];
        }
        vkUpdateDescriptorSets(c.device, 9, writes.data(), 0, nullptr);
        return set;
    }
    void geometry(const Mesh& mesh, const std::vector<glm::mat4>& transforms, bool instanced = true) {
        auto it = c.meshes.find(mesh.gpuId);
        if (it == c.meshes.end())
            return;
        auto& g = it->second;
        VkBuffer buffers[] = {g.vertices.handle, VK_NULL_HANDLE};
        VkDeviceSize offsets[] = {0, 0};
        if (instanced) {
            if (transforms.empty())
                return;
            buffers[1] = temporary(transforms.size() * sizeof(glm::mat4), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, transforms.data()).handle;
        }
        vkCmdBindVertexBuffers(cmd, 0, instanced ? 2 : 1, buffers, offsets);
        vkCmdBindIndexBuffer(cmd, g.indices.handle, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, mesh.indexCount, instanced ? uint32_t(transforms.size()) : 1, 0, 0, 0);
    }
    void record(Job& job, bool mainAlready) {
        auto set = sceneSet(job);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &set, 0, nullptr);
        startPass(cmd, shadowPass, shadowFB, 2048, 2048, true);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow);
        if (job.uniform.counts.y)
            for (auto& d : job.casters)
                if (d.mesh && d.mesh->isReady())
                    geometry(*d.mesh, d.transforms);
        vkCmdEndRenderPass(cmd);
        startPass(cmd,
                  job.offscreen ? offscreenPass
                  : mainAlready ? loadPass
                                : mainPass,
                  job.offscreen ? previewFB : framebuffers[imageIndex], job.offscreen ? preview.width : extent.width,
                  job.offscreen ? preview.height : extent.height);
        if (job.environment) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sky);
            auto mat = materialSet(job, nullptr, true);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &mat, 0, nullptr);
            geometry(*cube, {}, false);
        }
        for (auto& d : job.draws) {
            if (!d.mesh || !d.mesh->isReady() || !d.material)
                continue;
            auto shader = d.material->getShader();
            if (!shader || !shader->isValid())
                continue;
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->isPbr() ? pbr : alpha);
            auto mat = materialSet(job, d.material.get());
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &mat, 0, nullptr);
            MaterialUniform push;
            if (auto p = std::dynamic_pointer_cast<PbrMaterial>(d.material)) {
                push.albedo = glm::vec4(p->albedoFactor(), 1);
                push.factors = glm::vec4(p->scalarFactors(), 0);
            }
            vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
            geometry(*d.mesh, d.transforms);
        }
        vkCmdEndRenderPass(cmd);
    }
    Job sceneJob(Scene& scene, float aspect) {
        Job job;
        job.uniform = cameraUniform(scene.getCamera(), aspect);
        auto dirs = scene.getLightManager().getDirLights();
        if (!dirs.empty()) {
            auto* light = dirs.front();
            glm::vec3 dir = light->getDirection();
            if (glm::length(dir) < .0001f)
                dir = {0, -1, 0};
            dir = glm::normalize(dir);
            job.uniform.direction = glm::vec4(dir, 0);
            job.uniform.diffuse = glm::vec4(light->diffuse, 0);
            job.uniform.ambient = glm::vec4(light->ambient, 0);
            job.uniform.counts.y = 1;
            glm::vec3 up = std::abs(dir.y) > .99f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
            job.uniform.lightSpace = clipCorrection() * glm::ortho(-15.f, 15.f, -15.f, 15.f, .1f, 30.f) * glm::lookAt(-dir * 10.f, glm::vec3(0), up);
        }
        for (auto* l : scene.getLightManager().getPointLights())
            job.lights.push_back(
                {glm::vec4(l->position, 1), glm::vec4(l->color, l->intensity), 5.f * std::sqrt(std::max(l->intensity, 0.f)), 1, .09f, .032f});
        job.uniform.counts.x = int(job.lights.size());
        job.environment = scene.getCubeMap();
        job.irradiance = scene.getIrradianceMap();
        job.prefilter = scene.getPrefilterMap();
        std::unordered_map<const Model*, std::vector<glm::mat4>> visible, all;
        auto& frustum = scene.getCamera().getFrustum(aspect);
        for (auto& object : scene.getObjectManager().getObjects())
            if (object->model) {
                auto world = object->getWorldMatrix();
                all[object->model.get()].push_back(world);
                if (object->isVisible(frustum, world))
                    visible[object->model.get()].push_back(world);
            }
        for (auto& [model, transforms] : all)
            for (auto& sm : model->subMeshes)
                job.casters.push_back({sm.mesh, sm.material, transforms});
        for (auto& [model, transforms] : visible)
            for (auto& sm : model->subMeshes)
                job.draws.push_back({sm.mesh, sm.material, transforms});
        return job;
    }
};
Renderer::Renderer() {
    (void)vk::context();
}
Renderer::~Renderer() {
    shutdown();
}
Renderer& Renderer::get() {
    static Renderer r;
    return r;
}
bool Renderer::init(GLFWwindow* window) {
    if (impl)
        return impl->window == window;
    if (!window || glfwGetWindowAttrib(window, GLFW_CLIENT_API) != GLFW_NO_API) {
        std::cerr << "[Error] Vulkan requires a GLFW_NO_API window\n";
        return false;
    }
    try {
        impl = std::make_unique<Impl>();
        impl->init(window);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Error] " << e.what() << '\n';
        shutdown();
        return false;
    }
}
void Renderer::shutdown() {
    impl.reset();
}
bool Renderer::isInitialized() const {
    return impl && impl->c.device;
}
void Renderer::waitIdle() {
    if (impl)
        vk::check(vkDeviceWaitIdle(impl->c.device), "wait idle");
}
void Renderer::setClearColor(const glm::vec4& color) {
    if (impl)
        impl->clear = color;
}
void Renderer::setVsync(bool enabled) {
    if (impl && impl->vsync != enabled) {
        impl->vsync = enabled;
        impl->recreate = true;
    }
}
bool Renderer::beginFrame(int w, int h) {
    if (!impl || w <= 0 || h <= 0)
        return false;
    auto& r = *impl;
    if (r.active)
        throw std::logic_error("endFrame must follow beginFrame");
    vk::check(vkWaitForFences(r.c.device, 1, &r.fence, VK_TRUE, UINT64_MAX), "wait frame");
    r.jobs.clear();
    for (auto& b : r.transient)
        r.c.destroy(b);
    r.transient.clear();
    vk::check(vkResetDescriptorPool(r.c.device, r.descriptors, 0), "reset frame descriptors");
    if (r.recreate || r.extent.width != uint32_t(w) || r.extent.height != uint32_t(h))
        r.createSwapchain();
    VkResult acquired = vkAcquireNextImageKHR(r.c.device, r.swapchain, UINT64_MAX, r.acquired, VK_NULL_HANDLE, &r.imageIndex);
    if (acquired == VK_ERROR_OUT_OF_DATE_KHR) {
        r.recreate = true;
        return false;
    }
    if (acquired == VK_SUBOPTIMAL_KHR)
        r.recreate = true;
    else
        vk::check(acquired, "acquire image");
    vk::check(vkResetCommandBuffer(r.cmd, 0), "reset frame command");
    r.active = true;
    return true;
}
bool Renderer::renderScene(Scene& scene, float aspect) {
    if (!impl || !impl->active || aspect <= 0)
        return false;
    impl->jobs.push_back(impl->sceneJob(scene, aspect));
    return true;
}
void Renderer::renderSingle(const std::shared_ptr<Model>& model, const glm::mat4& world, const Camera& camera, float aspect) {
    if (!impl || !impl->active || !model || aspect <= 0)
        return;
    Job job;
    job.uniform = cameraUniform(camera, aspect);
    for (auto& sm : model->subMeshes)
        job.draws.push_back({sm.mesh, sm.material, {world}});
    impl->jobs.push_back(std::move(job));
}
bool Renderer::renderObject(const Object& object, const Camera& camera, float aspect) {
    return renderObject({&object, object.getWorldMatrix()}, camera, aspect);
}
bool Renderer::renderObject(const VisibleInstance& instance, const Camera& camera, float aspect) {
    if (!impl || !impl->active || !instance.object || !instance.object->model)
        return false;
    renderSingle(instance.object->model, instance.worldMatrix, camera, aspect);
    return true;
}
void Renderer::renderInstanced(const std::shared_ptr<Model>& model, const std::vector<VisibleInstance>& instances, const Camera& camera,
                               float aspect) {
    if (!impl || !impl->active || !model || instances.empty() || aspect <= 0)
        return;
    Job job;
    job.uniform = cameraUniform(camera, aspect);
    std::vector<glm::mat4> transforms;
    for (auto& i : instances)
        transforms.push_back(i.worldMatrix);
    for (auto& sm : model->subMeshes)
        job.draws.push_back({sm.mesh, sm.material, transforms});
    impl->jobs.push_back(std::move(job));
}
void Renderer::renderSkybox(unsigned cubemap, const Camera& camera, float aspect) {
    if (!impl || !impl->active || !cubemap || aspect <= 0)
        return;
    Job job;
    job.uniform = cameraUniform(camera, aspect);
    job.environment = cubemap;
    impl->jobs.push_back(std::move(job));
}
VkDescriptorImageInfo Renderer::renderSceneToTexture(Scene* scene, int w, int h) {
    if (!impl || !impl->active || w <= 0 || h <= 0)
        return {};
    auto& r = *impl;
    if (r.preview.width != uint32_t(w) || r.preview.height != uint32_t(h)) {
        if (r.previewFB)
            vkDestroyFramebuffer(r.c.device, r.previewFB, nullptr);
        r.previewFB = {};
        r.c.destroy(r.preview);
        r.c.destroy(r.previewDepth);
        r.c.destroy(r.previewMSAA);
        r.preview = r.c.image(w, h, r.format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        r.previewDepth = r.c.image(w, h, r.depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, 1, 1, r.samples);
        std::vector<VkImageView> attachments{r.preview.view, r.previewDepth.view};
        if (r.samples != VK_SAMPLE_COUNT_1_BIT) {
            r.previewMSAA = r.c.image(w, h, r.format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 1, 1, r.samples);
            attachments = {r.previewMSAA.view, r.previewDepth.view, r.preview.view};
        }
        r.previewFB = r.framebuffer(r.offscreenPass, attachments, w, h);
    }
    Job job = scene ? r.sceneJob(*scene, float(w) / h) : Job{};
    job.offscreen = true;
    r.jobs.push_back(std::move(job));
    return {r.preview.sampler, r.preview.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
}
void Renderer::requestScreenshot(const std::string& path) {
    if (impl)
        impl->screenshot = path;
}
void Renderer::endFrame(const std::function<void(VkCommandBuffer)>& overlay) {
    if (!impl || !impl->active)
        return;
    auto& r = *impl;
    auto& c = r.c;
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vk::check(vkBeginCommandBuffer(r.cmd, &bi), "begin frame");
    bool mainDrawn = false;
    for (auto& job : r.jobs) {
        r.record(job, mainDrawn);
        if (!job.offscreen)
            mainDrawn = true;
    }
    if (overlay || !mainDrawn) {
        r.startPass(r.cmd, mainDrawn ? r.loadPass : r.mainPass, r.framebuffers[r.imageIndex], r.extent.width, r.extent.height);
        if (overlay)
            overlay(r.cmd);
        vkCmdEndRenderPass(r.cmd);
    }
    vk::Image swapImage;
    swapImage.handle = r.images[r.imageIndex];
    swapImage.width = r.extent.width;
    swapImage.height = r.extent.height;
    vk::Buffer capture{};
    if (!r.screenshot.empty()) {
        if (!r.canCapture)
            throw std::runtime_error("Surface does not support screenshot transfer");
        capture = r.temporary(VkDeviceSize(r.extent.width) * r.extent.height * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT, nullptr);
        c.barrier(r.cmd, swapImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                  VK_ACCESS_TRANSFER_READ_BIT);
        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageExtent = {r.extent.width, r.extent.height, 1};
        vkCmdCopyImageToBuffer(r.cmd, swapImage.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, capture.handle, 1, &copy);
        c.barrier(r.cmd, swapImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT,
                  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_TRANSFER_READ_BIT, 0);
    } else
        c.barrier(r.cmd, swapImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0);
    vk::check(vkEndCommandBuffer(r.cmd), "end frame");
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &r.acquired;
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &r.cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &r.finished[r.imageIndex];
    vk::check(vkResetFences(c.device, 1, &r.fence), "reset frame fence");
    vk::check(vkQueueSubmit(c.queue, 1, &submit, r.fence), "submit frame");
    r.active = false;
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &r.finished[r.imageIndex];
    present.swapchainCount = 1;
    present.pSwapchains = &r.swapchain;
    present.pImageIndices = &r.imageIndex;
    auto result = vkQueuePresentKHR(c.queue, &present);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        r.recreate = true;
    else
        vk::check(result, "present frame");
    if (capture.handle) {
        vk::check(vkWaitForFences(c.device, 1, &r.fence, VK_TRUE, UINT64_MAX), "wait capture");
        void* mapped;
        vk::check(vkMapMemory(c.device, capture.memory, 0, capture.size, 0, &mapped), "map capture");
        std::ofstream out(r.screenshot, std::ios::binary);
        out << "P6\n" << r.extent.width << " " << r.extent.height << "\n255\n";
        auto* bytes = static_cast<unsigned char*>(mapped);
        bool bgra = r.format == VK_FORMAT_B8G8R8A8_UNORM || r.format == VK_FORMAT_B8G8R8A8_SRGB;
        for (size_t i = 0; i < size_t(r.extent.width) * r.extent.height; ++i) {
            char rgb[] = {char(bytes[i * 4 + (bgra ? 2 : 0)]), char(bytes[i * 4 + 1]), char(bytes[i * 4 + (bgra ? 0 : 2)])};
            out.write(rgb, 3);
        }
        vkUnmapMemory(c.device, capture.memory);
        if (!out)
            throw std::runtime_error("Failed to write screenshot: " + r.screenshot);
        r.screenshot.clear();
    }
}
VkInstance Renderer::getInstance() const {
    return impl ? impl->c.instance : VK_NULL_HANDLE;
}
VkPhysicalDevice Renderer::getPhysicalDevice() const {
    return impl ? impl->c.physical : VK_NULL_HANDLE;
}
VkDevice Renderer::getDevice() const {
    return impl ? impl->c.device : VK_NULL_HANDLE;
}
VkQueue Renderer::getQueue() const {
    return impl ? impl->c.queue : VK_NULL_HANDLE;
}
uint32_t Renderer::getQueueFamily() const {
    return impl ? impl->c.family : 0;
}
VkRenderPass Renderer::getRenderPass() const {
    return impl ? impl->mainPass : VK_NULL_HANDLE;
}
uint32_t Renderer::getImageCount() const {
    return impl ? uint32_t(impl->images.size()) : 0;
}
VkSampleCountFlagBits Renderer::getSampleCount() const {
    return impl ? impl->samples : VK_SAMPLE_COUNT_1_BIT;
}
uint32_t Renderer::getValidationErrorCount() const {
    return vk::context().validationErrors;
}
} // namespace knot
