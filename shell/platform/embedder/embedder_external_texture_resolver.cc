// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/embedder/embedder_external_texture_resolver.h"

#include <memory>
#include <utility>

namespace flutter {

#ifdef SHELL_ENABLE_GL
EmbedderExternalTextureResolver::EmbedderExternalTextureResolver(
    EmbedderExternalTextureGL::ExternalTextureCallback gl_callback)
    : gl_callback_(std::move(gl_callback)) {}
#endif

#ifdef SHELL_ENABLE_METAL
EmbedderExternalTextureResolver::EmbedderExternalTextureResolver(
    EmbedderExternalTextureMetal::ExternalTextureCallback metal_callback)
    : metal_callback_(std::move(metal_callback)) {}
#endif

std::unique_ptr<Texture>
EmbedderExternalTextureResolver::ResolveExternalTexture(
    int64_t texture_id,
    FlutterTextureType type) {
#ifdef SHELL_ENABLE_GL
  if (gl_callback_) {
    if (type == FlutterTextureType::kFlutterGpuSurfaceTexture) {
      return std::make_unique<EmbedderExternalTextureGLImpellerSurface>(
          texture_id, gl_callback_);
    } else {
      return std::make_unique<EmbedderExternalTextureGLImpellerPixelBuffer>(
          texture_id, gl_callback_);
    }
  }
#endif

#ifdef SHELL_ENABLE_METAL
  if (metal_callback_) {
    return std::make_unique<EmbedderExternalTextureMetal>(texture_id,
                                                          metal_callback_);
  }
#endif

  return nullptr;
}

std::unique_ptr<Texture>
EmbedderExternalTextureResolver::ResolveExternalTexture(int64_t texture_id) {
#ifdef SHELL_ENABLE_GL
  if (gl_callback_) {
    return std::make_unique<EmbedderExternalTextureSkiaGL>(texture_id,
                                                           gl_callback_);
  }
#endif

#ifdef SHELL_ENABLE_METAL
  if (metal_callback_) {
    return std::make_unique<EmbedderExternalTextureMetal>(texture_id,
                                                          metal_callback_);
  }
#endif

  return nullptr;
}

bool EmbedderExternalTextureResolver::SupportsExternalTextures() {
#ifdef SHELL_ENABLE_GL
  if (gl_callback_) {
    return true;
  }
#endif

#ifdef SHELL_ENABLE_METAL
  if (metal_callback_) {
    return true;
  }
#endif

  return false;
}

}  // namespace flutter
