/**
 * @file        fs.h
 * @brief       LittleFS 对外接口。块设备细节留在 fs.c，调用方不要直接碰 Flash
 */
#ifndef FS_H
#define FS_H

#include "lfs.h"

int fs_init(void);
lfs_t *fs_get_handle(void);

#endif
