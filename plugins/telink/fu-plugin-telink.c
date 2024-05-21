#include "config.h"
#include <fwupdplugin.h>
#include "fu-plugin-vfuncs.h"
#include "fu-telink-config.h"
#include "fu-telink-firmware.h"

static gboolean
fu_plugin_telink_dev_startup(FuPlugin *plugin, GError **error)
{
    if (!g_file_test("/sys/class/hidraw", G_FILE_TEST_IS_DIR)) {
        g_set_error_literal(error, FWUPD_ERROR, FWUPD_ERROR_NOT_SUPPORTED, "kernel: CONFIG_HIDRAW not supported");
        return FALSE;
    }

    return TRUE;
}

static void
fu_plugin_telink_dev_init(FuPlugin *plugin)
{
    FuContext *ctx = fu_plugin_get_context(plugin);

    fu_plugin_add_udev_subsystem(plugin, "hidraw");
    fu_plugin_add_device_gtype(plugin, FU_TYPE_TELINK_CONFIG);
    fu_plugin_add_firmware_gtype(plugin, NULL, FU_TYPE_TELINK_FW);
    fu_context_add_quirk_key(ctx, "TelinkBootType");

    g_debug("telink-plugin-init");
}

void
fu_plugin_init_vfuncs(FuPluginVfuncs *vfuncs)
{
    vfuncs->build_hash = FU_BUILD_HASH;
    vfuncs->init = fu_plugin_telink_dev_init;
    vfuncs->startup = fu_plugin_telink_dev_startup;
}

