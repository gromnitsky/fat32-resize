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
        vfat32-resize info   FILE PART_NUM\n \
        vfat32-resize resize FILE PART_NUM [SIZE]\n \
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

char* transport(int type) {
  char *transport[] = {
    "unknown", "scsi", "ide", "dac960",
    "cpqarray", "file", "ataraid", "i2o",
    "ubd", "dasd", "viodasd", "sx8", "dm",
    "xvd", "sd/mmc", "virtblk", "aoe",
    "md", "loopback", "nvme", "brd",
    "pmem"
  };
  return transport[type];
}

char *last_error;

PedExceptionOption my_libparted_exception_handler(PedException *ex) {
  free(last_error);
  last_error = strdup(ex->message);
  return PED_EXCEPTION_UNHANDLED;
}

typedef struct {
  PedDevice *dev;
  PedDisk *disk;
  PedPartition *part;
  PedFileSystem *part_fs;
} BD;

void bd_close(BD *b) {
  if (b->part_fs) ped_file_system_close(b->part_fs);
  //  if (b->part) ped_disk_delete_partition(b->disk, b->part);
  if (b->disk) ped_disk_destroy(b->disk);
  if (b->dev) ped_device_close(b->dev);
}

char* bd_open(BD *b, char *file, int part_num) {
  if ( !(b->dev = ped_device_get(file))) return "ped_device_get";
  if (!ped_device_open(b->dev)) return "ped_device_open";

  if ( !(b->disk = ped_disk_new(b->dev)))
    return "ped_disk_new";

  if ( !(b->part = ped_disk_get_partition(b->disk, part_num)))
    return "ped_disk_get_partition";

  b->part_fs = ped_file_system_open(&b->part->geom);
  if (!b->part_fs) return "ped_file_system_open";

  return NULL;                  /* OK, no error */
}

char* info(BD *b) {
  printf("%-25s %s\n", "transport", transport(b->dev->type));
  printf("%-25s %lld\n", "sectors", b->dev->length);
  printf("%-25s %s\n", "partition_table", b->disk->type->name);
  printf("%-25s %d\n", "partitions",
         ped_disk_get_last_partition_num(b->disk));
  printf("%-25s %lld\n", "sector_size", b->dev->sector_size);

  printf("%-25s %lld\n", "partition_start", b->part->geom.start);

  printf("%-25s %lld\n", "partition_end", b->part->geom.end);
  printf("%-25s %lld\n", "partition_length", b->part->geom.length);

  printf("%-25s %s\n", "fs_type", b->part_fs->type->name);
  printf("%-25s %lld\n", "fs_start", b->part_fs->geom->start);
  printf("%-25s %lld\n", "fs_end", b->part_fs->geom->end);
  printf("%-25s %lld\n", "fs_length", b->part_fs->geom->length);

  printf("%-25s %d\n", "partition_used_percent",
         (int)((b->part_fs->geom->length * 100)/b->part->geom.length));

  return NULL;
}

char* parse_size(BD *b, char *spec, PedSector *length) {
  if (0 == strlen(spec)) return "invalid SIZE";

  char mode = spec[strlen(spec) - 1];
  PedSector r = 0;
  PedSector max_length = b->part->geom.length;
  if (getenv("V")) warnx("*** max_length=%lld", max_length);

  if (mode == '%') {
    spec[strlen(spec) - 1] = '\0';
    int percent = strtoll(spec, NULL, 10);
    if (abs(percent) <= 0 || abs(percent) > 100)
      return "invalid SIZE percentage";
    if (spec[0] == '-' || spec[0] == '+') {
      r = (percent/100.0) * b->part_fs->geom->length;
      r = b->part_fs->geom->length + r;
    } else {
      r = (percent/100.0) * max_length;
    }

  } else {
    r = strtoll(spec, NULL, 10);
    if (spec[0] == '-' || spec[0] == '+') {
      r = b->part_fs->geom->length + r;
    }
  }

  if (r > max_length) return "SIZE is too big";
  if (r < FAT32_MIN) return "SIZE is too small";
  *length = r;

  return NULL;
}

char* resize(BD *b, char *spec) {
  if (0 != strcmp(b->part_fs->type->name, "fat32"))
    return (char*)b->part_fs->type->name;

  PedSector new_length = 0;
  char *err = parse_size(b, spec, &new_length);
  if (err) return err;
  if (getenv("V")) warnx("*** new_length=%lld", new_length);

  PedTimer *g_timer = NULL;     /* FIXME */
  char *r = NULL;
  PedGeometry* new_geom = ped_geometry_duplicate(b->part_fs->geom);
  ped_geometry_set_end(new_geom, b->part_fs->geom->start + new_length - 1);
  if (!ped_file_system_resize(b->part_fs, new_geom, g_timer))
    r = "ped_file_system_resize";
  ped_geometry_destroy(new_geom);

  if (!ped_device_sync(b->dev)) warnx("failed to sync");
  return r;
}

int main(int argc, char **argv) {
  if (argc < 4) { usage(); exit(1); }

  char *mode = argv[1];
  char *file = argv[2];
  int part_num = strtoll(argv[3], NULL, 10);
  char *size_spec = argv[4];
  ped_exception_set_handler(my_libparted_exception_handler);

  int exit_code = 0;
  BD b = {};
  char *err;
  if ( !(err = bd_open(&b, file, part_num))) {
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
