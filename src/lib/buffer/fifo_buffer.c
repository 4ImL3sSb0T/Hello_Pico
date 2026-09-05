#include "fifo_buffer.h"
#include <string.h>

int fifo_buffer_init(fifo_buffer_t *fifo, uint8_t *buffer, size_t size)
{
    if (!fifo || !buffer || size == 0) return -1; // Invalid parameters

    fifo->buffer = buffer;
    fifo->size = size;
    fifo->head = 0;
    fifo->tail = 0;

    return 0;
}

int fifo_buffer_write(fifo_buffer_t *fifo, const uint8_t *data, size_t length) {
    if (!fifo || !data || length == 0) return -1; // Invalid parameters

    
}

int fifo_buffer_read(fifo_buffer_t *fifo, uint8_t *data, size_t length);

int fifo_buffer_peek(fifo_buffer_t *fifo, uint8_t *data, size_t length, size_t offset);

int fifo_buffer_get_left(fifo_buffer_t *fifo);

int fifo_buffer_get_used(fifo_buffer_t *fifo);