/* fu-plugin-telink.c
 *
 * Copyright (C) 2024 Mike Chang <mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include <fwupdplugin.h>

#include "fu-telink-common.h"
#include "fu-telink-hid-dev.h"
#include "fu-telink-archive.h"
#include "fu-telink-firmware-beta.h"
#include "fu-telink-firmware-v1.h"

static gboolean
fu_plugin_telink_dev_startup(FuPlugin *plugin, GError **error)
{
    if (!g_file_test("/sys/class/hidraw", G_FILE_TEST_IS_DIR)) {
        g_set_error_literal(error, FWUPD_ERROR, FWUPD_ERROR_NOT_SUPPORTED, "kernel: CONFIG_HIDRAW not supported");
        return FALSE;
    }

    LOGD("start");

    return TRUE;
}

static void
fu_plugin_telink_dev_init(FuPlugin *plugin)
{
    FuContext *ctx = fu_plugin_get_context(plugin);

    LOGD("start");

    fu_plugin_add_udev_subsystem(plugin, "hidraw");
    fu_plugin_add_device_gtype(plugin, FU_TYPE_TELINK_HID_DEV);
    fu_plugin_add_firmware_gtype(plugin, NULL, FU_TYPE_TELINK_ARCHIVE);
    fu_plugin_add_firmware_gtype(plugin, NULL, FU_TYPE_TELINK_FW_BETA);
    fu_plugin_add_firmware_gtype(plugin, NULL, FU_TYPE_TELINK_FW_V1);
    fu_context_add_quirk_key(ctx, "TelinkBootType");
    fu_context_add_quirk_key(ctx, "TelinkBoardType");
}

void
fu_plugin_init_vfuncs(FuPluginVfuncs *vfuncs)
{
    vfuncs->build_hash = FU_BUILD_HASH;
    vfuncs->init = fu_plugin_telink_dev_init;
    vfuncs->startup = fu_plugin_telink_dev_startup;
}

