#include <knot/utility/cubemap_baker.h>
#include <knot/resources.h>
#include <algorithm>

namespace knot {
namespace {
unsigned int bakeCubemap(unsigned int source, int size, const char* shaderName, const char* textureName, bool prefilter) {
    if (!sg_isvalid() || sg_query_view_state({source}) != SG_RESOURCESTATE_VALID || size <= 0 || sg_query_pass_state() != SG_PASSSTATE_NONE)
        return 0;
    const auto path = getAssetRoot() + "shaders/" + shaderName;
    Shader shader(std::make_shared<ShaderSource>(path + ".vert", path + ".frag"), 0);
    if (!shader.isValid())
        return 0;
    auto cube = createCube();
    if (!cube->isReady())
        return 0;

    sg_image_desc image{};
    image.type = SG_IMAGETYPE_CUBE;
    image.width = image.height = size;
    image.num_mipmaps = prefilter ? std::min(5, 1 + static_cast<int>(std::log2(size))) : 1;
    image.pixel_format = SG_PIXELFORMAT_RGBA16F;
    image.usage.color_attachment = true;
    image.sample_count = 1;
    const auto texture = createTextureView(sg_make_image(image));
    if (!texture)
        return 0;
    const auto target = sg_query_view_image({texture});
    const glm::vec3 directions[] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    const glm::vec3 up[] = {{0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}};
    shader.setTexture(textureName, source);
    shader.set("projection", glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f));
    for (int mip = 0; mip < image.num_mipmaps; ++mip) {
        shader.set("roughness", image.num_mipmaps > 1 ? static_cast<float>(mip) / (image.num_mipmaps - 1) : 0.0f);
        for (int face = 0; face < 6; ++face) {
            sg_view_desc view{};
            view.color_attachment = {target, mip, face};
            const auto attachment = sg_make_view(view);
            if (sg_query_view_state(attachment) != SG_RESOURCESTATE_VALID) {
                sg_destroy_view(attachment);
                destroyTexture(texture);
                return 0;
            }
            sg_pass pass{};
            pass.attachments.colors[0] = attachment;
            sg_begin_pass(pass);
            shader.set("view", glm::lookAt(glm::vec3(0), directions[face], up[face]));
            shader.draw(*cube, ShaderPass::Cubemap);
            sg_end_pass();
            sg_destroy_view(attachment);
        }
    }
    return texture;
}
} // namespace

unsigned int bakeHDRMapToCubemap(unsigned int source, int size) {
    return bakeCubemap(source, size, "cubemap_bake", "equirectangularMap", false);
}
unsigned int bakeCubemapToIrradianceMap(unsigned int source, int size) {
    return bakeCubemap(source, size, "irradiance_convolution", "environmentMap", false);
}
unsigned int bakeCubemapToPrefilterMap(unsigned int source, int size) {
    return bakeCubemap(source, size, "prefilter", "environmentMap", true);
}
} // namespace knot
