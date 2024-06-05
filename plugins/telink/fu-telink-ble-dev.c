/* fu-telink-ble-dev.c
 *
 * Copyright (C) 2024 Mike Chang <mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include <fwupdplugin.h>
#include "fu-telink-common.h"
#include "fu-telink-ble-dev.h"

struct _FuTelinkBleDev {
    FuBluezDevice parent_instance;
};

G_DEFINE_TYPE(FuTelinkBleDev, fu_telink_ble_dev, FU_TYPE_BLUEZ_DEVICE)

static void
fu_telink_ble_dev_class_init(FuTelinkBleDevClass *klass)
{
    //todo
}

static void
fu_telink_ble_dev_init(FuTelinkBleDev *self)
{
    LOGD("start");

    fu_device_add_protocol(FU_DEVICE(self), "com.telink.ble");
    fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_UPDATABLE);
    fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_UNSIGNED_PAYLOAD);
}
