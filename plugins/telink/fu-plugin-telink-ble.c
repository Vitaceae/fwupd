/* fu-plugin-telink.c
 *
 * Copyright (C) 2024 Mike Chang <mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include <fwupdplugin.h>

#include "fu-telink-common.h"
#include "fu-telink-ble-dev.h"

static void
fu_plugin_telink_dev_init(FuPlugin *plugin)
{
    FuContext *ctx = fu_plugin_get_context(plugin);

    LOGD("start");

//    fu_plugin_add_udev_subsystem(plugin, "hidraw");
    fu_plugin_add_device_gtype(plugin, FU_TYPE_TELINK_BLE_DEV);
}

void
fu_plugin_init_vfuncs(FuPluginVfuncs *vfuncs)
{
    vfuncs->build_hash = FU_BUILD_HASH;
    vfuncs->init = fu_plugin_telink_dev_init;
}

