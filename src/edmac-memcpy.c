#include "edmac-memcpy.h"
#include "dryos.h"
#include "edmac.h"
#include "bmp.h"
#include "math.h"
#include "platform/state-object.h"

#ifdef CONFIG_EDMAC_MEMCPY

static struct semaphore *edmac_memcpy_sem = NULL; /* to allow only one memcpy running at a time */
static struct semaphore *edmac_read_done_sem = NULL; /* to know when memcpy is finished */

/* what to call when the running copy is finished; NULL for the blocking
 * copies, where the calling task waits for edmac_read_done_sem instead */
static void (*edmac_copy_done_cbr)(void *ctx) = NULL;
static void *edmac_copy_done_ctx = NULL;

/* pick some free (check using debug menu) EDMAC channels write: 0x00-0x06, 0x10-0x16, 0x20-0x21. read: 0x08-0x0D, 0x18-0x1D,0x28-0x2B */
#if defined(CONFIG_5D2) || defined(CONFIG_50D)
uint32_t edmac_memcpy_read_chan = 0x19;
uint32_t edmac_memcpy_write_chan = 0x03;
/*
50D
R 2-15
W 3-8 10-15
*/
#elif defined(CONFIG_650D) || defined(CONFIG_EOSM) || defined(CONFIG_700D) || defined(CONFIG_100D)
uint32_t edmac_memcpy_read_chan = 0x19;
uint32_t edmac_memcpy_write_chan = 0x13;
//~ r 2 3 5 7 8 9 10 11-13
//~ w 3 4 6 10 11-15
#elif defined(CONFIG_60D)
uint32_t edmac_memcpy_read_chan = 0x19;  /* free indices: 2, 3, 4, 5, 6, 7, 8, 9 */
uint32_t edmac_memcpy_write_chan = 0x06; /* 1, 4, 6, 10 */
#elif defined(CONFIG_6D) || defined(CONFIG_5D3)
uint32_t edmac_memcpy_read_chan = 0x19;  /* Read: 0 5 7 11 14 15 */
uint32_t edmac_memcpy_write_chan = 0x11; /* Write: 6 8 15 */
#elif defined(CONFIG_70D)
// 70D uses same read and write channels as 6D and 5D3
// just keep it separate with the comments
uint32_t edmac_memcpy_read_chan = 0x19;  /* Read decimal: 8 25 29 42 43 - hex: 0x08 0x19 0x1D 0x2A 0x2B*/
uint32_t edmac_memcpy_write_chan = 0x11; /* Write decimal: 6 17 33 - hex: 0x06 0x11 0x21*/
#elif defined(CONFIG_7D)
uint32_t edmac_memcpy_read_chan = 0x0A;  /*Read 0x19 0x0D 0x0B 0x0A(82MB/S)*/
uint32_t edmac_memcpy_write_chan = 0x06; /* Write 0x5 0x6 0x4 (LV) */
//5 zoom, 6 not - improved performance (no HDMI related tearing)
#elif defined(CONFIG_500D)
uint32_t edmac_memcpy_read_chan = 0x0D;
uint32_t edmac_memcpy_write_chan = 0x04;
#elif defined(CONFIG_550D)
uint32_t edmac_memcpy_read_chan = 0x19;
uint32_t edmac_memcpy_write_chan = 0x05;
#elif defined(CONFIG_600D)
uint32_t edmac_memcpy_read_chan = 0x19;
uint32_t edmac_memcpy_write_chan = 0x06;
#elif defined(CONFIG_1100D)
uint32_t edmac_memcpy_read_chan = 0x19;
uint32_t edmac_memcpy_write_chan = 0x04;
#elif defined(CONFIG_1300D)
uint32_t edmac_memcpy_read_chan = 0x19;
uint32_t edmac_memcpy_write_chan = 0x04;
#else
#error Please find some free EDMAC channels for your camera.
#endif

/* both channels get connected to this... lets call it service. it will just output the data it gets as input */
uint32_t edmac_memcpy_connection = 6;

/* see wiki, register map, EDMAC what the flags mean. they are for setting up copy block size */
#if defined(CONFIG_7D)
static const uint32_t edmac_memcpy_flags = EDMAC_2_BYTES_PER_TRANSFER; //Original are faster on 7D
#else
static const uint32_t edmac_memcpy_flags = EDMAC_16_BYTES_PER_TRANSFER; //Enhanced
#endif

static struct LockEntry * resLock = 0;

static void edmac_memcpy_init()
{
    edmac_memcpy_sem = create_named_semaphore("edmac_memcpy_sem", SEM_CREATE_UNLOCKED);
    edmac_read_done_sem = create_named_semaphore("edmac_read_done_sem", SEM_CREATE_LOCKED);
    
    /* lookup the edmac channel indices for reslock */
    int read_edmac_index = edmac_channel_to_index(edmac_memcpy_read_chan);
    int write_edmac_index = edmac_channel_to_index(edmac_memcpy_write_chan);
    ASSERT(read_edmac_index >= 0 && write_edmac_index >= 0);

    uint32_t resIds[] = {
        0x00000000 + write_edmac_index, /* write edmac channel */
        0x00010000 + read_edmac_index, /* read edmac channel */
        0x00020000 + edmac_memcpy_connection, /* write connection */
        0x00030000 + edmac_memcpy_connection, /* read connection */
    };
    resLock = CreateResLockEntry(resIds, 4);
    
    ASSERT(resLock);

    /* just to make sure we have this stub */
    static void *AbortEDmac_check __attribute__((used)) = &AbortEDmac;
}

INIT_FUNC("edmac_memcpy", edmac_memcpy_init);

void edmac_memcpy_res_lock()
{
    //~ bmp_printf(FONT_MED, 50, 50, "Locking");
    int r = LockEngineResources(resLock);
    if (r & 1)
    {
        NotifyBox(2000, "ResLock fail %x %x", resLock, r);
        return;
    }
    //~ bmp_printf(FONT_MED, 50, 50, "Locked!");
}

void edmac_memcpy_res_unlock()
{
    UnLockEngineResources(resLock);
}

/* cleanup channel configuration and release semaphore */
static void edmac_copy_cleanup()
{
    /* set default CBRs again and stop both DMAs */
    UnregisterEDmacCompleteCBR(edmac_memcpy_read_chan);
    UnregisterEDmacAbortCBR(edmac_memcpy_read_chan);
    UnregisterEDmacPopCBR(edmac_memcpy_read_chan);
    UnregisterEDmacCompleteCBR(edmac_memcpy_write_chan);
    UnregisterEDmacAbortCBR(edmac_memcpy_write_chan);
    UnregisterEDmacPopCBR(edmac_memcpy_write_chan);

    give_semaphore(edmac_memcpy_sem);
}

/* called (from interrupt) when the read channel is done with the source */
static void edmac_read_done_cbr(void *ctx)
{
    if (!edmac_copy_done_cbr)
    {
        /* blocking copy: wake up the task waiting in edmac_copy_finish */
        give_semaphore(edmac_read_done_sem);
    }
}

/* called (from interrupt) when the write channel has stored all the data */
static void edmac_write_done_cbr(void *ctx)
{
    if (edmac_copy_done_cbr)
    {
        /* non-blocking copy: nobody is waiting, so tell the caller
         * and get the channels ready for the next copy from here */
        edmac_copy_done_cbr(edmac_copy_done_ctx);
        edmac_copy_cleanup();
    }
}

/* set up both channels and start the transfer; returns without waiting.
 * with done_cbr, the copy finishes on its own, see edmac_write_done_cbr;
 * without, the caller must wait for it with edmac_copy_finish. */
static void *edmac_copy_start(void *dst, int dst_pitch,
                              void *src, int src_pitch,
                              int w, int h,
                              void (*done_cbr)(void *ctx), void *ctx)
{
    // "src_pitch" is width of the frame including dark areas, borders etc.
    // "w" is the width of the region to copy out, e.g. if stripping the borders.
    //
    // Why three widths?  Because the dst buffer can also conceptually be rectangular,
    // with the copy region a smaller rectangle within it.  Presumably Canon want this
    // for e.g. copying from a buffer with border to another, different sized buffer
    // that also has a border.

    /* dmaFlags: 16 (DIGIC 5) or 4 (DIGIC 4) bytes per transfer
     * in order to successfully stop the EDMAC transfer,
     * w * h must be mod number of bytes per transfer
     * (not sure why it works that way, found experimentally)
     *
     * Do not remove this check, or risk permanent camera bricking.
     */
    uint32_t bpt = edmac_bytes_per_transfer(edmac_memcpy_flags);
    if ((w * h) % bpt)
    {
        printf("Invalid EDMAC output size: %d x %d (mod%d = %d)\n", w, h, bpt, (w * h) % bpt);
        return 0;
    }

    /* make sure we are writing to uncacheable memory */
    ASSERT(dst == UNCACHEABLE(dst));

    /* clean the cache before reading from regular (cacheable) memory */
    /* see FIO_WriteFile for more info */
    if (src == CACHEABLE(src))
    {
        sync_caches();
    }

    take_semaphore(edmac_memcpy_sem, 0);

    /* the previous copy is finished, now its callback can be replaced */
    edmac_copy_done_cbr = done_cbr;
    edmac_copy_done_ctx = ctx;

    uint32_t src_adjusted = (uint32_t)src & 0x1FFFFFFF;
    uint32_t dst_adjusted = (uint32_t)dst & 0x1FFFFFFF;

    /* only read channel will emit a callback when reading from memory is done. write channels would just continue */
    RegisterEDmacCompleteCBR(edmac_memcpy_read_chan, &edmac_read_done_cbr, NULL);
    RegisterEDmacAbortCBR(edmac_memcpy_read_chan, &edmac_read_done_cbr, NULL);
    RegisterEDmacPopCBR(edmac_memcpy_read_chan, &edmac_read_done_cbr, NULL);
    RegisterEDmacCompleteCBR(edmac_memcpy_write_chan, &edmac_write_done_cbr, NULL);
    RegisterEDmacAbortCBR(edmac_memcpy_write_chan, &edmac_write_done_cbr, NULL);
    RegisterEDmacPopCBR(edmac_memcpy_write_chan, &edmac_write_done_cbr, NULL);
    
    /* connect the selected channels to 6 so any data read from RAM is passed to write channel */
    ConnectWriteEDmac(edmac_memcpy_write_chan, edmac_memcpy_connection);
    ConnectReadEDmac(edmac_memcpy_read_chan, edmac_memcpy_connection);
    
    /* xb is width */
    /* yb is height-1 (number of repetitions) */
    /* off1b is the number of bytes to skip after every xb bytes being transferred */
    struct edmac_info src_edmac_info = {
        .xb = w,
        .yb = h-1,
        .off1b = src_pitch - w,
    };
    
    /* destination setup has no special cropping */
    struct edmac_info dst_edmac_info = {
        .xb = w,
        .yb = h-1,
        .off1b = dst_pitch - w,
    };
    
    SetEDmac(edmac_memcpy_read_chan, (void*)src_adjusted, &src_edmac_info, edmac_memcpy_flags);
    SetEDmac(edmac_memcpy_write_chan, (void*)dst_adjusted, &dst_edmac_info, edmac_memcpy_flags);
    
    /* start transfer. no flags for write, 2 for read channels */
    StartEDmac(edmac_memcpy_write_chan, 0);
    StartEDmac(edmac_memcpy_read_chan, 2);
    
    return dst;
}

/* this function waits for the DMA transfer being finished by blocked wait on the read semaphore.
   as soon the semaphore was taken, cleanup edmac configuration and release global EDMAC semaphore.
 */
static void edmac_copy_finish()
{
    /* wait until read is finished */
    int r = take_semaphore(edmac_read_done_sem, 1000);
    if(r != 0)
    {
        NotifyBox(2000, "EDMAC timeout");
    }
    
    edmac_copy_cleanup();
}

void *edmac_copy_rect(void *dst, int dst_pitch,
                      void *src, int src_pitch,
                      int w, int h)
{
    void *ans = edmac_copy_start(dst, dst_pitch, src, src_pitch, w, h, NULL, NULL);
    if (ans) edmac_copy_finish();
    return ans;
}

static void edmac_copy_no_cbr(void *ctx)
{
}

void *edmac_copy_rect_async(void *dst, int dst_pitch,
                            void *src, int src_pitch,
                            int w, int h,
                            void (*done_cbr)(void *ctx), void *ctx)
{
    /* without a callback, this would be taken for a blocking copy,
     * and the channels would never be released */
    if (!done_cbr) done_cbr = &edmac_copy_no_cbr;

    return edmac_copy_start(dst, dst_pitch, src, src_pitch, w, h, done_cbr, ctx);
}

uint32_t edmac_find_divider(size_t length, size_t transfer_size)
{
    uint32_t max_width = (uint32_t) sqrtf(length);

    /* find a fitting divider for length = width * height */
    /* note: (width * height) % (bytes per transfer) must be 0 */
    if (!transfer_size)
    {
        transfer_size = edmac_bytes_per_transfer(edmac_memcpy_flags);
    }

    if (length % transfer_size)
    {
        /* this will crash */
        return 0;
    }

    for (uint32_t width = max_width; width > 0; width--)
    {
        if (length % width == 0)
        {
            return width;
        }
    }

    /* should be unreachable */
    return 0;
}

void *edmac_memcpy(void *dst, void *src, size_t length)
{
    int width = edmac_find_divider(length, 0);

    if (!width)
    {
        printf("[edmac] warning: using memcpy (size=%d)\n", length);
        return memcpy(dst, src, length);
    }

    return edmac_copy_rect(dst, width, src, width, width, length / width);
}

#else // !CONFIG_EDMAC_MEMCPY

// Modules (mlv_lite, silent) refer to these regardless of EDMAC_MEMCPY
// support; these values make edmac_get_base() fail, so the channels
// cannot be touched by accident.
uint32_t edmac_memcpy_read_chan = 0xffffffff;
uint32_t edmac_memcpy_write_chan = 0xffffffff;
uint32_t edmac_memcpy_connection = 0xffffffff;

#endif // CONFIG_EDMAC_MEMCPY

/** this method bypasses Canon's lv_save_raw and slurps the raw data directly from connection #0 */
#ifndef CONFIG_EDMAC_RAW_SLURP
// mlv_lite requires the symbol to exist regardless of RAW_SLURP support,
// this value makes edmac_start_spy() fail to start (good, since it requires RAW_SLURP to work)
uint32_t edmac_raw_write_chan = 0xffffffff;
#endif

#ifdef CONFIG_EDMAC_RAW_SLURP

#if defined(CONFIG_5D3)
uint32_t edmac_raw_write_chan = 0x4;  /* 0x12 gives corrupted frames on 1.2.3, http://www.magiclantern.fm/forum/index.php?topic=10443 */
#elif defined(EVF_STATE)
uint32_t edmac_raw_write_chan = 0x12; /* 60D and newer, including all DIGIC V */
#endif

static void edmac_slurp_complete_cbr (void* ctx)
{
    /* set default CBRs again and stop both DMAs */
    /* idk what to do with those; if I uncomment them, the camera crashes at startup in movie mode with raw_rec enabled */
    //~ UnregisterEDmacCompleteCBR(edmac_raw_write_chan);
    //~ UnregisterEDmacAbortCBR(edmac_raw_write_chan);
    //~ UnregisterEDmacPopCBR(edmac_raw_write_chan);
}

void edmac_raw_slurp(void* dst, int w, int h)
{
    /* see wiki, register map, EDMAC what the flags mean. they are for setting up copy block size */
#if defined(CONFIG_650D) || defined(CONFIG_700D) || defined(CONFIG_EOSM) || defined(CONFIG_100D)
    uint32_t dmaFlags = EDMAC_2_BYTES_PER_TRANSFER;
#elif defined(CONFIG_6D)
    uint32_t dmaFlags = EDMAC_4_BYTES_PER_TRANSFER;
#else
    uint32_t dmaFlags = EDMAC_8_BYTES_PER_TRANSFER;
#endif

    /* @g3gg0: this callback does get called */
    RegisterEDmacCompleteCBR(edmac_raw_write_chan, &edmac_slurp_complete_cbr, 0);
    RegisterEDmacAbortCBR(edmac_raw_write_chan, &edmac_slurp_complete_cbr, 0);
    RegisterEDmacPopCBR(edmac_raw_write_chan, &edmac_slurp_complete_cbr, 0);
    
    /* connect the selected channels to 0 so the raw data from sensor is passed to write channel */
    ConnectWriteEDmac(edmac_raw_write_chan, 0);
    
    /* xb is width */
    /* yb is height-1 (number of repetitions) */
    struct edmac_info dst_edmac_info = {
        .xb = w,
        .yb = h-1,
    };
    
    SetEDmac(edmac_raw_write_chan, (void*)dst, &dst_edmac_info, dmaFlags);
    
    /* start transfer. no flags for write, 2 for read channels */
    StartEDmac(edmac_raw_write_chan, 0);
}
#endif /* CONFIG_EDMAC_RAW_SLURP */
