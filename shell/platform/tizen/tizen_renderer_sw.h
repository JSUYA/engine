// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_EVAS_GL_H_
#define EMBEDDER_TIZEN_RENDERER_EVAS_GL_H_

#undef EFL_BETA_API_SUPPORT
#include <Ecore.h>
#include <Elementary.h>
#include <Evas_GL.h>

#include "flutter/shell/platform/tizen/tizen_renderer.h"

namespace flutter {

class TizenRendererSW : public TizenRenderer {
 public:
  explicit TizenRendererSW(Geometry geometry, bool transparent, bool focusable,
                           bool top_level, void* parent, Delegate& delegate);
  virtual ~TizenRendererSW();

  bool OnMakeCurrent() override;
  bool OnClearCurrent() override;
  bool OnMakeResourceCurrent() override;
  bool OnPresent() override;
  uint32_t OnGetFBO() override;
  void* OnProcResolver(const char* name) override;

  Geometry GetWindowGeometry() override;
  Geometry GetScreenGeometry() override;
  int32_t GetDpi() override;
  uintptr_t GetWindowId() override;

  void* GetWindowHandle() override { return nullptr; }
  void* GetImageHandle() override { return nullptr; }

  void SetRotate(int angle) override;
  void SetGeometry(int32_t x, int32_t y, int32_t width,
                   int32_t height) override;
  void ResizeWithRotation(int32_t x, int32_t y, int32_t width, int32_t height,
                          int32_t angle) override;
  void SetPreferredOrientations(const std::vector<int>& rotations) override;

  bool IsSupportedExtension(const char* name) override;
  void FirstFrame() override{};

 private:
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_RENDERER_EVAS_GL_H_
