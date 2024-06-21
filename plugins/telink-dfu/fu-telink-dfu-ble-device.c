/*
 * Copyright 2024 Mike Chang <Mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "fu-telink-dfu-common.h"
#include "fu-telink-dfu-archive.h"
#include "fu-telink-dfu-ble-device.h"
#include "fu-telink-dfu-firmware.h"
#include "fu-telink-dfu-struct.h"

/* this can be set using Flags=example in the quirk file  */
#define FU_TELINK_DFU_BLE_DEVICE_FLAG_EXAMPLE (1 << 0)

struct _FuTelinkDfuBleDevice {
	FuBluezDevice parent_instance;
	gchar *board_name;
	gchar *bl_name;
	guint16 start_addr;
};

G_DEFINE_TYPE(FuTelinkDfuBleDevice, fu_telink_dfu_ble_device, FU_TYPE_BLUEZ_DEVICE)

#define FU_TELINK_DFU_HID_DEVICE_RETRY_INTERVAL 50 /* ms */
#define STREAM_START_ADDR 0x5000

#define CMD_OTA_FW_VERSION	0xff00
#define CMD_OTA_START		0xff01
#define CMD_OTA_END			0xff02
#define CMD_OTA_START_REQ	0xff03
#define CMD_OTA_START_RSP	0xff04
#define CMD_OTA_TEST		0xff05
#define CMD_OTA_TEST_RSP	0xff06
#define CMD_OTA_ERROR		0xff07

#define OTA_PREAMBLE_SIZE	2
#define OTA_PAYLOAD_SIZE	16
#define OTA_CRC_SIZE		2

typedef struct _TelinkDfuBlePacket {
	guint8 preamble[OTA_PREAMBLE_SIZE];
	guint8 payload[OTA_PAYLOAD_SIZE];
	guint8 crc16[OTA_CRC_SIZE];
} DfuBlePkt;

static void
fu_telink_dfu_ble_device_to_string(FuDevice *device, guint idt, GString *str)
{
	FuTelinkDfuBleDevice *self = FU_TELINK_DFU_BLE_DEVICE(device);
	fwupd_codec_string_append(str, idt, "BoardName", self->board_name);
	fwupd_codec_string_append(str, idt, "Bootloader", self->bl_name);

	LOGD("BoardName=%s,Bootloader=%s", self->board_name, self->bl_name);
}

#if DEVEL_STAGE_IGNORED == 1
	//todo: not used
#else
static gboolean
fu_telink_dfu_ble_device_detach(FuDevice *device, FuProgress *progress, GError **error)
{
	FuTelinkDfuBleDevice *self = FU_TELINK_DFU_BLE_DEVICE(device);

	/* sanity check */
	if (fu_device_has_flag(device, FWUPD_DEVICE_FLAG_IS_BOOTLOADER)) {
		g_debug("already in bootloader mode, skipping");
		return TRUE;
	}

	/* TODO: switch the device into bootloader mode */
	g_assert(self != NULL);

	fu_device_add_flag(device, FWUPD_DEVICE_FLAG_WAIT_FOR_REPLUG);
	return TRUE;
}

static gboolean
fu_telink_dfu_ble_device_attach(FuDevice *device, FuProgress *progress, GError **error)
{
	FuTelinkDfuBleDevice *self = FU_TELINK_DFU_BLE_DEVICE(device);

	/* sanity check */
	if (!fu_device_has_flag(device, FWUPD_DEVICE_FLAG_IS_BOOTLOADER)) {
		g_debug("already in runtime mode, skipping");
		return TRUE;
	}

	/* TODO: switch the device into runtime mode */
	g_assert(self != NULL);

	fu_device_add_flag(device, FWUPD_DEVICE_FLAG_WAIT_FOR_REPLUG);
	return TRUE;
}
#endif //DEVEL_STAGE_IGNORED

static gboolean
fu_telink_dfu_ble_device_reload(FuDevice *device, GError **error)
{
	FuTelinkDfuBleDevice *self = FU_TELINK_DFU_BLE_DEVICE(device);
	/* TODO: reprobe the hardware, or delete this vfunc to use ->setup() as a fallback */
	g_assert(self != NULL);
	return TRUE;
}

static gboolean
fu_telink_dfu_ble_device_probe(FuDevice *device, GError **error)
{
	FuTelinkDfuBleDevice *self = FU_TELINK_DFU_BLE_DEVICE(device);

	g_assert(self != NULL);

	/* success */
	return TRUE;
}

static gboolean
fu_telink_dfu_ble_device_setup(FuDevice *device, GError **error)
{
	FuTelinkDfuBleDevice *self = FU_TELINK_DFU_BLE_DEVICE(device);

	/* TODO: get the version and other properties from the hardware while open */
	g_assert(self != NULL);
	//	fu_device_set_version(device, "1.2.3");

	/* success */
	return TRUE;
}

static gboolean
fu_telink_dfu_ble_device_prepare(FuDevice *device,
				 FuProgress *progress,
				 FwupdInstallFlags flags,
				 GError **error)
{
	FuTelinkDfuBleDevice *self = FU_TELINK_DFU_BLE_DEVICE(device);
#if DEBUG_GATT_CHAR_RW == 1
	g_autoptr(GByteArray)buf_read = g_byte_array_new();
	g_autoptr(GByteArray)buf_write = g_byte_array_new();
	gboolean res;
#endif

	/* TODO: anything the device has to do before the update starts */
	g_assert(self != NULL);


#if DEBUG_GATT_CHAR_RW == 1
	buf_read = fu_bluez_device_read(FU_BLUEZ_DEVICE(self), CHAR_UUID_BATT, error);
	if (buf_read) {
		for (guint i = 0; i < buf_read->len; i++) {
			LOGD("BATT:0x%x", buf_read->data[i]);
		}
	}
	buf_read = fu_bluez_device_read(FU_BLUEZ_DEVICE(self), CHAR_UUID_PNP, error);
	if (buf_read) {
		for (guint i = 0; i < buf_read->len; i++) {
			LOGD("PNP:0x%x", buf_read->data[i]);
		}
	}
	buf_read = fu_bluez_device_read(FU_BLUEZ_DEVICE(self), CHAR_UUID_OTA, error);
	if (buf_read) {
		for (guint i = 0; i < buf_read->len; i++) {
			LOGD("OTA:0x%x", buf_read->data[i]);
		}
	}
	g_byte_array_append (buf_write, (guint8 *) "12345abcde", 10);
	res = fu_bluez_device_write(FU_BLUEZ_DEVICE(self), CHAR_UUID_OTA, buf_write, error);
	LOGD("fu_bluez_device_write, res=%d", res);
#endif

	return TRUE;
}

static gboolean
fu_telink_dfu_ble_device_cleanup(FuDevice *device,
				 FuProgress *progress,
				 FwupdInstallFlags flags,
				 GError **error)
{
	FuTelinkDfuBleDevice *self = FU_TELINK_DFU_BLE_DEVICE(device);
	/* TODO: anything the device has to do when the update has completed */
	g_assert(self != NULL);
	return TRUE;
}

#if USE_FIRMWARE_GTYPE != 1
static FuFirmware *
fu_telink_dfu_ble_device_prepare_firmware(FuDevice *device,
					  GInputStream *stream,
					  FuProgress *progress,
					  FwupdInstallFlags flags,
					  GError **error)
{
	FuTelinkDfuBleDevice *self = FU_TELINK_DFU_BLE_DEVICE(device);
	g_autoptr(FuFirmware) firmware = fu_telink_dfu_firmware_new();

	/* TODO: you do not need to use this vfunc if not checking attributes */
	if (self->start_addr !=
	    fu_telink_dfu_firmware_get_crc32(FU_TELINK_DFU_FIRMWARE(firmware))) {
		g_set_error(error,
			    FWUPD_ERROR,
			    FWUPD_ERROR_INVALID_FILE,
			    "start address mismatch, got 0x%04x, expected 0x%04x",
			    fu_telink_dfu_firmware_get_crc32(FU_TELINK_DFU_FIRMWARE(firmware)),
			    self->start_addr);
		return NULL;
	}

	if (!fu_firmware_parse_stream(firmware, stream, 0x0, flags, error))
		return NULL;
	return g_steal_pointer(&firmware);
}
#endif //USE_FIRMWARE_GTYPE

#if DFU_WRITE_METHOD == DFU_WRITE_METHOD_CHUNKS
static gboolean
fu_telink_dfu_ble_device_write_blocks(FuTelinkDfuBleDevice *self,
				      FuChunkArray *chunks,
				      FuProgress *progress,
				      GError **error)
{
	/* progress */
	fu_progress_set_id(progress, G_STRLOC);
	fu_progress_set_steps(progress, fu_chunk_array_length(chunks));
	for (guint i = 0; i < fu_chunk_array_length(chunks); i++) {
		g_autoptr(FuChunk) chk = NULL;
		guint8 buf[64] = {0x12, 0x24, 0x0}; /* TODO: this is the preamble */

		/* prepare chunk */
		chk = fu_chunk_array_index(chunks, i, error);
		if (chk == NULL)
			return FALSE;

		/* TODO: send to hardware */
		if (!fu_memcpy_safe(buf,
				    sizeof(buf),
				    0x2, /* TODO: copy to dst at offset */
				    fu_chunk_get_data(chk),
				    fu_chunk_get_data_sz(chk),
				    0x0, /* src */
				    fu_chunk_get_data_sz(chk),
				    error))
			return FALSE;
#if 0
		g_autoptr(GByteArray)buf = fu_bluez_device_read(FU_BLUEZ_DEVICE(self), DI_SYSTEM_ID_UUID, error);
		if (buf == NULL)
			return FALSE;
#endif

		/* update progress */
		fu_progress_step_done(progress);
	}

	/* success */
	return TRUE;
}
#else
//DFU_WRITE_METHOD == DFU_WRITE_METHOD_CUST_PACKET or not defined
#if DEBUG_WRITE_METHOD_CUST_PACKET == 1
static void
dump_hex(gpointer buf, gsize buf_len)
{
	gpointer hex_str_buf = g_malloc(buf_len * 3 + 1);
	gsize i;
	gchar *hex_str = (gchar *)hex_str_buf;
	gchar *hex_buf = (gchar *)buf;

	for (i = 0; i < buf_len; i++) {
		g_snprintf(hex_str + i * 3, 4, "%02x ", (guint) (*((guint8 *)(hex_buf + i))));
	}
	hex_str[buf_len * 3] = 0;

	LOGD("%s", hex_str);

	g_free(hex_str_buf);
}
#endif

static guint16
calc_crc16(guint8 *buf, gsize buf_len)
{
	guint16 ret = 0;

	while (buf_len > 0) {
		buf_len--;
		ret += buf[buf_len];
	}

	return ret;
}

static void
create_dfu_packet(DfuBlePkt *pkt, guint16 preamble, const guint8 *payload)
{
	guint8 *d;
	guint16 crc_val;

	if (pkt == NULL) {
		return;
	}

	d = (guint8 *)&preamble;
	pkt->preamble[0] = d[0];
	pkt->preamble[1] = d[1];
	if (payload == NULL) {
		memset(pkt->payload, 0, OTA_PAYLOAD_SIZE);
		crc_val = 0;
	} else {
		memcpy(pkt->payload, payload, OTA_PAYLOAD_SIZE);
		crc_val = calc_crc16(pkt->payload, OTA_PREAMBLE_SIZE + OTA_PAYLOAD_SIZE);
	}
	d = (guint8 *)&crc_val;
	pkt->crc16[0] = d[0];
	pkt->crc16[1] = d[1];

#if DEBUG_WRITE_METHOD_CUST_PACKET == 1
	dump_hex((gpointer)pkt, sizeof(DfuBlePkt));
#endif
}

static gboolean
send_dfu_packet(FuTelinkDfuBleDevice *self, DfuBlePkt *pkt, GError **error)
{
	gboolean res = TRUE;
	g_autoptr(GByteArray)buf_write = g_byte_array_new();

	g_byte_array_append (buf_write, (guint8 *)pkt, OTA_PREAMBLE_SIZE + OTA_PAYLOAD_SIZE + OTA_CRC_SIZE);
	res = fu_bluez_device_write(FU_BLUEZ_DEVICE(self), CHAR_UUID_OTA, buf_write, error);
	LOGD("fu_bluez_device_write, res=%d", res);

	return res;
}

static gboolean
fu_telink_dfu_ble_device_write_packets(FuTelinkDfuBleDevice *self,
				      GBytes *blob,
				      FuProgress *progress,
				      GError **error)
{
	const guint8 *raw_data;
	const guint8 *d;
	guint32 i;
	gsize image_len = 0;
	DfuBlePkt pkt;
	guint8 payload[OTA_PAYLOAD_SIZE] = {0};

	//1. OTA start command
	create_dfu_packet(&pkt, CMD_OTA_START, NULL);
	send_dfu_packet(self, &pkt, error);

	//2. OTA firmware data
	raw_data = g_bytes_get_data(blob, &image_len);
	for (i = 0; i < image_len; i += OTA_PAYLOAD_SIZE) {
		d = raw_data + i;
		if ((i + OTA_PAYLOAD_SIZE) > image_len) {
			memcpy(payload, d, i % OTA_PAYLOAD_SIZE);
			create_dfu_packet(&pkt, (guint16)(i >> 4) + 1, payload);
		} else {
			create_dfu_packet(&pkt, (guint16)(i >> 4), d);
		}
		send_dfu_packet(self, &pkt, error);
	}

	//3. OTA stop command
	create_dfu_packet(&pkt, CMD_OTA_END, NULL);
	send_dfu_packet(self, &pkt, error);

	/* success */
	return TRUE;
}
#endif //DFU_WRITE_METHOD

static gboolean
fu_telink_dfu_ble_device_write_firmware(FuDevice *device,
					FuFirmware *firmware,
					FuProgress *progress,
					FwupdInstallFlags flags,
					GError **error)
{
	FuTelinkDfuBleDevice *self = FU_TELINK_DFU_BLE_DEVICE(device);
	g_autoptr(GInputStream) stream = NULL;
#if DFU_WRITE_METHOD == DFU_WRITE_METHOD_CHUNKS
	g_autoptr(FuChunkArray) chunks = NULL;
#endif
#if DFU_WRITE_METHOD == DFU_WRITE_METHOD_CUST_PACKET
	g_autoptr(GBytes) blob = NULL;
	g_autoptr(FuArchive) archive = NULL;
	const gchar *filename = "firmware.bin";
#if DEBUG_WRITE_METHOD_CUST_PACKET == 1
	const guint8 *d;
	gsize image_len = 0;
#endif
#endif

	/* progress */
	fu_progress_set_id(progress, G_STRLOC);
	fu_progress_add_flag(progress, FU_PROGRESS_FLAG_GUESSED);
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_WRITE, 44, NULL);
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_VERIFY, 35, NULL);

	/* get default image */
	stream = fu_firmware_get_stream(firmware, error);
	if (stream == NULL)
		return FALSE;

#if DFU_WRITE_METHOD == DFU_WRITE_METHOD_CHUNKS
	/* write each block */
	chunks =
	    fu_chunk_array_new_from_stream(stream, self->start_addr, TELINK_FW_CHUNCK_SIZE, error);
	if (chunks == NULL)
		return FALSE;
	if (!fu_telink_dfu_ble_device_write_blocks(self,
						   chunks,
						   fu_progress_get_child(progress),
						   error))
		return FALSE;
#else
	//DFU_WRITE_METHOD == DFU_WRITE_METHOD_CUST_PACKET or not defined
	archive = fu_archive_new_stream(stream, FU_ARCHIVE_FLAG_IGNORE_PATH, error);
	if (archive == NULL)
		return FALSE;

//	blob = fu_input_stream_read_bytes(stream, 0, G_MAXSIZE, error);
	blob = fu_archive_lookup_by_fn(archive, filename, error);
	if (blob == NULL) {
		return FALSE;
	}

#if DEBUG_WRITE_METHOD_CUST_PACKET == 1
	d = g_bytes_get_data(blob, &image_len);
	LOGD("image_len=%lu", image_len);
	dump_hex((gpointer)d, 16);
#endif

	if (!fu_telink_dfu_ble_device_write_packets(self,
						   blob,
						   fu_progress_get_child(progress),
						   error))
		return FALSE;
#endif
	fu_progress_step_done(progress);

	/* TODO: verify each block */
	fu_progress_step_done(progress);

	/* success! */
	return TRUE;
}

static gboolean
fu_telink_dfu_ble_device_set_quirk_kv(FuDevice *device,
				      const gchar *key,
				      const gchar *value,
				      GError **error)
{
	FuTelinkDfuBleDevice *self = FU_TELINK_DFU_BLE_DEVICE(device);

	/* parse value from quirk file */
	if (g_strcmp0(key, "TelinkDfuBootType") == 0) {
		if (g_strcmp0(value, "beta") == 0 || g_strcmp0(value, "otav1") == 0) {
			self->bl_name = g_strdup(value);
		} else {
			g_set_error_literal(error,
					    FWUPD_ERROR,
					    FWUPD_ERROR_INVALID_DATA,
					    "bad TelinkDfuBootType");
			return FALSE;
		}
	}
	if (g_strcmp0(key, "TelinkDfuBoardType") == 0) {
		if (g_strcmp0(value, "tlsr8278") == 0 || g_strcmp0(value, "tlsr8208") == 0) {
			self->board_name = g_strdup(value);
		} else {
			g_set_error_literal(error,
					    FWUPD_ERROR,
					    FWUPD_ERROR_INVALID_DATA,
					    "bad TelinkDfuBoardType");
			return FALSE;
		}
	}

	/* failed */
	g_set_error_literal(error,
			    FWUPD_ERROR,
			    FWUPD_ERROR_NOT_SUPPORTED,
			    "quirk key not supported");
	return FALSE;
}

static void
fu_telink_dfu_ble_device_set_progress(FuDevice *self, FuProgress *progress)
{
	fu_progress_set_id(progress, G_STRLOC);
	fu_progress_add_flag(progress, FU_PROGRESS_FLAG_GUESSED);
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_RESTART, 0, "detach");
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_WRITE, 57, "write");
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_RESTART, 0, "attach");
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_BUSY, 43, "reload");
}

static void
fu_telink_dfu_ble_device_init(FuTelinkDfuBleDevice *self)
{
	self->start_addr = STREAM_START_ADDR;
	fu_device_set_vendor(FU_DEVICE(self), "Telink");
	fu_device_set_version_format(FU_DEVICE(self), FWUPD_VERSION_FORMAT_QUAD);
	fu_device_set_remove_delay(FU_DEVICE(self), FU_DEVICE_REMOVE_DELAY_RE_ENUMERATE);
	fu_device_set_firmware_gtype(FU_DEVICE(self), FU_TYPE_TELINK_DFU_ARCHIVE);
	fu_device_add_protocol(FU_DEVICE(self), "com.telink.dfu");
	fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_UPDATABLE);
	// todo: FWUPD_DEVICE_FLAG_SIGNED_PAYLOAD?
	fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_UNSIGNED_PAYLOAD);
	fu_device_add_internal_flag(FU_DEVICE(self), FU_DEVICE_INTERNAL_FLAG_ONLY_WAIT_FOR_REPLUG);
	fu_device_retry_set_delay(FU_DEVICE(self), FU_TELINK_DFU_HID_DEVICE_RETRY_INTERVAL);
	fu_device_register_private_flag(FU_DEVICE(self),
					FU_TELINK_DFU_BLE_DEVICE_FLAG_EXAMPLE,
					"example");
}

static void
fu_telink_dfu_ble_device_finalize(GObject *object)
{
	FuTelinkDfuBleDevice *self = FU_TELINK_DFU_BLE_DEVICE(object);
	g_free(self->board_name);
	g_free(self->bl_name);
	G_OBJECT_CLASS(fu_telink_dfu_ble_device_parent_class)->finalize(object);
}

static void
fu_telink_dfu_ble_device_class_init(FuTelinkDfuBleDeviceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	FuDeviceClass *device_class = FU_DEVICE_CLASS(klass);
	object_class->finalize = fu_telink_dfu_ble_device_finalize;
	device_class->to_string = fu_telink_dfu_ble_device_to_string;
	device_class->probe = fu_telink_dfu_ble_device_probe;
	device_class->setup = fu_telink_dfu_ble_device_setup;
	device_class->reload = fu_telink_dfu_ble_device_reload;
	device_class->prepare = fu_telink_dfu_ble_device_prepare;
	device_class->cleanup = fu_telink_dfu_ble_device_cleanup;
#if DEVEL_STAGE_IGNORED == 1
	//todo: not used
#else
	device_class->attach = fu_telink_dfu_ble_device_attach;
	device_class->detach = fu_telink_dfu_ble_device_detach;
#endif
#if USE_FIRMWARE_GTYPE != 1
	device_class->prepare_firmware = fu_telink_dfu_ble_device_prepare_firmware;
#endif
	device_class->write_firmware = fu_telink_dfu_ble_device_write_firmware;
	device_class->set_quirk_kv = fu_telink_dfu_ble_device_set_quirk_kv;
	device_class->set_progress = fu_telink_dfu_ble_device_set_progress;
}
