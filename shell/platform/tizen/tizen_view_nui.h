// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_VIEW_NUI_H_
#define EMBEDDER_TIZEN_VIEW_NUI_H_

#include <dali-toolkit/public-api/controls/image-view/image-view.h>
#include <dali/devel-api/adaptor-framework/event-thread-callback.h>
#include <dali/devel-api/adaptor-framework/native-image-source-queue.h>
#include <dali/devel-api/common/stage.h>

#include <memory>

#include "flutter/shell/platform/tizen/tizen_view.h"

namespace flutter {

class TizenViewNui : public TizenView {
 public:
  TizenViewNui(int32_t width,
               int32_t height,
               Dali::Toolkit::ImageView* image_view,
               Dali::NativeImageSourceQueuePtr native_image_queue,
               int32_t default_window_id);

  ~TizenViewNui();

  TizenGeometry GetGeometry() override;

  bool SetGeometry(TizenGeometry geometry) override;

  void* GetRenderTarget() override { return native_image_queue_.Get(); }

  void* GetNativeHandle() override { return image_view_; }

  int32_t GetDpi() override;

  uintptr_t GetWindowId() override;

  void Show() override;

  Dali::EventThreadCallback* updateRenderCallback() {
    return updateRenderCallback_.get();
  };

 private:
  void RegisterEventHandlers();

  void UnregisterEventHandlers();

  void PrepareInputMethod();

  void OnUpdateRenderCallback();

  Dali::Toolkit::ImageView* image_view_;
  Dali::NativeImageSourceQueuePtr native_image_queue_;
  int32_t default_window_id_;
  std::unique_ptr<Dali::EventThreadCallback> updateRenderCallback_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_VIEW_NUI_H_
