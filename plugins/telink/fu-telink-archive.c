/* fu-telink-archive.c
 *
 * Copyright (C) 2024 Mike Chang <mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include "fu-telink-archive.h"

struct _FuTelinkArchive {
    FuFirmwareClass parent_instance;
};

G_DEFINE_TYPE(FuTelinkArchive, fu_telink_archive, FU_TYPE_FIRMWARE)

static gboolean
fu_telink_archive_parse(FuFirmware *firmware, GBytes *fw, guint64 addr_start, guint64 addr_end, FwupdInstallFlags flags, GError **error)
{
    //todo
    return TRUE;
}

static void
fu_telink_archive_class_init(FuTelinkArchiveClass *klass)
{
    FuFirmwareClass *klass_firmware = FU_FIRMWARE_CLASS(klass);
    klass_firmware->parse = fu_telink_archive_parse;
}

static void
fu_telink_archive_init(FuTelinkArchive *self)
{
    //todo
}

