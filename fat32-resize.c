#include <stdio.h>
#include <err.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/utsname.h>

#include <parted/parted.h>

#define VERSION   ("0.0.1")
#define MEGABYTE  (1024*1024)
#define TERABYTE  (1024*1024*1024*1024LL)
#define FAT32_MIN ( (32 * MEGABYTE) + 140800)
#define FAT32_MAX ( (2 * TERABYTE))

void usage() {
  fprintf(stderr, "Usage:\n \
        vfat32-resize info   FILE\n \
        vfat32-resize resize FILE [BYTES]\n \
\n\
Doesn't resize a partition, only a FAT32 filesystem within it.\n\
If BYTES is omitted, the fs is extended to the end of the partition.\n\
\n\
FLAVOR         MIN             MAX\n\
fat32   %10d   %10lld\n\n", FAT32_MIN, FAT32_MAX);

  struct utsname buf;
  if (-1 == uname(&buf)) err(1, NULL);
  fprintf(stderr, "%s (%s %s), libparted/%s\n",
          VERSION, buf.sysname, buf.machine, ped_get_version());
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

char *last_error;

PedExceptionOption my_libparted_exception_handler(PedException *ex) {
  last_error = strdup(ex->message);
  return PED_EXCEPTION_UNHANDLED;
}

typedef struct {
  PedDevice *dev;
  PedGeometry geom;
} BD;

void bd_close(BD *b) {
  if (b->dev) ped_device_close(b->dev);
}

char* bd_open(BD *b, char *file) {
  if (! (b->dev = ped_device_get(file))) return "ped_device_get";
  if (!ped_device_open(b->dev)) return "ped_device_open";

  if (!ped_geometry_init(&b->geom, b->dev, 0, b->dev->length))
    return "ped_geometry_init";

  return NULL;                  /* OK, no error */
}

char* info(BD *b) {
  PedFileSystem *fs = ped_file_system_open(&b->geom);
  if (!fs) return "ped_file_system_open";

  printf("%-15s %lld\n", "partition,s", b->dev->length);
  printf("%-15s %lld\n", "partition,b", b->dev->length*512);
  printf("%-15s %lld\n", "sector_size,b", b->dev->sector_size);
  printf("%-15s %s\n", "device_type", devtype(b->dev->type));
  printf("%-15s %s\n", "fs_type", fs->type->name);
  printf("%-15s %lld\n", "fs_end,b", fs->geom->end*512);
  printf("%-15s %lld\n", "fs_length,b", fs->geom->length*512);

  ped_file_system_close(fs);
  return NULL;
}

char* resize(BD *b, PedSector bytes) {
  PedSector length = bytes <= 0 ? b->dev->length : (bytes/512);
  if (getenv("V")) warnx("*** length=%lld", length);

  PedSector start = 0;
  /* if (getenv("START")) start = strtoll(getenv("START"), NULL, 10); */
  /* if (start < 0) start = 0; */

  PedGeometry new_geom;
  if (!ped_geometry_init(&new_geom, b->dev, start, length))
    return "ped_geometry_init: invalid length";

  PedFileSystem *fs = ped_file_system_open(&b->geom);
  if (!fs) return "ped_file_system_open";

  PedTimer *g_timer = NULL;     /* FIXME */
  char *r = NULL;
  if (!ped_file_system_resize(fs, &new_geom, g_timer))
    r = "ped_file_system_resize";

  ped_file_system_close(fs);
  return r;
}

int main(int argc, char **argv) {
  if (argc < 3) { usage(); exit(1); }

  char *mode = argv[1];
  char *file = argv[2];
  char *length = argv[3];
  ped_exception_set_handler(my_libparted_exception_handler);

  int exit_code = 0;
  BD b = {};
  char *err;
  if ( !(err = bd_open(&b, file))) {
    if (0 == strcmp(mode, "info")) {
      err = info(&b);

    } else if (0 == strcmp(mode, "resize")) {
      PedSector len = strtoll(length ? length : "end", NULL, 10);
      err = resize(&b, len);

    } else {
      warnx("unknown mode");
      exit_code = 1;
    }
  }

  if (err) {
    warnx("%s: %s", err, last_error);
    exit_code = 1;
  }

  bd_close(&b);
  if (last_error) free(last_error);
  return exit_code;
}
