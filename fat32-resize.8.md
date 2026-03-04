---
title: FAT32-RESIZE
section: 8
date: 2026-03-04
header: System Administration Utilities
footer:
---

# NAME

fat32-resize - resize a FAT32 filesystem

# SYNOPSIS

**fat32-resize** *info*   *FILE* *PART_NUM*\
**fat32-resize** *resize* *FILE* *PART_NUM* *SIZE*

# DESCRIPTION

**fat32-resize** resizes a FAT32 filesystem on the given block device
or image file to *SIZE* (hopefully) without data loss. Both shrinking
and growing are supported.

*FILE* may be a physical block device, an image file with partitions,
or a raw FAT32 image file. Neither *FILE* itself, nor partitions in it
are resized.

*SIZE* is given in sectors or as a percentage. A bare value sets the
exact filesystem size; a % suffix sets it relative to the partition's
available space. A leading + or - makes the value relative to the
current filesystem size.

# OPTIONS

None.

# EXIT STATUS

**0**
:   Success.

**1**
:   General error (invalid arguments, I/O failure, etc.).

# EXAMPLES

Print information about partition 2 in an image file:

    fat32-resize info sdcard.img 2

Set the FAT32 filesystem size on */dev/sdb2* to exactly 4 GB:

    fat32-resize resize /dev/sdb 2 $(( (4 * 1024**3)/512 ))

Set a filesystem to 50% of the available partition space:

    fat32-resize resize partition.img 1 50%

Shrink a filesystem by 20% of its current size:

    fat32-resize resize partition.img 1 -20%

# NOTES

The filesystem must be unmounted before resizing. If you don't know
what are doing, shrinking may cause data loss.

# SEE ALSO

**fsck.fat**(8), **mkfs.fat**(8), **parted**(8)

# BUGS

No progress report; no dry-run option.

https://github.com/gromnitsky/fat32-resize/issues
