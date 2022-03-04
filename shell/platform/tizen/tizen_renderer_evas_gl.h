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

class TizenRendererEvasGL : public TizenRenderer {
 public:
  explicit TizenRendererEvasGL(Geometry geometry,
                               bool transparent,
                               bool focusable,
                               bool top_level,
                               Delegate& delegate);
  virtual ~TizenRendererEvasGL();

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

  void* GetWindowHandle() override { return evas_window_; }
  void SetBuffer(void* buffer) override;
  
  void* GetBuffer() override { return mBuffer; }
  void SetUpdateCallback(void* updateCallback) override { }

  bool PresentSoftwareBitmap(const void* allocation, size_t row_bytes, size_t height) override;

  Evas_Object* GetImageHandle() { return graphics_adapter_; }

  void SetRotate(int angle) override;
  void SetGeometry(int32_t x,
                   int32_t y,
                   int32_t width,
                   int32_t height) override;
  void ResizeWithRotation(int32_t x,
                          int32_t y,
                          int32_t width,
                          int32_t height,
                          int32_t angle) override;
  void SetPreferredOrientations(const std::vector<int>& rotations) override;

  bool IsSupportedExtension(const char* name) override;

  void BindKeys(const std::vector<std::string>& keys) override;

 private:
  void Show();

  bool SetupEvasWindow();
  bool SetupEvasGL();
  void DestroyEvasWindow();
  void DestroyEvasGL();

  static void RotationEventCb(void* data, Evas_Object* obj, void* event_info);
  void SendRotationChangeDone();

  Evas_Object* evas_window_ = nullptr;
  Evas_Object* graphics_adapter_ = nullptr;

  Evas_GL* evas_gl_ = nullptr;
  Evas_GL_Config* gl_config_ = nullptr;
  Evas_GL_Context* gl_context_ = nullptr;
  Evas_GL_Context* gl_resource_context_ = nullptr;
  Evas_GL_Surface* gl_surface_ = nullptr;
  Evas_GL_Surface* gl_resource_surface_ = nullptr;



  void* mBuffer = nullptr;
  const void* mAlloc = nullptr;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_RENDERER_EVAS_GL_H_
