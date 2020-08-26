#include "porting/metal/MTLRenderPass.h"
#include "porting/metal/MTLGraphicsContext.h"
using namespace ngfx;

MTLRenderPassDescriptor* MTLRenderPass::getDescriptor(MTLGraphicsContext* mtlCtx, MTLFramebuffer* mtlFramebuffer,
       glm::vec4 clearColor, float clearDepth, uint32_t clearStencil) {
    MTLRenderPassDescriptor* mtlRenderPassDescriptor;
    MTLRenderPassColorAttachmentDescriptor *colorAttachment = nullptr;
    if (mtlFramebuffer->colorAttachments.empty()) {
        auto view = mtlCtx->mtkView;
        mtlRenderPassDescriptor = view.currentRenderPassDescriptor;
        colorAttachment = mtlRenderPassDescriptor.colorAttachments[0];
    } else {
        mtlRenderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        colorAttachment = mtlRenderPassDescriptor.colorAttachments[0];
        colorAttachment.texture = mtlFramebuffer->colorAttachments[0].texture;
        colorAttachment.resolveTexture = mtlFramebuffer->colorAttachments[0].resolveTexture;
    }
    colorAttachment.clearColor = { clearColor[0], clearColor[1], clearColor[2], clearColor[3] };
    colorAttachment.loadAction = MTLLoadActionClear;
    if (colorAttachment.resolveTexture)
        colorAttachment.storeAction = MTLStoreActionStoreAndMultisampleResolve;
    else colorAttachment.storeAction = MTLStoreActionStore;

    auto depthAttachment = mtlRenderPassDescriptor.depthAttachment;
    if (mtlFramebuffer->depthAttachment) {
        depthAttachment.clearDepth = clearDepth;
        depthAttachment.loadAction = MTLLoadActionClear;
        depthAttachment.resolveTexture = mtlFramebuffer->depthAttachment.resolveTexture;
        depthAttachment.texture = mtlFramebuffer->depthAttachment.texture;
        if (depthAttachment.resolveTexture) depthAttachment.storeAction = MTLStoreActionMultisampleResolve;
        else depthAttachment.storeAction = MTLStoreActionDontCare;
    }
    auto stencilAttachment = mtlRenderPassDescriptor.stencilAttachment;
    if (mtlFramebuffer->stencilAttachment) {
        stencilAttachment.texture = mtlFramebuffer->stencilAttachment.texture;
    }
    if (mtlRenderPassDescriptor.stencilAttachment) mtlRenderPassDescriptor.stencilAttachment.clearStencil = clearStencil;
    return mtlRenderPassDescriptor;
}
