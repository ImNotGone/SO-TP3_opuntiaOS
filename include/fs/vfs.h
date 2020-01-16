#ifndef __oneOS__FS__VFS_H
#define __oneOS__FS__VFS_H

#include <drivers/driver_manager.h>

#define VFS_MAX_FS_COUNT 5
#define VFS_MAX_DEV_COUNT 5
#define VFS_MAX_FILENAME 16
#define VFS_MAX_FILENAME_EXT 4

typedef struct {
    void* recognize;

    void* create_dir;
    void* lookup_dir;
    void* remove_dir;

    void* write_file;
    void* read_file;
    void* remove_file;

    void* eject_device;
} fs_desc_t;

typedef struct {
    int8_t fs;
    char disk_name;
    device_t dev;
} vfs_device_t;

typedef struct {
    char filename[VFS_MAX_FILENAME];
    char filename_ext[VFS_MAX_FILENAME_EXT];
    uint8_t attributes;
    uint16_t file_size;
} vfs_element_t;

void vfs_install();
void vfs_add_device(device_t *t_new_dev);
void vfs_add_fs(driver_t *t_new_fs);
void vfs_eject_device(device_t *t_new_dev);

// Test Func
// Will be deleted
void vfs_test();

void open();
void close();
uint32_t vfs_lookup_dir(const char *t_path, vfs_element_t *t_buf);
void vfs_write_file(const char *t_path, const char *t_file_name, const uint8_t *t_data, uint32_t t_size);
void* vfs_read_file(const char *t_path, const char *t_file_name, uint16_t t_offset, int16_t t_len);
bool vfs_create_dir(const char* t_path, const char* t_dir_name);
void remove_dir(); // NOT IMPLEMENTED

#endif // __oneOS__FS__VFS_H
