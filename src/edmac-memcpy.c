#include "edmac-memcpy.h"
#include "dryos.h"
#include "edmac.h"
#include "bmp.h"
#include "math.h"
#include "platform/state-object.h"

#ifdef CONFIG_EDMAC_MEMCPY

static struct semaphore *copy_sem = NULL;      /* one copy at a time */
static struct semaphore *copy_done_sem = NULL; /* for the blocking copies */

/* what to call when the running copy ends */
static edmac_copy_done_cbr copy_done = NULL;
static void *copy_done_ctx = NULL;

/* pick some free (check using debug menu) EDMAC channels write: 0x00-0x06, 0x10-0x16, 0x20-0x21. read: 0x08-0x0D, 0x18-0x1D,0x28-0x2B */
#if defined(CONFIG_5D2) || defined(CONFIG_50D)
uint32_t edmac_copy_read_chan = 0x19;
uint32_t edmac_copy_write_chan = 0x03;
/*
50D
R 2-15
W 3-8 10-15
*/
#elif defined(CONFIG_650D) || defined(CONFIG_EOSM) || defined(CONFIG_700D) || defined(CONFIG_100D)
uint32_t edmac_copy_read_chan = 0x19;
uint32_t edmac_copy_write_chan = 0x13;
//~ r 2 3 5 7 8 9 10 11-13
//~ w 3 4 6 10 11-15
#elif defined(CONFIG_60D)
uint32_t edmac_copy_read_chan = 0x19;  /* free indices: 2, 3, 4, 5, 6, 7, 8, 9 */
uint32_t edmac_copy_write_chan = 0x06; /* 1, 4, 6, 10 */
#elif defined(CONFIG_6D) || defined(CONFIG_5D3)
uint32_t edmac_copy_read_chan = 0x19;  /* Read: 0 5 7 11 14 15 */
uint32_t edmac_copy_write_chan = 0x11; /* Write: 6 8 15 */
#elif defined(CONFIG_70D)
// 70D uses same read and write channels as 6D and 5D3
// just keep it separate with the comments
uint32_t edmac_copy_read_chan = 0x19;  /* Read decimal: 8 25 29 42 43 - hex: 0x08 0x19 0x1D 0x2A 0x2B*/
uint32_t edmac_copy_write_chan = 0x11; /* Write decimal: 6 17 33 - hex: 0x06 0x11 0x21*/
#elif defined(CONFIG_7D)
uint32_t edmac_copy_read_chan = 0x0A;  /*Read 0x19 0x0D 0x0B 0x0A(82MB/S)*/
uint32_t edmac_copy_write_chan = 0x06; /* Write 0x5 0x6 0x4 (LV) */
//5 zoom, 6 not - improved performance (no HDMI related tearing)
#elif defined(CONFIG_500D)
uint32_t edmac_copy_read_chan = 0x0D;
uint32_t edmac_copy_write_chan = 0x04;
#elif defined(CONFIG_550D)
uint32_t edmac_copy_read_chan = 0x19;
uint32_t edmac_copy_write_chan = 0x05;
#elif defined(CONFIG_600D)
uint32_t edmac_copy_read_chan = 0x19;
uint32_t edmac_copy_write_chan = 0x06;
#elif defined(CONFIG_1100D)
uint32_t edmac_copy_read_chan = 0x19;
uint32_t edmac_copy_write_chan = 0x04;
#else
#error Please find some free EDMAC channels for your camera.
#endif

/* both channels get connected to this... lets call it service. it will just output the data it gets as input */
uint32_t edmac_copy_connection = 6;

/* see wiki, register map, EDMAC what the flags mean. they are for setting up copy block size */
#if defined(CONFIG_7D)
static const uint32_t copy_flags = EDMAC_2_BYTES_PER_TRANSFER; //Original are faster on 7D
#else
static const uint32_t copy_flags = EDMAC_16_BYTES_PER_TRANSFER; //Enhanced
#endif

static struct LockEntry * resLock = 0;

static void edmac_memcpy_init()
{
    copy_sem = create_named_semaphore("edmac_copy_sem", SEM_CREATE_UNLOCKED);
    copy_done_sem = create_named_semaphore("edmac_copy_done_sem", SEM_CREATE_LOCKED);

    uint32_t resIds[] = {
        edmac_channel_res_id(edmac_copy_write_chan),
        edmac_channel_res_id(edmac_copy_read_chan),
        EDMAC_WRITE_CONN_RES_ID(edmac_copy_connection),
        EDMAC_READ_CONN_RES_ID(edmac_copy_connection),
    };
    ASSERT(resIds[0] != 0xFFFFFFFF && resIds[1] != 0xFFFFFFFF);

    resLock = CreateResLockEntry(resIds, COUNT(resIds));
    ASSERT(resLock);

    /* just to make sure we have this stub */
    static void *AbortEDmac_check __attribute__((used)) = &AbortEDmac;
}

INIT_FUNC("edmac_memcpy", edmac_memcpy_init);

void edmac_copy_lock_resources(void)
{
    int r = LockEngineResources(resLock);
    if (r & 1)
    {
        NotifyBox(2000, "ResLock fail %x %x", resLock, r);
        return;
    }
}

void edmac_copy_unlock_resources(void)
{
    UnLockEngineResources(resLock);
}

/* Restore Canon's CBRs and let the next copy start. */
static void copy_release(void)
{
    edmac_clear_cbr(edmac_copy_read_chan);
    edmac_clear_cbr(edmac_copy_write_chan);
    copy_done = NULL;
    copy_done_ctx = NULL;
    give_semaphore(copy_sem);
}

static void copy_read_cbr(void *ctx)
{
    /* nothing to do, the copy is finished when the write channel is */
}

/* Interrupt context. */
static void copy_write_cbr(void *ctx)
{
    edmac_copy_done_cbr done = copy_done;
    void *done_ctx = copy_done_ctx;

    copy_release();

    if (done)
    {
        done(done_ctx);
    }
}

void *edmac_copy_rect_async(void *dst, void *src,
                            int src_pitch, int src_x, int src_y,
                            int dst_pitch, int dst_x, int dst_y,
                            int w, int h,
                            edmac_copy_done_cbr done, void *ctx)
{
    /* dmaFlags: 16 (DIGIC 5) or 4 (DIGIC 4) bytes per transfer
     * in order to successfully stop the EDMAC transfer,
     * w * h must be mod number of bytes per transfer
     * (not sure why it works that way, found experimentally)
     *
     * Do not remove this check, or risk permanent camera bricking.
     */
    uint32_t bpt = edmac_bytes_per_transfer(copy_flags);
    if ((w * h) % bpt)
    {
        printf("Invalid EDMAC output size: %d x %d (mod%d = %d)\n", w, h, bpt, (w * h) % bpt);
        return NULL;
    }

    /* make sure we are writing to uncacheable memory */
    ASSERT(dst == UNCACHEABLE(dst));

    /* clean the cache before reading from regular (cacheable) memory */
    /* see FIO_WriteFile for more info */
    if (src == CACHEABLE(src))
    {
        sync_caches();
    }

    take_semaphore(copy_sem, 0);

    uint32_t src_adjusted = ((uint32_t)src & 0x1FFFFFFF) + src_x + src_y * src_pitch;
    uint32_t dst_adjusted = ((uint32_t)dst & 0x1FFFFFFF) + dst_x + dst_y * dst_pitch;

    copy_done = done;
    copy_done_ctx = ctx;

    /* Both channels report completion; the copy is done when the write
     * channel has stored the last data. */
    edmac_set_cbr(edmac_copy_read_chan, copy_read_cbr, NULL);
    edmac_set_cbr(edmac_copy_write_chan, copy_write_cbr, NULL);

    /* connect both channels to the loopback connection, so any data read
     * from RAM is passed to the write channel */
    ConnectWriteEDmac(edmac_copy_write_chan, edmac_copy_connection);
    ConnectReadEDmac(edmac_copy_read_chan, edmac_copy_connection);

    /* xb is width */
    /* yb is height-1 (number of repetitions) */
    /* off1b is the number of bytes to skip after every xb bytes being transferred */
    struct edmac_info src_edmac_info = {
        .xb = w,
        .yb = h-1,
        .off1b = src_pitch - w,
    };

    struct edmac_info dst_edmac_info = {
        .xb = w,
        .yb = h-1,
        .off1b = dst_pitch - w,
    };

    SetEDmac(edmac_copy_read_chan, (void*)src_adjusted, &src_edmac_info, copy_flags);
    SetEDmac(edmac_copy_write_chan, (void*)dst_adjusted, &dst_edmac_info, copy_flags);

    /* start transfer. no flags for write, 2 for read channels */
    StartEDmac(edmac_copy_write_chan, 0);
    StartEDmac(edmac_copy_read_chan, 2);

    return dst;
}

static void copy_done_give_sem(void *ctx)
{
    give_semaphore(copy_done_sem);
}

void *edmac_copy_rect(void *dst, void *src,
                      int src_pitch, int src_x, int src_y,
                      int dst_pitch, int dst_x, int dst_y,
                      int w, int h)
{
    void *ans = edmac_copy_rect_async(dst, src,
                                      src_pitch, src_x, src_y,
                                      dst_pitch, dst_x, dst_y,
                                      w, h, copy_done_give_sem, NULL);
    if (!ans)
    {
        return NULL;
    }

    if (take_semaphore(copy_done_sem, 1000))
    {
        NotifyBox(2000, "EDMAC timeout");

        /* don't let a late completion leak into the next copy */
        int old = cli();
        if (copy_done == copy_done_give_sem)
        {
            copy_release();
        }
        sei(old);
    }

    return ans;
}

/* Largest width <= sqrt(length) that divides length, so that the copy is
 * a roughly square w x h rectangle.  0 if EDMAC can't copy this length. */
static uint32_t find_divider(size_t length)
{
    if (length % edmac_bytes_per_transfer(copy_flags))
    {
        /* this will crash */
        return 0;
    }

    for (uint32_t width = (uint32_t) sqrtf(length); width > 0; width--)
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
    int w = find_divider(length);

    if (!w)
    {
        printf("[edmac] warning: using memcpy (size=%d)\n", length);
        return memcpy(dst, src, length);
    }

    return edmac_copy_rect(dst, src,
                           w, 0, 0,
                           w, 0, 0,
                           w, length / w);
}

#else /* !CONFIG_EDMAC_MEMCPY */

/* DIGIC 6+ ports that implement the copy functions in function_overrides.c
 * don't use a fixed channel pair. */
uint32_t edmac_copy_read_chan = 0xFFFFFFFF;
uint32_t edmac_copy_write_chan = 0xFFFFFFFF;
uint32_t edmac_copy_connection = 0xFFFFFFFF;

#endif // CONFIG_EDMAC_MEMCPY

/** this method bypasses Canon's lv_save_raw and slurps the raw data directly from connection #0 */
#ifndef CONFIG_EDMAC_RAW_SLURP
// mlv_lite requires the symbol to exist regardless of RAW_SLURP support,
// this value makes edmac_start_spy() fail to start (good, since it requires RAW_SLURP to work)
uint32_t edmac_raw_slurp_chan = 0xffffffff;
#endif

#ifdef CONFIG_EDMAC_RAW_SLURP

#if defined(CONFIG_5D3)
uint32_t edmac_raw_slurp_chan = 0x4;  /* 0x12 gives corrupted frames on 1.2.3, http://www.magiclantern.fm/forum/index.php?topic=10443 */
#elif defined(EVF_STATE)
uint32_t edmac_raw_slurp_chan = 0x12; /* 60D and newer, including all DIGIC V */
#endif

static void edmac_slurp_complete_cbr (void* ctx)
{
    /* set default CBRs again and stop both DMAs */
    /* idk what to do with those; if I uncomment them, the camera crashes at startup in movie mode with raw_rec enabled */
    //~ edmac_clear_cbr(edmac_raw_slurp_chan);
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
    edmac_set_cbr(edmac_raw_slurp_chan, &edmac_slurp_complete_cbr, 0);

    /* connect the selected channels to 0 so the raw data from sensor is passed to write channel */
    ConnectWriteEDmac(edmac_raw_slurp_chan, 0);
    
    /* xb is width */
    /* yb is height-1 (number of repetitions) */
    struct edmac_info dst_edmac_info = {
        .xb = w,
        .yb = h-1,
    };
    
    SetEDmac(edmac_raw_slurp_chan, (void*)dst, &dst_edmac_info, dmaFlags);
    
    /* start transfer. no flags for write, 2 for read channels */
    StartEDmac(edmac_raw_slurp_chan, 0);
}
#endif /* CONFIG_EDMAC_RAW_SLURP */
