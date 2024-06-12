/*
 * Copyright 2024 Mike Chang <mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <fwupdplugin.h>

#define FU_TYPE_TELINK_HID_DEV (fu_telink_hid_dev_get_type())
G_DECLARE_FINAL_TYPE(FuTelinkHidDev, fu_telink_hid_dev, FU, TELINK_HID_DEV, FuUdevDevice)

FuTelinkHidDev *
fu_telink_hid_dev_new(guint8 dev_id);
