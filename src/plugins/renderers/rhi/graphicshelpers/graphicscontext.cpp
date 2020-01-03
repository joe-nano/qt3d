/****************************************************************************
**
** Copyright (C) 2020 Klaralvdalens Datakonsult AB (KDAB).
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "graphicscontext_p.h"

#include <Qt3DRender/qgraphicsapifilter.h>
#include <Qt3DRender/qparameter.h>
#include <Qt3DRender/private/renderlogging_p.h>
#include <Qt3DRender/private/shader_p.h>
#include <Qt3DRender/private/material_p.h>
#include <Qt3DRender/private/buffer_p.h>
#include <Qt3DRender/private/attribute_p.h>
#include <Qt3DRender/private/rendertarget_p.h>
#include <Qt3DRender/private/nodemanagers_p.h>
#include <Qt3DRender/private/buffermanager_p.h>
#include <Qt3DRender/private/managers_p.h>
#include <Qt3DRender/private/attachmentpack_p.h>
#include <Qt3DRender/private/qbuffer_p.h>
#include <Qt3DRender/private/attachmentpack_p.h>
#include <Qt3DRender/private/qbuffer_p.h>
#include <Qt3DRender/private/renderstateset_p.h>
#include <QOpenGLShaderProgram>
#include <resourcemanagers_p.h>
#include <graphicshelperinterface_p.h>
#include <texture_p.h>
#include <rendercommand_p.h>
#include <renderer_p.h>
#include <renderbuffer_p.h>
#include <shader_p.h>

#if !defined(QT_OPENGL_ES_2)
#include <QOpenGLFunctions_2_0>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLFunctions_4_3_Core>
#endif
#include <QShaderBaker>

#include <QSurface>
#include <QWindow>
#include <QOpenGLTexture>
#include <QOpenGLDebugLogger>

QT_BEGIN_NAMESPACE

#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER 0x8CA8
#endif

#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#endif

#ifndef GL_MAX_IMAGE_UNITS
#define GL_MAX_IMAGE_UNITS                0x8F38
#endif

namespace {

QOpenGLShader::ShaderType shaderType(Qt3DRender::QShaderProgram::ShaderType type)
{
    switch (type) {
    case Qt3DRender::QShaderProgram::Vertex: return QOpenGLShader::Vertex;
    case Qt3DRender::QShaderProgram::TessellationControl: return QOpenGLShader::TessellationControl;
    case Qt3DRender::QShaderProgram::TessellationEvaluation: return QOpenGLShader::TessellationEvaluation;
    case Qt3DRender::QShaderProgram::Geometry: return QOpenGLShader::Geometry;
    case Qt3DRender::QShaderProgram::Fragment: return QOpenGLShader::Fragment;
    case Qt3DRender::QShaderProgram::Compute: return QOpenGLShader::Compute;
    default: Q_UNREACHABLE();
    }
}

} // anonymous namespace

namespace Qt3DRender {
namespace Render {
namespace Rhi {

namespace {

void logOpenGLDebugMessage(const QOpenGLDebugMessage &debugMessage)
{
    qDebug() << "OpenGL debug message:" << debugMessage;
}

} // anonymous

GraphicsContext::GraphicsContext()
    : m_initialized(false)
    , m_maxTextureUnits(0)
    , m_maxImageUnits(0)
    , m_defaultFBO(0)
//*    , m_gl(nullptr)
//*    , m_glHelper(nullptr)
    , m_debugLogger(nullptr)
    , m_currentVAO(nullptr)
{
    m_contextInfo.m_api = QGraphicsApiFilter::RHI;
}

GraphicsContext::~GraphicsContext()
{
}

void GraphicsContext::initialize()
{
    m_initialized = true;

//*    Q_ASSERT(m_gl);
//*
//*    m_gl->functions()->glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_maxTextureUnits);
//*    qCDebug(Backend) << "context supports" << m_maxTextureUnits << "texture units";
//*    m_gl->functions()->glGetIntegerv(GL_MAX_IMAGE_UNITS, &m_maxImageUnits);
//*    qCDebug(Backend) << "context supports" << m_maxImageUnits << "image units";
//*
//*    if (m_gl->format().majorVersion() >= 3) {
//*        m_supportsVAO = true;
//*    } else {
//*        QSet<QByteArray> extensions = m_gl->extensions();
//*        m_supportsVAO = extensions.contains(QByteArrayLiteral("GL_OES_vertex_array_object"))
//*                || extensions.contains(QByteArrayLiteral("GL_ARB_vertex_array_object"))
//*                || extensions.contains(QByteArrayLiteral("GL_APPLE_vertex_array_object"));
//*    }
//*
//*    m_defaultFBO = m_gl->defaultFramebufferObject();
//*    qCDebug(Backend) << "VAO support = " << m_supportsVAO;

}

void GraphicsContext::clearBackBuffer(QClearBuffers::BufferTypeFlags buffers)
{
    if (buffers != QClearBuffers::None) {
        GLbitfield mask = 0;

        if (buffers & QClearBuffers::ColorBuffer)
            mask |= GL_COLOR_BUFFER_BIT;
        if (buffers & QClearBuffers::DepthBuffer)
            mask |= GL_DEPTH_BUFFER_BIT;
        if (buffers & QClearBuffers::StencilBuffer)
            mask |= GL_STENCIL_BUFFER_BIT;

        RHI_UNIMPLEMENTED;
//*        m_gl->functions()->glClear(mask);
    }
}

bool GraphicsContext::isInitialized() const
{
    return m_initialized;
}

bool GraphicsContext::makeCurrent(QSurface *surface)
{
    RHI_UNIMPLEMENTED;
//*    Q_ASSERT(m_gl);
//*    if (!m_gl->makeCurrent(surface)) {
//*        qCWarning(Backend) << Q_FUNC_INFO << "makeCurrent failed";
//*        return false;
//*    }
//*
//*    initializeHelpers(surface);

    return true;
}

void GraphicsContext::initializeHelpers(QSurface *surface)
{
    RHI_UNIMPLEMENTED;
    // Set the correct GL Helper depending on the surface
    // If no helper exists, create one
//*    m_glHelper = m_glHelpers.value(surface);
//*    if (!m_glHelper) {
//*        m_glHelper = resolveHighestOpenGLFunctions();
//*        m_glHelpers.insert(surface, m_glHelper);
//*    }
}

void GraphicsContext::doneCurrent()
{
    RHI_UNIMPLEMENTED;
    //* Q_ASSERT(m_gl);
    //* m_gl->doneCurrent();
    //* m_glHelper = nullptr;
}

// Called by GL Command Thread

static constexpr QShader::Stage rhiShaderStage(QShaderProgram::ShaderType type) noexcept
{
    switch(type)
    {
      case QShaderProgram::Vertex: return QShader::VertexStage;
      case QShaderProgram::Fragment: return QShader::FragmentStage;
      case QShaderProgram::TessellationControl: return QShader::TessellationControlStage;
      case QShaderProgram::TessellationEvaluation: return QShader::TessellationEvaluationStage;
      case QShaderProgram::Geometry: return QShader::GeometryStage;
      case QShaderProgram::Compute: return QShader::ComputeStage;
      default: std::abort();
    }
}
GraphicsContext::ShaderCreationInfo GraphicsContext::createShaderProgram(RHIShader *shader)
{
    // Compile shaders
    const auto& shaderCode = shader->shaderCode();
    QShaderBaker b;
    b.setGeneratedShaders({
                              {QShader::SpirvShader, 100},
                              {QShader::GlslShader, 120}, // Only GLSL version supported by RHI right now.
                              {QShader::HlslShader, 100},
                              {QShader::MslShader, 100},
                          });
    b.setGeneratedShaderVariants({QShader::Variant{},
                                  QShader::Variant{},
                                  QShader::Variant{},
                                  QShader::Variant{}});

    // TODO handle caching as QShader does not have a built-in mechanism for that
    QString logs;
    bool success = true;
    for (int i = QShaderProgram::Vertex; i <= QShaderProgram::Compute; ++i) {
        const QShaderProgram::ShaderType type = static_cast<QShaderProgram::ShaderType>(i);
        if (!shaderCode.at(i).isEmpty()) {
            // Note: logs only return the error but not all the shader code
            // we could append it

            const auto rhiStage = rhiShaderStage(type);
            b.setSourceString(shaderCode.at(i), rhiStage);
            auto bakedShader = b.bake();
            if(b.errorMessage() != QString{})
            {
                qDebug() << "Vertex Shader Error: " << b.errorMessage();
                logs += b.errorMessage();
                success = false;
            }
            shader->m_stages[rhiStage] = std::move(bakedShader);
        }
    }

    // Perform shader introspection
    introspectShaderInterface(shader);

    return {success, logs};
}

// That assumes that the shaderProgram in Shader stays the same

void GraphicsContext::introspectShaderInterface(RHIShader *shader)
{
    shader->introspect();
    RHI_UNIMPLEMENTED;
//*    QOpenGLShaderProgram *shaderProgram = shader->shaderProgram();
//*    GraphicsHelperInterface *glHelper = resolveHighestOpenGLFunctions();
//*    shader->initializeUniforms(glHelper->programUniformsAndLocations(shaderProgram->programId()));
//*    shader->initializeAttributes(glHelper->programAttributesAndLocations(shaderProgram->programId()));
//*     if (m_glHelper->supportsFeature(GraphicsHelperInterface::UniformBufferObject))
//*         shader->initializeUniformBlocks(m_glHelper->programUniformBlocks(shaderProgram->programId()));
//*     if (m_glHelper->supportsFeature(GraphicsHelperInterface::ShaderStorageObject))
//*         shader->initializeShaderStorageBlocks(m_glHelper->programShaderStorageBlocks(shaderProgram->programId()));
}


// Called by Renderer::updateGLResources
void GraphicsContext::loadShader(Shader *shaderNode,
                                 ShaderManager *shaderManager,
                                 RHIShaderManager *rhiShaderManager)
{
    RHI_UNIMPLEMENTED;
    const Qt3DCore::QNodeId shaderId = shaderNode->peerId();
    RHIShader *glShader = rhiShaderManager->lookupResource(shaderId);

    // We already have a shader associated with the node
    if (glShader != nullptr) {
        // We need to abandon it
        rhiShaderManager->abandon(glShader, shaderNode);
    }

    // We create or adopt an already created glShader
    glShader = rhiShaderManager->createOrAdoptExisting(shaderNode);

    const QVector<Qt3DCore::QNodeId> sharedShaderIds = rhiShaderManager->shaderIdsForProgram(glShader);
    if (sharedShaderIds.size() == 1) {
        // Shader in the cache hasn't been loaded yet
        glShader->setGraphicsContext(this);
        glShader->setShaderCode(shaderNode->shaderCode());
        const ShaderCreationInfo loadResult = createShaderProgram(glShader);
        shaderNode->setStatus(loadResult.linkSucceeded ? QShaderProgram::Ready : QShaderProgram::Error);
        shaderNode->setLog(loadResult.logs);
        // Loaded in the sense we tried to load it (and maybe it failed)
        glShader->setLoaded(true);
    } else {
        // Find an already loaded shader that shares the same QOpenGLShaderProgram
        for (const Qt3DCore::QNodeId sharedShaderId : sharedShaderIds) {
            if (sharedShaderId != shaderNode->peerId()) {
                Shader *refShader = shaderManager->lookupResource(sharedShaderId);
                // We only introspect once per actual OpenGL shader program
                // rather than once per ShaderNode.
                shaderNode->initializeFromReference(*refShader);
                break;
            }
        }
    }
    shaderNode->unsetDirty();
    // Ensure we will rebuilt material caches
    shaderNode->requestCacheRebuild();
}

void GraphicsContext::activateDrawBuffers(const AttachmentPack &attachments)
{
    RHI_UNIMPLEMENTED;
//*    const QVector<int> activeDrawBuffers = attachments.getGlDrawBuffers();
//*
//*    if (m_glHelper->checkFrameBufferComplete()) {
//*        if (activeDrawBuffers.size() > 1) {// We need MRT
//*            if (m_glHelper->supportsFeature(GraphicsHelperInterface::MRT)) {
//*                // Set up MRT, glDrawBuffers...
//*                m_glHelper->drawBuffers(activeDrawBuffers.size(), activeDrawBuffers.data());
//*            }
//*        }
//*    } else {
//*        qWarning() << "FBO incomplete";
//*    }
}

void GraphicsContext::rasterMode(GLenum faceMode, GLenum rasterMode)
{
    RHI_UNIMPLEMENTED;
//* m_glHelper->rasterMode(faceMode, rasterMode);
}


const GraphicsApiFilterData *GraphicsContext::contextInfo() const
{
    return &m_contextInfo;
}

bool GraphicsContext::supportsDrawBuffersBlend() const
{
    RHI_UNIMPLEMENTED;
    return false;
//*    return m_glHelper->supportsFeature(GraphicsHelperInterface::DrawBuffersBlend);
}

/*!
 * \internal
 * Wraps an OpenGL call to glDrawElementsInstanced.
 * If the call is not supported by the system's OpenGL version,
 * it is simulated with a loop.
 */
void GraphicsContext::drawElementsInstancedBaseVertexBaseInstance(GLenum primitiveType,
                                                                  GLsizei primitiveCount,
                                                                  GLint indexType,
                                                                  void *indices,
                                                                  GLsizei instances,
                                                                  GLint baseVertex,
                                                                  GLint baseInstance)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->drawElementsInstancedBaseVertexBaseInstance(primitiveType,
//*                                                            primitiveCount,
//*                                                            indexType,
//*                                                            indices,
//*                                                            instances,
//*                                                            baseVertex,
//*                                                            baseInstance);
}

/*!
 * \internal
 * Wraps an OpenGL call to glDrawArraysInstanced.
 */
void GraphicsContext::drawArraysInstanced(GLenum primitiveType,
                                          GLint first,
                                          GLsizei count,
                                          GLsizei instances)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->drawArraysInstanced(primitiveType,
//*                                    first,
//*                                    count,
//*                                    instances);
}

void GraphicsContext::drawArraysInstancedBaseInstance(GLenum primitiveType, GLint first, GLsizei count, GLsizei instances, GLsizei baseinstance)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->drawArraysInstancedBaseInstance(primitiveType,
//*                                                first,
//*                                                count,
//*                                                instances,
//*                                                baseinstance);
}

/*!
 * \internal
 * Wraps an OpenGL call to glDrawElements.
 */
void GraphicsContext::drawElements(GLenum primitiveType,
                                   GLsizei primitiveCount,
                                   GLint indexType,
                                   void *indices,
                                   GLint baseVertex)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->drawElements(primitiveType,
//*                             primitiveCount,
//*                             indexType,
//*                             indices,
//*                             baseVertex);
}

void GraphicsContext::drawElementsIndirect(GLenum mode,
                                           GLenum type,
                                           void *indirect)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->drawElementsIndirect(mode, type, indirect);
}

/*!
 * \internal
 * Wraps an OpenGL call to glDrawArrays.
 */
void GraphicsContext::drawArrays(GLenum primitiveType,
                                 GLint first,
                                 GLsizei count)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->drawArrays(primitiveType,
//*                           first,
//*                           count);
}

void GraphicsContext::drawArraysIndirect(GLenum mode, void *indirect)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->drawArraysIndirect(mode, indirect);
}

void GraphicsContext::setVerticesPerPatch(GLint verticesPerPatch)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->setVerticesPerPatch(verticesPerPatch);
}

void GraphicsContext::blendEquation(GLenum mode)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->blendEquation(mode);
}

void GraphicsContext::blendFunci(GLuint buf, GLenum sfactor, GLenum dfactor)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->blendFunci(buf, sfactor, dfactor);
}

void GraphicsContext::blendFuncSeparatei(GLuint buf, GLenum sRGB, GLenum dRGB, GLenum sAlpha, GLenum dAlpha)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->blendFuncSeparatei(buf, sRGB, dRGB, sAlpha, dAlpha);
}

void GraphicsContext::alphaTest(GLenum mode1, GLenum mode2)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->alphaTest(mode1, mode2);
}

void GraphicsContext::bindFramebuffer(GLuint fbo, GraphicsHelperInterface::FBOBindMode mode)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->bindFrameBufferObject(fbo, mode);
}

void GraphicsContext::depthRange(GLdouble nearValue, GLdouble farValue)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->depthRange(nearValue, farValue);
}

void GraphicsContext::depthTest(GLenum mode)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->depthTest(mode);
}

void GraphicsContext::depthMask(GLenum mode)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->depthMask(mode);
}

void GraphicsContext::frontFace(GLenum mode)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->frontFace(mode);
}

void GraphicsContext::bindFragOutputs(GLuint shader, const QHash<QString, int> &outputs)
{
    RHI_UNIMPLEMENTED;
//*    if (m_glHelper->supportsFeature(GraphicsHelperInterface::MRT) &&
//*            m_glHelper->supportsFeature(GraphicsHelperInterface::BindableFragmentOutputs))
//*        m_glHelper->bindFragDataLocation(shader, outputs);
}

void GraphicsContext::bindImageTexture(GLuint imageUnit, GLuint texture,
                                       GLint mipLevel, GLboolean layered,
                                       GLint layer, GLenum access, GLenum format)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->bindImageTexture(imageUnit,
//*                                 texture,
//*                                 mipLevel,
//*                                 layered,
//*                                 layer,
//*                                 access,
//*                                 format);
}

void GraphicsContext::bindUniformBlock(GLuint programId, GLuint uniformBlockIndex, GLuint uniformBlockBinding)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->bindUniformBlock(programId, uniformBlockIndex, uniformBlockBinding);
}

void GraphicsContext::bindShaderStorageBlock(GLuint programId, GLuint shaderStorageBlockIndex, GLuint shaderStorageBlockBinding)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->bindShaderStorageBlock(programId, shaderStorageBlockIndex, shaderStorageBlockBinding);
}

void GraphicsContext::bindBufferBase(GLenum target, GLuint bindingIndex, GLuint buffer)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->bindBufferBase(target, bindingIndex, buffer);
}

void GraphicsContext::buildUniformBuffer(const QVariant &v, const ShaderUniform &description, QByteArray &buffer)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->buildUniformBuffer(v, description, buffer);
}

void GraphicsContext::setMSAAEnabled(bool enabled)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->setMSAAEnabled(enabled);
}

void GraphicsContext::setAlphaCoverageEnabled(bool enabled)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->setAlphaCoverageEnabled(enabled);
}

void GraphicsContext::clearBufferf(GLint drawbuffer, const QVector4D &values)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->clearBufferf(drawbuffer, values);
}

GLuint GraphicsContext::boundFrameBufferObject()
{
    RHI_UNIMPLEMENTED;
    return false;
//*    return m_glHelper->boundFrameBufferObject();
}

void GraphicsContext::clearColor(const QColor &color)
{
    RHI_UNIMPLEMENTED;
//*    m_gl->functions()->glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

void GraphicsContext::clearDepthValue(float depth)
{
    RHI_UNIMPLEMENTED;
//*    m_gl->functions()->glClearDepthf(depth);
}

void GraphicsContext::clearStencilValue(int stencil)
{
    RHI_UNIMPLEMENTED;
//*    m_gl->functions()->glClearStencil(stencil);
}

void GraphicsContext::enableClipPlane(int clipPlane)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->enableClipPlane(clipPlane);
}

void GraphicsContext::disableClipPlane(int clipPlane)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->disableClipPlane(clipPlane);
}

void GraphicsContext::setClipPlane(int clipPlane, const QVector3D &normal, float distance)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->setClipPlane(clipPlane, normal, distance);
}

GLint GraphicsContext::maxClipPlaneCount()
{
    RHI_UNIMPLEMENTED;
//*    return m_glHelper->maxClipPlaneCount();
}

GLint GraphicsContext::maxTextureUnitsCount() const
{
    RHI_UNIMPLEMENTED;
//*    return m_maxTextureUnits;
}

GLint GraphicsContext::maxImageUnitsCount() const
{
    RHI_UNIMPLEMENTED;
//*    return m_maxImageUnits;
}


void GraphicsContext::enablePrimitiveRestart(int restartIndex)
{
    RHI_UNIMPLEMENTED;
//*    if (m_glHelper->supportsFeature(GraphicsHelperInterface::PrimitiveRestart))
//*        m_glHelper->enablePrimitiveRestart(restartIndex);
}

void GraphicsContext::disablePrimitiveRestart()
{
    RHI_UNIMPLEMENTED;
//*    if (m_glHelper->supportsFeature(GraphicsHelperInterface::PrimitiveRestart))
//*        m_glHelper->disablePrimitiveRestart();
}

void GraphicsContext::pointSize(bool programmable, GLfloat value)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->pointSize(programmable, value);
}

void GraphicsContext::dispatchCompute(int x, int y, int z)
{
    RHI_UNIMPLEMENTED;
//*    if (m_glHelper->supportsFeature(GraphicsHelperInterface::Compute))
//*        m_glHelper->dispatchCompute(x, y, z);
}

GLboolean GraphicsContext::unmapBuffer(GLenum target)
{
    RHI_UNIMPLEMENTED;
    return true;
//*    return m_glHelper->unmapBuffer(target);
}

char *GraphicsContext::mapBuffer(GLenum target, GLsizeiptr size)
{
    RHI_UNIMPLEMENTED;
    return nullptr;
//*    return m_glHelper->mapBuffer(target, size);
}

void GraphicsContext::enablei(GLenum cap, GLuint index)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->enablei(cap, index);
}

void GraphicsContext::disablei(GLenum cap, GLuint index)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->disablei(cap, index);
}

void GraphicsContext::setSeamlessCubemap(bool enable)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->setSeamlessCubemap(enable);
}

void GraphicsContext::readBuffer(GLenum mode)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->readBuffer(mode);
}

void GraphicsContext::drawBuffer(GLenum mode)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->drawBuffer(mode);
}

void GraphicsContext::drawBuffers(GLsizei n, const int *bufs)
{
    RHI_UNIMPLEMENTED;
//*    m_glHelper->drawBuffers(n, bufs);
}

void GraphicsContext::applyUniform(const ShaderUniform &description, const UniformValue &v)
{
    RHI_UNIMPLEMENTED;
    //* const UniformType type = m_glHelper->uniformTypeFromGLType(description.m_type);
    //*
    //* switch (type) {
    //* case UniformType::Float:
    //*     // See QTBUG-57510 and uniform_p.h
    //*     if (v.storedType() == Int) {
    //*         float value = float(*v.constData<int>());
    //*         UniformValue floatV(value);
    //*         applyUniformHelper<UniformType::Float>(description, floatV);
    //*     } else {
    //*         applyUniformHelper<UniformType::Float>(description, v);
    //*     }
    //*     break;
    //* case UniformType::Vec2:
    //*     applyUniformHelper<UniformType::Vec2>(description, v);
    //*     break;
    //* case UniformType::Vec3:
    //*     applyUniformHelper<UniformType::Vec3>(description, v);
    //*     break;
    //* case UniformType::Vec4:
    //*     applyUniformHelper<UniformType::Vec4>(description, v);
    //*     break;
    //*
    //* case UniformType::Double:
    //*     applyUniformHelper<UniformType::Double>(description, v);
    //*     break;
    //* case UniformType::DVec2:
    //*     applyUniformHelper<UniformType::DVec2>(description, v);
    //*     break;
    //* case UniformType::DVec3:
    //*     applyUniformHelper<UniformType::DVec3>(description, v);
    //*     break;
    //* case UniformType::DVec4:
    //*     applyUniformHelper<UniformType::DVec4>(description, v);
    //*     break;
    //*
    //* case UniformType::Sampler:
    //* case UniformType::Image:
    //* case UniformType::Int:
    //*     applyUniformHelper<UniformType::Int>(description, v);
    //*     break;
    //* case UniformType::IVec2:
    //*     applyUniformHelper<UniformType::IVec2>(description, v);
    //*     break;
    //* case UniformType::IVec3:
    //*     applyUniformHelper<UniformType::IVec3>(description, v);
    //*     break;
    //* case UniformType::IVec4:
    //*     applyUniformHelper<UniformType::IVec4>(description, v);
    //*     break;
    //*
    //* case UniformType::UInt:
    //*     applyUniformHelper<UniformType::UInt>(description, v);
    //*     break;
    //* case UniformType::UIVec2:
    //*     applyUniformHelper<UniformType::UIVec2>(description, v);
    //*     break;
    //* case UniformType::UIVec3:
    //*     applyUniformHelper<UniformType::UIVec3>(description, v);
    //*     break;
    //* case UniformType::UIVec4:
    //*     applyUniformHelper<UniformType::UIVec4>(description, v);
    //*     break;
    //*
    //* case UniformType::Bool:
    //*     applyUniformHelper<UniformType::Bool>(description, v);
    //*     break;
    //* case UniformType::BVec2:
    //*     applyUniformHelper<UniformType::BVec2>(description, v);
    //*     break;
    //* case UniformType::BVec3:
    //*     applyUniformHelper<UniformType::BVec3>(description, v);
    //*     break;
    //* case UniformType::BVec4:
    //*     applyUniformHelper<UniformType::BVec4>(description, v);
    //*     break;
    //*
    //* case UniformType::Mat2:
    //*     applyUniformHelper<UniformType::Mat2>(description, v);
    //*     break;
    //* case UniformType::Mat3:
    //*     applyUniformHelper<UniformType::Mat3>(description, v);
    //*     break;
    //* case UniformType::Mat4:
    //*     applyUniformHelper<UniformType::Mat4>(description, v);
    //*     break;
    //* case UniformType::Mat2x3:
    //*     applyUniformHelper<UniformType::Mat2x3>(description, v);
    //*     break;
    //* case UniformType::Mat3x2:
    //*     applyUniformHelper<UniformType::Mat3x2>(description, v);
    //*     break;
    //* case UniformType::Mat2x4:
    //*     applyUniformHelper<UniformType::Mat2x4>(description, v);
    //*     break;
    //* case UniformType::Mat4x2:
    //*     applyUniformHelper<UniformType::Mat4x2>(description, v);
    //*     break;
    //* case UniformType::Mat3x4:
    //*     applyUniformHelper<UniformType::Mat3x4>(description, v);
    //*     break;
    //* case UniformType::Mat4x3:
    //*     applyUniformHelper<UniformType::Mat4x3>(description, v);
    //*     break;
    //*
    //* default:
    //*     break;
    //* }
}

void GraphicsContext::memoryBarrier(QMemoryBarrier::Operations barriers)
{
    RHI_UNIMPLEMENTED;
    //* m_glHelper->memoryBarrier(barriers);
}

GLint GraphicsContext::elementType(GLint type)
{
    switch (type) {
    case GL_FLOAT:
    case GL_FLOAT_VEC2:
    case GL_FLOAT_VEC3:
    case GL_FLOAT_VEC4:
        return GL_FLOAT;

#ifndef QT_OPENGL_ES_2 // Otherwise compile error as Qt defines GL_DOUBLE as GL_FLOAT when using ES2
    case GL_DOUBLE:
#ifdef GL_DOUBLE_VEC3 // For compiling on pre GL 4.1 systems
    case GL_DOUBLE_VEC2:
    case GL_DOUBLE_VEC3:
    case GL_DOUBLE_VEC4:
#endif
        return GL_DOUBLE;
#endif
    default:
        qWarning() << Q_FUNC_INFO << "unsupported:" << QString::number(type, 16);
    }

    return GL_INVALID_VALUE;
}

GLint GraphicsContext::tupleSizeFromType(GLint type)
{
    switch (type) {
    case GL_FLOAT:
#ifndef QT_OPENGL_ES_2 // Otherwise compile error as Qt defines GL_DOUBLE as GL_FLOAT when using ES2
    case GL_DOUBLE:
#endif
    case GL_UNSIGNED_BYTE:
    case GL_UNSIGNED_INT:
        break; // fall through

    case GL_FLOAT_VEC2:
#ifdef GL_DOUBLE_VEC2 // For compiling on pre GL 4.1 systems.
    case GL_DOUBLE_VEC2:
#endif
        return 2;

    case GL_FLOAT_VEC3:
#ifdef GL_DOUBLE_VEC3 // For compiling on pre GL 4.1 systems.
    case GL_DOUBLE_VEC3:
#endif
        return 3;

    case GL_FLOAT_VEC4:
#ifdef GL_DOUBLE_VEC4 // For compiling on pre GL 4.1 systems.
    case GL_DOUBLE_VEC4:
#endif
        return 4;

    default:
        qWarning() << Q_FUNC_INFO << "unsupported:" << QString::number(type, 16);
    }

    return 1;
}

GLuint GraphicsContext::byteSizeFromType(GLint type)
{
    switch (type) {
    case GL_FLOAT:          return sizeof(float);
#ifndef QT_OPENGL_ES_2 // Otherwise compile error as Qt defines GL_DOUBLE as GL_FLOAT when using ES2
    case GL_DOUBLE:         return sizeof(double);
#endif
    case GL_UNSIGNED_BYTE:  return sizeof(unsigned char);
    case GL_UNSIGNED_INT:   return sizeof(GLuint);

    case GL_FLOAT_VEC2:     return sizeof(float) * 2;
    case GL_FLOAT_VEC3:     return sizeof(float) * 3;
    case GL_FLOAT_VEC4:     return sizeof(float) * 4;
#ifdef GL_DOUBLE_VEC3 // Required to compile on pre GL 4.1 systems
    case GL_DOUBLE_VEC2:    return sizeof(double) * 2;
    case GL_DOUBLE_VEC3:    return sizeof(double) * 3;
    case GL_DOUBLE_VEC4:    return sizeof(double) * 4;
#endif
    default:
        qWarning() << Q_FUNC_INFO << "unsupported:" << QString::number(type, 16);
    }

    return 0;
}

GLint GraphicsContext::glDataTypeFromAttributeDataType(QAttribute::VertexBaseType dataType)
{
    switch (dataType) {
    case QAttribute::Byte:
        return GL_BYTE;
    case QAttribute::UnsignedByte:
        return GL_UNSIGNED_BYTE;
    case QAttribute::Short:
        return GL_SHORT;
    case QAttribute::UnsignedShort:
        return GL_UNSIGNED_SHORT;
    case QAttribute::Int:
        return GL_INT;
    case QAttribute::UnsignedInt:
        return GL_UNSIGNED_INT;
    case QAttribute::HalfFloat:
#ifdef GL_HALF_FLOAT
        return GL_HALF_FLOAT;
#endif
#ifndef QT_OPENGL_ES_2 // Otherwise compile error as Qt defines GL_DOUBLE as GL_FLOAT when using ES2
    case QAttribute::Double:
        return GL_DOUBLE;
#endif
    case QAttribute::Float:
        break;
    default:
        qWarning() << Q_FUNC_INFO << "unsupported dataType:" << dataType;
    }
    return GL_FLOAT;
}

QT3D_UNIFORM_TYPE_IMPL(UniformType::Float, float, glUniform1fv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::Vec2, float, glUniform2fv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::Vec3, float, glUniform3fv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::Vec4, float, glUniform4fv)

// OpenGL expects int* as values for booleans
QT3D_UNIFORM_TYPE_IMPL(UniformType::Bool, int, glUniform1iv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::BVec2, int, glUniform2iv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::BVec3, int, glUniform3iv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::BVec4, int, glUniform4iv)

QT3D_UNIFORM_TYPE_IMPL(UniformType::Int, int, glUniform1iv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::IVec2, int, glUniform2iv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::IVec3, int, glUniform3iv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::IVec4, int, glUniform4iv)

QT3D_UNIFORM_TYPE_IMPL(UniformType::UInt, uint, glUniform1uiv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::UIVec2, uint, glUniform2uiv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::UIVec3, uint, glUniform3uiv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::UIVec4, uint, glUniform4uiv)

QT3D_UNIFORM_TYPE_IMPL(UniformType::Mat2, float, glUniformMatrix2fv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::Mat3, float, glUniformMatrix3fv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::Mat4, float, glUniformMatrix4fv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::Mat2x3, float, glUniformMatrix2x3fv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::Mat3x2, float, glUniformMatrix3x2fv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::Mat2x4, float, glUniformMatrix2x4fv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::Mat4x2, float, glUniformMatrix4x2fv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::Mat3x4, float, glUniformMatrix3x4fv)
QT3D_UNIFORM_TYPE_IMPL(UniformType::Mat4x3, float, glUniformMatrix4x3fv)

} // namespace Rhi
} // namespace Render
} // namespace Qt3DRender of namespace

QT_END_NAMESPACE
