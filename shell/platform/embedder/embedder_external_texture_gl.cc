// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/embedder/embedder_external_texture_gl.h"

#include "flutter/fml/logging.h"
#include "flutter/impeller/display_list/dl_image_impeller.h"
#include "flutter/impeller/renderer/backend/gles/context_gles.h"
#include "flutter/impeller/renderer/backend/gles/texture_gles.h"
#include "impeller/aiks/aiks_context.h"
#include "impeller/renderer/backend/gles/gles.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "third_party/skia/include/core/SkAlphaType.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkColorType.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSize.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"
#include "third_party/skia/include/gpu/ganesh/SkImageGanesh.h"
#include "third_party/skia/include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "third_party/skia/include/gpu/gl/GrGLTypes.h"

namespace flutter {

EmbedderExternalTextureGL::EmbedderExternalTextureGL(
    int64_t texture_identifier,
    const ExternalTextureCallback& callback)
    : Texture(texture_identifier), external_texture_callback_(callback) {
  FML_DCHECK(external_texture_callback_);
}

EmbedderExternalTextureGL::~EmbedderExternalTextureGL() = default;

// |flutter::Texture|
void EmbedderExternalTextureGL::Paint(PaintContext& context,
                                      const SkRect& bounds,
                                      bool freeze,
                                      const DlImageSampling sampling) {
  if (last_image_ == nullptr) {
    last_image_ =
        ResolveTexture(Id(),                                           //
                       context,                                        //
                       SkISize::Make(bounds.width(), bounds.height())  //
        );
  }

  DlCanvas* canvas = context.canvas;
  const DlPaint* paint = context.paint;

  if (last_image_) {
    SkRect image_bounds = SkRect::Make(last_image_->bounds());
    if (bounds != image_bounds) {
      canvas->DrawImageRect(last_image_, image_bounds, bounds, sampling, paint);
    } else {
      canvas->DrawImage(last_image_, {bounds.x(), bounds.y()}, sampling, paint);
    }
  }
}

// |flutter::Texture|
void EmbedderExternalTextureGL::OnGrContextCreated() {}

// |flutter::Texture|
void EmbedderExternalTextureGL::OnGrContextDestroyed() {}

// |flutter::Texture|
void EmbedderExternalTextureGL::MarkNewFrameAvailable() {
  last_image_ = nullptr;
}

// |flutter::Texture|
void EmbedderExternalTextureGL::OnTextureUnregistered() {}

EmbedderExternalTextureSkiaGL::EmbedderExternalTextureSkiaGL(
    int64_t texture_identifier,
    const ExternalTextureCallback& callback)
    : EmbedderExternalTextureGL(texture_identifier, callback) {}

EmbedderExternalTextureSkiaGL::~EmbedderExternalTextureSkiaGL() = default;

sk_sp<DlImage> EmbedderExternalTextureSkiaGL::ResolveTexture(
    int64_t texture_id,
    PaintContext& context,
    const SkISize& size) {
  GrDirectContext* gr_context = context.gr_context;
  gr_context->flushAndSubmit();
  gr_context->resetContext(kAll_GrBackendState);
  std::unique_ptr<FlutterOpenGLTexture> texture =
      external_texture_callback_(texture_id, size.width(), size.height());

  if (!texture) {
    return nullptr;
  }

  GrGLTextureInfo gr_texture_info = {texture->target, texture->name,
                                     texture->format};

  size_t width = size.width();
  size_t height = size.height();

  if (texture->width != 0 && texture->height != 0) {
    width = texture->width;
    height = texture->height;
  }

  auto gr_backend_texture = GrBackendTextures::MakeGL(
      width, height, skgpu::Mipmapped::kNo, gr_texture_info);
  SkImages::TextureReleaseProc release_proc = texture->destruction_callback;
  auto image =
      SkImages::BorrowTextureFrom(gr_context,                // context
                                  gr_backend_texture,        // texture handle
                                  kTopLeft_GrSurfaceOrigin,  // origin
                                  kRGBA_8888_SkColorType,    // color type
                                  kPremul_SkAlphaType,       // alpha type
                                  nullptr,                   // colorspace
                                  release_proc,       // texture release proc
                                  texture->user_data  // texture release context
      );

  if (!image) {
    // In case Skia rejects the image, call the release proc so that
    // embedders can perform collection of intermediates.
    if (release_proc) {
      release_proc(texture->user_data);
    }
    return nullptr;
  }

  // This image should not escape local use by EmbedderExternalTextureGL
  return DlImage::Make(std::move(image));
}

EmbedderExternalTextureGLImpellerPixelBuffer::
    EmbedderExternalTextureGLImpellerPixelBuffer(
        int64_t texture_identifier,
        const ExternalTextureCallback& callback)
    : EmbedderExternalTextureGL(texture_identifier, callback) {}

sk_sp<DlImage> EmbedderExternalTextureGLImpellerPixelBuffer::ResolveTexture(
    int64_t texture_id,
    PaintContext& context,
    const SkISize& size) {
  std::unique_ptr<FlutterOpenGLTexture> texture =
      external_texture_callback_(texture_id, size.width(), size.height());

  if (!texture) {
    return nullptr;
  }

  size_t width = size.width();
  size_t height = size.height();

  if (texture->width != 0 && texture->height != 0) {
    width = texture->width;
    height = texture->height;
  }

  impeller::TextureDescriptor desc;
  desc.type = impeller::TextureType::kTexture2D;
  impeller::AiksContext* aiks_context = context.aiks_context;
  const auto& gl_context =
      impeller::ContextGLES::Cast(*aiks_context->GetContext());
  desc.storage_mode = impeller::StorageMode::kDevicePrivate;
  desc.format = impeller::PixelFormat::kR8G8B8A8UNormInt;
  desc.size = {static_cast<int>(width), static_cast<int>(height)};
  desc.mip_count = 1;
  auto textureGLES =
      std::make_shared<impeller::TextureGLES>(gl_context.GetReactor(), desc);
  if (!textureGLES->SetContents(texture->buffer, texture->buffer_size)) {
    if (texture->destruction_callback) {
      texture->destruction_callback(texture->user_data);
    }
    return nullptr;
  }
  if (texture->destruction_callback) {
    texture->destruction_callback(texture->user_data);
  }
  return impeller::DlImageImpeller::Make(textureGLES);
}

EmbedderExternalTextureGLImpellerPixelBuffer::
    ~EmbedderExternalTextureGLImpellerPixelBuffer() = default;

EmbedderExternalTextureGLImpellerSurface::
    EmbedderExternalTextureGLImpellerSurface(
        int64_t texture_identifier,
        const ExternalTextureCallback& callback)
    : EmbedderExternalTextureGL(texture_identifier, callback) {}

sk_sp<DlImage> EmbedderExternalTextureGLImpellerSurface::ResolveTexture(
    int64_t texture_id,
    PaintContext& context,
    const SkISize& size) {
  std::unique_ptr<FlutterOpenGLTexture> texture =
      external_texture_callback_(texture_id, size.width(), size.height());

  if (!texture) {
    return nullptr;
  }
  size_t width = size.width();
  size_t height = size.height();

  if (texture->width != 0 && texture->height != 0) {
    width = texture->width;
    height = texture->height;
  }

  impeller::TextureDescriptor desc;
  desc.type = impeller::TextureType::kTextureExternalOES;
  impeller::AiksContext* aiks_context = context.aiks_context;
  const auto& gl_context =
      impeller::ContextGLES::Cast(*aiks_context->GetContext());
  desc.storage_mode = impeller::StorageMode::kDevicePrivate;
  desc.format = impeller::PixelFormat::kR8G8B8A8UNormInt;
  desc.size = {static_cast<int>(width), static_cast<int>(height)};
  desc.mip_count = 1;
  auto textureGLES = std::make_shared<impeller::TextureGLES>(
      gl_context.GetReactor(), desc,
      impeller::TextureGLES::IsWrapped::kWrapped);
  textureGLES->SetCoordinateSystem(
      impeller::TextureCoordinateSystem::kUploadFromHost);
  if (!textureGLES->Bind()) {
    if (texture->destruction_callback) {
      texture->destruction_callback(texture->user_data);
    }
    return nullptr;
  }

  if (!texture->bind_callback(texture->user_data)) {
    if (texture->destruction_callback) {
      texture->destruction_callback(texture->user_data);
    }
    return nullptr;
  }

  if (texture->destruction_callback) {
    texture->destruction_callback(texture->user_data);
  }

  return impeller::DlImageImpeller::Make(textureGLES);
}

EmbedderExternalTextureGLImpellerSurface::
    ~EmbedderExternalTextureGLImpellerSurface() = default;
}  // namespace flutter
