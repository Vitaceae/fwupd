/* fu-telink-common.h
 *
 * Copyright (C) 2024 Mike Chang <mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#define DEBUG_TRACE						1
#define LOG_LEVEL                       3
#if DEBUG_TRACE == 1
 #define LOGE(fmt, ...)		do {if (LOG_LEVEL >= 1) {g_error("%s: "fmt, __FUNCTION__, ## __VA_ARGS__);}} while(0)
 #define LOGW(fmt, ...)		do {if (LOG_LEVEL >= 2) {g_warning("%s: "fmt, __FUNCTION__, ## __VA_ARGS__);}} while(0)
 #define LOGD(fmt, ...)		do {if (LOG_LEVEL >= 3) {g_debug("%s: "fmt, __FUNCTION__, ## __VA_ARGS__);}} while(0)
 #define LOGI(fmt, ...)		do {if (LOG_LEVEL >= 0) {g_info("%s: "fmt, __FUNCTION__, ## __VA_ARGS__);}} while(0)
 #define LOGM(fmt, ...)		do {if (LOG_LEVEL >= 0) {g_message("%s: "fmt, __FUNCTION__, ## __VA_ARGS__);}} while(0)
#else
 #define LOGE(...)
 #define LOGW(...)
 #define LOGD(...)
 #define LOGI(...)
 #define LOGM(...)
#endif
