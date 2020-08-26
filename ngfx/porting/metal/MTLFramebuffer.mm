#include "porting/metal/MTLFramebuffer.h"
#include "porting/metal/MTLTexture.h"
#include "DebugUtil.h"
using namespace ngfx;

void MTLFramebuffer::create(uint32_t w, uint32_t h, const ColorAttachments &colorAttachments,
        MTLRenderPassDepthAttachmentDescriptor* depthAttachment,
        MTLRenderPassStencilAttachmentDescriptor* stencilAttachment) {
    this->w = w; this->h = h;
    this->colorAttachments = colorAttachments;
    this->depthAttachment = depthAttachment;
    this->stencilAttachment = stencilAttachment;
}

Framebuffer* Framebuffer::create(Device* device, RenderPass* renderPass,
        const std::vector<Attachment> &attachments, uint32_t w, uint32_t h, uint32_t layers) {
    MTLFramebuffer* mtlFramebuffer = new MTLFramebuffer();
    mtlFramebuffer->attachments = attachments;
    MTLFramebuffer::ColorAttachments colorAttachments;
    MTLRenderPassColorAttachmentDescriptor* colorAttachment = nullptr;
    MTLRenderPassDepthAttachmentDescriptor* depthAttachment = nullptr;
    MTLRenderPassStencilAttachmentDescriptor* stencilAttachment = nullptr;
    for (uint32_t j = 0; j<attachments.size(); j++) {
        auto& attachment = attachments[j];
        auto mtlTexture = mtl(attachment.texture);
        if (!mtlTexture->depthTexture && !mtlTexture->stencilTexture) {
            if (!colorAttachment) colorAttachment = [MTLRenderPassColorAttachmentDescriptor new];
            if (mtlTexture->numSamples > 1) colorAttachment.texture = mtlTexture->v;
            else if (colorAttachment.texture) colorAttachment.resolveTexture = mtlTexture->v;
            else colorAttachment.texture = mtlTexture->v;
            continue;
        }
        if (mtlTexture->depthTexture) {
            if (!depthAttachment) depthAttachment = [MTLRenderPassDepthAttachmentDescriptor new];
            if (mtlTexture->numSamples > 1) depthAttachment.texture = mtlTexture->v;
            else if (depthAttachment.texture) depthAttachment.resolveTexture = mtlTexture->v;
            else depthAttachment.texture = mtlTexture->v;
        } if (mtlTexture->stencilTexture) {
            if (!stencilAttachment) stencilAttachment = [MTLRenderPassStencilAttachmentDescriptor new];
            if (mtlTexture->numSamples > 1) stencilAttachment.texture = mtlTexture->v;
            else if (stencilAttachment.texture) stencilAttachment.resolveTexture = mtlTexture->v;
            else stencilAttachment.texture = mtlTexture->v;
        }
    }
    colorAttachments.emplace_back(std::move(colorAttachment));
    mtlFramebuffer->create(w, h, colorAttachments, depthAttachment, stencilAttachment);
    return mtlFramebuffer;
}
