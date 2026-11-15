#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include "../core/Log.h"

#include <cstdlib>

#define VK_CHECK(expr)                                                    \
    do {                                                                  \
        VkResult vkCheckResult_ = (expr);                                 \
        if (vkCheckResult_ != VK_SUCCESS) {                               \
            ::vsp::log::error("{} failed: {} ({}:{})",                    \
                              #expr, string_VkResult(vkCheckResult_),     \
                              __FILE__, __LINE__);                        \
            std::abort();                                                 \
        }                                                                 \
    } while (0)