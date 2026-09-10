#ifndef FIFO_BUFFER_H
#define FIFO_BUFFER_H

#include <stdint.h>
#include <stddef.h>

#ifdef FIFO_BUFFER_USING_MUTEX
typedef void (*fifo_buffer_mutex_lock_t)(void *mutex);
typedef void (*fifo_buffer_mutex_unlock_t)(void *mutex);
#endif

typedef struct {
    uint8_t *buffer;
    size_t size;
    size_t head; // write index
    size_t tail; // read index
#ifdef FIFO_BUFFER_USING_MUTEX
    void *mutex;
    fifo_buffer_mutex_lock_t lock;
    fifo_buffer_mutex_unlock_t unlock;
#endif
} fifo_buffer_t;

#ifdef FIFO_BUFFER_USING_MUTEX
int fifo_buffer_init(fifo_buffer_t *fifo, uint8_t *buffer, uint32_t size, void *mutex, fifo_buffer_mutex_lock_t lock, fifo_buffer_mutex_unlock_t unlock);
#else
int fifo_buffer_init(fifo_buffer_t *fifo, uint8_t *buffer, uint32_t size);
#endif

int fifo_buffer_write(fifo_buffer_t *fifo, const uint8_t *data, uint32_t length);

int fifo_buffer_read(fifo_buffer_t *fifo, uint8_t *data, uint32_t length);

int fifo_buffer_peek(fifo_buffer_t *fifo, uint8_t *data, uint32_t length, uint32_t offset);

int fifo_buffer_get_left(fifo_buffer_t *fifo);

int fifo_buffer_get_used(fifo_buffer_t *fifo);

#endif /* FIFO_BUFFER_H */