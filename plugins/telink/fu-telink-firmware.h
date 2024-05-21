/* fu-telink-firmware.h
 *
 * Copyright (C) 2024 Mike Chang <mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <fwupdplugin.h>

#define FU_TYPE_TELINK_FW (fu_telink_fw_get_type())
G_DECLARE_DERIVABLE_TYPE(FuTelinkFw, fu_telink_fw, FU, TELINK_FW, FuFirmware)

struct _FuTelinkFwClass {
	FuFirmwareClass parent_class;
};
