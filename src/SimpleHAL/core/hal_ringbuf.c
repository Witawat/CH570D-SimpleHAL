#include "hal_ringbuf.h"

void hal_ringbuf_init(hal_ringbuf_t *rb, uint8_t *buf, uint16_t size)
{
    rb->buf  = buf;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
}

hal_status_t hal_ringbuf_put(hal_ringbuf_t *rb, uint8_t data)
{
    uint16_t next = (rb->head + 1) % rb->size;
    if (next == rb->tail) {
        return HAL_BUSY;
    }
    rb->buf[rb->head] = data;
    rb->head = next;
    return HAL_OK;
}

hal_status_t hal_ringbuf_get(hal_ringbuf_t *rb, uint8_t *data)
{
    if (rb->head == rb->tail) {
        return HAL_BUSY;
    }
    *data = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    return HAL_OK;
}

uint16_t hal_ringbuf_available(hal_ringbuf_t *rb)
{
    return (rb->head + rb->size - rb->tail) % rb->size;
}

uint16_t hal_ringbuf_free_space(hal_ringbuf_t *rb)
{
    return (rb->size - hal_ringbuf_available(rb) - 1);
}

void hal_ringbuf_flush(hal_ringbuf_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
}

uint8_t hal_ringbuf_is_empty(hal_ringbuf_t *rb)
{
    return (rb->head == rb->tail);
}

uint8_t hal_ringbuf_is_full(hal_ringbuf_t *rb)
{
    return (((rb->head + 1) % rb->size) == rb->tail);
}
