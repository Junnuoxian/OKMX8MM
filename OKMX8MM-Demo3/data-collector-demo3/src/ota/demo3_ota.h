#ifndef DEMO3_OTA_H
#define DEMO3_OTA_H

int demo3_ota_stage_package(const char *package_path,
                            const char *staging_path);
int demo3_ota_write_reboot_marker(const char *marker_path,
                                  const char *staging_path);

#endif
