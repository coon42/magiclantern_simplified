#ifndef _edmac_memcpy_h_
#define _edmac_memcpy_h_

#include "sys/types.h"
#include "stdint.h"

// This value is chosen so that it is higher than the known requirement
// for buffer start alignment on any cam.
// E.g. 5d3 1.2.3 overallocates by 0x80 in _alloc_dma_memory(), 0x17f9c,
// and returns aligned to 0x40.
#define EDMAC_ALIGNMENT 0x80
// Arguably, this should be in edmac.h, but so far it's only used with
// edmac memcpy funcs.

void* edmac_memcpy(void* dst, void* src, size_t length);
void* edmac_memset(void* dst, int value, size_t length);
uint32_t edmac_find_divider(size_t length, size_t transfer_size);

/* crop a rectangle from an image buffer; all sizes in bytes */
void* edmac_copy_rectangle(void* dst, void* src, int src_width, int x, int y, int w, int h);
void* edmac_copy_rectangle_adv(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h);

/* non-blocking versions */
void* edmac_memcpy_start(void* dst, void* src, size_t length);
void* edmac_copy_rectangle_start(void* dst, void* src, int src_width, int x, int y, int w, int h);
void* edmac_copy_rectangle_adv_start(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h);

/* these are blocking tho */
void edmac_memcpy_finish();
void edmac_copy_rectangle_finish();

/* Copy h lines of w bytes each, without waiting; done_cbr(ctx) is called
 * once the data has arrived.
 * Lines start every src_pitch bytes in the source buffer and every
 * dst_pitch bytes in the destination, so with a pitch larger than w
 * the rectangle is part of a wider image.  To crop, let src point to
 * the first byte wanted: src = image + x + y * src_pitch.
 * Keep done_cbr short and non-blocking: it is called from interrupt
 * context.  On some cameras the copy is already done when this function
 * returns, and done_cbr was called on the way, from an interrupt or from
 * the caller's task.  It is not called when NULL is returned. */
void *edmac_copy_rect_async(void *dst, int dst_pitch,
                            void *src, int src_pitch,
                            int w, int h,
                            void (*done_cbr)(void *ctx), void *ctx);

/* Lock/unlock engine resources used by edmac_memcpy (only if ported for your camera) */
void edmac_memcpy_res_lock();
void edmac_memcpy_res_unlock();

/* The channels and the connection used by the functions above, for code
 * that watches or aborts a copy; 0xFFFFFFFF without CONFIG_EDMAC_MEMCPY */
extern uint32_t edmac_memcpy_read_chan;
extern uint32_t edmac_memcpy_write_chan;
extern uint32_t edmac_memcpy_connection;

/* pulls the raw data from EDMAC without Canon's lv_save_raw (for raw backend) */
void edmac_raw_slurp(void* dst, int w, int h);

/* The write channel used for this; 0xFFFFFFFF if not available */
extern uint32_t edmac_raw_write_chan;

#endif // _edmac_memcpy_h_
