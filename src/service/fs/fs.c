/**
 * @file        fs.c
 * @brief       LittleFS 挂在板载 W25Q32 尾部 256KB；擦写走 QMI，不用 SPI
 */

#include "fs.h"

#include <stdint.h>
#include <string.h>

#include "hardware/flash.h"
#include "pico/flash.h"

#define FS_SIZE     (256 * 1024)
#define FS_OFFSET   (PICO_FLASH_SIZE_BYTES - FS_SIZE)

extern char __flash_binary_end;

static uint8_t lfs_read_buf[256];
static uint8_t lfs_prog_buf[256];
static uint8_t lfs_lookahead_buf[16];

static int pico_read(const struct lfs_config *c, lfs_block_t block,
                     lfs_off_t off, void *buffer, lfs_size_t size)
{
    memcpy(buffer, (const uint8_t *)(XIP_BASE + FS_OFFSET)
           + (size_t)block * c->block_size + off, size);
    return LFS_ERR_OK;
}

struct flash_job {
    uint32_t offs;
    const uint8_t *data;
    size_t count;
};

static void do_erase(void *p)
{
    struct flash_job *j = p;
    flash_range_erase(j->offs, j->count);
}

static void do_program(void *p)
{
    struct flash_job *j = p;
    flash_range_program(j->offs, j->data, j->count);
}

static int pico_erase(const struct lfs_config *c, lfs_block_t block)
{
    struct flash_job j = {
        .offs  = FS_OFFSET + (uint32_t)block * c->block_size,
        .count = c->block_size,
    };
    return flash_safe_execute(do_erase, &j, UINT32_MAX)
           ? LFS_ERR_IO : LFS_ERR_OK;
}

static int pico_prog(const struct lfs_config *c, lfs_block_t block,
                     lfs_off_t off, const void *buffer, lfs_size_t size)
{
    struct flash_job j = {
        .offs  = FS_OFFSET + (uint32_t)block * c->block_size + off,
        .data  = buffer,
        .count = size,
    };
    return flash_safe_execute(do_program, &j, UINT32_MAX)
           ? LFS_ERR_IO : LFS_ERR_OK;
}

static int pico_sync(const struct lfs_config *c)
{
    (void)c;
    return LFS_ERR_OK;
}

static const struct lfs_config pico_lfs_cfg = {
    .read  = pico_read,
    .prog  = pico_prog,
    .erase = pico_erase,
    .sync  = pico_sync,
    .read_size = 1,
    .prog_size = FLASH_PAGE_SIZE,
    .block_size = FLASH_SECTOR_SIZE,
    .block_count = FS_SIZE / FLASH_SECTOR_SIZE,
    .cache_size = FLASH_PAGE_SIZE,
    .lookahead_size = 16,
    .block_cycles = 500,
    .read_buffer = lfs_read_buf,
    .prog_buffer = lfs_prog_buf,
    .lookahead_buffer = lfs_lookahead_buf,
};

static lfs_t lfs;
static bool fs_ready;

int fs_init(void)
{
    uint32_t fw_end = (uint32_t)&__flash_binary_end - XIP_BASE;
    int err;

    /* 分区在 Flash 尾巴，固件不能长进 FS_OFFSET */
    if (fw_end > FS_OFFSET) {
        return LFS_ERR_IO;
    }

    err = lfs_mount(&lfs, &pico_lfs_cfg);
    if (err) {
        /* 无超块时 format 会擦完全部分区；已有数据 mount 失败也会被清掉 */
        err = lfs_format(&lfs, &pico_lfs_cfg);
        if (err) {
            return err;
        }
        err = lfs_mount(&lfs, &pico_lfs_cfg);
    }
    fs_ready = (err == LFS_ERR_OK);
    return err;
}

lfs_t *fs_get_handle(void)
{
    if (!fs_ready) {
        return NULL;
    }
    return &lfs;
}
