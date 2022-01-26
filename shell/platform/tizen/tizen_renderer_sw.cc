// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_renderer_sw.h"

#include "tizen_evas_gl_helper.h"
Evas_GL* g_evas_gl = nullptr;
EVAS_GL_GLOBAL_GLES3_DEFINE();

#include "flutter/shell/platform/tizen/logger.h"

#ifndef __X64_SHELL__
#include <ui/efl_util.h>
#endif

namespace flutter {

TizenRendererSW::TizenRendererSW(Geometry geometry, bool transparent,
                                 bool focusable, bool top_level,
                                 void* parent_elm_window, Delegate& delegate)
    : TizenRenderer(geometry, transparent, focusable, top_level, delegate) {
  is_valid_ = true;
}

TizenRendererSW::~TizenRendererSW() {}

bool TizenRendererSW::OnMakeCurrent() {
  if (!IsValid()) {
    return false;
  }

  return true;
}

bool TizenRendererSW::OnClearCurrent() {
  if (!IsValid()) {
    return false;
  }

  return true;
}

bool TizenRendererSW::OnMakeResourceCurrent() {
  if (!IsValid()) {
    return false;
  }

  return true;
}

bool TizenRendererSW::OnPresent() {
  if (!IsValid()) {
    return false;
  }

  return true;
}

uint32_t TizenRendererSW::OnGetFBO() {
  if (!IsValid()) {
    return 999;
  }
  return 0;
}

void* TizenRendererSW::OnProcResolver(const char* name) {
  FT_LOG(Warn) << "Could not resolve: " << name;
  return nullptr;
}

TizenRenderer::Geometry TizenRendererSW::GetWindowGeometry() {
  Geometry result = {};

  return result;
}

TizenRenderer::Geometry TizenRendererSW::GetScreenGeometry() {
  Geometry result = {};

  return result;
}

int32_t TizenRendererSW::GetDpi() { return 0; }

uintptr_t TizenRendererSW::GetWindowId() { return 0; }

}  // namespace flutter
