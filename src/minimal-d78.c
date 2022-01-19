/** \file
 * Minimal test code for DIGIC 6
 * ROM dumper & other experiments
 */

#include "dryos.h"
#include "bmp.h"
// #include "log-d678.h"
#include "extfunctions.h"


extern void gui_main_task();

//For DOOM
extern void D_DoomMain(void);
extern void ml_gui_main_task();
//event ring buffer
 int inited = 0;
struct
{
    int a;
    int b;
    int c;
    char ssid[0x24];
    int d;
    int e;
    int f;
    int g;
    int h;
    char pass[0x3f];
} * wifisettings;
extern int wlanconnect(void *);

int wifiConnect(){
    #if defined(SSID) && defined(PASS) && defined(IP)
    msleep(1000);
    wifisettings = _AllocateMemory(0xfc);
    uart_printf("Allocated 0x%x bytes at 0x%x\n", 0xfc, wifisettings);
    memset(wifisettings, 0, 0xfc);
    strcpy(wifisettings->ssid, SSID);
    strcpy(wifisettings->pass, PASS);
    char *ip = IP;
    wifisettings->a = 0;
    wifisettings->b = 2;
    wifisettings->c = 2;
    wifisettings->d = 0;
    wifisettings->e = 6;
    wifisettings->f = 4;
    wifisettings->g = 0;
    wifisettings->h = 6;
    // hexDump("A",&wifisettings,0xfc);
    //Turn lime core on
    call("NwLimeInit");
    call("NwLimeOn");
    call("wlanpoweron");
    call("wlanup");
    call("wlanchk");
    call("wlanipset", ip);
    if (wlanconnect(wifisettings) != 0)
    {
        _FreeMemory(wifisettings);
        uart_printf("Cant connect to WIFI!\n");
        return 1;
    }
    while (MEM(0x1d90c) == 0xe073631f)
    {
        msleep(100);
    }              //wait for lime core to power on
    msleep(10000); //wait for the Lime core to init.
    return 0;
    #else
     return 1;
    #endif
}



uint16_t htons(uint16_t v) {
  return (v >> 8) | (v << 8);
}

void printError(const char* pErrorMsg) {
  uart_printf(pErrorMsg);
}
extern int drysh_ml_update(int argc, char const *argv[]);

/*
int buttonEvents[] = {
  MPU_Q_SET_UNPRESS
  MPU_Q_SET
  MPU_UNPRESS_UP
  MPU_PRESS_UP
  MPU_UNPRESS_DOWN
  MPU_PRESS_DOWN
  MPU_UNPRESS_LEFT
  MPU_PRESS_LEFT
  MPU_UNPRESS_RIGHT
  MPU_PRESS_RIGHT
};

int buttonMap[] = {KEY_FIRE, KEY_UPARROW, KEY_DOWNARROW, KEY_LEFTARROW, KEY_RIGHTARROW};
*/

int (*uiLock)(int doLock_maybe, int param_2, int param_3) = 0xe044b1e9;

static void DUMP_ASM task_doom()
{
    uart_printf("Starting doom!\n");
    msleep(2000);

    uiLock(1, 0, 2);

    D_DoomMain();

    while (true) {
        //uart_printf("Hello! I am in a error state so um yeah\n");
        msleep(1000);
    }
}
#if defined(SSID) && defined(PASS) && defined(IP)
static void DUMP_ASM task_update()
{
    while (!bmp_vram_raw())
    {
        msleep(100);
    }
    drysh_ml_update(0,"");
    while (1)
    {
         uart_printf("Waiting for reboot!\n");
        msleep(10);

    }
    
}
#endif

static void
my_task_dispatch_hook(
    struct context **p_context_old, /* on new DryOS (6D+), this argument is different (small number, unknown meaning) */
    struct task *prev_task_unused,  /* only present on new DryOS */
    struct task *next_task_new      /* only present on new DryOS; old versions use HIJACK_TASK_ADDR */
)
{
  // using gui button handler does not work properly for RP yet.
  // reading buttons directly from MPU queue instead.

  return;

    struct task *next_task = next_task_new;
    if (!next_task)
        return;

    struct context *context = next_task->context;
    if (!context)
        return;

    // Do nothing unless a new task is starting via the trampoile
    if (context->pc != (uint32_t)task_trampoline)
        return;

    thunk entry = (thunk)next_task->entry;

    if (entry == &gui_main_task)
    { //gui_main_task entry
        next_task->entry = &ml_gui_main_task;
    }
}
/* called before Canon's init_task */
void boot_pre_init_task(void)
{
    task_dispatch_hook = &my_task_dispatch_hook;
}

/* called right after Canon's init_task, while their initialization continues in background */
void boot_post_init_task(void)
{
    //MEM(0x115d8) = (int)&uart_printf | 1;
    //MEM(0x115e4) =  (int)&uart_printf | 1;
    msleep(1000);
    inited = 1;
    task_create("DOOM", 0x1f, 0x1000, task_doom, 0);

    #if defined(SSID) && defined(PASS) && defined(IP)
    task_create("Updater",0x10,0x1000,task_update,0);
    #endif
}

/* used by font_draw */
/* we don't have a valid display buffer yet */
void disp_set_pixel(int x, int y, int c)
{
}

#ifndef CONFIG_5D4
/* dummy */

#endif

void ml_assert_handler(char *msg, char *file, int line, const char *func){};
