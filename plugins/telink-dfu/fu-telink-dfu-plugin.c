/*
 * Copyright 2024 Mike Chang <Mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "fu-telink-dfu-archive.h"
#include "fu-telink-dfu-device.h"
#include "fu-telink-dfu-firmware.h"
#include "fu-telink-dfu-plugin.h"

struct _FuTelinkDfuPlugin {
	FuPlugin parent_instance;
};

G_DEFINE_TYPE(FuTelinkDfuPlugin, fu_telink_dfu_plugin, FU_TYPE_PLUGIN)

static void
fu_telink_dfu_plugin_init(FuTelinkDfuPlugin *self)
{
}

static void
fu_telink_dfu_plugin_constructed(GObject *obj)
{
	FuPlugin *plugin = FU_PLUGIN(obj);
	FuContext *ctx = fu_plugin_get_context(plugin);
	fu_context_add_quirk_key(ctx, "TelinkDfuBootType");
	fu_context_add_quirk_key(ctx, "TelinkDfuBoardType");
	fu_plugin_add_device_gtype(plugin, FU_TYPE_TELINK_DFU_DEVICE);
	fu_plugin_add_firmware_gtype(plugin, NULL, FU_TYPE_TELINK_DFU_ARCHIVE);
	fu_plugin_add_firmware_gtype(plugin, NULL, FU_TYPE_TELINK_DFU_FIRMWARE);
	fu_plugin_add_udev_subsystem(plugin, "hidraw");
}

#if 0
static gboolean
fu_plugin_telink_dfu_plugin_startup(FuPlugin *plugin, GError **error)
{
	if (!g_file_test("/sys/class/hidraw", G_FILE_TEST_IS_DIR)) {
		g_set_error_literal(error,
				    FWUPD_ERROR,
				    FWUPD_ERROR_NOT_SUPPORTED,
				    "kernel: CONFIG_HIDRAW not supported");
		return FALSE;
	}
	return TRUE;
}
#endif

static void
fu_telink_dfu_plugin_class_init(FuTelinkDfuPluginClass *klass)
{
	FuPluginClass *plugin_class = FU_PLUGIN_CLASS(klass);
	plugin_class->constructed = fu_telink_dfu_plugin_constructed;
	//	plugin_class->startup = fu_telink_dfu_plugin_startup;
}
