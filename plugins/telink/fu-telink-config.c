/* fu-telink-config.c
 *
 * Copyright (C) 2024 Mike Chang <mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#ifdef HAVE_HIDRAW_H
#include <linux/hidraw.h>
#include <linux/input.h>
#endif
#include <fwupdplugin.h>
#include "fu-telink-config.h"
#include "fu-telink-firmware.h"

#define TELINK_DEVEL                     0
#define FU_TELINK_CONFIG_RETRY_INTERVAL     50 //ms

typedef enum {
    DFU_STATE_INACTIVE,
    DFU_STATE_ACTIVE,
    DFU_STATE_STORING,
    DFU_STATE_CLEANING,
} FuTelinkConfigSyncState;

typedef struct {
    guint8 idx;
    gchar *name;
} FuTelinkConfigModuleOption;

typedef struct {
    guint8 idx;
    gchar *name;
    GPtrArray *options; //FuTelinkConfigModuleOption
} FuTelinkConfigModule;

typedef struct {
    guint8 dfu_state;
    guint32 img_length;
    guint32 img_csum;
    guint32 offset;
    guint16 sync_buffer_size;
} FuTelinkConfigDfuInfo;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(FuTelinkConfigDfuInfo, g_free);

struct _FuTelinkConfig {
    FuUdevDevice parent_instance;
    gchar *board_name;
    gchar *bl_name;
    guint8 flash_area_id;
    guint32 flashed_image_len;
    guint8 peer_id;
    GPtrArray *modules; //FuTelinkConfigModuleOption
};

G_DEFINE_TYPE(FuTelinkConfig, fu_telink_config, FU_TYPE_UDEV_DEVICE)

#if TELINK_DEVEL == 1
static void
fu_telink_config_module_option_free(FuTelinkConfigModuleOption *opt)
{
    g_free(opt->name);
    g_free(opt);
}
#endif

static void
fu_telink_config_module_free(FuTelinkConfigModule *m)
{
    if (m->options != NULL) {
        g_ptr_array_unref(m->options);
    }
    g_free(m->name);
    g_free(m);
}

static gboolean
fu_telink_config_probe(FuDevice *device, GError **error)
{
    /* FuUdevDevice->probe */
    if (!FU_DEVICE_CLASS(fu_telink_config_parent_class)->probe(device, error)) {
        g_debug("telink device probe error");
        return FALSE;
    }

    g_debug("telink device probed");

    //todo: other than hid device?
    return fu_udev_device_set_physical_id(FU_UDEV_DEVICE(device), "hid", error);
}

static void
fu_telink_config_finalize(GObject *object)
{
    FuTelinkConfig *self = FU_TELINK_CONFIG(object);
    g_free(self->board_name);
    g_free(self->bl_name);
    g_ptr_array_unref(self->modules);
    G_OBJECT_CLASS(fu_telink_config_parent_class)->finalize(object);
}

static void
fu_telink_config_set_progress(FuDevice *self, FuProgress *progress)
{
    fu_progress_set_id(progress, G_STRLOC); //use the current unique address as ID
    fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_RESTART, 1); //detach
    fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_WRITE, 97); //write
    fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_RESTART, 1); //attach
    fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_BUSY, 1); //reload
}

static gboolean
fu_telink_config_set_quirk_kv(FuDevice *device, const gchar *key, const gchar *value, GError **error)
{
    FuTelinkConfig *self = FU_TELINK_CONFIG(device);

    if (g_strcmp0(key, "TelinkBootType") == 0) {
        if (g_strcmp0(value, "Beta") == 0)
            self->bl_name = g_strdup("Beta");
        else if (g_strcmp0(value, "HOG") == 0)
            self->bl_name = g_strdup("HOG");
        else {
            g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "shold be 'Beta' or 'HOG'");
            return FALSE;
        }
        return TRUE;
    }

    g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "quirk key not supported");
    return FALSE;
}


static gboolean
fu_telink_config_setup(FuDevice *device, GError **error)
{
    FuTelinkConfig *self = FU_TELINK_CONFIG(device);
    g_autofree gchar *target_id = NULL;

#if TELINK_DEVEL == 1
    /* get the board name */
    if (!fu_telink_config_get_board_name(self, error))
        return FALSE;
    /* detect available modules first */
    if (!fu_telink_config_get_modinfo(self, error))
        return FALSE;
    /* detect bootloader type */
    if (!fu_telink_config_get_bl_name(self, error))
        return FALSE;
    /* set the physical id based on name, HW id and bootloader type of the board
     * to detect if the device is connected via several interfaces */
    if (!fu_telink_config_get_hwid(self, error))
        return FALSE;
    /* get device info and version */
    if (!fu_telink_config_dfu_fwinfo(self, error))
        return FALSE;
    /* check if any peer is connected via this device */
    if (!fu_telink_config_add_peers(self, error))
        return FALSE;
#endif

    /* generate the custom visible name for the device if absent */
    if (fu_device_get_name(device) == NULL) {
        const gchar *physical_id = NULL;
        physical_id = fu_device_get_physical_id(device);
        fu_device_set_name(device, physical_id);
    }

    /* additional GUID based on VID/PID and target area to flash
     * needed to distinguish images aimed to different bootloaders */
    target_id = g_strdup_printf("HIDRAW\\VEN_%04X&DEV_%04X&BOARD_%s&BL_%s",
                                fu_udev_device_get_vendor(FU_UDEV_DEVICE(device)),
                                fu_udev_device_get_model(FU_UDEV_DEVICE(device)),
                                self->board_name,
                                self->bl_name);
    fu_device_add_guid(device, target_id);

    return TRUE;
}

static void
fu_telink_config_module_to_string(FuTelinkConfigModule *mod, guint idt, GString *str)
{
    for (guint i = 0; i < mod->options->len; i++) {
        FuTelinkConfigModuleOption *opt = g_ptr_array_index(mod->options, i);
        g_autofree gchar *title = g_strdup_printf("Option%02x", i);
        fu_common_string_append_kv(str, idt, title, opt->name);
    }
}

static void
fu_telink_config_to_string(FuDevice *device, guint idt, GString *str)
{
    FuTelinkConfig *self = FU_TELINK_CONFIG(device);
    fu_common_string_append_kv(str, idt, "BoardName", self->board_name);
    fu_common_string_append_kv(str, idt, "Bootloader", self->bl_name);
    fu_common_string_append_kx(str, idt, "FlashAreaId", self->flash_area_id);
    fu_common_string_append_kx(str, idt, "FlashedImageLen", self->flashed_image_len);
    fu_common_string_append_kx(str, idt, "PeerId", self->peer_id);
    for (guint i = 0; i < self->modules->len; i++) {
        FuTelinkConfigModule *mod = g_ptr_array_index(self->modules, i);
        g_autofree gchar *title = g_strdup_printf("Module%02x", i);
        fu_common_string_append_kv(str, idt, title, mod->name);
        fu_telink_config_module_to_string(mod, idt + 1, str);
    }
}

static gboolean
fu_telink_config_dfu_sync(FuTelinkConfig *self, FuTelinkConfigDfuInfo *dfu_info, guint8 expecting_state, GError **error)
{
    //todo
    g_debug("fu_telink_config_dfu_sync");
    return TRUE;
}

static gboolean
fu_telink_config_dfu_start(FuTelinkConfig *self, gsize img_length, guint32 img_crc, guint32 offset, GError **error)
{
    //todo
    g_debug("fu_telink_config_dfu_start");
    return TRUE;
}

static gboolean
fu_telink_config_write_firmware_blob(FuTelinkConfig *self, GBytes *blob, FuProgress *progress, GError **error)
{
    //todo
    g_debug("fu_telink_config_write_firmware_blob");
    return TRUE;
}

static gboolean
fu_telink_config_dfu_reboot(FuTelinkConfig *self, GError **error)
{
    //todo
    g_debug("fu_telink_config_dfu_reboot");
    return TRUE;
}

static gboolean
fu_telink_config_write_firmware(FuDevice *device, FuFirmware *firmware, FuProgress *progress, FwupdInstallFlags flags, GError **error)
{
    FuTelinkConfig *self = FU_TELINK_CONFIG(device);
    guint32 checksum;
    g_autofree gchar *csum_str = NULL;
    g_autofree gchar *image_id = NULL;
    g_autoptr(GBytes) blob = NULL;
    g_autoptr(FuTelinkConfigDfuInfo) dfu_info = g_new0(FuTelinkConfigDfuInfo, 1);

    /* select correct firmware per target board, bootloader and bank */
    image_id = g_strdup_printf("%s_%s_bank%01u", self->board_name, self->bl_name, self->flash_area_id);
    firmware = fu_firmware_get_image_by_id(firmware, image_id, error);
    if (firmware == NULL) {
        return FALSE;
    }

    csum_str = fu_firmware_get_checksum(firmware, -1, error);
    if (csum_str == NULL) {
        return FALSE;
    }
    checksum = g_ascii_strtoull(csum_str, NULL, 16);

    fu_progress_set_id(progress, G_STRLOC);
    fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_ERASE, 1);
    fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_WRITE, 99);
    fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_BUSY, 0);

    /* TODO: check if there is unfinished operation before? */
    blob = fu_firmware_get_bytes(firmware, error);
    if (blob == NULL) {
        return FALSE;
    }
    if (!fu_telink_config_dfu_sync(self, dfu_info, DFU_STATE_INACTIVE, error)) {
        return FALSE;
    }
    if (!fu_telink_config_dfu_start(self, g_bytes_get_size(blob), checksum, 0x0 /* offset */, error)) {
        return FALSE;
    }
    fu_progress_step_done(progress);

    /* write */
    if (!fu_telink_config_write_firmware_blob(self, blob, fu_progress_get_child(progress), error)) {
        return FALSE;
    }
    fu_progress_step_done(progress);

    /* attach */
    if (!fu_telink_config_dfu_reboot(self, error)) {
        return FALSE;
    }
    fu_progress_step_done(progress);

    return TRUE;
}

static void
fu_telink_config_class_init(FuTelinkConfigClass *klass)
{
    FuDeviceClass *klass_device = FU_DEVICE_CLASS(klass);
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    klass_device->probe = fu_telink_config_probe;
    klass_device->set_progress = fu_telink_config_set_progress;
    klass_device->set_quirk_kv = fu_telink_config_set_quirk_kv;
    klass_device->setup = fu_telink_config_setup;
    klass_device->to_string = fu_telink_config_to_string;
    klass_device->write_firmware = fu_telink_config_write_firmware;
    object_class->finalize = fu_telink_config_finalize;
}

static void
fu_telink_config_init(FuTelinkConfig *self)
{
    self->modules = g_ptr_array_new_with_free_func((GDestroyNotify)fu_telink_config_module_free);

    fu_device_set_vendor(FU_DEVICE(self), "Telink");
    //todo: FWUPD_DEVICE_FLAG_SIGNED_PAYLOAD or FWUPD_DEVICE_FLAG_UNSIGNED_PAYLOAD
    fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_UPDATABLE | FWUPD_DEVICE_FLAG_UNSIGNED_PAYLOAD);
    fu_device_set_version_format(FU_DEVICE(self), FWUPD_VERSION_FORMAT_TRIPLET); //0xCCDD.BB.AA
    fu_device_add_protocol(FU_DEVICE(self), "com.telink.dfu"); //todo: what for?
    fu_device_retry_set_delay(FU_DEVICE(self), FU_TELINK_CONFIG_RETRY_INTERVAL);
    fu_device_set_firmware_gtype(FU_DEVICE(self), FU_TYPE_TELINK_FW);
}

FuTelinkConfig *
fu_telink_config_new(guint8 dev_id)
{
    FuTelinkConfig *self = g_object_new(FU_TYPE_TELINK_CONFIG, NULL);
    self->peer_id = dev_id;
    return self;
}
