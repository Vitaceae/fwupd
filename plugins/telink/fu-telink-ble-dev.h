/* fu-telink-ble-dev.h
 *
 * Copyright (C) 2024 Mike Chang <mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <fwupdplugin.h>

#define FU_TYPE_TELINK_BLE_DEV (fu_telink_ble_dev_get_type())
G_DECLARE_FINAL_TYPE(FuTelinkBleDev, fu_telink_ble_dev, FU, TELINK_BLE_DEV, FuBluezDevice)

FuTelinkBleDev *
fu_telink_ble_dev_new(guint8 dev_id);
