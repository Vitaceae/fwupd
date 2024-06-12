/*
 * Copyright 2024 Mike Chang <mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "fu-telink-firmware.h"

#define FU_TYPE_TELINK_FW_BETA (fu_telink_fw_beta_get_type())
G_DECLARE_FINAL_TYPE(FuTelinkFwBeta, fu_telink_fw_beta, FU, TELINK_FW_BETA, FuTelinkFw)

FuFirmware *
fu_telink_fw_beta_new(void);
