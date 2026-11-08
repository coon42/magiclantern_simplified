#ifndef _edmac_h_
#define _edmac_h_

#include "module.h"
#include "compiler.h" // for SIZE_CHECK_STRUCT


#define EDMAC_DIR_READ    0
#define EDMAC_DIR_WRITE   1
#define EDMAC_DIR_UNUSED  2

#define EDMAC_16_BYTES_PER_TRANSFER         0x40001000
#define EDMAC_8_BYTES_PER_TRANSFER          0x20001000
#define EDMAC_4_BYTES_PER_TRANSFER          0x40000000
#define EDMAC_2_BYTES_PER_TRANSFER          0x20000000

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

void SetEDmac(unsigned int channel, void *address, struct edmac_info *ptr, int flags);
void StartEDmac(unsigned int channel, int flags);
void AbortEDmac(unsigned int channel);
void ConnectWriteEDmac(unsigned int channel, unsigned int where);
void ConnectReadEDmac(unsigned int channel, unsigned int where);

uint32_t edmac_channel_to_index(uint32_t channel);

uint32_t edmac_get_flags(uint32_t channel);
uint32_t edmac_get_state(uint32_t channel);     /* 0=idle, 1=running (from hardware) */
uint32_t edmac_get_base(uint32_t channel);      /* base register */
uint32_t edmac_get_channel(uint32_t reg);       /* channel from register */
uint32_t edmac_get_address(uint32_t channel);   /* start address */
uint32_t edmac_get_pointer(uint32_t channel);   /* current address (from hardware) */
uint32_t edmac_get_length(uint32_t channel);    /* yb,xb (hi,lo) */
uint32_t edmac_get_connection(uint32_t channel, uint32_t direction);
uint32_t edmac_get_dir(uint32_t channel);

struct edmac_info edmac_get_info(uint32_t channel);
uint32_t edmac_get_total_size(struct edmac_info * info, int include_offsets);

uint32_t edmac_bytes_per_transfer(uint32_t flags);

/* provided by edmac.mo */
#if defined(MODULE)
char * edmac_format_size(struct edmac_info * info);
#else
static char * (*edmac_format_size)(struct edmac_info * info) = MODULE_FUNCTION(edmac_format_size);
#endif

struct LockEntry *CreateResLockEntry(uint32_t *resIds, uint32_t resIdCount);
unsigned int LockEngineResources(struct LockEntry *lockEntry);
unsigned int UnLockEngineResources(struct LockEntry *lockEntry);

void RegisterEDmacCompleteCBR(int channel, void (*cbr)(void*), void* cbr_ctx);
void RegisterEDmacAbortCBR(int channel, void (*cbr)(void*), void* cbr_ctx);
void RegisterEDmacPopCBR(int channel, void (*cbr)(void*), void* cbr_ctx);

void UnregisterEDmacCompleteCBR(int channel);
void UnregisterEDmacAbortCBR(int channel);
void UnregisterEDmacPopCBR(int channel);

/* Register cbr(ctx) for completion, abort and pop on a channel, i.e. for
 * every way a transfer can end.  edmac_clear_cbr() restores Canon's
 * defaults.  The callback runs in interrupt context. */
void edmac_set_cbr(uint32_t channel, void (*cbr)(void *), void *ctx);
void edmac_clear_cbr(uint32_t channel);

#endif // _edmac_h_
