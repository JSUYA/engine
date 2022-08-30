// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/tizen_view_nui.h"

#include <dali/devel-api/common/stage.h>
#include <efl_extension.h>
#include <ui/efl_util.h>

#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/tizen_view_event_handler_delegate.h"

namespace flutter {

TizenViewNui::TizenViewNui(int32_t width,
                           int32_t height,
                           void* image_view,
                           void* native_image_queue,
                           int default_window_id)
    : TizenView(width, height),
      image_view_(reinterpret_cast<Dali::Toolkit::ImageView*>(image_view)),
      native_image_queue_(
          reinterpret_cast<Dali::NativeImageSourceQueue*>(native_image_queue)),
      default_window_id_(default_window_id),
      keepRenderingEventThreadCallback_(nullptr) {
  RegisterEventHandlers();
  PrepareInputMethod();
  Show();
}

TizenViewNui::~TizenViewNui() {
  UnregisterEventHandlers();
}

void TizenViewNui::RegisterEventHandlers() {
  keepRenderingEventThreadCallback_ =
      std::make_unique<Dali::EventThreadCallback>(Dali::MakeCallback(
          this, &TizenViewNui::OnKeepRenderingEventThreadCallback));
}

void TizenViewNui::UnregisterEventHandlers() {
  keepRenderingEventThreadCallback_.release();
}

TizenGeometry TizenViewNui::GetGeometry() {
  auto size = image_view_->GetProperty(Dali::Actor::Property::SIZE)
                  .Get<Dali::Vector2>();
  TizenGeometry result = {0, 0, static_cast<int32_t>(size.width),
                          static_cast<int32_t>(size.height)};
  return result;
}

bool TizenViewNui::SetGeometry(TizenGeometry geometry) {
  image_view_->SetProperty(Dali::Actor::Property::SIZE,
                           Dali::Vector2(geometry.width, geometry.height));

  native_image_queue_->SetSize(static_cast<uint32_t>(geometry.width),
                               static_cast<uint32_t>(geometry.height));

  view_delegate_->OnResize(0, 0, geometry.width, geometry.height);
  return true;
}

int32_t TizenViewNui::GetDpi() {
  Dali::Vector2 dpi = Dali::Stage::GetCurrent().GetDpi();
  auto resultDpi = static_cast<int32_t>((dpi.height + dpi.width) * 0.5);
  return resultDpi;
}

uintptr_t TizenViewNui::GetWindowId() {
  return default_window_id_;
}

void TizenViewNui::Show() {
  // Do nothing.
}

void TizenViewNui::PrepareInputMethod() {
  input_method_context_ =
      std::make_unique<TizenInputMethodContext>(GetWindowId());

  // Set input method callbacks.
  input_method_context_->SetOnPreeditStart(
      [this]() { view_delegate_->OnComposeBegin(); });
  input_method_context_->SetOnPreeditChanged(
      [this](std::string str, int cursor_pos) {
        view_delegate_->OnComposeChange(str, cursor_pos);
      });
  input_method_context_->SetOnPreeditEnd(
      [this]() { view_delegate_->OnComposeEnd(); });
  input_method_context_->SetOnCommit(
      [this](std::string str) { view_delegate_->OnCommit(str); });
}

void TizenViewNui::OnKeepRenderingEventThreadCallback() {
  Dali::Stage::GetCurrent().KeepRendering(0.0f);
}

}  // namespace flutter
