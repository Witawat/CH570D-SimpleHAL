#ifndef __HAL_RINGBUF_H__
#define __HAL_RINGBUF_H__

#include <stdint.h>
#include "hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *buf;
    uint16_t size;
    uint16_t head;
    uint16_t tail;
} hal_ringbuf_t;

void     hal_ringbuf_init(hal_ringbuf_t *rb, uint8_t *buf, uint16_t size);
hal_status_t hal_ringbuf_put(hal_ringbuf_t *rb, uint8_t data);
hal_status_t hal_ringbuf_get(hal_ringbuf_t *rb, uint8_t *data);
uint16_t hal_ringbuf_available(hal_ringbuf_t *rb);
uint16_t hal_ringbuf_free_space(hal_ringbuf_t *rb);
void     hal_ringbuf_flush(hal_ringbuf_t *rb);
uint8_t  hal_ringbuf_is_empty(hal_ringbuf_t *rb);
uint8_t  hal_ringbuf_is_full(hal_ringbuf_t *rb);

#ifdef __cplusplus
}
#endif

#endif
