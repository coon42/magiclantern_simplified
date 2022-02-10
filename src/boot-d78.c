/** \file
 * Startup code for DIGIC 7 & 8
 */

#include "dryos.h"
#include "boot.h"

/** These are called when new tasks are created */
static int my_init_task(int a, int b, int c, int d);
extern int early_uart_printf(const char * fmt, ...);

/** This just goes into the bss */
#define RELOCSIZE 0x1000 // look in HIJACK macros for the highest address, and subtract ROMBASEADDR
static uint32_t _reloc[ RELOCSIZE / 4 ];
#define RELOCADDR ((uintptr_t) _reloc)

/** Encoding for the Thumb instructions used for patching the boot code */
#define THUMB_RET_INSTR 0x00004770        /* BX LR */

static inline uint32_t thumb_branch_instr(uint32_t pc, uint32_t dest, uint32_t opcode)
{
    /* thanks atonal */
    //uint32_t offset = dest - ((pc + 4) & ~3); /* according to datasheets, this should be the correct calculation -> ALIGN(PC, 4) */
    uint32_t offset = dest - (pc + 4);  /* this one works for BX */
    uint32_t s = (offset >> 24) & 1;
    uint32_t i1 = (offset >> 23) & 1;
    uint32_t i2 = (offset >> 22) & 1;
    uint32_t imm10 = (offset >> 12) & 0x3ff;
    uint32_t imm11 = (offset >> 1) & 0x7ff;
    uint32_t j1 = (!(i1 ^ s)) & 0x1;
    uint32_t j2 = (!(i2 ^ s)) & 0x1;

    return opcode | (s << 10) | imm10 | (j1 << 29) | (j2 << 27) | (imm11 << 16);
}

#define THUMB_B_W_INSTR(pc,dest)    thumb_branch_instr((uint32_t)(pc), (uint32_t)(dest), 0x9000f000)
#define THUMB_BL_INSTR(pc,dest)     thumb_branch_instr((uint32_t)(pc), (uint32_t)(dest), 0xd000f000)
#define THUMB_BLX_INSTR(pc,dest)    thumb_branch_instr((uint32_t)(pc), (uint32_t)(dest), 0xc000f000)

#define INSTR( addr ) ( *(uint32_t*)( (addr) - ROMBASEADDR + RELOCADDR ) )

/** Fix a branch instruction in the relocated firmware image */
#define FIXUP_BRANCH( rom_addr, dest_addr ) \
    early_uart_printf("[BOOT] fixing up branch at 0x%08x", &INSTR( rom_addr )); \
    early_uart_printf(" (ROM: 0x%08x) to 0x%08x\n", rom_addr, dest_addr); \
    INSTR( rom_addr ) = THUMB_BL_INSTR( &INSTR( rom_addr ), (dest_addr) )

static void my_bzero32(void* buf, size_t len)
{
    bzero32(buf, len);
}

static void my_create_init_task(uint32_t a, uint32_t b, uint32_t c)
{
    create_init_task(a, b, c);
}

/* M50: Canon code calling these cache functions expects R3 to be preserved after the call */
/* trick to prevent our C compiler for overwriting R3: two unused parameters */
static void my_dcache_clean(uint32_t addr, uint32_t size, uint32_t keep1, uint32_t keep2)
{
    extern void dcache_clean(uint32_t, uint32_t, uint32_t, uint32_t);
    dcache_clean(addr, size, keep1, keep2);
}

static void my_icache_invalidate(uint32_t addr, uint32_t size, uint32_t keep1, uint32_t keep2)
{
    extern void icache_invalidate(uint32_t, uint32_t, uint32_t, uint32_t);
    icache_invalidate(addr, size, keep1, keep2);
}

void
__attribute__((noreturn,noinline,naked))
copy_and_restart( int offset )
{
    zero_bss();

    // Copy the firmware to somewhere safe in memory
    const uint8_t * const firmware_start = (void*) ROMBASEADDR;
    const uint32_t firmware_len = RELOCSIZE;
    uint32_t * const new_image = (void*) RELOCADDR;

    blob_memcpy( new_image, firmware_start, firmware_start + firmware_len );

    /*
     * in cstart() make these changes:
     * calls bzero(), then loads bs_end and calls
     * create_init_task
     */
    // Reserve memory at the end of malloc pool for our application
    // Note: unlike most (all?) DIGIC 4/5 cameras,
    // the malloc heap is specified as start + size (not start + end)
    // easiest way is to reduce its size and load ML right after it
    ml_reserved_mem = 0x46000;
    early_uart_printf("[BOOT] reserving memory: %d\n", ml_reserved_mem);
    early_uart_printf("before: user_mem_size = %d\n", INSTR(HIJACK_INSTR_HEAP_SIZE));
    INSTR( HIJACK_INSTR_HEAP_SIZE ) -= ml_reserved_mem;
    early_uart_printf(" after: user_mem_size = %d\n", INSTR(HIJACK_INSTR_HEAP_SIZE));

    // Fix cache maintenance calls before cstart
    FIXUP_BRANCH( HIJACK_FIXBR_DCACHE_CLN_1, my_dcache_clean );
    FIXUP_BRANCH( HIJACK_FIXBR_DCACHE_CLN_2, my_dcache_clean );
    FIXUP_BRANCH( HIJACK_FIXBR_ICACHE_INV_1, my_icache_invalidate );
    FIXUP_BRANCH( HIJACK_FIXBR_ICACHE_INV_2, my_icache_invalidate );

    // Fix the absolute jump to cstart
    FIXUP_BRANCH( HIJACK_INSTR_BL_CSTART, &INSTR( cstart ) );

    // Fix the calls to bzero32() and create_init_task() in cstart
    FIXUP_BRANCH( HIJACK_FIXBR_BZERO32, my_bzero32 );
    FIXUP_BRANCH( HIJACK_FIXBR_CREATE_ITASK, my_create_init_task );

    /* there are two more functions in cstart that don't require patching */
    /* the first one is within the relocated code; it initializes the per-CPU data structure at VA 0x1000 */
    /* the second one is called only when running on CPU1; assuming our code only runs on CPU0 */

    // Set our init task to run instead of the firmware one
    early_uart_printf("[BOOT] changing init_task from 0x%08x", INSTR(HIJACK_INSTR_MY_ITASK));
    early_uart_printf(" to 0x%08x\n", my_init_task);
    INSTR( HIJACK_INSTR_MY_ITASK ) = (uint32_t) my_init_task;

    // Make sure that our self-modifying code clears the cache
    sync_caches();

    // jump to Canon firmware (Thumb code)
    thunk __attribute__((long_call)) reloc_entry = (thunk)( RELOCADDR + 1 );
    early_uart_printf("[BOOT] jumping to relocated startup code at 0x%08x\n", reloc_entry);
    reloc_entry();

    // Unreachable
    while(1)
        ;
}
