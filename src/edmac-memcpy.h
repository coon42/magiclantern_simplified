#ifndef _edmac_memcpy_h_
#define _edmac_memcpy_h_

/*
 * RAM to RAM copies using EDMAC (or whatever the camera's port uses for
 * that; DIGIC 6+ ports implement this in function_overrides.c).
 *
 * Only one copy runs at a time; starting a copy waits for the previous one.
 *
 * Rules, because the hardware does not forgive mistakes:
 * - dst must be an uncacheable pointer (src may be either, cacheable src
 *   gets the caches cleaned first);
 * - w * h must be a multiple of the transfer size (16 bytes to be safe),
 *   otherwise the transfer cannot be stopped and the copy is refused;
 * - all sizes and positions are in bytes; a "pitch" is the length of one
 *   row of the whole buffer, w is the length of one row of the copied area.
 */

#include "sys/types.h"
#include "stdint.h"

// This value is chosen so that it is higher than the known requirement
// for buffer start alignment on any cam.
// E.g. 5d3 1.2.3 overallocates by 0x80 in _alloc_dma_memory(), 0x17f9c,
// and returns aligned to 0x40.
#define EDMAC_ALIGNMENT 0x80

/* Plain copy.  Blocking; falls back to memcpy() for sizes EDMAC can't do. */
void *edmac_memcpy(void *dst, void *src, size_t length);

/* Copy the w x h area at (src_x, src_y) of src to (dst_x, dst_y) of dst.
 * Blocking.  Returns dst, or NULL if the copy was refused. */
void *edmac_copy_rect(void *dst, void *src,
                      int src_pitch, int src_x, int src_y,
                      int dst_pitch, int dst_x, int dst_y,
                      int w, int h);

/* Same as edmac_copy_rect(), but returns once the copy is started.
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

/* Raw LiveView data straight from the sensor connection, bypassing Canon's
 * lv_save_raw (CONFIG_EDMAC_RAW_SLURP).  Starts the transfer, does not wait. */
void edmac_raw_slurp(void *dst, int w, int h);
extern uint32_t edmac_raw_slurp_chan; /* 0xFFFFFFFF without CONFIG_EDMAC_RAW_SLURP */

#endif // _edmac_memcpy_h_
