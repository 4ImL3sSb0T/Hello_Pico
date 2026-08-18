#include "lfs.h"
#include "hardware/flash.h"
#include "pico/stdlib.h"
#include "pico/flash.h"

int fs_init(void);

lfs_t* fs_get_handle(void);