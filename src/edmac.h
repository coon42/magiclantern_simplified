#ifndef _edmac_c
#define _edmac_c

#include <stdint.h>
#include "compiler.h" // for SIZE_CHECK_STRUCT

/**
 * EDMAC: the DMA engine of the image processor.
 *
 * A channel moves image data between RAM and a "connection", i.e. some
 * processing module inside the DIGIC.  Write channels store data coming
 * from a connection into RAM, read channels feed data from RAM into one.
 *
 * This header has three parts:
 *  - the data structures describing a channel and its transfer geometry,
 *  - the Canon firmware functions driving a channel (stubs),
 *  - ML helpers reading back how a channel is configured (edmac.c).
 *
 * For copying memory with EDMAC, see edmac-memcpy.h.
 */

/* Direction of a channel, see edmac_get_dir() */
#define EDMAC_DIR_READ    0
#define EDMAC_DIR_WRITE   1
#define EDMAC_DIR_UNUSED  2

/* Channel flags (4th argument of SetEDmac) selecting the transfer block
 * size.  DIGIC 4 can do up to 4 bytes per transfer, DIGIC 5 up to 16.
 * See the EDMAC section of the register map on the wiki. */
#define EDMAC_16_BYTES_PER_TRANSFER         0x40001000
#define EDMAC_8_BYTES_PER_TRANSFER          0x20001000
#define EDMAC_4_BYTES_PER_TRANSFER          0x40000000
#define EDMAC_2_BYTES_PER_TRANSFER          0x20000000

/* Transfer geometry of a channel (3rd argument of SetEDmac).
 * The simplest transfer is yb+1 lines of xb bytes each, skipping off1b
 * bytes after every line; all other fields are 0. */
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

/**
 * Canon firmware functions (stubs).
 *
 * To run a transfer: register CBRs, connect the channel, SetEDmac,
 * StartEDmac.  The CBRs are called from interrupt context.
 */
void SetEDmac(unsigned int channel, void *address, struct edmac_info *ptr, int flags);
void StartEDmac(unsigned int channel, int flags);
void AbortEDmac(unsigned int channel);
void ConnectWriteEDmac(unsigned int channel, unsigned int where);
void ConnectReadEDmac(unsigned int channel, unsigned int where);

void RegisterEDmacCompleteCBR(int channel, void (*cbr)(void*), void* cbr_ctx);
void RegisterEDmacAbortCBR(int channel, void (*cbr)(void*), void* cbr_ctx);
void RegisterEDmacPopCBR(int channel, void (*cbr)(void*), void* cbr_ctx);

void UnregisterEDmacCompleteCBR(int channel);
void UnregisterEDmacAbortCBR(int channel);
void UnregisterEDmacPopCBR(int channel);

/* Engine resource locks: exclusive access to a set of EDMAC channels,
 * connections and processing modules.  Channels are identified in the
 * resource IDs by their index, see edmac_channel_to_index(). */
struct LockEntry *CreateResLockEntry(uint32_t *resIds, uint32_t resIdCount);
unsigned int LockEngineResources(struct LockEntry *lockEntry);
unsigned int UnLockEngineResources(struct LockEntry *lockEntry);

/**
 * ML helpers: how is a channel configured, and what is it doing?
 *
 * Unless noted otherwise, these return the values last written by Canon
 * code (from the shadow memory of the registers), not what the hardware
 * reports.  The register getters return 0xFFFFFFFF for channel numbers
 * out of range.
 */

/* EDMAC_DIR_READ, EDMAC_DIR_WRITE or EDMAC_DIR_UNUSED */
uint32_t edmac_get_dir(uint32_t channel);

/* Position of a channel among the read (or write) channels, as used
 * in the resource IDs for CreateResLockEntry; -1 for unused channels */
int edmac_channel_to_index(uint32_t channel);

/* Address of the first register of a channel, and the other way around */
uint32_t edmac_get_base(uint32_t channel);
uint32_t edmac_base_to_channel(uint32_t base);

/* 0 = idle, 1 = running (from hardware) */
uint32_t edmac_get_state(uint32_t channel);

/* Channel flags, as passed to SetEDmac */
uint32_t edmac_get_flags(uint32_t channel);

/* Memory address where the transfer started */
uint32_t edmac_get_start_address(uint32_t channel);

/* Memory address the transfer has reached by now (from hardware) */
uint32_t edmac_get_current_address(uint32_t channel);

/* Connection used by a channel.  The direction decides where to look it
 * up; for read channels, 0xFF means the connection could not be found. */
uint32_t edmac_get_connection(uint32_t channel, uint32_t direction);

/* Transfer geometry of a channel */
struct edmac_info edmac_get_info(uint32_t channel);

/* Number of bytes moved by a transfer with the given geometry */
uint32_t edmac_transfer_size(struct edmac_info *info);

/* Size of the memory range covered by a transfer with the given
 * geometry, i.e. the bytes moved plus the bytes skipped (offsets) */
uint32_t edmac_transfer_span(struct edmac_info *info);

/* Transfer block size selected by the given channel flags, in bytes */
uint32_t edmac_bytes_per_transfer(uint32_t flags);

#endif
