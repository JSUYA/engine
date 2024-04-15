// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_EMBEDDER_EMBEDDER_EXTERNAL_TEXTURE_GL_H_
#define FLUTTER_SHELL_PLATFORM_EMBEDDER_EMBEDDER_EXTERNAL_TEXTURE_GL_H_

#include "flutter/common/graphics/texture.h"
#include "flutter/fml/macros.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "third_party/skia/include/core/SkSize.h"

namespace flutter {

class EmbedderExternalTextureGL : public flutter::Texture {
 public:
  using ExternalTextureCallback = std::function<
      std::unique_ptr<FlutterOpenGLTexture>(int64_t, size_t, size_t)>;

  EmbedderExternalTextureGL(int64_t texture_identifier,
                            const ExternalTextureCallback& callback);

  ~EmbedderExternalTextureGL();

  // |flutter::Texture|
  void Paint(PaintContext& context,
             const SkRect& bounds,
             bool freeze,
             const DlImageSampling sampling) override;

  // |flutter::Texture|
  void OnGrContextCreated() override;

  // |flutter::Texture|
  void OnGrContextDestroyed() override;

  // |flutter::Texture|
  void MarkNewFrameAvailable() override;

  // |flutter::Texture|
  void OnTextureUnregistered() override;

 protected:
  virtual sk_sp<DlImage> ResolveTexture(int64_t texture_id,
                                        PaintContext& context,
                                        const SkISize& size) = 0;
  const ExternalTextureCallback& external_texture_callback_;
  sk_sp<DlImage> last_image_;

 private:
  FML_DISALLOW_COPY_AND_ASSIGN(EmbedderExternalTextureGL);
};

class EmbedderExternalTextureSkiaGL
    : public flutter::EmbedderExternalTextureGL {
 public:
  EmbedderExternalTextureSkiaGL(int64_t texture_identifier,
                                const ExternalTextureCallback& callback);

  ~EmbedderExternalTextureSkiaGL();

 protected:
  sk_sp<DlImage> ResolveTexture(int64_t texture_id,
                                PaintContext& context,
                                const SkISize& size) override;

 private:
  FML_DISALLOW_COPY_AND_ASSIGN(EmbedderExternalTextureSkiaGL);
};

class EmbedderExternalTextureGLImpellerPixelBuffer
    : public flutter::EmbedderExternalTextureGL {
 public:
  EmbedderExternalTextureGLImpellerPixelBuffer(
      int64_t texture_identifier,
      const ExternalTextureCallback& callback);

  ~EmbedderExternalTextureGLImpellerPixelBuffer();

 protected:
  sk_sp<DlImage> ResolveTexture(int64_t texture_id,
                                PaintContext& context,
                                const SkISize& size) override;

 private:
  FML_DISALLOW_COPY_AND_ASSIGN(EmbedderExternalTextureGLImpellerPixelBuffer);
};

class EmbedderExternalTextureGLImpellerSurface
    : public flutter::EmbedderExternalTextureGL {
 public:
  EmbedderExternalTextureGLImpellerSurface(
      int64_t texture_identifier,
      const ExternalTextureCallback& callback);

  ~EmbedderExternalTextureGLImpellerSurface();

 protected:
  sk_sp<DlImage> ResolveTexture(int64_t texture_id,
                                PaintContext& context,
                                const SkISize& size) override;

 private:
  FML_DISALLOW_COPY_AND_ASSIGN(EmbedderExternalTextureGLImpellerSurface);
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_EMBEDDER_EMBEDDER_EXTERNAL_TEXTURE_GL_H_
