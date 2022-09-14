// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_context_vulkan.h"

#include <Ecore_Wl2.h>
#include <set>

#include <vulkan/vulkan_wayland.h>
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

const int MAX_FRAMES_IN_FLIGHT = 3;

TizenContextVulkan::TizenContextVulkan() {
  // Create instance.
  VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "Flutter Engine",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "Flutter Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_MAKE_VERSION(1, 1, 0),
  };

  instance_extensions_ = {"VK_KHR_surface", "VK_KHR_wayland_surface"};
  const VkInstanceCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .pApplicationInfo = &app_info,
      .enabledExtensionCount =
          static_cast<uint32_t>(instance_extensions_.size()),
      .ppEnabledExtensionNames = instance_extensions_.data(),
  };

  if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS) {
    FT_LOG(Error) << "Create instance failed.";
  }

  // Get vkInstace proc address
  GetInstanceProcAddr = reinterpret_cast<void* (*)(VkInstance, const char*)>(
      &vkGetInstanceProcAddr);
  if (!GetInstanceProcAddr) {
    FT_LOG(Error) << "Could not acquire vkGetInstanceProcAddr.";
  }
}

TizenContextVulkan::~TizenContextVulkan() {
  size_t i = 0;
  for (; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyFence(logical_device_, image_ready_fence_[i], nullptr);
    vkDestroyFence(logical_device_, submit_done_fence_[i], nullptr);
    vkDestroySemaphore(logical_device_, present_transition_semaphore_[i],
                       nullptr);
  }

  for (i = 0; i < swapchain_images_.size(); i++) {
    vkFreeCommandBuffers(logical_device_, swapchain_command_pool_, 1,
                         &present_transition_buffers_[i]);
    vkDestroyImage(logical_device_, swapchain_images_[i], nullptr);
  }
  vkDestroyCommandPool(logical_device_, swapchain_command_pool_, nullptr);

  vkDestroySwapchainKHR(logical_device_, swapchain_, nullptr);
  vkDestroySurfaceKHR(instance_, surface_, nullptr);
  vkDestroyDevice(logical_device_, nullptr);
  vkDestroyInstance(instance_, nullptr);
}

bool TizenContextVulkan::CreateSurface(void* render_target,
                                       void* render_target_display,
                                       int32_t width,
                                       int32_t height) {
  FT_LOG(Info) << "TizenContextVulkan::CreateSurface";

  VkWaylandSurfaceCreateInfoKHR createInfo;
  memset(&createInfo, 0, sizeof(createInfo));
  createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
  createInfo.pNext = NULL;
  createInfo.flags = 0;
  createInfo.display = static_cast<wl_display*>(render_target_display);
  createInfo.surface = static_cast<wl_surface*>(render_target);

  PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR;
  vkCreateWaylandSurfaceKHR =
      (PFN_vkCreateWaylandSurfaceKHR)GetInstanceProcAddr(
          instance_, "vkCreateWaylandSurfaceKHR");

  if (!vkCreateWaylandSurfaceKHR) {
    FT_LOG(Error)
        << "Wayland: Vulkan instance missing VK_KHR_wayland_surface extension";
    return false;
  }

  VkResult err =
      vkCreateWaylandSurfaceKHR(instance_, &createInfo, NULL, &surface_);
  if (err != VK_SUCCESS) {
    FT_LOG(Error) << "Failed to create surface.";
    return false;
  }

  CreatePhysicalDevice();
  CreateLogicalDeviceAndQueue();
  CreateCommandPool();

  width_ = width;
  height_ = height;
  CreateSwapChain(width_, height_);

  is_valid_ = true;

  return true;
}

void TizenContextVulkan::OnResize(uint32_t w, uint32_t h) {
  width_ = w;
  height_ = h;
}

void TizenContextVulkan::CreatePhysicalDevice() {
  // Select a compatible physical device.
  uint32_t count;
  vkEnumeratePhysicalDevices(instance_, &count, nullptr);
  std::vector<VkPhysicalDevice> physical_devices(count);
  vkEnumeratePhysicalDevices(instance_, &count, physical_devices.data());
  FT_LOG(Info) << "physical_devices size =" << physical_devices.size();

  uint32_t selected_score = 0;
  for (const auto& pdevice : physical_devices) {
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(pdevice, &properties);
    vkGetPhysicalDeviceFeatures(pdevice, &features);

    uint32_t score = 0;
    std::vector<const char*> supported_extensions;
    uint32_t qfp_count;
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &qfp_count, nullptr);
    std::vector<VkQueueFamilyProperties> qfp(qfp_count);
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &qfp_count, qfp.data());
    FT_LOG(Info) << "DeviceQueueFamilyProperties size =" << qfp_count;
    std::optional<uint32_t> graphics_queue_family;
    std::optional<uint32_t> present_queue_family;
    for (uint32_t i = 0; i < qfp.size(); i++) {
      VkBool32 surface_present_supported;
      vkGetPhysicalDeviceSurfaceSupportKHR(pdevice, i, surface_,
                                           &surface_present_supported);

      if (!graphics_queue_family.has_value() &&
          !present_queue_family.has_value() &&
          qfp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
          surface_present_supported) {
        // Always identical?
        // Same queue family -> VK_SHARING_MODE_EXCLUSIVE
        graphics_queue_family = i;
        present_queue_family = i;
      }
    }

    // Skip physical devices that don't have a graphics queue and a present
    // queue.
    if (!graphics_queue_family.has_value() ||
        !present_queue_family.has_value()) {
      continue;
    }

    // Prefer discrete GPUs.
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      score += 1 << 30;
    }

    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(pdevice, nullptr, &extension_count,
                                         nullptr);
    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(pdevice, nullptr, &extension_count,
                                         available_extensions.data());

    bool supports_swapchain = false;
    for (const auto& available_extension : available_extensions) {
      if (strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                 available_extension.extensionName) == 0) {
        supports_swapchain = true;
        supported_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
      }
      // The spec requires VK_KHR_portability_subset be enabled whenever it's
      // available on a device. It's present on compatibility ICDs like
      // MoltenVK.
      else if (strcmp("VK_KHR_portability_subset",
                      available_extension.extensionName) == 0) {
        supported_extensions.push_back("VK_KHR_portability_subset");
      }

      // Prefer GPUs that support VK_KHR_get_memory_requirements2.
      else if (strcmp(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
                      available_extension.extensionName) == 0) {
        score += 1 << 29;
        supported_extensions.push_back(
            VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
      }
    }

    // Skip physical devices that don't have swapchain support.
    if (!supports_swapchain) {
      continue;
    }

    // Prefer GPUs with larger max texture sizes.
    score += properties.limits.maxImageDimension2D;

    if (selected_score < score) {
      selected_score = score;
      physical_device_ = pdevice;
      enabled_device_extensions_ = supported_extensions;
      graphics_queue_family_index_ =
          graphics_queue_family.value_or(std::numeric_limits<uint32_t>::max());
      present_queue_family_index_ =
          present_queue_family.value_or(std::numeric_limits<uint32_t>::max());
    }
  }

  if (physical_device_ == VK_NULL_HANDLE) {
    FT_LOG(Error) << "Failed to find a compatible Vulkan physical device.";
    return;
  }
}

void TizenContextVulkan::CreateLogicalDeviceAndQueue() {
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {graphics_queue_family_index_,
                                            present_queue_family_index_};
  float priority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &priority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures device_features = {};
  VkDeviceCreateInfo device_info = {};
  device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_info.enabledExtensionCount = enabled_device_extensions_.size();
  device_info.ppEnabledExtensionNames = enabled_device_extensions_.data();
  device_info.pEnabledFeatures = &device_features;
  device_info.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  device_info.pQueueCreateInfos = queueCreateInfos.data();
  device_info.enabledLayerCount = 0;

  if (vkCreateDevice(physical_device_, &device_info, nullptr,
                     &logical_device_) != VK_SUCCESS) {
    FT_LOG(Error) << "Failed to create Vulkan logical device.";
    return;
  }

  vkGetDeviceQueue(logical_device_, graphics_queue_family_index_, 0,
                   &graphics_queue_);
}

void TizenContextVulkan::CreateCommandPool() {
  present_transition_semaphore_.resize(MAX_FRAMES_IN_FLIGHT);
  image_ready_fence_.resize(MAX_FRAMES_IN_FLIGHT);
  submit_done_fence_.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(logical_device_, &semaphoreInfo, nullptr,
                          &present_transition_semaphore_[i]) != VK_SUCCESS ||
        vkCreateFence(logical_device_, &fenceInfo, nullptr,
                      &image_ready_fence_[i]) != VK_SUCCESS ||
        vkCreateFence(logical_device_, &fenceInfo, nullptr,
                      &submit_done_fence_[i]) != VK_SUCCESS) {
      FT_LOG(Error) << "Failed to create fence and semaphore";
    }
  }

  VkCommandPoolCreateInfo pool_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = graphics_queue_family_index_,
  };
  vkCreateCommandPool(logical_device_, &pool_info, nullptr,
                      &swapchain_command_pool_);
  frame_index_ = 0;
}

void TizenContextVulkan::CreateSwapChain(uint32_t w, uint32_t h) {
  /// --------------------------------------------------------------------------
  /// Choose an image format that can be presented to the surface, preferring
  /// the common BGRA+sRGB if available.
  /// --------------------------------------------------------------------------

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_,
                                       &format_count, nullptr);
  std::vector<VkSurfaceFormatKHR> formats(format_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_,
                                       &format_count, formats.data());

  surface_format_ = formats[0];
  for (const auto& format : formats) {
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      surface_format_ = format;
    }
  }

  /// --------------------------------------------------------------------------
  /// Choose the presentable image size that's as close as possible to the
  /// window size.
  /// --------------------------------------------------------------------------

  VkExtent2D extent;
  VkSurfaceCapabilitiesKHR surface_capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_,
                                            &surface_capabilities);

  if (surface_capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    // If the surface reports a specific extent, we must use it.
    // TODO: Even if not full screen?
    extent = surface_capabilities.currentExtent;
  } else {
    extent = {
        .width = w,
        .height = h,
    };
    // TODO: Use std::clamp().
    extent.width = std::max(
        surface_capabilities.minImageExtent.width,
        std::min(surface_capabilities.maxImageExtent.width, extent.width));
    extent.height = std::max(
        surface_capabilities.minImageExtent.height,
        std::min(surface_capabilities.maxImageExtent.height, extent.height));
  }

  /// --------------------------------------------------------------------------
  /// Choose the present mode.
  /// --------------------------------------------------------------------------

  uint32_t mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_,
                                            &mode_count, nullptr);
  std::vector<VkPresentModeKHR> modes(mode_count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_,
                                            &mode_count, modes.data());

  // If the preferred mode isn't available, just choose the first one.
  VkPresentModeKHR present_mode = modes[0];
  for (const auto& mode : modes) {
    if (mode == VK_PRESENT_MODE_FIFO_KHR) {
      present_mode = mode;
      break;
    }
  }

  /// --------------------------------------------------------------------------
  /// Create the swapchain.
  /// --------------------------------------------------------------------------

  VkSwapchainCreateInfoKHR info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface_,
      // TODO: This should not exceed surface_capabilities.maxImageCount.
      .minImageCount = surface_capabilities.minImageCount + 1,
      .imageFormat = surface_format_.format,
      .imageColorSpace = surface_format_.colorSpace,
      // TODO: It is recommended to store extent as a member variable.. (will be
      // needed in future chapters)
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .preTransform = surface_capabilities.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = present_mode,
      .clipped = VK_TRUE,
      // TODO: should specify an old handle when recreating swapchain (resizing)
      // Should learn more.. revisit later
      .oldSwapchain = VK_NULL_HANDLE,
  };

  VkResult err =
      vkCreateSwapchainKHR(logical_device_, &info, nullptr, &swapchain_);
  if (err != VK_SUCCESS)
    FT_LOG(Error) << "CreateSwapChain failed." << err;

  /// --------------------------------------------------------------------------
  /// Fetch swapchain images.
  /// --------------------------------------------------------------------------

  uint32_t image_count;
  vkGetSwapchainImagesKHR(logical_device_, swapchain_, &image_count, nullptr);
  swapchain_images_.resize(image_count);
  vkGetSwapchainImagesKHR(logical_device_, swapchain_, &image_count,
                          swapchain_images_.data());
  FT_LOG(Info) << "SwapChain image count = " << image_count;

  /// --------------------------------------------------------------------------
  /// Record a command buffer for each of the images to be executed prior to
  /// presenting.
  /// --------------------------------------------------------------------------

  present_transition_buffers_.resize(swapchain_images_.size());

  VkCommandBufferAllocateInfo buffers_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = swapchain_command_pool_,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount =
          static_cast<uint32_t>(present_transition_buffers_.size()),
  };
  vkAllocateCommandBuffers(logical_device_, &buffers_info,
                           present_transition_buffers_.data());

  for (size_t i = 0; i < swapchain_images_.size(); i++) {
    auto image = swapchain_images_[i];
    auto buffer = present_transition_buffers_[i];

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        // .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(buffer, &begin_info);

    // Layout transition.
    // Save the command to present_transition_buffers_ and use it right before present.

    // Flutter Engine hands back the image after writing to it
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    vkCmdPipelineBarrier(
        buffer,                                         // commandBuffer
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // srcStageMask
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,           // dstStageMask
        0,                                              // dependencyFlags
        0,                                              // memoryBarrierCount
        nullptr,                                        // pMemoryBarriers
        0,        // bufferMemoryBarrierCount
        nullptr,  // pBufferMemoryBarriers
        1,        // imageMemoryBarrierCount
        &barrier  // pImageMemoryBarriers
    );

    vkEndCommandBuffer(buffer);
  }
}

FlutterVulkanImage TizenContextVulkan::GetNextImageCallback(
    const FlutterFrameInfo* frame_info) {
  VkResult result = vkAcquireNextImageKHR(
      logical_device_, swapchain_, UINT64_MAX, VK_NULL_HANDLE,
      image_ready_fence_[frame_index_], &image_index_);

  if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
    FT_LOG(Info) << "Recreate SwapChain.";
    RecreateSwapChain();
  } else if (result == VK_SUCCESS) {
  } else {
    FT_LOG(Error) << "AcquireNextImageKHR Error Code = " << result;
  }

  // Flutter Engine expects the image to be available for transitioning and
  // attaching immediately, and so we need to force a host sync here before
  // returning.
  vkWaitForFences(logical_device_, 1, &image_ready_fence_[frame_index_],
                  VK_TRUE, UINT64_MAX);
  vkResetFences(logical_device_, 1, &image_ready_fence_[frame_index_]);

  return {
      .struct_size = sizeof(FlutterVulkanImage),
      .image = reinterpret_cast<uint64_t>(swapchain_images_[image_index_]),
      .format = surface_format_.format,
  };
}

bool TizenContextVulkan::PresentCallback(const FlutterVulkanImage* image) {
  // Submit present_transition_buffers_ to perform layout transition.
  VkPipelineStageFlags stage_flags =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = &stage_flags,
      .commandBufferCount = 1,
      .pCommandBuffers = &present_transition_buffers_[image_index_],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &present_transition_semaphore_[frame_index_],
  };
  vkQueueSubmit(graphics_queue_, 1, &submit_info,
                // Is submit_done_fence_ necessary?
                submit_done_fence_[frame_index_]);

  VkPresentInfoKHR present_info = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &present_transition_semaphore_[frame_index_],
      .swapchainCount = 1,
      .pSwapchains = &swapchain_,
      .pImageIndices = &image_index_,
  };
  VkResult result = vkQueuePresentKHR(graphics_queue_, &present_info);
  frame_index_ = (frame_index_ + 1) % MAX_FRAMES_IN_FLIGHT;

  // If the swapchain is no longer compatible with the surface, discard the
  // swapchain and create a new one.
  if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
    RecreateSwapChain();
  }

  return result == VK_SUCCESS;
}

void TizenContextVulkan::RecreateSwapChain() {
  vkDeviceWaitIdle(logical_device_);

  // Is this necessary? Not vkResetFences?
  size_t i = 0;
  for (; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyFence(logical_device_, image_ready_fence_[i], nullptr);
    vkDestroyFence(logical_device_, submit_done_fence_[i], nullptr);
    vkDestroySemaphore(logical_device_, present_transition_semaphore_[i],
                       nullptr);
  }

  for (i = 0; i < swapchain_images_.size(); i++) {
    vkFreeCommandBuffers(logical_device_, swapchain_command_pool_, 1,
                         &present_transition_buffers_[i]);
    vkDestroyImage(logical_device_, swapchain_images_[i], nullptr);
  }
  vkDestroyCommandPool(logical_device_, swapchain_command_pool_, nullptr);
  // Not calling vkDestroySwapchainKHR?

  CreateCommandPool();
  CreateSwapChain(width_, height_);
}

}  // namespace flutter
