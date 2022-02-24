// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_renderer_ecore_wl2.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <dali/public-api/common/intrusive-ptr.h>
#include <dali-toolkit/public-api/image-loader/image-url.h>

#include <dali-toolkit/devel-api/image-loader/texture-manager.h>

#include "flutter/shell/platform/tizen/logger.h"




namespace flutter {

using namespace Dali;

TizenRendererEcoreWl2::TizenRendererEcoreWl2(Geometry geometry,
                                             bool transparent,
                                             bool focusable,
                                             bool top_level,
                                             Delegate& delegate)
    : TizenRenderer(geometry, transparent, focusable, top_level, delegate) {
  if (!SetupEcoreWl2()) {
    FT_LOG(Error) << "Could not set up Ecore Wl2.";
    return;
  }
  if (!SetupEGL()) {
    FT_LOG(Error) << "Could not set up EGL.";
    return;
  }
  Show();

  is_valid_ = true;
}

TizenRendererEcoreWl2::~TizenRendererEcoreWl2() {
  DestroyEGL();
  DestroyEcoreWl2();
}

bool TizenRendererEcoreWl2::OnMakeCurrent() {
  if (!IsValid()) {
    return false;
  }
    
  if (eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_) !=
      EGL_TRUE) {
    PrintEGLError();
    FT_LOG(Error) << "Could not make the onscreen context current.";
    return false;
  }
  return true;
}

bool TizenRendererEcoreWl2::OnClearCurrent() {
  if (!IsValid()) {
    return false;
  }
  if (eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                     EGL_NO_CONTEXT) != EGL_TRUE) {
    PrintEGLError();
    FT_LOG(Error) << "Could not clear the context.";
    return false;
  }
  return true;
}

bool TizenRendererEcoreWl2::OnMakeResourceCurrent() {
  if (!IsValid()) {
    return false;
  }
  if (eglMakeCurrent(egl_display_, egl_resource_surface_, egl_resource_surface_,
                     egl_resource_context_) != EGL_TRUE) {
    PrintEGLError();
    FT_LOG(Error) << "Could not make the offscreen context current.";
    return false;
  }
  return true;
}

bool TizenRendererEcoreWl2::OnPresent() {
  if (!IsValid()) {
    return false;
  }

  if (received_rotation_) {
    SendRotationChangeDone();
    received_rotation_ = false;
  }

  if (eglSwapBuffers(egl_display_, egl_surface_) != EGL_TRUE) {
    PrintEGLError();
    FT_LOG(Error) << "Could not swap EGL buffers.";
    return false;
  }
  return true;
}

uint32_t TizenRendererEcoreWl2::OnGetFBO() {
  if (!IsValid()) {
    return 999;
  }
  return 0;
}

void* TizenRendererEcoreWl2::OnProcResolver(const char* name) {
  auto address = eglGetProcAddress(name);
  if (address != nullptr) {
    return reinterpret_cast<void*>(address);
  }
#define GL_FUNC(FunctionName)                     \
  else if (strcmp(name, #FunctionName) == 0) {    \
    return reinterpret_cast<void*>(FunctionName); \
  }
  GL_FUNC(eglGetCurrentDisplay)
  GL_FUNC(eglQueryString)
  GL_FUNC(glActiveTexture)
  GL_FUNC(glAttachShader)
  GL_FUNC(glBindAttribLocation)
  GL_FUNC(glBindBuffer)
  GL_FUNC(glBindFramebuffer)
  GL_FUNC(glBindRenderbuffer)
  GL_FUNC(glBindTexture)
  GL_FUNC(glBlendColor)
  GL_FUNC(glBlendEquation)
  GL_FUNC(glBlendFunc)
  GL_FUNC(glBufferData)
  GL_FUNC(glBufferSubData)
  GL_FUNC(glCheckFramebufferStatus)
  GL_FUNC(glClear)
  GL_FUNC(glClearColor)
  GL_FUNC(glClearStencil)
  GL_FUNC(glColorMask)
  GL_FUNC(glCompileShader)
  GL_FUNC(glCompressedTexImage2D)
  GL_FUNC(glCompressedTexSubImage2D)
  GL_FUNC(glCopyTexSubImage2D)
  GL_FUNC(glCreateProgram)
  GL_FUNC(glCreateShader)
  GL_FUNC(glCullFace)
  GL_FUNC(glDeleteBuffers)
  GL_FUNC(glDeleteFramebuffers)
  GL_FUNC(glDeleteProgram)
  GL_FUNC(glDeleteRenderbuffers)
  GL_FUNC(glDeleteShader)
  GL_FUNC(glDeleteTextures)
  GL_FUNC(glDepthMask)
  GL_FUNC(glDisable)
  GL_FUNC(glDisableVertexAttribArray)
  GL_FUNC(glDrawArrays)
  GL_FUNC(glDrawElements)
  GL_FUNC(glEnable)
  GL_FUNC(glEnableVertexAttribArray)
  GL_FUNC(glFinish)
  GL_FUNC(glFlush)
  GL_FUNC(glFramebufferRenderbuffer)
  GL_FUNC(glFramebufferTexture2D)
  GL_FUNC(glFrontFace)
  GL_FUNC(glGenBuffers)
  GL_FUNC(glGenerateMipmap)
  GL_FUNC(glGenFramebuffers)
  GL_FUNC(glGenRenderbuffers)
  GL_FUNC(glGenTextures)
  GL_FUNC(glGetBufferParameteriv)
  GL_FUNC(glGetError)
  GL_FUNC(glGetFramebufferAttachmentParameteriv)
  GL_FUNC(glGetIntegerv)
  GL_FUNC(glGetProgramInfoLog)
  GL_FUNC(glGetProgramiv)
  GL_FUNC(glGetRenderbufferParameteriv)
  GL_FUNC(glGetShaderInfoLog)
  GL_FUNC(glGetShaderiv)
  GL_FUNC(glGetShaderPrecisionFormat)
  GL_FUNC(glGetString)
  GL_FUNC(glGetUniformLocation)
  GL_FUNC(glIsTexture)
  GL_FUNC(glLineWidth)
  GL_FUNC(glLinkProgram)
  GL_FUNC(glPixelStorei)
  GL_FUNC(glReadPixels)
  GL_FUNC(glRenderbufferStorage)
  GL_FUNC(glScissor)
  GL_FUNC(glShaderSource)
  GL_FUNC(glStencilFunc)
  GL_FUNC(glStencilFuncSeparate)
  GL_FUNC(glStencilMask)
  GL_FUNC(glStencilMaskSeparate)
  GL_FUNC(glStencilOp)
  GL_FUNC(glStencilOpSeparate)
  GL_FUNC(glTexImage2D)
  GL_FUNC(glTexParameterf)
  GL_FUNC(glTexParameterfv)
  GL_FUNC(glTexParameteri)
  GL_FUNC(glTexParameteriv)
  GL_FUNC(glTexSubImage2D)
  GL_FUNC(glUniform1f)
  GL_FUNC(glUniform1fv)
  GL_FUNC(glUniform1i)
  GL_FUNC(glUniform1iv)
  GL_FUNC(glUniform2f)
  GL_FUNC(glUniform2fv)
  GL_FUNC(glUniform2i)
  GL_FUNC(glUniform2iv)
  GL_FUNC(glUniform3f)
  GL_FUNC(glUniform3fv)
  GL_FUNC(glUniform3i)
  GL_FUNC(glUniform3iv)
  GL_FUNC(glUniform4f)
  GL_FUNC(glUniform4fv)
  GL_FUNC(glUniform4i)
  GL_FUNC(glUniform4iv)
  GL_FUNC(glUniformMatrix2fv)
  GL_FUNC(glUniformMatrix3fv)
  GL_FUNC(glUniformMatrix4fv)
  GL_FUNC(glUseProgram)
  GL_FUNC(glVertexAttrib1f)
  GL_FUNC(glVertexAttrib2fv)
  GL_FUNC(glVertexAttrib3fv)
  GL_FUNC(glVertexAttrib4fv)
  GL_FUNC(glVertexAttribPointer)
  GL_FUNC(glViewport)
#undef GL_FUNC

  FT_LOG(Warn) << "Could not resolve: " << name;
  return nullptr;
}

TizenRenderer::Geometry TizenRendererEcoreWl2::GetWindowGeometry() {
  Geometry result = {0,0,0,0};
  //ecore_wl2_window_geometry_get(ecore_wl2_window_, &result.x, &result.y,
//                                &result.w, &result.h);
  return result;
}

TizenRenderer::Geometry TizenRendererEcoreWl2::GetScreenGeometry() {
  Geometry result = {};
  //ecore_wl2_display_screen_size_get(ecore_wl2_display_, &result.w, &result.h);
  return result;
}

int32_t TizenRendererEcoreWl2::GetDpi() {
  /*auto* output = ecore_wl2_window_output_find(ecore_wl2_window_);
  if (!output) {
    FT_LOG(Error) << "Could not find an output associated with the window.";
    return 0;
  }*/
  return 0;//ecore_wl2_output_dpi_get(output);
}

uintptr_t TizenRendererEcoreWl2::GetWindowId() {
  return 0;//ecore_wl2_window_id_get(ecore_wl2_window_);
}

void TizenRendererEcoreWl2::Show() {
  //ecore_wl2_window_show(ecore_wl2_window_);
}

bool TizenRendererEcoreWl2::SetupEcoreWl2() {
  if (!ecore_wl2_init()) {
    FT_LOG(Error) << "Could not initialize Ecore Wl2.";
    return false;
  }

  ecore_wl2_display_ = ecore_wl2_display_connect(nullptr);
  if (!ecore_wl2_display_) {
    FT_LOG(Error) << "Ecore Wl2 display not found.";
    return false;
  }
  ecore_wl2_display_sync(ecore_wl2_display_);
  //ecore_wl2_sync();

  int32_t width = 0, height = 0;
  ecore_wl2_display_screen_size_get(ecore_wl2_display_, &width, &height);
  if (width == 0 || height == 0) {
    FT_LOG(Error) << "Invalid screen size: " << width << " x " << height;
    return false;
  }
  FT_LOG(Error) << "CJS screen size: " << width << " x " << height;

  if (initial_geometry_.w == 0) {
    initial_geometry_.w = width;
  }
  if (initial_geometry_.h == 0) {
    initial_geometry_.h = height;
  }
/*
  ecore_wl2_window_ = ecore_wl2_window_new(
      ecore_wl2_display_, nullptr, initial_geometry_.x, initial_geometry_.y,
      initial_geometry_.w, initial_geometry_.h);*/
                                        

  // Change the window type to use the tizen policy for notification window
  // according to top_level_.
  // Note: ECORE_WL2_WINDOW_TYPE_TOPLEVEL is similar to "ELM_WIN_BASIC" and it
  // does not mean that the window always will be overlaid on other apps :(
  /*ecore_wl2_window_type_set(ecore_wl2_window_,
                            top_level_ ? ECORE_WL2_WINDOW_TYPE_NOTIFICATION
                                       : ECORE_WL2_WINDOW_TYPE_TOPLEVEL);*/
  /*if (top_level_) {
    SetTizenPolicyNotificationLevel(TIZEN_POLICY_LEVEL_TOP);
  }*/
/*
  ecore_wl2_window_position_set(ecore_wl2_window_, initial_geometry_.x,
                                initial_geometry_.y);
  ecore_wl2_window_aux_hint_add(ecore_wl2_window_, 0,
                                "wm.policy.win.user.geometry", "1");

  if (transparent_) {
    ecore_wl2_window_alpha_set(ecore_wl2_window_, EINA_TRUE);
  } else {
    ecore_wl2_window_alpha_set(ecore_wl2_window_, EINA_FALSE);
  }

  if (!focusable_) {
    ecore_wl2_window_focus_skip_set(ecore_wl2_window_, EINA_TRUE);
  }

  ecore_wl2_window_indicator_state_set(ecore_wl2_window_,
                                       ECORE_WL2_INDICATOR_STATE_ON);
  ecore_wl2_window_indicator_opacity_set(ecore_wl2_window_,
                                         ECORE_WL2_INDICATOR_OPAQUE);
  ecore_wl2_indicator_visible_type_set(ecore_wl2_window_,
                                       ECORE_WL2_INDICATOR_VISIBLE_TYPE_SHOWN);

  int rotations[4] = {0, 90, 180, 270};
  ecore_wl2_window_available_rotations_set(ecore_wl2_window_, rotations,
                                           sizeof(rotations) / sizeof(int));*/
/*  ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_ROTATE, RotationEventCb, this);
*/
  return true;
}

bool TizenRendererEcoreWl2::SetupEGL() {

  //Dali::Mutex::ScopedLock lock( mMutex );

  /*ecore_wl2_egl_window_ = ecore_wl2_egl_window_create(
      ecore_wl2_window_, initial_geometry_.w, initial_geometry_.h);
  if (!ecore_wl2_egl_window_) {
    FT_LOG(Error) << "Could not create an EGL window.";
    return false;
  }*/

  

  if (!ChooseEGLConfiguration()) {
    FT_LOG(Error) << "Could not choose an EGL configuration.";
    return false;
  }

  egl_extension_str_ = eglQueryString(egl_display_, EGL_EXTENSIONS);

  {
    const EGLint attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

    egl_context_ =
        eglCreateContext(egl_display_, egl_config_, EGL_NO_CONTEXT, attribs);
    if (egl_context_ == EGL_NO_CONTEXT) {
      PrintEGLError();
      FT_LOG(Error) << "Could not create an onscreen context.";
      return false;
    }

    egl_resource_context_ =
        eglCreateContext(egl_display_, egl_config_, egl_context_, attribs);
    if (egl_resource_context_ == EGL_NO_CONTEXT) {
      PrintEGLError();
      FT_LOG(Error) << "Could not create an offscreen context.";
      return false;
    }
  }

  {
    const EGLint attribs[] = {EGL_NONE};
    
    


  //  auto* egl_window = static_cast<EGLNativeWindowType*>(
    //    ecore_wl2_egl_window_native_get(ecore_wl2_egl_window_));
    
    FT_LOG(Error) << "CJS Create tbm surface";
    mTbmQueue = tbm_surface_queue_create(3 /* TBM_SURFACE_QUEUE_SIZE */, 500, 500, TBM_FORMAT_ARGB8888, 0);
    FT_LOG(Error) << "CJS Call NativeImageSourceQueue New";
    mNativeImageQueue = Dali::NativeImageSourceQueue::New(mTbmQueue);
    mNativeTexture = Dali::Texture::New(*mNativeImageQueue);
    FT_LOG(Error) << "CJS Call texture " + std::to_string(mNativeTexture.GetHeight());
    Dali::Toolkit::ImageUrl imageUrl = Dali::Toolkit::ImageUrl::New(mNativeTexture);
    
    const std::string a = Dali::Toolkit::TextureManager::AddTexture(mNativeTexture);

    //const std::string a = imageUrl.GetUrl();
    //if(mImageUrl)
     // FT_LOG(Error) << "CJS mImageUrl " + std::string(mImageUrl);

    //Dali::NativeImageSourcePtr mNativeImageSourcePtr = nullptr;
    //Dali::Toolkit::ImageUrl imageUrl2 =  Dali::Toolkit::Image::GenerateUrl(mNativeImageSourcePtr);

    //std::string mUrl = Dali::Toolkit::TextureManager::AddTexture(mNativeTexture);

    //mImageUrl = s.c_str();

   



    //Dali::ImageUrl u  = Dali::Toolkit::Image::GenerateUrl(mNativeImageQueue);

    auto egl_window = reinterpret_cast<EGLNativeWindowType>(mTbmQueue);
    //auto* egl_window = static_cast<EGLNativeWindowType*>(
    //    ecore_wl2_egl_window_native_get(ecore_wl2_egl_window_));

    egl_surface_ =
        eglCreateWindowSurface(egl_display_, egl_config_, egl_window, attribs);

    FT_LOG(Error) << "CJS Create WindowSurface";

    if (egl_surface_ == EGL_NO_SURFACE) {
      FT_LOG(Error) << "Could not create an onscreen window surface.";
      return false;
    }
  }

  {
    const EGLint attribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};

    egl_resource_surface_ =
        eglCreatePbufferSurface(egl_display_, egl_config_, attribs);
    if (egl_resource_surface_ == EGL_NO_SURFACE) {
      FT_LOG(Error) << "Could not create an offscreen window surface.";
      return false;
    }
  }

  return true;
}

bool TizenRendererEcoreWl2::ChooseEGLConfiguration() {
  EGLint config_attribs[] = {
      // clang-format off
      EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
      EGL_RED_SIZE,        8,
      EGL_GREEN_SIZE,      8,
      EGL_BLUE_SIZE,       8,
      EGL_ALPHA_SIZE,      EGL_DONT_CARE,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_SAMPLE_BUFFERS,  EGL_DONT_CARE,
      EGL_SAMPLES,         EGL_DONT_CARE,
      EGL_NONE
      // clang-format on
  };

  egl_display_ = eglGetDisplay(ecore_wl2_display_get(ecore_wl2_display_));
  //egl_display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (EGL_NO_DISPLAY == egl_display_) {
    PrintEGLError();
    FT_LOG(Error) << "Could not get EGL display.";
    return false;
  }

  if (!eglInitialize(egl_display_, nullptr, nullptr)) {
    PrintEGLError();
    FT_LOG(Error) << "Could not initialize the EGL display.";
    return false;
  }

  if (!eglBindAPI(EGL_OPENGL_ES_API)) {
    PrintEGLError();
    FT_LOG(Error) << "Could not bind the ES API.";
    return false;
  }

  EGLint config_size = 0;
  if (!eglGetConfigs(egl_display_, nullptr, 0, &config_size)) {
    PrintEGLError();
    FT_LOG(Error) << "Could not query framebuffer configurations.";
    return false;
  }

  EGLConfig* configs = (EGLConfig*)calloc(config_size, sizeof(EGLConfig));
  EGLint num_config;
  if (!eglChooseConfig(egl_display_, config_attribs, configs, config_size,
                       &num_config)) {
    free(configs);
    PrintEGLError();
    FT_LOG(Error) << "No matching configurations found.";
    return false;
  }

  int buffer_size = 32;
  EGLint size;
  for (int i = 0; i < num_config; i++) {
    eglGetConfigAttrib(egl_display_, configs[i], EGL_BUFFER_SIZE, &size);
    if (buffer_size == size) {
      egl_config_ = configs[i];
      break;
    }
  }
  free(configs);
  if (!egl_config_) {
    FT_LOG(Error) << "No matching configuration found.";
    return false;
  }

  return true;
}

void TizenRendererEcoreWl2::PrintEGLError() {
  EGLint error = eglGetError();
  switch (error) {
#define CASE_PRINT(value)                     \
  case value: {                               \
    FT_LOG(Error) << "EGL error: " << #value; \
    break;                                    \
  }
    CASE_PRINT(EGL_NOT_INITIALIZED)
    CASE_PRINT(EGL_BAD_ACCESS)
    CASE_PRINT(EGL_BAD_ALLOC)
    CASE_PRINT(EGL_BAD_ATTRIBUTE)
    CASE_PRINT(EGL_BAD_CONTEXT)
    CASE_PRINT(EGL_BAD_CONFIG)
    CASE_PRINT(EGL_BAD_CURRENT_SURFACE)
    CASE_PRINT(EGL_BAD_DISPLAY)
    CASE_PRINT(EGL_BAD_SURFACE)
    CASE_PRINT(EGL_BAD_MATCH)
    CASE_PRINT(EGL_BAD_PARAMETER)
    CASE_PRINT(EGL_BAD_NATIVE_PIXMAP)
    CASE_PRINT(EGL_BAD_NATIVE_WINDOW)
    CASE_PRINT(EGL_CONTEXT_LOST)
#undef CASE_PRINT
    default: {
      FT_LOG(Error) << "Unknown EGL error: " << error;
    }
  }
}

void TizenRendererEcoreWl2::DestroyEcoreWl2() {
  /*if (ecore_wl2_window_) {
    ecore_wl2_window_free(ecore_wl2_window_);
    ecore_wl2_window_ = nullptr;
  }*/

  if (ecore_wl2_display_) {
    ecore_wl2_display_disconnect(ecore_wl2_display_);
    ecore_wl2_display_ = nullptr;
  }
  ecore_wl2_shutdown();
}

void TizenRendererEcoreWl2::DestroyEGL() {
  if (egl_display_) {
    eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                   EGL_NO_CONTEXT);

    if (EGL_NO_SURFACE != egl_surface_) {
      eglDestroySurface(egl_display_, egl_surface_);
      egl_surface_ = EGL_NO_SURFACE;
    }

    if (EGL_NO_CONTEXT != egl_context_) {
      eglDestroyContext(egl_display_, egl_context_);
      egl_context_ = EGL_NO_CONTEXT;
    }

    if (EGL_NO_SURFACE != egl_resource_surface_) {
      eglDestroySurface(egl_display_, egl_resource_surface_);
      egl_resource_surface_ = EGL_NO_SURFACE;
    }

    if (EGL_NO_CONTEXT != egl_resource_context_) {
      eglDestroyContext(egl_display_, egl_resource_context_);
      egl_resource_context_ = EGL_NO_CONTEXT;
    }

    eglTerminate(egl_display_);
    egl_display_ = EGL_NO_DISPLAY;
  }

  /*if (ecore_wl2_egl_window_) {
    ecore_wl2_egl_window_destroy(ecore_wl2_egl_window_);
    ecore_wl2_egl_window_ = nullptr;
  }*/
}

Eina_Bool TizenRendererEcoreWl2::RotationEventCb(void* data,
                                                 int type,
                                                 void* event) {
  auto* self = reinterpret_cast<TizenRendererEcoreWl2*>(data);
  auto* rotation_event =
      reinterpret_cast<Ecore_Wl2_Event_Window_Rotation*>(event);
  self->delegate_.OnOrientationChange(rotation_event->angle);
  return ECORE_CALLBACK_PASS_ON;
}

void TizenRendererEcoreWl2::SetRotate(int angle) {
  //ecore_wl2_window_rotation_set(ecore_wl2_window_, angle);
  received_rotation_ = true;
}

void TizenRendererEcoreWl2::SetGeometry(int32_t x,
                                        int32_t y,
                                        int32_t width,
                                        int32_t height) {
  /*ecore_wl2_window_geometry_set(ecore_wl2_window_, x, y, width, height);
  ecore_wl2_window_position_set(ecore_wl2_window_, x, y);*/
}

void TizenRendererEcoreWl2::ResizeWithRotation(int32_t x,
                                               int32_t y,
                                               int32_t width,
                                               int32_t height,
                                               int32_t angle) {
  /*ecore_wl2_egl_window_resize_with_rotation(ecore_wl2_egl_window_, x, y, width,
                                            height, angle);*/
}

void TizenRendererEcoreWl2::SendRotationChangeDone() {
  //int x, y, w, h;
  /*ecore_wl2_window_geometry_get(ecore_wl2_window_, &x, &y, &w, &h);
  ecore_wl2_window_rotation_change_done_send(
      ecore_wl2_window_, ecore_wl2_window_rotation_get(ecore_wl2_window_), w,
      h);*/
}

void TizenRendererEcoreWl2::SetPreferredOrientations(
    const std::vector<int>& rotations) {
  /*ecore_wl2_window_available_rotations_set(ecore_wl2_window_, rotations.data(),
                                           rotations.size());*/
}

bool TizenRendererEcoreWl2::IsSupportedExtension(const char* name) {
  return strstr(egl_extension_str_.c_str(), name);
}

void TizenRendererEcoreWl2::SetTizenPolicyNotificationLevel(int level) {
  /*Eina_Iterator* iter = ecore_wl2_display_globals_get(ecore_wl2_display_);
  struct wl_registry* registry =
      ecore_wl2_display_registry_get(ecore_wl2_display_);

  if (iter && registry) {
    Ecore_Wl2_Global* global = nullptr;

    // Retrieve global objects to bind tizen policy
    EINA_ITERATOR_FOREACH(iter, global) {
      if (strcmp(global->interface, tizen_policy_interface.name) == 0) {
        tizen_policy_ = static_cast<tizen_policy*>(
            wl_registry_bind(registry, global->id, &tizen_policy_interface, 1));
        break;
      }
    }
  }
  eina_iterator_free(iter);

  if (tizen_policy_ == nullptr) {
    FT_LOG(Error)
        << "Failed to initialize the tizen policy handle, the top_level "
           "attribute is ignored.";
    return;
  }

  tizen_policy_set_notification_level(
      tizen_policy_, ecore_wl2_window_surface_get(ecore_wl2_window_), level);*/
}

void TizenRendererEcoreWl2::BindKeys(const std::vector<std::string>& keys) {
  /*for (const auto& key : keys) {
    ecore_wl2_window_keygrab_set(ecore_wl2_window_, key.c_str(), 0, 0, 0,
                                 ECORE_WL2_WINDOW_KEYGRAB_TOPMOST);
  }*/
}

}  // namespace flutter
