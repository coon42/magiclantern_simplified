#ifndef _edmac_h_
#define _edmac_h_

/*
 * EDMAC: the "engine DMA" controller Canon uses to move image data between
 * RAM and the image processing blocks (and, via a loopback connection,
 * between two RAM buffers; see edmac-memcpy.h for that).
 *
 * A transfer uses one *channel*.  Write channels move data from a
 * *connection* (an image processing block output) into RAM; read channels
 * move data from RAM into a connection.  The channel number alone tells
 * the direction, see edmac_get_dir().
 *
 * This header has three parts:
 * - the register layout and transfer geometry (struct edmac_info),
 * - Canon's own functions to drive a channel (stubs, Canon names kept so
 *   they can be found in the firmware),
 * - ML helpers to inspect channels; these read the registers directly and
 *   only know the DIGIC 4/5 register map.
 */

#include "module.h"
#include "compiler.h" // for SIZE_CHECK_STRUCT

#define EDMAC_DIR_READ    0
#define EDMAC_DIR_WRITE   1
#define EDMAC_DIR_UNUSED  2

/* Transfer size, for the flags argument of SetEDmac().
 * DIGIC 4 can do up to 4 bytes per transfer, DIGIC 5 up to 16. */
#define EDMAC_16_BYTES_PER_TRANSFER         0x40001000
#define EDMAC_8_BYTES_PER_TRANSFER          0x20001000
#define EDMAC_4_BYTES_PER_TRANSFER          0x40000000
#define EDMAC_2_BYTES_PER_TRANSFER          0x20000000

/* Transfer geometry, see edmac_data_size() for how the fields combine.
 * The common case is a w x h rectangle out of a buffer with a larger pitch:
 * .xb = w, .yb = h - 1, .off1b = pitch - w. */
#if defined(CONFIG_DIGIC_8X)
struct edmac_info
{
    // Extra "S" fields, probably 3rd dimension?
    unsigned int off1s; //D8+
    unsigned int off1a;
    unsigned int off1b;
    unsigned int off2s; //D8+
    unsigned int off2a;
    unsigned int off2b;
    unsigned int off3;
    unsigned int xs; //D8+
    unsigned int xa;
    unsigned int xb;
    unsigned int ys; //D8+
    unsigned int ya;
    unsigned int yb;
    unsigned int xn;
    unsigned int yn;
};
#else
struct edmac_info
{
    unsigned int off1a;
    unsigned int off1b;
    unsigned int off2a;
    unsigned int off2b;
    unsigned int off3;
    unsigned int xa;
    unsigned int xb;
    unsigned int ya;
    unsigned int yb;
    unsigned int xn;
    unsigned int yn;
};
#endif


// This represents an EDmac "channel", also called a "port"
// from Digic 8 onwards.  These are very likely not really 0x100
// in size, probably the DMA controller aligns to this value
// for hardware reasons.  I sized it that way to allow easy
// pointer increment, etc.
#if defined(CONFIG_DIGIC_8X)
// Digic 8 changes the struct significantly
struct edmac_mmio
{
    uint32_t unk_01[0xb];
    uint32_t ModeInfo;
    uint32_t unk_02[0x3];
    uint32_t cbr_registered;
    uint32_t unk03[0x2];
    uint32_t ys_xs;
    uint32_t ya_xa;
    uint32_t yb_xb;
    uint32_t yn_xn;
    uint32_t off1s;
    uint32_t off2s;
    uint32_t off1a;
    uint32_t off2a;
    uint32_t off1b;
    uint32_t off2b;
    uint32_t off3;
    uint32_t unk_04[0xb];
    uint32_t ram_addr;
    uint32_t unk_05[0x7];
    uint32_t transfer_mode;
    uint32_t unk_06[0x3];
    uint32_t PackUnpackInfo;
    uint32_t unk_07[0xb]; // some of this is padding, but I don't know how much
};
#else
// For some more, not terribly clear info on pre-DIGIC8, see:
// https://magiclantern.fandom.com/wiki/Register_Map#EDMAC
// near "SDRAM destination offset"
struct edmac_mmio
{
    uint32_t dma_state;
    uint32_t dma_flags;
    uint32_t ram_addr;
    uint32_t yn_xn;
    uint32_t yb_xb;
    uint32_t ya_xa;
    uint32_t off1b;
    uint32_t off2b;
    uint32_t off1a;
    uint32_t off2a;
    uint32_t off3;
    uint32_t unk_01;
    uint32_t irq_reason;
    uint32_t irq_state_related;
    uint32_t unk_02;
    uint32_t unk_03;
    uint32_t fencing_related_maybe;
    uint32_t unk_04[0x2f]; // some of this is padding, but I don't know how much
};
#endif
SIZE_CHECK_STRUCT(edmac_mmio, 0x100);

/*
 * Canon firmware.  On many DIGIC 6+ ports these are empty placeholders in
 * function_overrides.c, so code using them is in practice D4/D5 only.
 */
void ConnectWriteEDmac(unsigned int channel, unsigned int connection);
void ConnectReadEDmac(unsigned int channel, unsigned int connection);
void SetEDmac(unsigned int channel, void *address, struct edmac_info *info, int flags);
void StartEDmac(unsigned int channel, int flags); /* ML uses 0 for write, 2 for read channels */
void AbortEDmac(unsigned int channel);

void RegisterEDmacCompleteCBR(int channel, void (*cbr)(void *), void *ctx);
void RegisterEDmacAbortCBR(int channel, void (*cbr)(void *), void *ctx);
void RegisterEDmacPopCBR(int channel, void (*cbr)(void *), void *ctx);
void UnregisterEDmacCompleteCBR(int channel);
void UnregisterEDmacAbortCBR(int channel);
void UnregisterEDmacPopCBR(int channel);

/* Engine resource locks: the firmware's way of reserving EDMAC channels,
 * connections and image processing blocks.  A resource ID is
 * (resource type << 16) | number; see edmac_channel_res_id() and
 * EDMAC_*_CONN_RES_ID() for the EDMAC ones. */
struct LockEntry *CreateResLockEntry(uint32_t *resIds, uint32_t resIdCount);
unsigned int LockEngineResources(struct LockEntry *lockEntry);
unsigned int UnLockEngineResources(struct LockEntry *lockEntry);

/*
 * ML helpers (edmac.c)
 */

/* Register cbr(ctx) for completion, abort and pop on a channel, i.e. for
 * every way a transfer can end.  edmac_clear_cbr() restores Canon's
 * defaults.  The callback runs in interrupt context. */
void edmac_set_cbr(uint32_t channel, void (*cbr)(void *), void *ctx);
void edmac_clear_cbr(uint32_t channel);

/* Resource IDs for CreateResLockEntry(); 0xFFFFFFFF if the channel is unused. */
uint32_t edmac_channel_res_id(uint32_t channel);
#define EDMAC_WRITE_CONN_RES_ID(conn) (0x00020000 | (conn))
#define EDMAC_READ_CONN_RES_ID(conn)  (0x00030000 | (conn))

/* Channel number <-> register block address.  Both return 0xFFFFFFFF if
 * there is no such channel / register block. */
uint32_t edmac_reg_base(uint32_t channel);
uint32_t edmac_channel_from_reg(uint32_t reg);

uint32_t edmac_get_dir(uint32_t channel);          /* EDMAC_DIR_* */
uint32_t edmac_get_state(uint32_t channel);        /* 0 = idle, 1 = running (live register) */
uint32_t edmac_get_flags(uint32_t channel);        /* as passed to SetEDmac() */
uint32_t edmac_get_start_addr(uint32_t channel);   /* buffer address as set up */
uint32_t edmac_get_current_addr(uint32_t channel); /* transfer position (live register) */
uint32_t edmac_get_connection(uint32_t channel, uint32_t direction);
struct edmac_info edmac_get_info(uint32_t channel);

/* Bytes actually moved by a transfer with this geometry, and the size of
 * the RAM region it covers (data plus the gaps from the offsets). */
uint32_t edmac_data_size(struct edmac_info *info);
uint32_t edmac_span_size(struct edmac_info *info);

/* Decodes EDMAC_*_BYTES_PER_TRANSFER from SetEDmac() flags; 0 if unknown. */
uint32_t edmac_bytes_per_transfer(uint32_t flags);

/* provided by edmac.mo */
#if defined(MODULE)
char * edmac_format_size(struct edmac_info * info);
#else
static char * (*edmac_format_size)(struct edmac_info * info) = MODULE_FUNCTION(edmac_format_size);
#endif

#endif // _edmac_h_
