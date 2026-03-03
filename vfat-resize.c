#include <stdio.h>
#include <err.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <parted/parted.h>

#define VERSION   ("0.0.1")
#define MEBIBYTE  (1024*1024)
#define TEBIBYTE  (1024*1024*1024*1024LL)
#define FAT16_MIN (16 * MEBIBYTE)
#define FAT16_MAX ((long)(4096-1) * MEBIBYTE)
#define FAT32_MIN (33 * MEBIBYTE)
#define FAT32_MAX (2 * TEBIBYTE)

void usage() {
  fprintf(stderr, "Usage:\n \
        vfat-resize info   file\n \
        vfat-resize resize file new-size-in-sectors\n \
\n\
It doesn't resize partitions, \
only FAT16/32 filesystems within partitions.\n\
You may loose data if you shrink the filesystem.\n\
\n\
FLAVOR         MIN,B          MAX,B          MIN,s          MAX,s\n\
fat16  %13d  %13ld  %13d  %13ld\n\
fat32  %13d  %13lld  %13d  %13lld\n",
          FAT16_MIN, FAT16_MAX, FAT16_MIN/512, FAT16_MAX/512,
          FAT32_MIN, FAT32_MAX, FAT32_MIN/512, FAT32_MAX/512);
}

char* devtype(int type) {
  switch (type) {
  case PED_DEVICE_SCSI     : return "scsi";
  case PED_DEVICE_IDE      : return "ide";
  case PED_DEVICE_DAC960   : return "dac960";
  case PED_DEVICE_CPQARRAY : return "cpqarray";
  case PED_DEVICE_FILE     : return "file";
  case PED_DEVICE_ATARAID  : return "";
  case PED_DEVICE_I2O      : return "I2O";
  case PED_DEVICE_UBD      : return "ubd";
  case PED_DEVICE_DASD     : return "dasd";
  case PED_DEVICE_VIODASD  : return "viodasd";
  case PED_DEVICE_SX8      : return "sx8";
  case PED_DEVICE_DM       : return "dm";
  case PED_DEVICE_XVD      : return "xvd";
  case PED_DEVICE_SDMMC    : return "sdmmc";
  case PED_DEVICE_VIRTBLK  : return "virtblk";
  case PED_DEVICE_AOE      : return "AoE";
  case PED_DEVICE_MD       : return "md";
  case PED_DEVICE_LOOP     : return "loop";
  case PED_DEVICE_NVME     : return "nvme";
  case PED_DEVICE_RAM      : return "ram";
  case PED_DEVICE_PMEM     : return "pmem";
  }
  return "unknown";
}

PedExceptionOption my_libparted_exception_handler(PedException *ex) {
  warnx("%s", ex->message);
  return PED_EXCEPTION_UNHANDLED;
}

typedef struct {
  PedDevice *dev;
  PedFileSystem *fs;
} BD;

void bd_close(BD *b) {
  ped_file_system_close(b->fs);
  ped_device_close(b->dev);
}

void bd_open(BD *b, char *file) {
  if (! (b->dev = ped_device_get(file))) errx(1, "ped_device_get");
  if (!ped_device_open(b->dev)) err(1, "ped_device_open");

  PedGeometry geom;
  PedSector start = 0;
  if (getenv("START")) start = strtoll(getenv("START"), NULL, 10);
  if (start < 0) start = 0;
  if (!ped_geometry_init(&geom, b->dev, start, b->dev->length))
    err(1, "ped_geometry_init");

  b->fs = ped_file_system_open(&geom);
  if (!b->fs) errx(1, "ped_file_system_open");
}

void print_fs_info(BD *b) {
  printf("%-15s %lld\n", "sectors", b->dev->length);
  printf("%-15s %lld\n", "sector_size", b->dev->sector_size);
  printf("%-15s %s\n", "device_type", devtype(b->dev->type));
  printf("%-15s %s\n", "fs_type", b->fs->type->name);
  printf("%-15s %lld\n", "fs_start", b->fs->geom->start);
  printf("%-15s %lld\n", "fs_end", b->fs->geom->end);
  printf("%-15s %lld\n", "fs_length", b->fs->geom->length);
}

int main(int argc, char **argv) {
  if (argc < 3) { usage(); exit(1); }

  char *file = argv[2];
  ped_exception_set_handler(my_libparted_exception_handler);

  BD b;
  bd_open(&b, file);
  if (0 == strcmp(argv[1], "info")) {
    print_fs_info(&b);

  } else if (0 == strcmp(argv[1], "resize")) {
    warnx("TODO");

  } else {
    errx(1, "invalid mode");
  }

  bd_close(&b);
}
