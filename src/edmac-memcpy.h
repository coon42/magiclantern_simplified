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

/* crop a rectangle from an image buffer; all sizes in bytes */
void* edmac_copy_rectangle_adv(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h);

/* non-blocking versions */
void* edmac_memcpy_start(void* dst, void* src, size_t length);
void* edmac_copy_rectangle_adv_start(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h);

/* these are blocking tho */
void edmac_memcpy_finish();
void edmac_copy_rectangle_finish();

/* Copy the w x h area at (src_x, src_y) of src to (dst_x, dst_y) of dst.
 * Returns once the copy is started; dst must be uncacheable, all sizes
 * are in bytes, a "pitch" is the length of one row of the whole buffer.
 * done(ctx) is called when it has finished (or was aborted), from interrupt
 * context, after the copy engine has been released, so it may start the
 * next copy.  done is not called if this returns NULL.
 *
 * Some ports finish the copy before returning, and call done() from the
 * calling task. */
typedef void (*edmac_copy_done_cbr)(void *ctx);
void *edmac_copy_rect_async(void *dst, void *src,
                            int src_pitch, int src_x, int src_y,
                            int dst_pitch, int dst_x, int dst_y,
                            int w, int h,
                            edmac_copy_done_cbr done, void *ctx);

/* Reserve the copy channels against other users in the firmware, around
 * a series of copies (e.g. while recording raw video).  No-op on ports
 * that don't need it. */
void edmac_copy_lock_resources(void);
void edmac_copy_unlock_resources(void);

/* Channels and connection used by the copy functions above, for code that
 * needs to talk to the same hardware (spying, tests, other image engine
 * users).  0xFFFFFFFF where copies don't go through a known channel pair. */
extern uint32_t edmac_copy_read_chan;
extern uint32_t edmac_copy_write_chan;
extern uint32_t edmac_copy_connection;

/* pulls the raw data from EDMAC without Canon's lv_save_raw (for raw backend) */
void edmac_raw_slurp(void* dst, int w, int h);
extern uint32_t edmac_raw_slurp_chan; /* 0xFFFFFFFF without CONFIG_EDMAC_RAW_SLURP */

#endif // _edmac_memcpy_h_
