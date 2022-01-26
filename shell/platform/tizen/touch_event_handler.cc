// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "touch_event_handler.h"

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"

static const int kScrollDirectionVertical = 0;
static const int kScrollDirectionHorizontal = 1;

namespace flutter {

TouchEventHandler::TouchEventHandler(FlutterTizenEngine* engine)
    : engine_(engine) {
  touch_event_handlers_.push_back(
      ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, OnTouch, this));
  touch_event_handlers_.push_back(
      ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, OnTouch, this));
  touch_event_handlers_.push_back(
      ecore_event_handler_add(ECORE_EVENT_MOUSE_WHEEL, OnTouch, this));
  touch_event_handlers_.push_back(
      ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, OnTouch, this));
  window_id_ = engine_->renderer()->GetWindowId();
}

TouchEventHandler::~TouchEventHandler() {
  for (auto handler : touch_event_handlers_) {
    ecore_event_handler_del(handler);
  }
  touch_event_handlers_.clear();
}

void TouchEventHandler::SendFlutterPointerEvent(
    FlutterPointerPhase phase, double x, double y, double scroll_delta_x,
    double scroll_delta_y, size_t timestamp, int device_id = 0) {
  // Correct errors caused by window rotation.
#ifdef TIZEN_RENDERER_EVAS_GL
  auto windowGeometry = engine_->renderer()->GetWindowGeometry();
#endif
  auto screenGeometry = engine_->renderer()->GetScreenGeometry();
  double width = screenGeometry.w;
  double height = screenGeometry.h;
  double new_x = x, new_y = y;

  if (rotation == 90) {
    new_x = height - y;
    new_y = x;
  } else if (rotation == 180) {
    new_x = width - x;
    new_y = height - y;
  } else if (rotation == 270) {
    new_x = y;
    new_y = width - x;
  }

  FlutterPointerEvent event = {};
  event.struct_size = sizeof(event);
  event.phase = phase;
#ifdef TIZEN_RENDERER_EVAS_GL
  event.x = new_x - windowGeometry.x;
  event.y = new_y - windowGeometry.y;
#else
  event.x = new_x;
  event.y = new_y;
#endif
  if (scroll_delta_x != 0 || scroll_delta_y != 0) {
    event.signal_kind = kFlutterPointerSignalKindScroll;
  }
  event.scroll_delta_x = scroll_delta_x * 2;
  event.scroll_delta_y = scroll_delta_y * 2;
  event.timestamp = timestamp * 1000;
  event.device = device_id;
  event.device_kind = kFlutterPointerDeviceKindTouch;

  engine_->SendPointerEvent(event);
}

Eina_Bool TouchEventHandler::OnTouch(void* data, int type, void* event) {
  auto* self = reinterpret_cast<TouchEventHandler*>(data);

  if (type == ECORE_EVENT_MOUSE_BUTTON_DOWN) {
    auto* button_event = reinterpret_cast<Ecore_Event_Mouse_Button*>(event);
    if (self->window_id_ == button_event->window) {
      self->pointer_state_ = true;
      self->SendFlutterPointerEvent(kDown, button_event->x, button_event->y, 0,
                                    0, button_event->timestamp,
                                    button_event->multi.device);
      return ECORE_CALLBACK_DONE;
    }

  } else if (type == ECORE_EVENT_MOUSE_BUTTON_UP) {
    auto* button_event = reinterpret_cast<Ecore_Event_Mouse_Button*>(event);
    if (self->window_id_ == button_event->window) {
      self->pointer_state_ = false;
      self->SendFlutterPointerEvent(kUp, button_event->x, button_event->y, 0, 0,
                                    button_event->timestamp,
                                    button_event->multi.device);
      return ECORE_CALLBACK_DONE;
    }
  } else if (type == ECORE_EVENT_MOUSE_MOVE) {
    auto* move_event = reinterpret_cast<Ecore_Event_Mouse_Move*>(event);
    if (self->window_id_ == move_event->window) {
      if (self->pointer_state_) {
        self->SendFlutterPointerEvent(kMove, move_event->x, move_event->y, 0, 0,
                                      move_event->timestamp,
                                      move_event->multi.device);
        return ECORE_CALLBACK_DONE;
      }
    }
  } else if (type == ECORE_EVENT_MOUSE_WHEEL) {
    auto* wheel_event = reinterpret_cast<Ecore_Event_Mouse_Wheel*>(event);
    if (self->window_id_ == wheel_event->window) {
      double scroll_delta_x = 0.0, scroll_delta_y = 0.0;
      if (wheel_event->direction == kScrollDirectionVertical) {
        scroll_delta_y += wheel_event->z;
      } else if (wheel_event->direction == kScrollDirectionHorizontal) {
        scroll_delta_x += wheel_event->z;
      }
      const int kScrollOffsetMultiplier = 20;
      scroll_delta_x *= kScrollOffsetMultiplier;
      scroll_delta_y *= kScrollOffsetMultiplier;
      self->SendFlutterPointerEvent(
          self->pointer_state_ ? kMove : kHover, wheel_event->x, wheel_event->y,
          scroll_delta_x, scroll_delta_y, wheel_event->timestamp);
      return ECORE_CALLBACK_DONE;
    }
  }
  return ECORE_CALLBACK_PASS_ON;
}

}  // namespace flutter
