/*
 * Copyright 2024 Mike Chang <Mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include <fwupdplugin.h>

#include "fu-telink-dfu-firmware.h"
#include "fu-telink-dfu-struct.h"
#include "fu-telink-dfu-common.h"

struct _FuTelinkDfuFirmware {
	FuFirmware parent_instance;
	guint32 crc32;
};

G_DEFINE_TYPE(FuTelinkDfuFirmware, fu_telink_dfu_firmware, FU_TYPE_FIRMWARE)

#define TELINK_IMAGE_MAGIC_1 0x00112233
#define TELINK_IMAGE_MAGIC_2 0x44556677

static void
fu_telink_dfu_firmware_export(FuFirmware *firmware, FuFirmwareExportFlags flags, XbBuilderNode *bn)
{
	FuTelinkDfuFirmware *self = FU_TELINK_DFU_FIRMWARE(firmware);
LOGD("start");
	fu_xmlb_builder_insert_kx(bn, "crc32", self->crc32);
}

static gboolean
fu_telink_dfu_firmware_build(FuFirmware *firmware, XbNode *n, GError **error)
{
	FuTelinkDfuFirmware *self = FU_TELINK_DFU_FIRMWARE(firmware);
	guint64 tmp;
LOGD("start");

	/* TODO: load from .builder.xml */
	tmp = xb_node_query_text_as_uint(n, "crc32", NULL);
	if (tmp != G_MAXUINT64 && tmp <= G_MAXUINT16)
		self->crc32 = tmp;

	/* success */
	return TRUE;
}

static gboolean
fu_telink_dfu_validate(FuFirmware *firmware, GInputStream *stream, gsize offset, GError **error)
{
LOGD("start");
	return fu_struct_telink_dfu_hdr_validate_stream(stream, offset, error);
}

static FuStructTelinkDfuHdr *
fu_telink_dfu_firmware_parse_stream(GInputStream *stream, gsize offset, GError **error)
{
	g_autoptr(FuStructTelinkDfuHdr) st_hdr = NULL;
LOGD("start");

	st_hdr = fu_struct_telink_dfu_hdr_parse_stream(stream, offset, error);
	if (st_hdr == NULL)
		return NULL;
	if (fu_struct_telink_dfu_hdr_get_magic(st_hdr) != TELINK_IMAGE_MAGIC_1 ||
	    fu_struct_telink_dfu_hdr_get_magic(st_hdr) != TELINK_IMAGE_MAGIC_2) {
		g_set_error(error,
			    FWUPD_ERROR,
			    FWUPD_ERROR_INVALID_DATA,
			    "invalid magic: 0x%x",
			    fu_struct_telink_dfu_hdr_get_magic(st_hdr));
		return NULL;
	}
	return g_steal_pointer(&st_hdr);
}

static gboolean
fu_telink_dfu_firmware_parse(FuFirmware *firmware,
			     GInputStream *stream,
			     gsize offset,
			     FwupdInstallFlags flags,
			     GError **error)
{
	FuTelinkDfuFirmware *self = FU_TELINK_DFU_FIRMWARE(firmware);
	guint32 hdr_offset[5] = {0x0000, 0x0200, 0x400, 0x800, 0x1000};
	guint32 version_raw;
	g_autofree gchar *version = NULL;
	g_autoptr(FuStructTelinkDfuHdr) st_hdr = NULL;
LOGD("start");

	/* calculate checksum of entire image */
	if (!fu_input_stream_compute_crc32(stream, &self->crc32, 0xEDB88320, error))
		return FALSE;

	/* find header */
	for (guint32 i = 0; i < G_N_ELEMENTS(hdr_offset); i++) {
		st_hdr = fu_telink_dfu_firmware_parse_stream(stream, offset + hdr_offset[i], NULL);
		if (st_hdr != NULL)
			break;
	}
	if (st_hdr == NULL) {
		g_set_error_literal(error, FWUPD_ERROR, FWUPD_ERROR_INVALID_DATA, "invalid header");
		return FALSE;
	}

	/* parse version */
	version_raw = fu_struct_telink_dfu_hdr_get_version(st_hdr);
	version = fu_version_from_uint32(version_raw, FWUPD_VERSION_FORMAT_QUAD);
	fu_firmware_set_version_raw(firmware, version_raw);
	fu_firmware_set_version(firmware, version);
	return TRUE;
}

static GByteArray *
fu_telink_dfu_firmware_write(FuFirmware *firmware, GError **error)
{
	//	FuTelinkDfuFirmware *self = FU_TELINK_DFU_FIRMWARE(firmware);
	g_autoptr(FuStructTelinkDfuHdr) st_hdr = fu_struct_telink_dfu_hdr_new();
	g_autoptr(GBytes) fw = NULL;
LOGD("start");

	/* header */
	fu_struct_telink_dfu_hdr_set_magic(st_hdr, TELINK_IMAGE_MAGIC_1);
	fu_struct_telink_dfu_hdr_set_version(st_hdr, fu_firmware_get_version_raw(firmware));

	/* data first */
	fw = fu_firmware_get_bytes_with_patches(firmware, error);
	if (fw == NULL)
		return NULL;

	/* TODO: write to the buffer with the correct format */
	fu_byte_array_append_bytes(st_hdr, fw);

	/* success */
	return g_steal_pointer(&st_hdr);
}

guint32
fu_telink_dfu_firmware_get_crc32(FuTelinkDfuFirmware *self)
{
	g_return_val_if_fail(FU_IS_TELINK_DFU_FIRMWARE(self), G_MAXUINT16);
LOGD("start");
	return self->crc32;
}

static void
fu_telink_dfu_firmware_init(FuTelinkDfuFirmware *self)
{
LOGD("start");
	//	fu_firmware_add_flag(FU_FIRMWARE(self), FU_FIRMWARE_FLAG_HAS_STORED_SIZE);
	fu_firmware_add_flag(FU_FIRMWARE(self), FU_FIRMWARE_FLAG_HAS_CHECKSUM);
	//	fu_firmware_add_flag(FU_FIRMWARE(self), FU_FIRMWARE_FLAG_HAS_VID_PID);
}

static void
fu_telink_dfu_firmware_class_init(FuTelinkDfuFirmwareClass *klass)
{
LOGD("start");
	FuFirmwareClass *firmware_class = FU_FIRMWARE_CLASS(klass);
	firmware_class->validate = fu_telink_dfu_validate;
	firmware_class->parse = fu_telink_dfu_firmware_parse;
	firmware_class->write = fu_telink_dfu_firmware_write;
	firmware_class->build = fu_telink_dfu_firmware_build;
	firmware_class->export = fu_telink_dfu_firmware_export;
}

FuFirmware *
fu_telink_dfu_firmware_new(void)
{
LOGD("start");
	return FU_FIRMWARE(g_object_new(FU_TYPE_TELINK_DFU_FIRMWARE, NULL));
}
