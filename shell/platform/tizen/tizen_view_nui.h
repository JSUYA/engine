// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_VIEW_NUI_H_
#define EMBEDDER_TIZEN_VIEW_NUI_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <dali-toolkit/public-api/controls/image-view/image-view.h>
#include <dali/devel-api/adaptor-framework/event-thread-callback.h>
#include <dali/devel-api/adaptor-framework/native-image-source-queue.h>
#include <dali/devel-api/common/stage.h>

#include "flutter/shell/platform/tizen/tizen_view.h"

namespace flutter {

class TizenViewNui : public TizenView {
 public:
  TizenViewNui(int32_t width,
               int32_t height,
               void* image_view,
               void* native_image_queue,
               int default_window_id);

  ~TizenViewNui();

  TizenGeometry GetGeometry() override;

  bool SetGeometry(TizenGeometry geometry) override;

  void* GetRenderTarget() override { return native_image_queue_.Get(); }

  void* GetRenderTargetContainer() override { return image_view_; }

  int32_t GetDpi() override;

  uintptr_t GetWindowId() override;

  void Show() override;

  Dali::EventThreadCallback* KeepRenderingEventThreadCallback() {
    return keepRenderingEventThreadCallback_.get();
  };

 private:
  void RegisterEventHandlers();

  void UnregisterEventHandlers();

  void PrepareInputMethod();

  void OnKeepRenderingEventThreadCallback();

  Dali::Toolkit::ImageView* image_view_;
  Dali::NativeImageSourceQueuePtr native_image_queue_;
  int default_window_id_;
  std::unique_ptr<Dali::EventThreadCallback> keepRenderingEventThreadCallback_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_VIEW_NUI_H_
