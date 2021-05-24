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

// #define FIO_CreateFile _FIO_CreateFile
extern FILE* _FIO_CreateFile(const char* filename );

extern FILE* _FIO_OpenFile(const char* pFileName, uint32_t mode);

extern int _FIO_ReadFile(FILE* pFile, void* pBuffer, size_t count);

// #define FIO_GetFileSize _FIO_GetFileSize
extern int _FIO_GetFileSize(const char * filename, uint32_t * size);

// #define FIO_WriteFile _FIO_WriteFile
extern int _FIO_WriteFile( FILE* stream, const void* ptr, size_t count );

// #define FIO_RemoveFile _FIO_RemoveFile
extern int _FIO_RemoveFile(const char * filename);

extern int FIO_Flush(const char * filename);

extern int _FIO_RenameFile(const char* pSrc, const char* pDst);

// Micro ML

// options:

typedef void(*OptionEnableCallback_t)(bool enable);

struct Option_t {
  const char* pName;
  bool enabled;
  OptionEnableCallback_t pOptionEnableCallback;
};

static struct Option_t _options[1];
const int _numOptions = sizeof(_options) / sizeof(struct Option_t);

static bool _displayMenu = false;
static uint8_t* _pVram1 = (uint8_t*)0x41100100;
static uint8_t* _pVram2 = (uint8_t*)0x41340400;

#define X_RES 736
#define Y_RES 480

static const uint16_t _xRes = X_RES;
static const uint16_t _yRes = Y_RES;
static uint8_t* _pRgbaBuffer = NULL;
const uint32_t _rgbaSize = 4 * X_RES * Y_RES;

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

static void displayMenu(bool display) {
  if (display) {
    if (!uiLock(1, 0, 2))
      _displayMenu = true;
  }
  else {
    if (!uiLock(0, 0, 2))
      _displayMenu = false;
  }
}

static void drawClear() { 
  memset(_pRgbaBuffer, 0xFF, _rgbaSize);
}

static void drawUptime() {
  const uint* p32BitTimer1 = 0xD020000C;
  uint snap32_1 = *p32BitTimer1;

  uint32_t t = snap32_1 / 1000;
  uint32_t s = (t / 1000) % 60;
  uint32_t m = (t / (1000 * 60)) % 60;
  uint32_t h = (t / (1000 * 60 * 60)) % 24;

  lcdPrintf(0, 325, "uptime: %02d:%02d:%02d", h, m, s);
}

static void drawOptions() {
  for (int i = 0; i < _numOptions; ++i)  
    lcdPrintf(10, 100, "%s: %s", _options[i].pName, _options[i].enabled ? "On" : "Off");
}

// sht_EnableManualSilent and sht_DisableManualSilent do call po.ignore and
// po.respect in a wrong way which leads to a crash on camera when not executed
// via UART or Canon Basic. Those functions do expect a null terminated list of
// strings. In canon code this null element is missing which leads to undefined
// behaviour.
//
// This implementations calls them in the proper way:

void (*setPoProlog)(int ignore) = 0xe05ad60b;
int (*setPo)(char** ppPoSetList, uint ignore) = 0xe05fd6f1;

static void sht_EnableManualSilentFixed() {
  setPoProlog(1);
  call("po.ignore", "MECHA", 0);
}

static void sht_DisableManualSilentFixed() {
  setPoProlog(0);
  call("po.respect", "MECHA", 0);
}

static void enableElectronicShutterMode(bool enable) {
  uart_printf("enableElectronicShutterMode; %d\n", enable);

  if (enable)
    sht_EnableManualSilentFixed();
  else
    sht_DisableManualSilentFixed();
}

static void onToggleMenuOption(int optionNo) {
  if (optionNo < 0 || optionNo > _numOptions) {
    uart_printf("[ML] cannot toggle option, invalid option no!\n");
    return;
  }

  const bool enable = _options[optionNo].enabled ? false : true;
  const char* pActionStr = enable ? "Enable" : "Disable";
  const char* pName = _options[optionNo].pName;

  uart_printf("%s %s\n", pActionStr, pName);

  if (_options[optionNo].pOptionEnableCallback) {
    _options[optionNo].pOptionEnableCallback(enable);
    _options[optionNo].enabled = enable;
  }
  else
    uart_printf("[ML] Error, no callback defined!\n");
}

static void onButtonPress(uint8_t buttonId, bool press) {
  const char* pButtonName = "undefined";

  // only allow trashcan button, if menu is not open:
  if (!_displayMenu && buttonId != 0x11)
    return;

  switch (buttonId) {
    case 0x05: pButtonName = "Sw1 (half shutter)"; break;
    case 0x0D: {
      pButtonName = "Set";

      if (press)
        onToggleMenuOption(0);

      break;
    }
    case 0x10: pButtonName = "Info"; break;
    case 0x11: {
      pButtonName = "Trashcan";

      if (press) {
        if (!_displayMenu)
          displayMenu(true);
        else
          displayMenu(false);
      }
       
      break;
    }

    default:
      pButtonName = "Unknown";
  }

  uart_printf("Button %s (0x%X) %s\n", pButtonName, buttonId, press ? "press" : "release");
}

static void render() {
  const uint8_t** ppCurrentBufferIndex = 0xE0255234;
  const uint8_t* pCurrentBufferIndex = (*ppCurrentBufferIndex) - 0xC;

  // uart_printf("[ML] current buffer index: [%p - 0xC] -> [%p] -> %d\n", *ppCurrentBufferIndex, pCurrentBufferIndex, *pCurrentBufferIndex);
  const uint32_t* pMarvBase = 0xE0255238;
  const struct MARV** ppMarv = *pMarvBase - 0x5c + 0 * 0x28 + (1 - *pCurrentBufferIndex) * 4;
  const struct MARV* pMarv = *ppMarv;

  // uart_printf("[ML] pMarv: %p\n", pMarv);

  if (!pMarv) {
    uart_printf("[ML] VRAM buffer not available. Is camera off!?");
    return;
  }

  // uart_printf("[ML] MARV: [%p] -> [%p]\n", pMarv, pMarv->bitmap_data);
  // uart_printf("[ML] Resolution: %dx%d\n", pMarv->width, pMarv->height);

  if (pMarv->bitmap_data == _pVram1 || pMarv->bitmap_data == _pVram2)
    bgra2uyvyaa(pMarv->bitmap_data, _pRgbaBuffer);
  else
    uart_printf("[ML]: error: Invalid frame buffer: %p\n", pMarv->bitmap_data);
}

static void renderMenu() {
  drawClear();
  drawUptime();
  drawOptions();

  render();
}

static void onDialRotate(uint8_t buttonId, int8_t direction) {
  
}

static void onMpuUnknown() {

}

static void onMpuButtonPress(uint8_t buttonId, int8_t param) {
  switch (buttonId) {
      case 0x05:
      case 0x0D:
      case 0x10:
      case 0x11:
        onButtonPress(buttonId, param > 0);
	break;  
    }
}

static void mpu_decode(const char* in, char* out, int max_len) {
  int len = 0;
  int size = (unsigned char) in[0];

  /* print each byte as hex */
  for (const char * c = in; c < in + size; c++)
    len += snprintf(out+len, max_len-len, "%02x ", *c);
  
  /* trim the last space */
  if (len)
    out[len-1] = 0;
}

extern const char* const mpu_recv_ring_buffer[105];
extern const int mpu_recv_ring_buffer_tail;

static void dispatchMpu() {
  static int _last_tail = 0;
  const max_mpu_recv = 80; // see RP 1.6.0: 0xe009b2ce
  const int diff = mpu_recv_ring_buffer_tail - _last_tail;
  const int numNewMpuMessages = diff >= 0 ? diff : diff + max_mpu_recv;

  for (int i = 0; i < numNewMpuMessages; ++i) {
    const uint8_t* pCurMsg = &mpu_recv_ring_buffer[_last_tail][4];
    const int msgId = pCurMsg[2];

    if (msgId == 0x03) {
      const int buttonId = pCurMsg[3];
      const int buttonParam = pCurMsg[4];

      onMpuButtonPress(buttonId, buttonParam);
    }

    _last_tail = (_last_tail + 1) % max_mpu_recv;

    uint8_t pMpuMsg[105];
    mpu_decode(pCurMsg, pMpuMsg, sizeof(pMpuMsg));
    // uart_printf("[ML] <%d> *** mpu_recv(%s)\n", _last_tail, pMpuMsg);
  }
}

static void addOption(int optionNo, const char* pName, OptionEnableCallback_t pOptionEnableCallback) {
  if (optionNo < 0 || optionNo > _numOptions) {
    uart_printf("[ML] Error: option %d cannot be added!\n", optionNo);
    return; 
  }

  _options[optionNo].pName = pName;
  _options[optionNo].enabled = false;
  _options[optionNo].pOptionEnableCallback = pOptionEnableCallback;

  uart_printf("Added option %d: %s with callback 0x%X\n", optionNo, pName , pOptionEnableCallback);
}

typedef enum WlanSettingsMode_t {
  ADHOC_WIFI=1,
  INFRA=2,
  ADHOC_G=3
} WlanSettingsMode_t;

typedef enum WlanSettingsAuthMode_t {
  OPEN=0,
  SHARED=1,
  WPA2PSK=5,
  BOTH=6
} WlanSettingsAuthMode_t;

typedef enum WlanSettingsCipherMode_t {
  NONE=0,
  WEP=1,
  AES=4
} WlanSettingsCipherMode_t;

typedef struct {
  int a;                       // set to 0
  int mode;
  int modeSomething;           // set to 0 on INFRA
  char pSSID[36];
  int channel;
  int authMode;
  int cipherMode;
  int f;                       // set to 0
  int g;                       // set to 6 when using INFRA or BOTH
  char pKey[63];
  unsigned char pUnknown[121]; // set to 0
} WlanSettings_t;

// Socket

int* (*errno_get_pointer_to)(void) = 0xe0288785;

void printError(const char* pErrorMsg) {
  uart_printf("Error [%d]; %s\n", *errno_get_pointer_to(), pErrorMsg);
}

#define DRY_AF_INET 1
#define DRY_SOCK_STREAM 1
#define DRY_INADDR_ANY 0 // TODO: verify!
#define DRY_SOL_SOCKET 0xFFFF
#define DRY_SO_REUSEADDR 0x8004 // TODO: verify!
#define DRY_SO_REUSEPORT 0x8005 // TODO: verify!

#define socklen_t unsigned int

typedef struct {
  unsigned int s_addr;
} DryInAddr;

typedef struct {
  short sin_family;
  unsigned short sin_port;
  DryInAddr sin_addr;
} DrySockaddr_in;

typedef struct {
  unsigned short sa_family; // address family
  char sa_data[8]; // protocol address
} DrySockaddr;

typedef struct {
  int lo;
  int hi;
} DryOpt_t;

uint16_t htons(uint16_t v) {
  return (v >> 8) | (v << 8);
}

DryInAddr (*ipStrToIp)(const char* pIpStr) = 0xe0103033;

int (*socket_create)(int domain, int type, int protocol) = 0xe013dc59;
int (*socket_setsockopt)(int sockfd, int level, int optname, void* optval, socklen_t optlen) = 0xe013de59;
int (*socket_bind)(int sockfd, DrySockaddr* addr, socklen_t addrlen) = 0xe013dc91;
int (*socket_listen)(int sockfd, int backlogl) = 0xe013dd05;
int (*socket_accept)(int sockfd, DrySockaddr* addr, socklen_t* addrlen) = 0xe013dd39;
int (*socket_connect)(int sockfd, DrySockaddr* addr, socklen_t addrlen) = 0xe013dccb;
int (*socket_recv)(int sockfd,void* buf, size_t len, int flags) = 0xe013dd73;
int (*socket_send)(int sockfd,void* buf, size_t len, int flags) = 0xe013ddf9;
void (*socket_close_caller)(int sockfd) = 0xe01347f5;

// ----

static int wifiConnect() {
  uart_printf("exec NwLimeOn\n");
  call("NwLimeOn");
  uart_printf("exec wlanchk\n");
  call("wlanchk");

  WlanSettings_t wlanSettings;
  memset(&wlanSettings, 0, sizeof(wlanSettings));

  wlanSettings.mode = INFRA;
  wlanSettings.g = 6; // set to 6 when using INFRA or BOTH
  strcpy(wlanSettings.pSSID, "speedport_2.4 Ghz");
  wlanSettings.channel = 6;
  wlanSettings.authMode = WPA2PSK;
  wlanSettings.cipherMode = AES;
  strcpy(wlanSettings.pKey, "<Add WiFi Password Here");

  uart_printf("now connecting to '%s' AP...\n", wlanSettings.pSSID);

  int (*wlanconnect)(WlanSettings_t* pWlanSettings) = 0xe040d5b5;
  int wlanResult = wlanconnect(&wlanSettings);

  uart_printf("wlan connect result: %d\n", wlanResult);

  if (wlanResult != 0)
    return wlanResult;

  uart_printf("set ip...\n");
  call("wlanipset", "192.168.1.100");

  return 0;
}

typedef struct {
  uint pBitlen[2];
  uint pH[4];
  uint8_t pBuffer[64];
} Md5Ctx;

void (*Md5_Init)(Md5Ctx* pCtx) = 0xe05a73b9;
void (*Md5_Update)(Md5Ctx* pCtx, void* pData, size_t size) = 0xe05a73db;
void (*Md5_Final)(Md5Ctx *pCtx, uint8_t* pMd5HashOut) = 0xe05a745f;

int (*Sha256Init)(void** ppSha256Ctx) = 0xe07062e9;
int (*ShaXUpdate)(void* pCtx, void* pTransformFunction, uint8_t* pData, size_t size) = 0xe07060b1;
void (*Sha256_Transform)(void* pData, uint32_t* pH) = 0xe073aac5;
int (*ShaXFinal)(void* pCtx, void* pTransFormFunction, uint8_t* pFinalHash) = 0xe0706047;
int (*ShaXFree)(void** pCtx) = 0xe0706269;

void (*_reboot)(uint32_t value) = 0xe0579b7f;

static void reboot() {
  _reboot(0);
}

typedef struct {
  uint32_t lo;
  uint32_t hi;
} size64_t;

typedef enum {
  ANNOUNCE_STATUS_OK = 0,
  ANNOUNCE_STATUS_UNSUPPORTED_VERSION = 1,
} AnnounceStatus_t;

typedef struct {
  char pFileName[64];
  uint64_t fileSize;
  uint8_t pSha256Hash[32];
  int protocolVersion;
} AnnounceFileReqMsg_t;

typedef struct {
  uint32_t status;
} AnnounceFileRspMsg_t;

static int createServer(int serverFd, int port) {
  DryOpt_t opt;
  opt.lo = 1;

  uart_printf("set socket option1\n");

  if (socket_setsockopt(serverFd, DRY_SOL_SOCKET, DRY_SO_REUSEADDR, &opt, sizeof(opt)) < 0 ) {
    printError("setsockopt");
    return 1;
  }

  uart_printf("set socket option2\n");

  if (socket_setsockopt(serverFd, DRY_SOL_SOCKET, DRY_SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
    printError("setsockopt");
    return 1;
  }

  DrySockaddr_in address;
  address.sin_family = htons(DRY_AF_INET);
  address.sin_addr.s_addr = DRY_INADDR_ANY;
  address.sin_port = htons(port);

  uart_printf("bind at port: %d\n", port);

  if (socket_bind(serverFd, &address, sizeof(address)) < 0) {
    printError("bind failed");
    return 1;
  }

  uart_printf("listen\n");

  if (socket_listen(serverFd, 1) < 0) {
    printError("listen");
    return 1;
  }

  uart_printf("accept\n");
  uart_printf("now waiting for connection on port %d...\n", port);

  unsigned int addrlen = sizeof(address);
  int c = socket_accept(serverFd, &address, &addrlen);

  if (c < 0) {
    printError("accept");
    return 1;
  }

  uart_printf("client connected.\n");

  return c;
}

static int recvRequest(int clientFd, AnnounceFileReqMsg_t* pReq) {
  // TODO: check for TCP error:
  socket_recv(clientFd, pReq, sizeof(AnnounceFileReqMsg_t), 0);

  if (pReq->protocolVersion != 1)
    return ANNOUNCE_STATUS_UNSUPPORTED_VERSION;

  pReq->pFileName[sizeof(pReq->pFileName) -1] = 0;

  uart_printf("File announce message received: \n");
  uart_printf("File Name: %s\n", pReq->pFileName);
  uart_printf("File size: %lld\n", pReq->fileSize);
  uart_printf("SHA-256 Hash: ");

  for (int i = 0; i < sizeof(pReq->pSha256Hash); ++i)
    uart_printf("%02X", pReq->pSha256Hash[i]);

  uart_printf("\n");
  uart_printf("Protocol Version: %d\n", pReq->protocolVersion);

  return ANNOUNCE_STATUS_OK;
}

static int performUpdate(int clientFd) {
  AnnounceFileReqMsg_t req;
  AnnounceFileRspMsg_t rsp;
  rsp.status = recvRequest(clientFd, &req);
  int sent = socket_send(clientFd, &rsp, sizeof(rsp), 0);

  uart_printf("sent %d bytes\n", sent);

  if (rsp.status == ANNOUNCE_STATUS_UNSUPPORTED_VERSION) {
    uart_printf("Client protocol version is unsupported! Aborting.\n");
    return 1;
  }

  uart_printf("Now receiving file...\n");

  const char* pTempFile = "B:/FILE.TMP";

  _FIO_RemoveFile(pTempFile);

  FILE* pFile = _FIO_CreateFile(pTempFile);

  if (pFile == (FILE*)-1) {
    printError("Unable to create temporary file!\n");
    return 1;
  }

  const size_t recvBufferSize = 1024;
  uint8_t* pBuffer = _alloc_dma_memory(recvBufferSize);

  uint64_t bytesReceived;

  for (bytesReceived = 0; bytesReceived < req.fileSize;) {
    int chunkSize = socket_recv(clientFd, pBuffer, recvBufferSize, 0);

    uart_printf("received: %d\n", chunkSize);

    if (chunkSize <= 0) {
      printError("transmission failed");
      break;
    }

    uart_printf("pFile is: 0x%X. now writing\n", pFile);

    int bytesWritten;

    if ((bytesWritten = _FIO_WriteFile(pFile, pBuffer, chunkSize)) != chunkSize) {
      uart_printf("write to file failed. %d bytes written but expected to write %d!\n", bytesWritten, chunkSize);
      break;
    }

    uart_printf("written: %d\n", bytesWritten);

    bytesReceived += chunkSize;
  }

  _free_dma_memory(pBuffer);
  pBuffer = 0;

  FIO_CloseFile(pFile);

  uart_printf("Bytes received: %d\n", bytesReceived);

  if (bytesReceived != req.fileSize) {
     uart_printf("File transmission failed!\n");
     return 1;
  }

  uart_printf("File transmission finished!\n");

  int dummy = 42;
  socket_send(clientFd, &dummy, 1, 0);

  uart_printf("Now checking SHA-256\n");

  size64_t fileSize64 = {0};

  if (_FIO_GetFileSize(pTempFile, &fileSize64) == -1) {
    uart_printf("failed to get file size! aborting\n");
    return 1;
  }

  uint32_t fileSize = fileSize64.lo; // TODO: will break on files bigger than 4GB! Fix!

  void* pSha256Ctx = 0;
  Sha256Init(&pSha256Ctx);

  uart_printf("file size of reopened file is: %d (lo: %d, hi: %d)\n", fileSize, fileSize64.lo, fileSize64.hi);

  pBuffer = _alloc_dma_memory(recvBufferSize);

  if (!pBuffer) {
    uart_printf("failed to create SHA-256 working buffer!\n");
    return 0;
  }

  uart_printf("Start SHA-256 calc\n");

  pFile = _FIO_OpenFile(pTempFile, O_RDONLY);

  if (pFile == (FILE*)-1) {
    printError("Unable to reopen temporary file!\n");
    return 1;
  }

  uart_printf("reopened pFile=%X\n", pFile);

  int bytesRead;
  for (bytesRead = 0; bytesRead < fileSize;) {
    int chunkSize = _FIO_ReadFile(pFile, pBuffer, recvBufferSize);

    if (chunkSize < 0) {
      uart_printf("error on reading file during SHA-256 check!\n");
      return 1;
    }

    uart_printf("read: %d\n", chunkSize);
    ShaXUpdate(pSha256Ctx, Sha256_Transform, pBuffer, chunkSize);

    bytesRead += chunkSize;
  }

  _free_dma_memory(pBuffer);
  FIO_CloseFile(pFile);

  uint8_t pSha256Hash[32];
  ShaXFinal(pSha256Ctx, Sha256_Transform, pSha256Hash);
  ShaXFree(&pSha256Ctx);

  uart_printf("SHA-256 calc finish\n");
  uart_printf("calculated: ");

  for (int i = 0; i < sizeof(pSha256Hash); ++i)
    uart_printf("%02X", pSha256Hash[i]);

  uart_printf("\n");

  uart_printf("expected: ");

  for (int i = 0; i < sizeof(pSha256Hash); ++i)
    uart_printf("%02X", req.pSha256Hash[i]);

  uart_printf("\n");

  for (int i = 0; i < sizeof(pSha256Hash); ++i) {
    if (pSha256Hash[i] != req.pSha256Hash[i]) {
      uart_printf("Error: SHA-256 checksum mismatch!\n");
      return -1;
    }
  }

  uart_printf("checksum ok\n");

  char pTargetFileName[128];
  snprintf(pTargetFileName, sizeof(pTargetFileName), "B:/%s", req.pFileName);

  _FIO_RemoveFile(pTargetFileName);

  int error;

  if ((error = _FIO_RenameFile(pTempFile, pTargetFileName)) != 0) {
    uart_printf("[%d] Error on rename '%s' -> '%s'!\n", error, pTempFile, pTargetFileName);
    return 1;
  }

  return 0;
}

int drysh_ml_update(int argc, char const *argv[]) {
  int error;

  error = wifiConnect();

  if (error) {
    printError("connection to wifi failed!\n");
    return 1;
  }

  uart_printf("creating socket\n");

  int serverFd;

  if ((serverFd = socket_create(DRY_AF_INET, DRY_SOCK_STREAM, 0)) < 0) {
    printError("socket failed");
    return 1;
  }

  int clientFd = createServer(serverFd, 2342);

  if (clientFd < 0) {
    printError("Failed to create server");
    return 1;
  }

  if ((error = performUpdate(clientFd)) != 0) {
    printError("Update failed!");
    return 1;
  }

  uart_printf("Update successful!\n");
  uart_printf("Rebooting...\n");

  reboot();

  return 0;
}

int (*drysh_add_category)(char* pCategoryName) = 0xe06687b5;
int (*drysh_add_command)(int categoryId, void* pNullUnknown, char* pName, void* pFunction, char* pUsageInfo) = 0xe06688d5;

static void printConvertedProp(uint32_t propertyId) {
  uint16_t (*PTPS_ReplacePropIDWithPtpID)(int propertyId) = 0xe067d5a3;

  const uint16_t ptpId = PTPS_ReplacePropIDWithPtpID(propertyId);
  uart_printf("ptpId=0x%04X\n", ptpId);
}

static void DUMP_ASM microml_task() {
  uart_printf("[ML] Hello from %s!\n", get_current_task_name());

  int hCategory = drysh_add_category("Micro ML");
  drysh_add_command(hCategory, 0, "ml_update", drysh_ml_update, "Performs an update of autoexec.bin over wifi");

  // msleep(5000);

  // call("dmprint", -1, 0);

  /*
  int error;
  if (error = wifiConnect()) {
    while (1)
      ;
  }

  xmitDiscord();
  */

  _pRgbaBuffer = malloc(_rgbaSize);

  if (!_pRgbaBuffer) {
    uart_printf("[ML] Error: Unable to allocate frame buffer!\n");

    led_blink(1, 50, 50);
  }

  uart_printf("[ML] Frame buffer allocated at %p\n", _pRgbaBuffer);

  const max_mpu_send = 50; // see RP 1.6.0: 0xe009b2dc

  // add options
  addOption(0, "Electronic Shutter Mode", enableElectronicShutterMode);

  uart_printf("[ML] Number of options: %d", _numOptions);

  while(true) {
    dispatchMpu();

    if (_displayMenu)
      renderMenu();

    led_blink(2, 50, 50);

    msleep(500);
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

