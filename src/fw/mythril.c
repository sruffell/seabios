// Mythril VM Support
//
// Copyright (C) 2020 Dan Robertson <daniel.robertson@starlab.io>
//
// Authors:
//  Dan Robertson <daniel.robertson@starlab.io>
//
// This file may be distributed under the terms of the GNU LGPLv3 license.

#include "byteorder.h" // be32_to_cpu
#include "paravirt.h" // PlatformRunningOn
#include "string.h" // memcpy
#include "output.h" // dprintf
#include "util.h" // dprintf

#define INFO_PHYSICAL_ADDR 0x40000000

// TODO(dlrobertson): Do we have other info we want to pass to seabios?
struct mythril_info {
    u8 signature[9];
    void* rsdp;
};

static struct mythril_info info = {
    .rsdp = NULL,
    .signature = {0},
};

void print_info(int level)
{
    if (info.rsdp) {
        dprintf(level, "mythril_info {\t.rsdp=%p,\t.signature=%s,\t }\n",
                info.rsdp, info.signature);
    } else {
        dprintf(level, "mythril info not found\n");
    }
}

void mythril_preinit(void)
{
    u8* info_pointer = (u8*)INFO_PHYSICAL_ADDR;
    u8 tag[9];

    if (!CONFIG_MYTHRIL)
        return;

    memcpy(tag, info_pointer, 8);
    tag[8] = '\0';
    info_pointer += 8;

    // Check if this is a mythril tag
    if (memcmp(tag, "MYTHRIL ", 8) != 0) {
        dprintf(3, "incorrect mythril signature! %s\n", tag);
        return;
    }

    dprintf(3, "seabios mythril info found\n");

    info.rsdp = info_pointer;
    memcpy(info.signature, info_pointer, 8);

    // Mythril should give us the RSDP
    if (memcmp(info.signature, "RSD PTR ", 8) != 0) {
        dprintf(3, "incorrect rsdp signature!\n");
        return;
    }

    // Mythril _always_ creates a XSDT. Both the checksum
    // and extended checksum should be valid.
    if (checksum(info.rsdp, 20) || checksum(info.rsdp, 36)) {
        dprintf(3, "incorrect rsdp checksum!\n");
        return;
    }

    print_info(3);
    PlatformRunningOn = PF_QEMU|PF_MYTHRIL;
}


void mythril_biostable_setup(void) {
    dprintf(3, "mythril biostable setup\n");
    if (info.rsdp) {
        copy_acpi_rsdp(info.rsdp);
    }
}
