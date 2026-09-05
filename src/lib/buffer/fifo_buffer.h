#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t *buffer;
    size_t size;
    size_t head;
    size_t tail;
} fifo_buffer_t;

int fifo_buffer_init(fifo_buffer_t *fifo, uint8_t *buffer, size_t size);

int fifo_buffer_write(fifo_buffer_t *fifo, const uint8_t *data, size_t length);

int fifo_buffer_read(fifo_buffer_t *fifo, uint8_t *data, size_t length);

int fifo_buffer_peek(fifo_buffer_t *fifo, uint8_t *data, size_t length);

int fifo_buffer_get_left(fifo_buffer_t *fifo);

int fifo_buffer_get_used(fifo_buffer_t *fifo);