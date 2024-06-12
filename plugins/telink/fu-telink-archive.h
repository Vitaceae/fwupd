/*
 * Copyright 2024 Mike Chang <mike.chang@telink-semi.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <fwupdplugin.h>

#define FU_TYPE_TELINK_ARCHIVE (fu_telink_archive_get_type())
G_DECLARE_FINAL_TYPE(FuTelinkArchive, fu_telink_archive, FU, TELINK_ARCHIVE, FuArchiveFirmware)
