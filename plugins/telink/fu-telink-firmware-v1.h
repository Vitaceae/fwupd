/*
 * Copyright 2024 Mike Chang <mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "fu-telink-firmware.h"

#define FU_TYPE_TELINK_FW_V1 (fu_telink_fw_v1_get_type())
G_DECLARE_FINAL_TYPE(FuTelinkFwV1, fu_telink_fw_v1, FU, TELINK_FW_V1, FuTelinkFw)

FuFirmware *
fu_telink_fw_v1_new(void);
