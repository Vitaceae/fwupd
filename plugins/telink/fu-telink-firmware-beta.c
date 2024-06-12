/*
 * Copyright 2024 Mike Chang <mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "fu-telink-common.h"
#include "fu-telink-firmware-beta.h"

#define TELINK_DEVEL                    0

//todo
#define TELINK_IMAGE_MAGIC_COMMON       0x12345678
#define TELINK_IMAGE_MAGIC_FWINFO       0x13572468
#define TELINK_IMAGE_MAGIC_1            0x00112233
#define TELINK_IMAGE_MAGIC_2            0x44556677

struct _FuTelinkFwBeta {
    FuTelinkFwBetaClass parent_instance;
};

G_DEFINE_TYPE(FuTelinkFwBeta, fu_telink_fw_beta, FU_TYPE_TELINK_FW)

static GBytes *
fu_telink_fw_beta_write(FuFirmware *firmware, GError **error)
{
    g_autoptr(GByteArray) buf = g_byte_array_new();
    g_autoptr(GBytes) blob = NULL;
    fu_byte_array_append_uint32(buf, TELINK_IMAGE_MAGIC_COMMON, G_LITTLE_ENDIAN);
    fu_byte_array_append_uint32(buf, TELINK_IMAGE_MAGIC_FWINFO, G_LITTLE_ENDIAN);
    fu_byte_array_append_uint32(buf, TELINK_IMAGE_MAGIC_1, G_LITTLE_ENDIAN);
    fu_byte_array_append_uint32(buf, 0x00, G_LITTLE_ENDIAN);
    fu_byte_array_append_uint32(buf, 0x00, G_LITTLE_ENDIAN);
    /* version */
    fu_byte_array_append_uint32(buf, 0x63, G_LITTLE_ENDIAN);
    blob = fu_firmware_get_bytes_with_patches(firmware, error);
    if (blob == NULL) {
        return NULL;
    }
    fu_byte_array_append_bytes(buf, blob);
    return g_byte_array_free_to_bytes(g_steal_pointer(&buf));
}

static gboolean
fu_telink_fw_beta_read_fwinfo(FuFirmware *firmware, guint8 const *buf, gsize bufsz, GError **error)
{
    guint32 magic_common;
    guint32 magic_fwinfo;
    guint32 magic_compat;
    guint32 offset;
    guint32 hdr_offset[5] = {0x0000, 0x0200, 0x400, 0x800, 0x1000};
    guint8 ver_major = 0;
    guint8 ver_minor = 0;
    guint16 ver_rev = 0;
    guint32 ver_build_nr = 0;
    g_autofree gchar *version = NULL;

    LOGD("start");

#if TELINK_DEVEL == 1
    //todo
    //invalidate image: find correct offset to fwinfo
    for (guint32 i = 0; i < G_N_ELEMENTS(hdr_offset); i++) {
        offset = hdr_offset[i];

        if (!fu_common_read_uint32_safe(buf, bufsz, offset, &magic_common, G_LITTLE_ENDIAN, error)) {
            return FALSE;
        }

        if (!fu_common_read_uint32_safe(buf, bufsz, offset + 0x04, &magic_fwinfo, G_LITTLE_ENDIAN, error)) {
            return FALSE;
        }

        if (!fu_common_read_uint32_safe(buf, bufsz, offset + 0x08, &magic_compat, G_LITTLE_ENDIAN, error)) {
            return FALSE;
        }

        /* version */
        if (!fu_common_read_uint32_safe(buf, bufsz, offset + 0x14, &ver_build_nr, G_LITTLE_ENDIAN, error)) {
            return FALSE;
        }

        if (magic_common != UPDATE_IMAGE_MAGIC_COMMON || magic_fwinfo != UPDATE_IMAGE_MAGIC_FWINFO) {
            continue;
        }

        switch (magic_compat) {
        case TELINK_IMAGE_MAGIC_1:
        case TELINK_IMAGE_MAGIC_2:
            /* currently only the build number is saved into the image */
            version = g_strdup_printf("%u.%u.%u.%u", ver_major, ver_minor, ver_rev, ver_build_nr);
            fu_firmware_set_version(firmware, version);
            return TRUE;
        default:
            break;
        }
    }
#else
    ver_major = 1;
    ver_minor = 2;
    ver_rev = 3;
    ver_build_nr = 4;
    version = g_strdup_printf("%u.%u.%u.%u", ver_major, ver_minor, ver_rev, ver_build_nr);
    fu_firmware_set_version(firmware, version);

    return TRUE;
#endif //TELINK_DEVEL

    g_set_error_literal(error, FWUPD_ERROR, FWUPD_ERROR_INVALID_FILE, "unable to validate the update binary");

    return FALSE;
}

static gboolean
fu_telink_fw_beta_parse(FuFirmware *firmware, GBytes *fw, guint64 addr_start, guint64 addr_end, FwupdInstallFlags flags, GError **error)
{
    const guint8 *buf;
    gsize bufsz = 0;

    LOGD("start");

    if (!FU_FIRMWARE_CLASS(fu_telink_fw_beta_parent_class)->parse(firmware, fw, addr_start, addr_end, flags, error)) {
        return FALSE;
    }

    buf = g_bytes_get_data(fw, &bufsz);
    if (buf == NULL) {
        g_set_error_literal(error, FWUPD_ERROR, FWUPD_ERROR_INVALID_FILE, "unable to get the image binary");
        return FALSE;
    }

    return fu_telink_fw_beta_read_fwinfo(firmware, buf, bufsz, error);
}

static void
fu_telink_fw_beta_init(FuTelinkFwBeta *self)
{
    LOGD("start");
    //todo
}

static void
fu_telink_fw_beta_class_init(FuTelinkFwBetaClass *klass)
{
    FuFirmwareClass *klass_firmware = FU_FIRMWARE_CLASS(klass);
    klass_firmware->parse = fu_telink_fw_beta_parse;
    klass_firmware->write = fu_telink_fw_beta_write;
}

FuFirmware *
fu_telink_fw_beta_new(void)
{
    return FU_FIRMWARE(g_object_new(FU_TYPE_TELINK_FW_BETA, NULL));
}
