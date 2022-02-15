// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TEXT_INPUT_CHANNEL_H_
#define EMBEDDER_TEXT_INPUT_CHANNEL_H_

#define EFL_BETA_API_SUPPORT
#include <Ecore_IMF.h>
#include <Ecore_Input.h>

#include <memory>
#include <string>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/method_channel.h"
#include "flutter/shell/platform/common/text_input_model.h"
#include "flutter/shell/platform/tizen/tizen_input_method_context.h"
#include "rapidjson/document.h"

namespace flutter {

enum class EditStatus { kNone, kPreeditStart, kPreeditEnd, kCommit };

struct TextEditingContext {
  EditStatus edit_status_ = EditStatus::kNone;
  bool is_in_select_mode_ = false;
};

class TextInputChannel {
 public:
  explicit TextInputChannel(
      BinaryMessenger* messenger,
      std::unique_ptr<TizenInputMethodContext> input_method_context);
  virtual ~TextInputChannel();

  bool IsSoftwareKeyboardShowing() { return is_software_keyboard_showing_; }

  bool SendKeyEvent(Ecore_Event_Key* key, bool is_down);

 private:
  void HandleMethodCall(
      const MethodCall<rapidjson::Document>& method_call,
      std::unique_ptr<MethodResult<rapidjson::Document>> result);
  void SendStateUpdate(const TextInputModel& model);
  bool FilterEvent(Ecore_Event_Key* event, bool is_down);
  void HandleUnfilteredEvent(Ecore_Event_Key* event);
  void EnterPressed(TextInputModel* model, bool select);
  void ResetTextEditingContext() {
    text_editing_context_ = TextEditingContext();
  }
  bool ShouldNotFilterEvent(std::string key, bool is_ime);

  std::unique_ptr<MethodChannel<rapidjson::Document>> channel_;
  std::unique_ptr<TextInputModel> active_model_;
  std::unique_ptr<TizenInputMethodContext> input_method_context_;

  int client_id_ = 0;
  bool is_software_keyboard_showing_ = false;
  std::string input_action_;
  std::string input_type_;
  TextEditingContext text_editing_context_;
};

}  // namespace flutter

#endif  // EMBEDDER_TEXT_INPUT_CHANNEL_H_
