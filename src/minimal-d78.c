/** \file
 * Minimal test code for DIGIC 7 & 8
 * ROM dumper & other experiments
 */

#include "dryos.h"


static void led_blink(int times, int delay_on, int delay_off)
{
    for (int i = 0; i < times; i++)
    {
        MEM(CARD_LED_ADDRESS) = LEDON;
        msleep(delay_on);
        MEM(CARD_LED_ADDRESS) = LEDOFF;
        msleep(delay_off);
    }
}

int (*uiLock)(int doLock_maybe, int param_2, int param_3) = 0xe044b1e9;
extern int uart_printf(const char * fmt, ...);

#undef malloc
#undef free
#undef _alloc_dma_memory
#define malloc _alloc_dma_memory
#define free _free_dma_memory
extern void * _alloc_dma_memory(size_t);
extern void _free_dma_memory(void *);

#define FIO_CreateFile _FIO_CreateFile
extern FILE* _FIO_CreateFile(const char* filename );

#define FIO_GetFileSize _FIO_GetFileSize
extern int _FIO_GetFileSize(const char * filename, uint32_t * size);

#define FIO_WriteFile _FIO_WriteFile
extern int _FIO_WriteFile( FILE* stream, const void* ptr, size_t count );

#define FIO_RemoveFile _FIO_RemoveFile
extern int _FIO_RemoveFile(const char * filename);

extern int FIO_Flush(const char * filename);

// Micro ML

static bool _displayMenu = false;
static uint8_t* _pVram1 = (uint8_t*)0x41100100;
static uint8_t* _pVram2 = (uint8_t*)0x41340400;

static const uint16_t _xRes = 736;
static const uint16_t _yRes = 480;
static uint8_t* _pRgbaBuffer = NULL;

enum COLORS {
  COLOR_WHITE  = 0x80FF80FF,
  COLOR_RED    = 0x544CFFFF,
  COLOR_GREEN  = 0x2B9515FF,
  COLOR_BLUE   = 0xFF1D6BFF,
  COLOR_YELLOW = 0xD469EAFF
};

typedef union {
  struct {
    uint8_t a;
    uint8_t v;
    uint8_t y;
    uint8_t u;
  } yuv;

  uint32_t val;
} YuvColor;

static void bgra2uyvyaa(uint8_t* pYuv, const uint8_t* pRgba) {
  for (int y = 0; y < _yRes; ++y) {
    for (int x = 0; x < _xRes / 2; ++x) {
      const int rgbaIdx = 4 * y * _xRes + 8 * x;
      const int yuvIdx = 3 * y * _xRes + 6 * x;

      const uint8_t b1 = pRgba[rgbaIdx + 0];
      const uint8_t g1 = pRgba[rgbaIdx + 1];
      const uint8_t r1 = pRgba[rgbaIdx + 2];
      const uint8_t a1 = pRgba[rgbaIdx + 3];
      const uint8_t b2 = pRgba[rgbaIdx + 4];
      const uint8_t g2 = pRgba[rgbaIdx + 5];
      const uint8_t r2 = pRgba[rgbaIdx + 6];
      const uint8_t a2 = pRgba[rgbaIdx + 7];

      /* Accurate but slow. Therefore using indexed color table as a workaround for now:
      const uint8_t u   = r1 * -.168736 + g1 * -.331264 + b1 *  .500000 + 128;
      const uint8_t y1  = r1 *  .299000 + g1 *  .587000 + b1 *  .114000;
      const uint8_t v   = r1 *  .500000 + g1 * -.418688 + b1 * -.081312 + 128;
      const uint8_t y2  = r2 *  .299000 + g2 *  .587000 + b2 *  .114000;
      */

      static uint32_t pColor[2];
      pColor[0] = (b1 << 24) | (g1 << 16) | (r1 << 8) | (a1 << 0);
      pColor[1] = (b2 << 24) | (g2 << 16) | (r2 << 8) | (a2 << 0);

      static uint8_t u,y1,v,y2;
      YuvColor yuvC;

      for (int i = 0; i < 2; ++i) {
        switch (pColor[i] & 0xFFFFFF00) {
          case 0x0000FF00: yuvC.val = COLOR_RED;    break;
          case 0x00FF0000: yuvC.val = COLOR_GREEN;  break;
          case 0xFF000000: yuvC.val = COLOR_BLUE;   break;
          case 0xFF00FF00: yuvC.val = COLOR_YELLOW; break;

          case 0xFFFFFF00:
          default:
            yuvC.val = COLOR_WHITE;
            break;
        }

        if (i == 0) {
          u =  yuvC.yuv.u;
          y1 = yuvC.yuv.y;
          v =  yuvC.yuv.v;
	}
        else
          y2 = yuvC.yuv.y;
      }

      pYuv[yuvIdx + 0] = u;
      pYuv[yuvIdx + 1] = y1;
      pYuv[yuvIdx + 2] = v;
      pYuv[yuvIdx + 3] = y2;
      pYuv[yuvIdx + 4] = a1;
      pYuv[yuvIdx + 5] = a2;
    }
  }
}

extern void font_draw(uint32_t, uint32_t, uint32_t, uint32_t, char*);

void lcdPrintf(int x, int y, char* pFormat, ...) {
  char pBuffer[64];

  va_list args;
  va_start(args, pFormat);
  vsnprintf(pBuffer, sizeof(pBuffer), pFormat, args);
  font_draw(x, y, 0xFF000000, 3, pBuffer);
  va_end(args);
}

// used by font_draw:
void disp_set_pixel(int x, int y, int c) {
  const int rgbIdx = 4 * y * _xRes + 4 * x;

  if (!_pRgbaBuffer)
    return;

  uint8_t r = (c >> 24) & 0xFF;
  uint8_t g = (c >> 16) & 0xFF;
  uint8_t b = (c >> 8)  & 0xFF;
  uint8_t a = (c >> 0)  & 0xFF;

  _pRgbaBuffer[rgbIdx + 0] = b;
  _pRgbaBuffer[rgbIdx + 1] = g;
  _pRgbaBuffer[rgbIdx + 2] = r;
  _pRgbaBuffer[rgbIdx + 3] = a;
}

static void mpu_decode(const char* in, char* out, int max_len) {
  int len = 0;
  int size = (unsigned char) in[0];

  // print each byte as hex
  for (const char * c = in; c < in + size; c++)
    len += snprintf(out + len, max_len - len, "%02x ", *c);

  // trim the last space
  if (len)
    out[len - 1] = 0;

  const int msgId = in[2];

  if (msgId == 0x03) { // Button press
    const int buttonId = in[3];
    const int buttonPress = in[4];

    const char* pButtonName;

    switch (buttonId) {
      case 0x05: pButtonName = "Sw1 (half shutter)"; break;
      case 0x10: pButtonName = "Info"; break;
      case 0x11: {
	pButtonName = "Trashcan";

	if (buttonPress) {
	  if (!_displayMenu) {
            if (!uiLock(1, 0, 2))
              _displayMenu = true;
	  }
	  else {
	   if (!uiLock(0, 0, 2))
             _displayMenu = false;
	  }
	}
	
	break;
      }

      default:
        pButtonName = "Unknown";
    }

    uart_printf("Button %s (0x%X) %s\n", pButtonName, buttonId, buttonPress == 0x00 ? "release" : "press");
  }
}

static void DUMP_ASM microml_task() {
  uart_printf("[ML] Hello from %s!\n", get_current_task_name());

  const uint32_t rgbaSize = 4 * _xRes * _yRes;
  _pRgbaBuffer = malloc(rgbaSize);

  if (!_pRgbaBuffer) {
    uart_printf("[ML] Error: Unable to allocate frame buffer!\n");

    led_blink(1, 50, 50);
  }

  uart_printf("[ML] Frame buffer allocated at %p\n", _pRgbaBuffer);

  while(true) {
    memset(_pRgbaBuffer, 0xFF, rgbaSize);

    extern const int mpu_recv_ring_buffer_tail;
    lcdPrintf(50, 125, "%s: %d", "ring buffer tail index", mpu_recv_ring_buffer_tail);

    extern const char* const mpu_recv_ring_buffer[105];

    static const char* pLastMessage = "none";
    static int last_tail = 0;
    static char pMpuMsg[105];

    const max_mpu_recv = 80; // see RP 1.6.0: 0xe009b2ce
    const max_mpu_send = 50; // see RP 1.6.0: 0xe009b2dc
    const int diff = mpu_recv_ring_buffer_tail - last_tail;
    const int numNewMpuMessages = diff > 0 ? diff : diff + max_mpu_recv;

    for (int i = 0; i < numNewMpuMessages; ++i) {
      pLastMessage = &mpu_recv_ring_buffer[last_tail][4];
      mpu_decode(pLastMessage, pMpuMsg, sizeof(pMpuMsg));
      uart_printf("[ML] <%d> *** mpu_recv(%s)\n", last_tail, pMpuMsg);

      last_tail = (last_tail + 1) % max_mpu_recv;
    }

    if (last_tail != mpu_recv_ring_buffer_tail) {
      pLastMessage = &mpu_recv_ring_buffer[last_tail][4];
      mpu_decode(pLastMessage, pMpuMsg, sizeof(pMpuMsg));
      //qprintf("[%d] mpu_recv(%s)\n", last_tail, msg);
      //DryosDebugMsg(0, 15, "[ML] <%d> *** mpu_recv(%s)", last_tail, mpu_msg);

      last_tail = mpu_recv_ring_buffer_tail;
    }

    const uint* p32BitTimer1 = 0xD020000C;
    uint snap32_1 = *p32BitTimer1;

    uint32_t t = snap32_1 / 1000;
    uint32_t s = (t / 1000) % 60;
    uint32_t m = (t / (1000 * 60)) % 60;
    uint32_t h = (t / (1000 * 60 * 60)) % 24;

    lcdPrintf(0, 325, "uptime: %02d:%02d:%02d", h, m, s);

    uint8_t** ppCurrentBufferIndex = 0xE0255234;
    uint8_t* pCurrentBufferIndex = (*ppCurrentBufferIndex) - 0xC;

    // uart_printf("[ML] current buffer index: [%p - 0xC] -> [%p] -> %d\n", *ppCurrentBufferIndex, pCurrentBufferIndex, *pCurrentBufferIndex);
    uint32_t* pMarvBase = 0xE0255238;
    struct MARV** ppMarv = *pMarvBase - 0x5c + 0 * 0x28 + (1 - *pCurrentBufferIndex) * 4;
    struct MARV* pMarv = *ppMarv;

    // uart_printf("[ML] pMarv: %p\n", pMarv);

    if (!pMarv) {
      uart_printf("[ML] VRAM buffer not available. Is camera off!? Exitting task...");
      return;
    }

    // uart_printf("[ML] MARV: [%p] -> [%p]\n", pMarv, pMarv->bitmap_data);
    // uart_printf("[ML] Resolution: %dx%d\n", pMarv->width, pMarv->height);

    if (pMarv->bitmap_data == _pVram1 || pMarv->bitmap_data == _pVram2) {
      if (_displayMenu)
        bgra2uyvyaa(pMarv->bitmap_data, _pRgbaBuffer);
    }
    else
      uart_printf("[ML]: error: Invalid frame buffer: %p\n", pMarv->bitmap_data);

    led_blink(2, 50, 50);

    msleep(1000);
  }
}

void ml_assert_handler(char* msg, char* file, int line, const char* func) { };

/* called before Canon's init_task */

void boot_pre_init_task(void)
{
    // void (*lowLevel_uartPrintf)(char* pFormat, ...) = 0xE000BE8F;
    // lowLevel_uartPrintf("boot_pre_init_task; [0x%x]: [0x%x]\n", mpu_recv_cbr, *mpu_recv_cbr);

    // nothing to do
}

/* called right after Canon's init_task, while their initialization continues in background */
void boot_post_init_task(void)
{
    msleep(1000);
    // uart_printf("boot_post_init_task; [0x%x]: [0x%x]\n", mpu_recv_cbr, *mpu_recv_cbr);

    task_create("microml_task", 0x1e, 0x1000, microml_task, 0 );
}

