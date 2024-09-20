/*
 * Copyright 2024 Mike Chang <Mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#ifdef HAVE_HIDRAW_H
#include <linux/hidraw.h>
#include <linux/input.h>
#endif

#include "fu-telink-dfu-archive.h"
#include "fu-telink-dfu-hid-device.h"
#include "fu-telink-dfu-struct.h"

struct _FuTelinkDfuHidDevice {
	FuHidDevice parent_instance;;
};

G_DEFINE_TYPE(FuTelinkDfuHidDevice, fu_telink_dfu_hid_device, FU_TYPE_HID_DEVICE)

#define FU_TELINK_DFU_HID_DEVICE_START_ADDR 0x0000
#define FU_TELINK_DFU_HID_DEVICE_RETRY_DELAY	50 /* ms */
#define FU_TELINK_DFU_HID_DEVICE_IOCTL_TIMEOUT	500 /* ms */
#define FU_TELINK_DFU_HID_DEVICE_REPORT_ID		7
#define FU_TELINK_DFU_HID_DEVICE_REPORT_TIMEOUT	500 /* ms */
#define HID_IFACE	2
#define HID_EP_IN	(0x80 | 4)
#define HID_EP_OUT	(0x00 | 5)

static gboolean
fu_telink_dfu_hid_device_setup(FuDevice *device, GError **error)
{
	/* FuUsbDevice->setup */
	if (!FU_DEVICE_CLASS(fu_telink_dfu_hid_device_parent_class)->setup(device, error))
		return FALSE;

	fu_device_set_name(device, "telink-hid-device");

	/* success */
	return TRUE;
}

static FuStructTelinkDfuHidPkt *
fu_telink_dfu_hid_device_create_packet(guint16 preamble, const guint8 *buf, gsize bufsz)
{
	FuStructTelinkDfuHidPkt *pkt = fu_struct_telink_dfu_hid_pkt_new();
	fu_struct_telink_dfu_hid_pkt_set_preamble(pkt, preamble);
	if (buf != NULL)
		fu_struct_telink_dfu_hid_pkt_set_payload(pkt, buf, bufsz, NULL);
	fu_struct_telink_dfu_hid_pkt_set_crc(pkt, ~fu_crc16(pkt->data, pkt->len - 2));
	return pkt;
}

static gboolean
fu_telink_dfu_hid_device_write(FuTelinkDfuHidDevice *self,
			       guint8 *buf,
			       gsize bufsz,
			       GError **error)
{
#ifdef HAVE_HIDRAW_H
	FuHidDevice *hid_device = FU_HID_DEVICE(self);
	FuUsbInterface *iface = NULL;
	g_autoptr(GPtrArray) ifaces = NULL;
	guint32 idx;

	ifaces = fu_usb_device_get_interfaces(FU_USB_DEVICE(FU_DEVICE(self)), error);
	//debuging: the last interface is used for DFU(here should be interface 2)
	for (idx = 0; idx < ifaces->len; idx++) {
		iface = g_ptr_array_index(ifaces, idx);
		g_debug("intf %u class=%u", idx, fu_usb_interface_get_class(iface));
	}

	fu_hid_device_set_interface(hid_device, ifaces->len - 1);
	//endpoint 4
	fu_hid_device_set_ep_addr_in(hid_device, HID_EP_IN);
	//endpoint 5
	fu_hid_device_set_ep_addr_out(hid_device, HID_EP_OUT);

	//set feature report: report id 7
/**
08:35:45.650 FuPluginTelinkDfu    intf 0 class=3
08:35:45.650 FuPluginTelinkDfu    intf 1 class=3
08:35:45.650 FuPluginTelinkDfu    intf 2 class=3
08:35:45.650 FuHidDevice          HID::SetReport [wValue=0x0307, wIndex=2]:01 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff 11 a8
libusb: error [submit_control_transfer] submiturb failed, errno=16
08:35:45.650 FuHidDevice          ignoring: USB error: Entity not found [-5]
08:35:45.651 FuHistory            modifying device telink-hid-device [1f914e619b232dc4da68ec3389b98b20163003a2]
08:35:45.921 FuHistory            modifying device telink-hid-device [1f914e619b232dc4da68ec3389b98b20163003a2]
08:35:46.201 FuEngine             failed write-firmware 'failed to SetReport: USB error: Input/Output Error [-1]': FuTelinkDfuHidDevice:
  DeviceId:             1f914e619b232dc4da68ec3389b98b20163003a2
  Name:                 telink-hid-device
  Status:               device-write
  Percentage:           1
  Guid:                 dcb27849-1baa-5fbf-8f42-433980d700b4 ← USB\VID_248A&PID_881C
  Serial:               TLSR9518
  Plugin:               telink_dfu
  Protocol:             com.telink.dfu
  Flags:                updatable|registered|dual-image|unsigned-payload
  Vendor:               Maxxter
  VendorId:             USB:0x248A
  Version:              2.0
  VersionFormat:        pair
  VersionRaw:           0x00000200
  Created:              2024-10-23
  Modified:             2024-10-23
  UpdateState:          failed
  UpdateMessage:        Device firmware update for TLSR8272 Dongle
  Guid[quirk]:          4abeb920-10d6-57f7-8d88-ae65c98a9ff2 ← USB\VID_248A
  Guid[quirk]:          571229d5-caed-5cbc-91c1-5c216601a194 ← USB\CLASS_03
  Guid[quirk]:          8eeac4f3-d98d-57aa-a1fc-b44e8d3d8ba3 ← USB\CLASS_03&SUBCLASS_01
  Guid[quirk]:          c33b161d-a538-57df-b1c8-90817ada2387 ← USB\CLASS_03&SUBCLASS_01&PROT_01
  Guid[quirk]:          557f3c78-a2c2-5d76-a7d5-11580491c894 ← USB\CLASS_03&SUBCLASS_01&PROT_02
  Guid[quirk]:          cee9c5bc-aa12-51eb-a220-d0ef588d7cb2 ← USB\CLASS_03&SUBCLASS_00
  Guid[quirk]:          afa5518d-7c55-51cb-8e1c-bf3b11b67dfa ← USB\CLASS_03&SUBCLASS_00&PROT_00
  PhysicalId:           usb:01:00:03
  BackendId:            01:2c
  RemoveDelay:          10000
  AcquiesceDelay:       2500
  GType:                FuTelinkDfuHidDevice
  FirmwareGType:        FuTelinkDfuArchive
  Order:                0
  PossiblePlugin:       telink_dfu
  ParentPhysicalIds:    usb:01:00,usb:01
  InternalFlags:        only-wait-for-replug
  Events:
  BusNum:               0x1
  DevNum:               0x2c
  Class:                interface-desc
  Interfaces:
    FuUsbInterface:
        InterfaceClass: 0x3
        Length:         0x9
        DescriptorType: 0x4
        InterfaceSubClass: 0x1
        InterfaceProtocol: 0x1
        UsbEndpoints:
            EndpointAddress: 0x81
            DescriptorType: 0x5
            Interval:   0xa
            MaxPacketSize: 0x8
        ExtraData:      CSERASEBIjsA
    FuUsbInterface:
        DescriptorType: 0x4
        Length:         0x9
        ExtraData:      CSERASEBIo4A
        InterfaceClass: 0x3
        InterfaceSubClass: 0x1
        InterfaceNumber: 0x1
        UsbEndpoints:
            EndpointAddress: 0x82
            DescriptorType: 0x5
            Interval:   0x4
            MaxPacketSize: 0x8
        InterfaceProtocol: 0x2
    FuUsbInterface:
        InterfaceClass: 0x3
        Length:         0x9
        DescriptorType: 0x4
        UsbEndpoints:
            EndpointAddress: 0x84
            DescriptorType: 0x5
            Interval:   0x10
            MaxPacketSize: 0x40
        InterfaceNumber: 0x2
        ExtraData:      CSERASEBIh8A
  InterfaceAutodetect:  false
  Interface:            0x2
  EpAddrIn:             0x84
  EpAddrOut:            0x5
 */
	if (!fu_hid_device_set_report(FU_HID_DEVICE(self),
					FU_TELINK_DFU_HID_DEVICE_REPORT_ID,
					buf,
					bufsz,
					FU_TELINK_DFU_HID_DEVICE_REPORT_TIMEOUT,
					FU_HID_DEVICE_FLAG_IS_FEATURE,
					error))
		return FALSE;

	return TRUE;
#else
	g_set_error_literal(error,
			    FWUPD_ERROR,
			    FWUPD_ERROR_NOT_SUPPORTED,
			    "<linux/hidraw.h> not available");
	return FALSE;
#endif
}

static gboolean
fu_telink_dfu_hid_device_write_blocks(FuTelinkDfuHidDevice *self,
				      FuChunkArray *chunks,
				      FuProgress *progress,
				      GError **error)
{
	/* progress */
	fu_progress_set_id(progress, G_STRLOC);
	fu_progress_set_steps(progress, fu_chunk_array_length(chunks));
	for (guint i = 0; i < fu_chunk_array_length(chunks); i++) {
		g_autoptr(FuChunk) chk = NULL;
		g_autoptr(FuStructTelinkDfuHidPkt) pkt = NULL;

		/* send chunk */
		chk = fu_chunk_array_index(chunks, i, error);
		if (chk == NULL)
			return FALSE;
		pkt = fu_telink_dfu_hid_device_create_packet((guint16)i,
							     fu_chunk_get_data(chk),
							     fu_chunk_get_data_sz(chk));
		if (!fu_telink_dfu_hid_device_write(self,
					   pkt->data,
					   pkt->len,
					   error))
			return FALSE;
		fu_device_sleep(FU_DEVICE(self), 5);

		/* update progress */
		fu_progress_step_done(progress);
	}

	/* success */
	return TRUE;
}


static gboolean
fu_telink_dfu_hid_device_ota_start(FuTelinkDfuHidDevice *self, GError **error)
{
	g_autoptr(FuStructTelinkDfuHidPkt) pkt = NULL;

	pkt = fu_telink_dfu_hid_device_create_packet(FU_TELINK_DFU_CMD_OTA_START, NULL, 0);
	if (!fu_telink_dfu_hid_device_write(self,
				   pkt->data,
				   pkt->len,
				   error))
		return FALSE;

	/* success */
	fu_device_sleep(FU_DEVICE(self), 5);
	return TRUE;
}

static gboolean
fu_telink_dfu_hid_device_ota_stop(FuTelinkDfuHidDevice *self, guint number_chunks, GError **error)
{
	guint8 pkt_stop_data[4] = {0};
	g_autoptr(FuStructTelinkDfuHidPkt) pkt = NULL;

	/* last data packet index */
	fu_memwrite_uint16(pkt_stop_data, (number_chunks >> 4) - 1, G_LITTLE_ENDIAN);
	pkt_stop_data[2] = ~pkt_stop_data[0];
	pkt_stop_data[3] = ~pkt_stop_data[1];
	pkt = fu_telink_dfu_hid_device_create_packet(FU_TELINK_DFU_CMD_OTA_END,
						     pkt_stop_data,
						     sizeof(pkt_stop_data));
	if (!fu_telink_dfu_hid_device_write(self,
				   pkt->data,
				   pkt->len,
				   error))
		return FALSE;

	/* success */
	fu_device_sleep(FU_DEVICE(self), 20000);
	return TRUE;
}

static gboolean
fu_telink_dfu_hid_device_write_blob(FuTelinkDfuHidDevice *self,
				    GBytes *blob,
				    FuProgress *progress,
				    GError **error)
{
	g_autoptr(FuChunkArray) chunks = NULL;
	g_autoptr(FuStructTelinkDfuHidPkt) pkt = NULL;

	/* progress */
	fu_progress_set_id(progress, G_STRLOC);
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_WRITE, 1, "ota-start");
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_WRITE, 70, "ota-data");
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_WRITE, 29, "ota-stop");

	/* OTA start command */
	if (!fu_telink_dfu_hid_device_ota_start(self, error))
		return FALSE;
	fu_progress_step_done(progress);

	/* OTA firmware data */
	chunks = fu_chunk_array_new_from_bytes(blob,
					       FU_TELINK_DFU_HID_DEVICE_START_ADDR,
					       FU_STRUCT_TELINK_DFU_BLE_PKT_SIZE_PAYLOAD);
	if (!fu_telink_dfu_hid_device_write_blocks(self,
						   chunks,
						   fu_progress_get_child(progress),
						   error))
		return FALSE;
	fu_progress_step_done(progress);

	/* OTA stop command */
	if (!fu_telink_dfu_hid_device_ota_stop(self, fu_chunk_array_length(chunks), error))
		return FALSE;
	fu_progress_step_done(progress);

	/* success */
	fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_WAIT_FOR_REPLUG);
	return TRUE;
}

static gboolean
fu_telink_dfu_hid_device_write_firmware(FuDevice *device,
					FuFirmware *firmware,
					FuProgress *progress,
					FwupdInstallFlags flags,
					GError **error)
{
	FuTelinkDfuHidDevice *self = FU_TELINK_DFU_HID_DEVICE(device);
	g_autoptr(FuArchive) archive = NULL;
	g_autoptr(GBytes) blob = NULL;
	g_autoptr(GInputStream) stream = NULL;

	/* get default image */
	stream = fu_firmware_get_stream(firmware, error);
	if (stream == NULL)
		return FALSE;
	archive = fu_archive_new_stream(stream, FU_ARCHIVE_FLAG_IGNORE_PATH, error);
	if (archive == NULL)
		return FALSE;
	// FIXME, check not using .cab in fwupdtool install-blob
	blob = fu_archive_lookup_by_fn(archive, "firmware.bin", error);
	if (blob == NULL)
		return FALSE;
	return fu_telink_dfu_hid_device_write_blob(self, blob, progress, error);
}

static void
fu_telink_dfu_hid_device_set_progress(FuDevice *self, FuProgress *progress)
{
	fu_progress_set_id(progress, G_STRLOC);
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_RESTART, 0, "detach");
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_WRITE, 100, "write");
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_RESTART, 0, "attach");
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_BUSY, 0, "reload");
}

static void
fu_telink_dfu_hid_device_init(FuTelinkDfuHidDevice *self)
{
	fu_device_set_vendor(FU_DEVICE(self), "Telink");
	/* read the ReleaseNumber field of USB descriptor */
	fu_device_set_version_format(FU_DEVICE(self), FWUPD_VERSION_FORMAT_PAIR);
	fu_device_set_remove_delay(FU_DEVICE(self), 10000); /* ms */
	fu_device_set_firmware_gtype(FU_DEVICE(self), FU_TYPE_TELINK_DFU_ARCHIVE);
	fu_device_add_protocol(FU_DEVICE(self), "com.telink.dfu");
	fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_UPDATABLE);
	fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_UNSIGNED_PAYLOAD);
	fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_DUAL_IMAGE);
	fu_device_add_internal_flag(FU_DEVICE(self), FU_DEVICE_INTERNAL_FLAG_ONLY_WAIT_FOR_REPLUG);
}

static void
fu_telink_dfu_hid_device_class_init(FuTelinkDfuHidDeviceClass *klass)
{
	FuDeviceClass *device_class = FU_DEVICE_CLASS(klass);
	device_class->setup = fu_telink_dfu_hid_device_setup;
	device_class->write_firmware = fu_telink_dfu_hid_device_write_firmware;
	device_class->set_progress = fu_telink_dfu_hid_device_set_progress;
}
