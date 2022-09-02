// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_VIEW_ELEMENTARY_H_
#define EMBEDDER_TIZEN_VIEW_ELEMENTARY_H_

#define EFL_BETA_API_SUPPORT
#include <Ecore.h>
#include <Elementary.h>

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "flutter/shell/platform/tizen/tizen_view.h"

namespace flutter {

class TizenViewElementary : public TizenView {
 public:
  TizenViewElementary(int32_t width, int32_t height, Evas_Object* parent);

  ~TizenViewElementary();

  TizenGeometry GetGeometry() override;

  bool SetGeometry(TizenGeometry geometry) override;

  void* GetRenderTarget() override { return image_; }

  void* GetNativeHandle() override { return container_; }

  int32_t GetDpi() override;

  uintptr_t GetWindowId() override;

  void Show() override;

 private:
  bool CreateView();

  void DestroyView();

  void RegisterEventHandlers();

  void UnregisterEventHandlers();

  void PrepareInputMethod();

  Evas_Object* parent_ = nullptr;
  Evas_Object* container_ = nullptr;
  Evas_Object* image_ = nullptr;
  Evas_Object* event_layer_ = nullptr;

  std::unordered_map<Evas_Callback_Type, Evas_Object_Event_Cb>
      evas_object_callbacks_;
  std::vector<Ecore_Event_Handler*> ecore_event_key_handlers_;

  bool scroll_hold_ = false;

  uint32_t last_down_event_time_ = 0;
  uint32_t last_up_event_time_ = 0;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_VIEW_ELEMENTARY_H_
