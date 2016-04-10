// Copyright (C) 2016 Elviss Strazdins
// This file is part of the Ouzel engine.

#pragma once

#include "CompileConfig.h"
#include "MeshBuffer.h"

#ifdef __OBJC__
#import <Metal/Metal.h>
typedef id<MTLBuffer> MTLBufferPtr;
#else
#include <objc/objc.h>
typedef id MTLBufferPtr;
#endif

namespace ouzel
{
    namespace video
    {
        class RendererMetal;

        class MeshBufferMetal: public MeshBuffer
        {
            friend RendererMetal;
        public:
            virtual ~MeshBufferMetal();

            virtual bool initFromData(const void* indices, uint32_t indexSize, uint32_t indexCount, bool dynamicIndexBuffer, const void* vertices, uint32_t vertexAttributes, uint32_t vertexCount, bool dynamicVertexBuffer) override;

            virtual bool uploadIndices(const void* indices, uint32_t indexCount) override;
            virtual bool uploadVertices(const void* vertices, uint32_t vertexCount) override;

        protected:
            MeshBufferMetal();

            void destroy();

            MTLBufferPtr _indexBuffer = Nil;
            MTLBufferPtr _vertexBuffer = Nil;
        };
    } // namespace video
} // namespace ouzel
