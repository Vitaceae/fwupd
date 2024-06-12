/*
 * Copyright 2024 Mike Chang <mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "fu-telink-common.h"
#include "fu-telink-firmware.h"

typedef struct {
	guint32 crc32;
} FuTelinkFwPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(FuTelinkFw, fu_telink_fw, FU_TYPE_FIRMWARE)
#define GET_PRIVATE(x) (fu_telink_fw_get_instance_private(x))

static void
fu_telink_fw_export(FuFirmware *firmware, FuFirmwareExportFlags flags, XbBuilderNode *bn)
{
	FuTelinkFw *self = FU_TELINK_FW(firmware);
	FuTelinkFwPrivate *priv = GET_PRIVATE(self);
	fu_xmlb_builder_insert_kx(bn, "crc32", priv->crc32);
}

static gchar *
fu_telink_fw_get_checksum(FuFirmware *firmware, GChecksumType csum_kind, GError **error)
{
	FuTelinkFw *self = FU_TELINK_FW(firmware);
	FuTelinkFwPrivate *priv = GET_PRIVATE(self);
	if (!fu_firmware_has_flag(firmware, FU_FIRMWARE_FLAG_HAS_CHECKSUM)) {
		g_set_error_literal(error,
				    G_IO_ERROR,
				    G_IO_ERROR_NOT_SUPPORTED,
				    "unable to calculate the checksum of the update binary");
		return NULL;
	}
	return g_strdup_printf("%x", priv->crc32);
}

static guint32
fu_telink_fw_crc32(const guint8 *buf, gsize bufsz)
{
	guint crc32 = 0x01;
	/* maybe skipped "^" step in fu_common_crc32_full()?
	 * according https://github.com/madler/zlib/blob/master/crc32.c#L225 */
	crc32 ^= 0xFFFFFFFFUL;
	return fu_common_crc32_full(buf, bufsz, crc32, 0xEDB88320);
}

static gboolean
fu_telink_fw_parse(FuFirmware *firmware,
		   GBytes *fw,
		   guint64 addr_start,
		   guint64 addr_end,
		   FwupdInstallFlags flags,
		   GError **error)
{
	FuTelinkFw *self = FU_TELINK_FW(firmware);
	FuTelinkFwPrivate *priv = GET_PRIVATE(self);
	const guint8 *buf;
	gsize bufsz = 0;

	LOGD("start");

	buf = g_bytes_get_data(fw, &bufsz);
	if (buf == NULL) {
		g_set_error_literal(error,
				    FWUPD_ERROR,
				    FWUPD_ERROR_INVALID_FILE,
				    "unable to get the image binary");
		return FALSE;
	}

	LOGD("\n\nfirmware header=\n%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X "
	     "%02X %02X %02X %02X\n",
	     buf[0],
	     buf[1],
	     buf[2],
	     buf[3],
	     buf[4],
	     buf[5],
	     buf[6],
	     buf[7],
	     buf[8],
	     buf[9],
	     buf[10],
	     buf[11],
	     buf[12],
	     buf[13],
	     buf[14],
	     buf[15]);

	fu_firmware_add_flag(FU_FIRMWARE(self), FU_FIRMWARE_FLAG_HAS_CHECKSUM);
	priv->crc32 = fu_telink_fw_crc32(buf, bufsz);

	/* do not strip the header */
	fu_firmware_set_bytes(firmware, fw);

	return TRUE;
}

static void
fu_telink_fw_class_init(FuTelinkFwClass *klass)
{
	FuFirmwareClass *klass_firmware = FU_FIRMWARE_CLASS(klass);

	klass_firmware->export = fu_telink_fw_export;
	klass_firmware->get_checksum = fu_telink_fw_get_checksum;
	klass_firmware->parse = fu_telink_fw_parse;
}

static void
fu_telink_fw_init(FuTelinkFw *self)
{
	LOGD("start");
	// todo
}
