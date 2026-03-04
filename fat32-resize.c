#include <stdio.h>
#include <err.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/utsname.h>

#include <parted/parted.h>

#define VERSION   ("0.0.1")
#define TERABYTE  (1024*1024*1024*1024LL)
#define FAT32_MIN 525226        /* sectors, or ~256MB */
#define FAT32_MAX ( ( 2 * TERABYTE) / 512 )

void usage() {
  fprintf(stderr, "Usage:\n \
        vfat32-resize info   FILE\n \
        vfat32-resize resize FILE [SIZE]\n \
\n\
Expand/shrink an unmounted filesystem located at FILE.\n\
Doesn't resize FILE, only a FAT32 filesystem within it.\n\
\n\
SIZE formats: [-]sectors or [-]number%%.\n\
Sectors range: %d...%lld.\n", FAT32_MIN, FAT32_MAX);
}

void version() {
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

  printf("%-15s %lld\n", "partition", b->dev->length);
  printf("%-15s %lld\n", "sector_size", b->dev->sector_size);
  printf("%-15s %s\n", "device_type", devtype(b->dev->type));
  printf("%-15s %s\n", "fs_type", fs->type->name);
  printf("%-15s %lld\n", "fs_end", fs->geom->end);
  printf("%-15s %lld\n", "fs_length", fs->geom->length);

  ped_file_system_close(fs);
  return NULL;
}

char* parse_size(BD *b, char *spec, PedSector *length) {
  char mode = spec[strlen(spec) - 1];
  PedSector r = 0;

  if (mode == '%') {
    spec[strlen(spec) - 1] = '\0';
    int percent = strtoll(spec, NULL, 10);
    if (abs(percent) <= 0 || abs(percent) > 100)
      return "invalid SIZE percentage";
    r = (percent/100.0) * b->dev->length;

  } else {
    r = strtoll(spec, NULL, 10);
  }

  if (r < 0) r = b->dev->length + r;
  if (r > b->dev->length) return "SIZE is too big";
  if (r < FAT32_MIN) return "SIZE is too small";
  *length = r;

  return NULL;
}

char* resize(BD *b, char *spec) {
  PedSector length = 0;
  char *err = parse_size(b, spec, &length);
  if (err) return err;
  if (getenv("V")) warnx("*** length=%lld", length);

  PedGeometry new_geom;
  if (!ped_geometry_init(&new_geom, b->dev, 0 /* TODO */, length))
    return "ped_geometry_init: invalid SIZE spec";

  PedFileSystem *fs = ped_file_system_open(&b->geom);
  if (!fs) return "ped_file_system_open";

  if (0 != strcmp(fs->type->name, "fat32"))
    return (char*)fs->type->name;

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
  char *size_spec = argv[3];
  ped_exception_set_handler(my_libparted_exception_handler);

  int exit_code = 0;
  BD b = {};
  char *err;
  if ( !(err = bd_open(&b, file))) {
    if (0 == strcmp(mode, "info")) {
      if (size_spec) {
        warnx("extra args");
        exit_code = 1;
      } else
        err = info(&b);

    } else if (0 == strcmp(mode, "resize")) {
      if (size_spec) {
        err = resize(&b, size_spec);
      } else {
        warnx("missing a size spec");
        exit_code = 1;
      }

    } else {
      warnx("unknown mode");
      exit_code = 1;
    }
  }

  if (err) {
    warnx(last_error ? "%s: %s" : "%s", err, last_error);
    exit_code = 1;
  }

  bd_close(&b);
  free(last_error);
  return exit_code;
}
