#ifndef _edmac_memcpy_h_
#define _edmac_memcpy_h_

#include "sys/types.h"
#include "stdint.h"

/**
 * Copying memory with EDMAC.
 *
 * A read channel and a write channel are connected back to back, so the
 * image processor moves the data without the CPU touching it.  This is
 * much faster than memcpy, and it can crop: the unit of work is a
 * rectangle, i.e. a number of lines taken from a larger image buffer.
 *
 * Rules, for all copy functions:
 *  - dst must be uncacheable memory (e.g. from fio_malloc).
 *  - w * h must be a multiple of the transfer block size (16 bytes on
 *    DIGIC 5, 4 on DIGIC 4), or nothing is copied and NULL is returned.
 *  - one copy at a time: starting a copy waits until the previous one
 *    has finished.
 *
 * Needs CONFIG_EDMAC_MEMCPY.  Cameras without it may still provide
 * edmac_copy_rect_async and the resource lock from their own code
 * (see their function_overrides.c), as that is what raw video needs.
 */

// This value is chosen so that it is higher than the known requirement
// for buffer start alignment on any cam.
// E.g. 5d3 1.2.3 overallocates by 0x80 in _alloc_dma_memory(), 0x17f9c,
// and returns aligned to 0x40.
#define EDMAC_ALIGNMENT 0x80

/* Copy length bytes and wait until done; returns dst, like memcpy.
 * Falls back to memcpy for sizes EDMAC cannot handle. */
void *edmac_memcpy(void *dst, void *src, size_t length);

/* Copy h lines of w bytes each and wait until done; returns dst.
 * Lines start every src_pitch bytes in the source buffer and every
 * dst_pitch bytes in the destination, so with a pitch larger than w
 * the rectangle is part of a wider image.  To crop, let src point to
 * the first byte wanted: src = image + x + y * src_pitch. */
void *edmac_copy_rect(void *dst, int dst_pitch,
                      void *src, int src_pitch,
                      int w, int h);

/* Same, but only starts the copy; done_cbr(ctx) is called once the data
 * has arrived.  Keep done_cbr short and non-blocking: it is called from
 * interrupt context.  On some cameras the copy is already done when this
 * function returns, and done_cbr was called on the way, from an interrupt
 * or from the caller's task.  It is not called when NULL is returned. */
void *edmac_copy_rect_async(void *dst, int dst_pitch,
                            void *src, int src_pitch,
                            int w, int h,
                            void (*done_cbr)(void *ctx), void *ctx);

/* While LiveView is running, Canon code may want the channels used for
 * copying.  Hold this lock around a series of copies to keep them. */
void edmac_memcpy_res_lock();
void edmac_memcpy_res_unlock();

/* Split length bytes into lines of equal width, for use as a rectangle:
 * returns the largest width that divides length and is not larger than
 * the resulting number of lines.  Returns 0 if length is not a multiple
 * of transfer_size (0: the block size used by the copy functions). */
uint32_t edmac_find_divider(size_t length, size_t transfer_size);

/* The channels and the connection used by the functions above, for code
 * that watches or aborts a copy; 0xFFFFFFFF without CONFIG_EDMAC_MEMCPY */
extern uint32_t edmac_memcpy_read_chan;
extern uint32_t edmac_memcpy_write_chan;
extern uint32_t edmac_memcpy_connection;

/**
 * Raw data straight from the sensor (CONFIG_EDMAC_RAW_SLURP)
 */

/* Capture the next LiveView raw frame (h lines of w bytes) into dst,
 * bypassing Canon's lv_save_raw; does not wait.  For the raw backend. */
void edmac_raw_slurp(void* dst, int w, int h);

/* The write channel used for this; 0xFFFFFFFF if not available */
extern uint32_t edmac_raw_write_chan;

#endif // _edmac_memcpy_h_
