// Copyright (C) 2016 Elviss Strazdins
// This file is part of the Ouzel engine.

#include <sstream>
#include <iterator>

#include "RendererOGL.h"
#include "TextureOGL.h"
#include "RenderTargetOGL.h"
#include "ShaderOGL.h"
#include "MeshBufferOGL.h"
#include "IndexBufferOGL.h"
#include "VertexBufferOGL.h"
#include "BlendStateOGL.h"
#include "core/Engine.h"
#include "core/Window.h"
#include "core/Cache.h"
#include "utils/Log.h"
#include "stb_image_write.h"

#if OUZEL_SUPPORTS_OPENGL
#include "ColorPSGL2.h"
#include "ColorVSGL2.h"
#include "TexturePSGL2.h"
#include "TextureVSGL2.h"
#if OUZEL_SUPPORTS_OPENGL3
#include "ColorPSGL3.h"
#include "ColorVSGL3.h"
#include "TexturePSGL3.h"
#include "TextureVSGL3.h"
#endif
#endif

#if OUZEL_SUPPORTS_OPENGLES
#include "ColorPSGLES2.h"
#include "ColorVSGLES2.h"
#include "TexturePSGLES2.h"
#include "TextureVSGLES2.h"
#if OUZEL_SUPPORTS_OPENGLES3
#include "ColorPSGLES3.h"
#include "ColorVSGLES3.h"
#include "TexturePSGLES3.h"
#include "TextureVSGLES3.h"
#endif
#endif

#if OUZEL_OPENGL_INTERFACE_EGL
#ifdef GL_OES_vertex_array_object
    PFNGLGENVERTEXARRAYSOESPROC genVertexArraysOES;
    PFNGLBINDVERTEXARRAYOESPROC bindVertexArrayOES;
    PFNGLDELETEVERTEXARRAYSOESPROC deleteVertexArraysOES;
#endif

#ifdef GL_OES_mapbuffer
    PFNGLMAPBUFFEROESPROC mapBufferOES;
    PFNGLUNMAPBUFFEROESPROC unmapBufferOES;
#endif

#ifdef GL_EXT_map_buffer_range
    PFNGLMAPBUFFERRANGEEXTPROC mapBufferRangeEXT;
#endif

#ifdef GL_IMG_multisampled_render_to_texture
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG renderbufferStorageMultisampleIMG;
    PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG framebufferTexture2DMultisampleIMG;
#endif

#endif

namespace ouzel
{
    namespace graphics
    {
        RendererOGL::RendererOGL():
            Renderer(Driver::OPENGL)
        {
            projectionTransform = Matrix4(1.0f, 0.0f, 0.0f, 0.0f,
                                          0.0f, 1.0f, 0.0f, 0.0f,
                                          0.0f, 0.0f, 2.0f, -1.0f,
                                          0.0f, 0.0f, 0.0f, 1.0f);

            renderTargetProjectionTransform = Matrix4(1.0f, 0.0f, 0.0f, 0.0f,
                                                      0.0f, -1.0f, 0.0f, 0.0f,
                                                      0.0f, 0.0f, 2.0f, -1.0f,
                                                      0.0f, 0.0f, 0.0f, 1.0f);
        }

        RendererOGL::~RendererOGL()
        {
            if (colorRenderBufferId)
            {
                glDeleteRenderbuffers(1, &colorRenderBufferId);
            }

            if (depthRenderBufferId)
            {
                glDeleteRenderbuffers(1, &depthRenderBufferId);
            }

            if (frameBufferId)
            {
                glDeleteFramebuffers(1, &frameBufferId);
            }
        }

        bool RendererOGL::init(Window* newWindow,
                               const Size2& newSize,
                               uint32_t newSampleCount,
                               TextureFilter newTextureFilter,
                               PixelFormat newBackBufferFormat,
                               bool newVerticalSync,
                               uint32_t newDepthBits)
        {
            if (!Renderer::init(newWindow, newSize, newSampleCount, newTextureFilter, newBackBufferFormat, newVerticalSync, newDepthBits))
            {
                return false;
            }

            //const GLubyte* deviceVendor = glGetString(GL_VENDOR);
            const GLubyte* deviceName = glGetString(GL_RENDERER);

            if (checkOpenGLError() || !deviceName)
            {
                Log(Log::Level::WARN) << "Failed to get OpenGL renderer";
            }
            else
            {
                Log(Log::Level::INFO) << "Using " << reinterpret_cast<const char*>(deviceName) << " for rendering";
            }

#if OUZEL_SUPPORTS_OPENGLES
            if (apiMajorVersion >= 3)
            {
#if OUZEL_OPENGL_INTERFACE_EGL
#ifdef GL_OES_vertex_array_object
                genVertexArraysOES = (PFNGLGENVERTEXARRAYSOESPROC)eglGetProcAddress("glGenVertexArraysOES");
                bindVertexArrayOES = (PFNGLBINDVERTEXARRAYOESPROC)eglGetProcAddress("glBindVertexArrayOES");
                deleteVertexArraysOES = (PFNGLDELETEVERTEXARRAYSOESPROC)eglGetProcAddress("glDeleteVertexArraysOES");
#endif

#ifdef GL_OES_mapbuffer
                mapBufferOES = (PFNGLMAPBUFFEROESPROC)eglGetProcAddress("glMapBufferOES");
                unmapBufferOES = (PFNGLUNMAPBUFFEROESPROC)eglGetProcAddress("glUnmapBufferOES");
#endif

#ifdef GL_EXT_map_buffer_range
                mapBufferRangeEXT = (PFNGLMAPBUFFERRANGEEXTPROC)eglGetProcAddress("glMapBufferRangeEXT");
#endif
#endif
            }
            else
            {
                npotTexturesSupported = false;
                
                const GLubyte* extensionPtr = glGetString(GL_EXTENSIONS);

                if (checkOpenGLError() || !extensionPtr)
                {
                    Log(Log::Level::WARN) << "Failed to get OpenGL extensions";
                }
                else
                {
                    std::string extensions(reinterpret_cast<const char*>(extensionPtr));

                    std::istringstream extensionStringStream(extensions);

                    for (std::string extension; extensionStringStream >> extension;)
                    {
                        if (extension == "GL_OES_texture_npot")
                        {
                            npotTexturesSupported = true;
                        }
#if OUZEL_OPENGL_INTERFACE_EGL
#ifdef GL_OES_vertex_array_object
                        else if (extension == "GL_OES_vertex_array_object")
                        {
                            genVertexArraysOES = (PFNGLGENVERTEXARRAYSOESPROC)eglGetProcAddress("glGenVertexArraysOES");
                            bindVertexArrayOES = (PFNGLBINDVERTEXARRAYOESPROC)eglGetProcAddress("glBindVertexArrayOES");
                            deleteVertexArraysOES = (PFNGLDELETEVERTEXARRAYSOESPROC)eglGetProcAddress("glDeleteVertexArraysOES");
                        }
#endif
#ifdef GL_OES_mapbuffer
                        else if (extension == "GL_OES_mapbuffer")
                        {
                            mapBufferOES = (PFNGLMAPBUFFEROESPROC)eglGetProcAddress("glMapBufferOES");
                            unmapBufferOES = (PFNGLUNMAPBUFFEROESPROC)eglGetProcAddress("glUnmapBufferOES");
                        }
#endif
#ifdef GL_EXT_map_buffer_range
                        else if (extension == "GL_EXT_map_buffer_range")
                        {
                            mapBufferRangeEXT = (PFNGLMAPBUFFERRANGEEXTPROC)eglGetProcAddress("glMapBufferRangeEXT");
                        }
#endif
#endif
                    }
                }
            }
#endif

            frameBufferWidth = static_cast<GLsizei>(newSize.v[0]);
            frameBufferHeight = static_cast<GLsizei>(newSize.v[1]);

            if (!createFrameBuffer())
            {
                return false;
            }

            ShaderPtr textureShader = createShader();

            switch (apiMajorVersion)
            {
                case 2:
#if OUZEL_SUPPORTS_OPENGL
                    textureShader->initFromBuffers(std::vector<uint8_t>(std::begin(TexturePSGL2_glsl), std::end(TexturePSGL2_glsl)),
                                                   std::vector<uint8_t>(std::begin(TextureVSGL2_glsl), std::end(TextureVSGL2_glsl)),
                                                   VertexPCT::ATTRIBUTES,
                                                   {{"color", 4 * sizeof(float)}},
                                                   {{"modelViewProj", sizeof(Matrix4)}});
#elif OUZEL_SUPPORTS_OPENGLES
                    textureShader->initFromBuffers(std::vector<uint8_t>(std::begin(TexturePSGLES2_glsl), std::end(TexturePSGLES2_glsl)),
                                                   std::vector<uint8_t>(std::begin(TextureVSGLES2_glsl), std::end(TextureVSGLES2_glsl)),
                                                   VertexPCT::ATTRIBUTES,
                                                   {{"color", 4 * sizeof(float)}},
                                                   {{"modelViewProj", sizeof(Matrix4)}});
#endif
                    break;
                case 3:
#if OUZEL_SUPPORTS_OPENGL3
                    textureShader->initFromBuffers(std::vector<uint8_t>(std::begin(TexturePSGL3_glsl), std::end(TexturePSGL3_glsl)),
                                                   std::vector<uint8_t>(std::begin(TextureVSGL3_glsl), std::end(TextureVSGL3_glsl)),
                                                   VertexPCT::ATTRIBUTES,
                                                   {{"color", 4 * sizeof(float)}},
                                                   {{"modelViewProj", sizeof(Matrix4)}});
#elif OUZEL_SUPPORTS_OPENGLES3
                    textureShader->initFromBuffers(std::vector<uint8_t>(std::begin(TexturePSGLES3_glsl), std::end(TexturePSGLES3_glsl)),
                                                   std::vector<uint8_t>(std::begin(TextureVSGLES3_glsl), std::end(TextureVSGLES3_glsl)),
                                                   VertexPCT::ATTRIBUTES,
                                                   {{"color", 4 * sizeof(float)}},
                                                   {{"modelViewProj", sizeof(Matrix4)}});
#endif
                    break;
                default:
                    Log(Log::Level::ERR) << "Unsupported OpenGL version";
                    return false;
            }

            sharedEngine->getCache()->setShader(SHADER_TEXTURE, textureShader);

            ShaderPtr colorShader = createShader();

            switch (apiMajorVersion)
            {
                case 2:
#if OUZEL_SUPPORTS_OPENGL
                    colorShader->initFromBuffers(std::vector<uint8_t>(std::begin(ColorPSGL2_glsl), std::end(ColorPSGL2_glsl)),
                                                 std::vector<uint8_t>(std::begin(ColorVSGL2_glsl), std::end(ColorVSGL2_glsl)),
                                                 VertexPC::ATTRIBUTES,
                                                 {{"color", 4 * sizeof(float)}},
                                                 {{"modelViewProj", sizeof(Matrix4)}});
#elif OUZEL_SUPPORTS_OPENGLES
                    colorShader->initFromBuffers(std::vector<uint8_t>(std::begin(ColorPSGLES2_glsl), std::end(ColorPSGLES2_glsl)),
                                                 std::vector<uint8_t>(std::begin(ColorVSGLES2_glsl), std::end(ColorVSGLES2_glsl)),
                                                 VertexPC::ATTRIBUTES,
                                                 {{"color", 4 * sizeof(float)}},
                                                 {{"modelViewProj", sizeof(Matrix4)}});
#endif
                    break;
                case 3:
#if OUZEL_SUPPORTS_OPENGL3
                    colorShader->initFromBuffers(std::vector<uint8_t>(std::begin(ColorPSGL3_glsl), std::end(ColorPSGL3_glsl)),
                                                 std::vector<uint8_t>(std::begin(ColorVSGL3_glsl), std::end(ColorVSGL3_glsl)),
                                                 VertexPC::ATTRIBUTES,
                                                 {{"color", 4 * sizeof(float)}},
                                                 {{"modelViewProj", sizeof(Matrix4)}});
#elif OUZEL_SUPPORTS_OPENGLES3
                    colorShader->initFromBuffers(std::vector<uint8_t>(std::begin(ColorPSGLES3_glsl), std::end(ColorPSGLES3_glsl)),
                                                 std::vector<uint8_t>(std::begin(ColorVSGLES3_glsl), std::end(ColorVSGLES3_glsl)),
                                                 VertexPC::ATTRIBUTES,
                                                 {{"color", 4 * sizeof(float)}},
                                                 {{"modelViewProj", sizeof(Matrix4)}});
#endif
                    break;
                default:
                    Log(Log::Level::ERR) << "Unsupported OpenGL version";
                    return false;
            }

            sharedEngine->getCache()->setShader(SHADER_COLOR, colorShader);

            BlendStatePtr noBlendState = createBlendState();

            noBlendState->init(false,
                               BlendState::BlendFactor::ONE, BlendState::BlendFactor::ZERO,
                               BlendState::BlendOperation::ADD,
                               BlendState::BlendFactor::ONE, BlendState::BlendFactor::ZERO,
                               BlendState::BlendOperation::ADD);

            sharedEngine->getCache()->setBlendState(BLEND_NO_BLEND, noBlendState);

            BlendStatePtr addBlendState = createBlendState();

            addBlendState->init(true,
                                BlendState::BlendFactor::ONE, BlendState::BlendFactor::ONE,
                                BlendState::BlendOperation::ADD,
                                BlendState::BlendFactor::ONE, BlendState::BlendFactor::ONE,
                                BlendState::BlendOperation::ADD);

            sharedEngine->getCache()->setBlendState(BLEND_ADD, addBlendState);

            BlendStatePtr multiplyBlendState = createBlendState();

            multiplyBlendState->init(true,
                                     BlendState::BlendFactor::DEST_COLOR, BlendState::BlendFactor::ZERO,
                                     BlendState::BlendOperation::ADD,
                                     BlendState::BlendFactor::ONE, BlendState::BlendFactor::ONE,
                                     BlendState::BlendOperation::ADD);

            sharedEngine->getCache()->setBlendState(BLEND_MULTIPLY, multiplyBlendState);

            BlendStatePtr alphaBlendState = createBlendState();

            alphaBlendState->init(true,
                                  BlendState::BlendFactor::SRC_ALPHA, BlendState::BlendFactor::INV_SRC_ALPHA,
                                  BlendState::BlendOperation::ADD,
                                  BlendState::BlendFactor::ONE, BlendState::BlendFactor::ONE,
                                  BlendState::BlendOperation::ADD);

            sharedEngine->getCache()->setBlendState(BLEND_ALPHA, alphaBlendState);

            TexturePtr whitePixelTexture = createTexture();
            whitePixelTexture->initFromBuffer({255, 255, 255, 255}, Size2(1.0f, 1.0f), false, false);
            sharedEngine->getCache()->setTexture(TEXTURE_WHITE_PIXEL, whitePixelTexture);

            glDepthFunc(GL_LEQUAL);
            glClearDepthf(1.0f);

#if OUZEL_SUPPORTS_OPENGL
            if (sampleCount > 1)
            {
                glEnable(GL_MULTISAMPLE);
            }
#endif

            return true;
        }

        bool RendererOGL::update()
        {
            clearMask = 0;
            if (uploadData.clearColorBuffer) clearMask |= GL_COLOR_BUFFER_BIT;
            if (uploadData.clearDepthBuffer) clearMask |= GL_DEPTH_BUFFER_BIT;

            frameBufferClearColor[0] = uploadData.clearColor.normR();
            frameBufferClearColor[1] = uploadData.clearColor.normG();
            frameBufferClearColor[2] = uploadData.clearColor.normB();
            frameBufferClearColor[3] = uploadData.clearColor.normA();

            if (frameBufferWidth != static_cast<GLsizei>(uploadData.size.v[0]) ||
                frameBufferHeight != static_cast<GLsizei>(uploadData.size.v[1]))
            {
                frameBufferWidth = static_cast<GLsizei>(uploadData.size.v[0]);
                frameBufferHeight = static_cast<GLsizei>(uploadData.size.v[1]);

                if (!createFrameBuffer())
                {
                    return false;
                }

#if OUZEL_PLATFORM_IOS || OUZEL_PLATFORM_TVOS
                Size2 backBufferSize = Size2(static_cast<float>(frameBufferWidth),
                                             static_cast<float>(frameBufferHeight));

                window->setSize(backBufferSize);
#endif
            }

            return true;
        }

        bool RendererOGL::present()
        {
            if (!Renderer::present())
            {
                return false;
            }

            deleteResources();

            if (drawQueue.empty())
            {
                frameBufferClearedFrame = currentFrame;

                if (clearMask)
                {
                    if (!bindFrameBuffer(frameBufferId))
                    {
                        return false;
                    }

                    if (!setViewport(0, 0,
                                     frameBufferWidth,
                                     frameBufferHeight))
                    {
                        return false;
                    }

                    glClearColor(frameBufferClearColor[0],
                                 frameBufferClearColor[1],
                                 frameBufferClearColor[2],
                                 frameBufferClearColor[3]);

                    glClear(clearMask);

                    if (checkOpenGLError())
                    {
                        Log(Log::Level::ERR) << "Failed to clear frame buffer";
                        return false;
                    }
                }
            }
            else for (const DrawCommand& drawCommand : drawQueue)
            {
#ifdef OUZEL_SUPPORTS_OPENGL
                setPolygonFillMode(drawCommand.wireframe ? GL_LINE : GL_FILL);
#else
                if (drawCommand.wireframe)
                {
                    continue;
                }
#endif

                // blend state
                std::shared_ptr<BlendStateOGL> blendStateOGL = std::static_pointer_cast<BlendStateOGL>(drawCommand.blendState);

                if (!blendStateOGL)
                {
                    // don't render if invalid blend state
                    continue;
                }

                if (!setBlendState(blendStateOGL->isGLBlendEnabled(),
                                   blendStateOGL->getModeRGB(),
                                   blendStateOGL->getModeAlpha(),
                                   blendStateOGL->getSourceFactorRGB(),
                                   blendStateOGL->getDestFactorRGB(),
                                   blendStateOGL->getSourceFactorAlpha(),
                                   blendStateOGL->getDestFactorAlpha()))
                {
                    return false;
                }

                bool texturesValid = true;

                // textures
                for (uint32_t layer = 0; layer < Texture::LAYERS; ++layer)
                {
                    std::shared_ptr<TextureOGL> textureOGL;

                    if (drawCommand.textures.size() > layer)
                    {
                        textureOGL = std::static_pointer_cast<TextureOGL>(drawCommand.textures[layer]);
                    }

                    if (textureOGL)
                    {
                        if (!textureOGL->getTextureId())
                        {
                            texturesValid = false;
                            break;
                        }

                        if (!bindTexture(textureOGL->getTextureId(), layer))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (!bindTexture(0, layer))
                        {
                            return false;
                        }
                    }
                }

                if (!texturesValid)
                {
                    continue;
                }

                // shader
                std::shared_ptr<ShaderOGL> shaderOGL = std::static_pointer_cast<ShaderOGL>(drawCommand.shader);

                if (!shaderOGL || !shaderOGL->getProgramId())
                {
                    // don't render if invalid shader
                    continue;
                }

                useProgram(shaderOGL->getProgramId());

                // pixel shader constants
                const std::vector<ShaderOGL::Location>& pixelShaderConstantLocations = shaderOGL->getPixelShaderConstantLocations();

                if (drawCommand.pixelShaderConstants.size() > pixelShaderConstantLocations.size())
                {
                    Log(Log::Level::ERR) << "Invalid pixel shader constant size";
                    return false;
                }

                for (size_t i = 0; i < drawCommand.pixelShaderConstants.size(); ++i)
                {
                    const ShaderOGL::Location& pixelShaderConstantLocation = pixelShaderConstantLocations[i];
                    const std::vector<float>& pixelShaderConstant = drawCommand.pixelShaderConstants[i];

                    uint32_t components = pixelShaderConstantLocation.size / 4;

                    switch (components)
                    {
                        case 1:
                            glUniform1fv(pixelShaderConstantLocation.location, static_cast<GLsizei>(pixelShaderConstant.size() / components), pixelShaderConstant.data());
                            break;
                        case 2:
                            glUniform2fv(pixelShaderConstantLocation.location, static_cast<GLsizei>(pixelShaderConstant.size() / components), pixelShaderConstant.data());
                            break;
                        case 3:
                            glUniform3fv(pixelShaderConstantLocation.location, static_cast<GLsizei>(pixelShaderConstant.size() / components), pixelShaderConstant.data());
                            break;
                        case 4:
                            glUniform4fv(pixelShaderConstantLocation.location, static_cast<GLsizei>(pixelShaderConstant.size() / components), pixelShaderConstant.data());
                            break;
                        case 9:
                            glUniformMatrix3fv(pixelShaderConstantLocation.location, static_cast<GLsizei>(pixelShaderConstant.size() / components), GL_FALSE, pixelShaderConstant.data());
                            break;
                        case 16:
                            glUniformMatrix4fv(pixelShaderConstantLocation.location, static_cast<GLsizei>(pixelShaderConstant.size() / components), GL_FALSE, pixelShaderConstant.data());
                            break;
                        default:
                            Log(Log::Level::ERR) << "Unsupported uniform size";
                            return false;
                    }
                }

                // vertex shader constants
                const std::vector<ShaderOGL::Location>& vertexShaderConstantLocations = shaderOGL->getVertexShaderConstantLocations();

                if (drawCommand.vertexShaderConstants.size() > vertexShaderConstantLocations.size())
                {
                    Log(Log::Level::ERR) << "Invalid vertex shader constant size";
                    return false;
                }

                for (size_t i = 0; i < drawCommand.vertexShaderConstants.size(); ++i)
                {
                    const ShaderOGL::Location& vertexShaderConstantLocation = vertexShaderConstantLocations[i];
                    const std::vector<float>& vertexShaderConstant = drawCommand.vertexShaderConstants[i];

                    uint32_t components = vertexShaderConstantLocation.size / 4;

                    switch (components)
                    {
                        case 1:
                            glUniform1fv(vertexShaderConstantLocation.location, static_cast<GLsizei>(vertexShaderConstant.size() / components), vertexShaderConstant.data());
                            break;
                        case 2:
                            glUniform2fv(vertexShaderConstantLocation.location, static_cast<GLsizei>(vertexShaderConstant.size() / components), vertexShaderConstant.data());
                            break;
                        case 3:
                            glUniform3fv(vertexShaderConstantLocation.location, static_cast<GLsizei>(vertexShaderConstant.size() / components), vertexShaderConstant.data());
                            break;
                        case 4:
                            glUniform4fv(vertexShaderConstantLocation.location, static_cast<GLsizei>(vertexShaderConstant.size() / components), vertexShaderConstant.data());
                            break;
                        case 9:
                            glUniformMatrix3fv(vertexShaderConstantLocation.location, static_cast<GLsizei>(vertexShaderConstant.size() / components), GL_FALSE, vertexShaderConstant.data());
                            break;
                        case 16:
                            glUniformMatrix4fv(vertexShaderConstantLocation.location, static_cast<GLsizei>(vertexShaderConstant.size() / components), GL_FALSE, vertexShaderConstant.data());
                            break;
                        default:
                            Log(Log::Level::ERR) << "Unsupported uniform size";
                            return false;
                    }
                }

                // render target
                GLuint newFrameBufferId = 0;
                GLbitfield newClearMask = 0;
                const float* newClearColor;

                if (drawCommand.renderTarget)
                {
                    std::shared_ptr<RenderTargetOGL> renderTargetOGL = std::static_pointer_cast<RenderTargetOGL>(drawCommand.renderTarget);

                    if (!renderTargetOGL->getFrameBufferId())
                    {
                        continue;
                    }

                    newFrameBufferId = renderTargetOGL->getFrameBufferId();

                    if (renderTargetOGL->getFrameBufferClearedFrame() != currentFrame)
                    {
                        renderTargetOGL->setFrameBufferClearedFrame(currentFrame);
                        newClearMask = renderTargetOGL->getClearMask();
                        newClearColor = renderTargetOGL->getFrameBufferClearColor();
                    }
                }
                else
                {
                    newFrameBufferId = frameBufferId;

                    if (frameBufferClearedFrame != currentFrame)
                    {
                        frameBufferClearedFrame = currentFrame;
                        newClearMask = clearMask;
                        newClearColor = frameBufferClearColor;
                    }
                }

                if (!bindFrameBuffer(newFrameBufferId))
                {
                    return false;
                }

                setViewport(static_cast<GLint>(drawCommand.viewport.position.v[0]),
                            static_cast<GLint>(drawCommand.viewport.position.v[1]),
                            static_cast<GLsizei>(drawCommand.viewport.size.v[0]),
                            static_cast<GLsizei>(drawCommand.viewport.size.v[1]));

                if (newClearMask)
                {
                    if (newClearMask & GL_DEPTH_BUFFER_BIT)
                    {
                        // allow clearing the depth buffer
                        depthMask(true);
                    }

                    glClearColor(newClearColor[0],
                                 newClearColor[1],
                                 newClearColor[2],
                                 newClearColor[3]);

                    glClear(newClearMask);

                    if (checkOpenGLError())
                    {
                        Log(Log::Level::ERR) << "Failed to clear frame buffer";
                        return false;
                    }
                }

                enableDepthTest(drawCommand.depthTest);

                GLint writeMask;
                glGetIntegerv(GL_DEPTH_WRITEMASK, &writeMask);
                depthMask(drawCommand.depthWrite);

                // scissor test
                setScissorTest(drawCommand.scissorTestEnabled,
                               static_cast<GLint>(drawCommand.scissorTest.position.v[0]),
                               static_cast<GLint>(drawCommand.scissorTest.position.v[1]),
                               static_cast<GLsizei>(drawCommand.scissorTest.size.v[0]),
                               static_cast<GLsizei>(drawCommand.scissorTest.size.v[1]));

                // mesh buffer
                std::shared_ptr<MeshBufferOGL> meshBufferOGL = std::static_pointer_cast<MeshBufferOGL>(drawCommand.meshBuffer);

                if (!meshBufferOGL)
                {
                    // don't render if invalid mesh buffer
                    continue;
                }

                std::shared_ptr<IndexBufferOGL> indexBufferOGL = std::static_pointer_cast<IndexBufferOGL>(meshBufferOGL->getIndexBuffer());
                std::shared_ptr<VertexBufferOGL> vertexBufferOGL = std::static_pointer_cast<VertexBufferOGL>(meshBufferOGL->getVertexBuffer());

                if (!indexBufferOGL || !indexBufferOGL->getBufferId() ||
                    !vertexBufferOGL || !vertexBufferOGL->getBufferId())
                {
                    continue;
                }

                // draw
                GLenum mode;

                switch (drawCommand.drawMode)
                {
                    case DrawMode::POINT_LIST: mode = GL_POINTS; break;
                    case DrawMode::LINE_LIST: mode = GL_LINES; break;
                    case DrawMode::LINE_STRIP: mode = GL_LINE_STRIP; break;
                    case DrawMode::TRIANGLE_LIST: mode = GL_TRIANGLES; break;
                    case DrawMode::TRIANGLE_STRIP: mode = GL_TRIANGLE_STRIP; break;
                    default: Log(Log::Level::ERR) << "Invalid draw mode"; return false;
                }

                if (!meshBufferOGL->bindBuffers())
                {
                    return false;
                }

                glDrawElements(mode,
                               static_cast<GLsizei>(drawCommand.indexCount),
                               indexBufferOGL->getType(),
                               static_cast<const char*>(nullptr) + (drawCommand.startIndex * indexBufferOGL->getBytesPerIndex()));

                if (checkOpenGLError())
                {
                    Log(Log::Level::ERR) << "Failed to draw elements";
                    return false;
                }
            }

#if OUZEL_SUPPORTS_OPENGL
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // draw to default frame buffer
            glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBufferId); // read from FBO
            glDrawBuffer(GL_BACK); // set the back buffer as the draw buffer

            if (checkOpenGLError())
            {
                Log(Log::Level::ERR) << "Failed to bind frame buffer";
                return false;
            }

            glBlitFramebuffer(0, 0, frameBufferWidth, frameBufferHeight,
                              0, 0, frameBufferWidth, frameBufferHeight,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);

            if (checkOpenGLError())
            {
                Log(Log::Level::ERR) << "Failed to blit framebuffer";
                return false;
            }

            // reset framebuffer
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            stateCache.frameBufferId = 0;
#endif

            return true;
        }

        std::vector<Size2> RendererOGL::getSupportedResolutions() const
        {
            return std::vector<Size2>();
        }

        BlendStatePtr RendererOGL::createBlendState()
        {
            std::shared_ptr<BlendStateOGL> blendState = std::make_shared<BlendStateOGL>();
            return blendState;
        }

        TexturePtr RendererOGL::createTexture()
        {
            std::shared_ptr<TextureOGL> texture = std::make_shared<TextureOGL>();
            return texture;
        }

        RenderTargetPtr RendererOGL::createRenderTarget()
        {
            std::shared_ptr<RenderTargetOGL> renderTarget = std::make_shared<RenderTargetOGL>();
            return renderTarget;
        }

        ShaderPtr RendererOGL::createShader()
        {
            std::shared_ptr<ShaderOGL> shader(new ShaderOGL());
            return shader;
        }

        MeshBufferPtr RendererOGL::createMeshBuffer()
        {
            std::shared_ptr<MeshBufferOGL> meshBuffer = std::make_shared<MeshBufferOGL>();
            return meshBuffer;
        }

        IndexBufferPtr RendererOGL::createIndexBuffer()
        {
            std::shared_ptr<IndexBufferOGL> meshBuffer = std::make_shared<IndexBufferOGL>();
            return meshBuffer;
        }

        VertexBufferPtr RendererOGL::createVertexBuffer()
        {
            std::shared_ptr<VertexBufferOGL> meshBuffer = std::make_shared<VertexBufferOGL>();
            return meshBuffer;
        }

        bool RendererOGL::generateScreenshot(const std::string& filename)
        {
            bindFrameBuffer(0);

            const GLsizei width = frameBufferWidth;
            const GLsizei height = frameBufferHeight;
            const GLsizei depth = 4;

            std::vector<uint8_t> data(static_cast<size_t>(width * height * depth));

            glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

            if (checkOpenGLError())
            {
                Log(Log::Level::ERR) << "Failed to read pixels from frame buffer";
                return false;
            }

            uint8_t temp;
            for (GLsizei row = 0; row < height / 2; ++row)
            {
                for (GLsizei col = 0; col < width; ++col)
                {
                    for (GLsizei z = 0; z < depth; ++z)
                    {
                        temp = data[static_cast<size_t>(((height - row - 1) * width + col) * depth + z)];
                        data[static_cast<size_t>(((height - row - 1) * width + col) * depth + z)] = data[static_cast<size_t>((row * width + col) * depth + z)];
                        data[static_cast<size_t>((row * width + col) * depth + z)] = temp;
                    }
                }
            }

            if (!stbi_write_png(filename.c_str(), width, height, depth, data.data(), width * depth))
            {
                Log(Log::Level::ERR) << "Failed to save image to file";
                return false;
            }

            return true;
        }

        bool RendererOGL::createFrameBuffer()
        {
#if !OUZEL_OPENGL_INTERFACE_EAGL
            if (!frameBufferId)
            {
                glGenFramebuffers(1, &frameBufferId);
            }

            if (sampleCount > 1)
            {
                if (!colorRenderBufferId) glGenRenderbuffers(1, &colorRenderBufferId);
                glBindRenderbuffer(GL_RENDERBUFFER, colorRenderBufferId);

#if OUZEL_SUPPORTS_OPENGL
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, sampleCount, GL_RGBA, frameBufferWidth, frameBufferHeight);
#else
                glRenderbufferStorageMultisampleIMG(GL_RENDERBUFFER, sampleCount, GL_RGBA, frameBufferWidth, frameBufferHeight);
#endif

                if (depthBits > 0)
                {
                    GLuint depthFormat = getDepthFormat(depthBits);

                    if (!depthFormat)
                    {
                        Log(Log::Level::ERR) << "Unsupported depth buffer format";
                        return false;
                    }

                    if (!depthRenderBufferId) glGenRenderbuffers(1, &depthRenderBufferId);
                    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBufferId);

#if OUZEL_SUPPORTS_OPENGL
                    glRenderbufferStorageMultisample(GL_RENDERBUFFER, sampleCount, depthFormat, frameBufferWidth, frameBufferHeight);
#else
                    glRenderbufferStorageMultisampleIMG(GL_RENDERBUFFER, sampleCount, depthFormat, frameBufferWidth, frameBufferHeight);
#endif
                }

                bindFrameBuffer(frameBufferId);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderBufferId);

                if (depthBits > 0)
                {
                    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBufferId);
                }

                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                {
                    Log(Log::Level::ERR) << "Failed to create framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER);
                    return false;
                }
            }
            else
            {
                if (!colorRenderBufferId) glGenRenderbuffers(1, &colorRenderBufferId);
                glBindRenderbuffer(GL_RENDERBUFFER, colorRenderBufferId);
                glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, frameBufferWidth, frameBufferHeight);

                if (depthBits > 0)
                {
                    GLuint depthFormat = getDepthFormat(depthBits);

                    if (!depthFormat)
                    {
                        Log(Log::Level::ERR) << "Unsupported depth buffer format";
                        return false;
                    }

                    if (!depthRenderBufferId) glGenRenderbuffers(1, &depthRenderBufferId);
                    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBufferId);
                    glRenderbufferStorage(GL_RENDERBUFFER, depthFormat, frameBufferWidth, frameBufferHeight);
                }


                bindFrameBuffer(frameBufferId);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderBufferId);

                if (depthBits > 0)
                {
                    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBufferId);
                }
            }

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                Log(Log::Level::ERR) << "Failed to create framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER);
                return false;
            }
#endif

            return true;
        }

        RendererOGL::StateCache RendererOGL::stateCache;
        std::queue<std::pair<GLuint, RendererOGL::ResourceType>> RendererOGL::deleteQueue;
        std::mutex RendererOGL::deleteMutex;

        void RendererOGL::deleteResource(GLuint resource, ResourceType resourceType)
        {
            if (sharedEngine->isActive())
            {
                std::lock_guard<std::mutex> lock(deleteMutex);

                deleteQueue.push(std::make_pair(resource, resourceType));
            }
        }

        void RendererOGL::deleteResources()
        {
            std::pair<GLuint, ResourceType> deleteResource;

            for (;;)
            {
                {
                    std::lock_guard<std::mutex> lock(deleteMutex);
                    if (deleteQueue.empty())
                    {
                        break;
                    }

                    deleteResource = deleteQueue.front();
                    deleteQueue.pop();
                }

                switch (deleteResource.second)
                {
                    case ResourceType::Buffer:
                        if (stateCache.elementArrayBufferId == deleteResource.first) stateCache.elementArrayBufferId = 0;
                        if (stateCache.arrayBufferId == deleteResource.first) stateCache.arrayBufferId = 0;
                        glDeleteBuffers(1, &deleteResource.first);
                        break;
                    case ResourceType::VertexArray:
#if OUZEL_PLATFORM_ANDROID
                        bindVertexArray(0); // workaround for Android (current VAO's element array buffer is set to 0 if glDeleteVertexArrays is called on Android)
#else
                        if (stateCache.vertexArrayId == deleteResource.first) stateCache.vertexArrayId = 0;
#endif

#if OUZEL_OPENGL_INTERFACE_EAGL
                        glDeleteVertexArraysOES(1, &deleteResource.first);
#elif OUZEL_OPENGL_INTERFACE_EGL
                        if (deleteVertexArraysOES) deleteVertexArraysOES(1, &deleteResource.first);
#else
                        glDeleteVertexArrays(1, &deleteResource.first);
#endif
                        break;
                    case ResourceType::RenderBuffer:
                        glDeleteRenderbuffers(1, &deleteResource.first);
                        break;
                    case ResourceType::FrameBuffer:
                        if (stateCache.frameBufferId == deleteResource.first) stateCache.frameBufferId = 0;
                        glDeleteFramebuffers(1, &deleteResource.first);
                        break;
                    case ResourceType::Program:
                        if (stateCache.programId == deleteResource.first) stateCache.programId = 0;
                        glDeleteProgram(deleteResource.first);
                        break;
                    case ResourceType::Shader:
                        glDeleteShader(deleteResource.first);
                        break;
                    case ResourceType::Texture:
                        for (uint32_t layer = 0; layer < Texture::LAYERS; ++layer)
                        {
                            if (stateCache.textureId[layer] == deleteResource.first)
                            {
                                stateCache.textureId[layer] = 0;
                            }
                        }
                        glDeleteTextures(1, &deleteResource.first);
                        break;
                }
            }
        }
    } // namespace graphics
} // namespace ouzel
