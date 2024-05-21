/* fu-telink-config.h
 *
 * Copyright (C) 2024 Mike Chang <mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <fwupdplugin.h>

#define FU_TYPE_TELINK_CONFIG (fu_telink_config_get_type())
G_DECLARE_FINAL_TYPE(FuTelinkConfig, fu_telink_config, FU, TELINK_CONFIG, FuUdevDevice)

FuTelinkConfig *
fu_telink_config_new(guint8 dev_id);
