/* drv_wasapi.c - Windows WASAPI output driver for MikMod
 *
 * Ported from C++ to C89.  Requires linking against ole32.lib and
 * mmdevapi is loaded at run-time through COM so no extra import lib is needed.
 *
 * Build guard: define DRV_WASAPI in your build system (or config.h).
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_WASAPI

#define COBJMACROS          /* get C-style COM macros: IFoo_Method(p, ...) */

#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if defined(_MSC_VER)
#define MIK_UINT64_C(c) c ## ui64
#elif defined(__LP64__) || defined(_LP64)
#define MIK_UINT64_C(c) c ## UL
#else
#define MIK_UINT64_C(c) c ## ULL
#endif

/* -------------------------------------------------------------------------
 * Namespace the GUID constants so that we don't bother with INITGUID, etc.
 * ---------------------------------------------------------------------- */

static const PROPERTYKEY MIKMOD_PKEY_Device_FriendlyName = {
{ 0xa45c254e, 0xdf1c, 0x4efd, { 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, } }, 14 };

static const GUID MIKMOD_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT =
{ 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };

static const CLSID MIKMOD_CLSID_MMDeviceEnumerator =
{ 0xbcde0395, 0xe52f, 0x467c, { 0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e } };

static const IID MIKMOD_IID_IMMDeviceEnumerator =
{ 0xa95664d2, 0x9614, 0x4f35, { 0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6 } };

static const IID MIKMOD_IID_IAudioClient =
{ 0x1cb9ad4c, 0xdbfa, 0x4c32, { 0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2 } };

static const IID MIKMOD_IID_IAudioRenderClient =
{ 0xf294acfc, 0x3146, 0x4483, { 0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2 } };

#ifdef __IAudioClient3_INTERFACE_DEFINED__
static const IID MIKMOD_IID_IAudioClient3 =
{ 0x7ed4ee07, 0x8e67, 0x4cd4, { 0x8c, 0x1a, 0x2b, 0x7a, 0x59, 0x87, 0xad, 0x42 } };
#endif

#ifndef PF_XMMI64_INSTRUCTIONS_AVAILABLE
#define PF_XMMI64_INSTRUCTIONS_AVAILABLE 10
#endif

/* =========================================================================
 * Internal constants
 * ====================================================================== */
#define WASAPI_MAX_ENDPOINTS  32
#define WASAPI_MAX_NAME       256
#define WASAPI_BUF_DURATION   (10 * 10000)   /* 10 ms in 100-ns units */
#define WASAPI_WAIT_MS        2000

 /* =========================================================================
  * Simple helpers
  * ====================================================================== */

  /* FNV-1a 64-bit hash – stable device id from the wide-char device-id string */
static uint64_t fnv1a64_w(const wchar_t* s)
{
    const uint64_t FNV_OFFSET = MIK_UINT64_C(14695981039346656037);
    const uint64_t FNV_PRIME = MIK_UINT64_C(1099511628211);
    uint64_t h = FNV_OFFSET;
    if (!s) return 0;
    while (*s) {
        h ^= (uint64_t)(*s++);
        h *= FNV_PRIME;
    }
    return h;
}

/* Truncating wide?UTF-8 conversion */
static void wide_to_utf8(const wchar_t* ws, char* out, int outSize)
{
    int n;
    if (!out || outSize <= 0) return;
    out[0] = '\0';
    if (!ws) return;
    n = WideCharToMultiByte(CP_UTF8, 0, ws, -1, out, outSize, NULL, NULL);
    if (n <= 0) out[0] = '\0';
    out[outSize - 1] = '\0';
}

static void safe_close_handle(HANDLE* h)
{
    if (*h) { CloseHandle(*h); *h = NULL; }
}

static __inline BOOL is_equal_guid (const GUID *guid1, const GUID *guid2)
{
  return memcmp(guid1, guid2, sizeof(GUID)) == 0;
}

/* Is the mix format 32-bit IEEE float? */
static BOOL is_float32_mix_format(const WAVEFORMATEX* wf)
{
    if (!wf) return FALSE;
    if (wf->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) return TRUE;
    if (wf->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        const WAVEFORMATEXTENSIBLE* wfe = (const WAVEFORMATEXTENSIBLE*)wf;
        return is_equal_guid(&wfe->SubFormat, &MIKMOD_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
    }
    return FALSE;
}

static uint32_t bytes_per_frame(const WAVEFORMATEX* wf)
{
    return wf ? (uint32_t)wf->nBlockAlign : 0;
}

/* =========================================================================
 * AudioDeviceEndpoint  (plain-C equivalent of sfx::AudioDeviceEndpoint)
 * ====================================================================== */
typedef struct {
    uint64_t id;
    char     name[WASAPI_MAX_NAME];
    uint32_t nominalSampleRate;
    uint32_t maxOutputChannels;
    uint32_t nominalGranularity;
    BOOL     isOutput;
    BOOL     isInput;
} AudioDeviceEndpoint;

/* =========================================================================
 * Render callback type
 *   user       – opaque pointer passed through from open()
 *   dst        – destination buffer (WASAPI render buffer)
 *   frames     – number of frames to fill
 *   returns TRUE if audio was written, FALSE to output silence
 * ====================================================================== */
typedef BOOL(*RenderCallback)(void* user, BYTE* dst, uint32_t frames);

/* =========================================================================
 * AudioDevice 
 * ====================================================================== */
typedef struct {
    /* COM interfaces – held as raw pointers, released explicitly */
    IMMDeviceEnumerator* enumerator;
    IMMDevice* device;
    IAudioClient* audioClient;
#ifdef __IAudioClient3_INTERFACE_DEFINED__
    IAudioClient3* audioClient3;   /* Win10+, may be NULL */
#endif
    IAudioRenderClient* renderClient;

    /* Event for event-driven rendering */
    HANDLE               event;

    /* Render thread */
    HANDLE               thread;
    volatile LONG        running;        /* 0 = stopped, 1 = running */

    /* Mix format (CoTaskMem allocated) */
    WAVEFORMATEX* mixFormat;

    /* Buffer geometry */
    UINT32               bufferFrames;

    /* Request parameters */
    uint32_t             reqSampleRate;
    uint32_t             reqChannels;
    uint32_t             reqFramesPerCallback;
    BOOL                 reqFloat;       /* TRUE = float32, FALSE = int16 */

    /* Gain (atomic-ish via InterlockedExchange on 32-bit float bits) */
    volatile LONG        gainBits;       /* float bits stored as LONG */

    /* Temporary int16 buffer for PCM?float conversion */
    int16_t* tmpS16;
    size_t               tmpS16Cap;      /* capacity in int16 elements */

    /* Render callback */
    RenderCallback       renderCb;
    void* user;

    /* Options */
    BOOL                 useDirect;      /* TRUE = try IAudioClient3 low-latency */

    /* Endpoint list */
    AudioDeviceEndpoint  endPoints[WASAPI_MAX_ENDPOINTS];
    int                  endPointCount;
} AudioDevice;

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */
static DWORD WINAPI render_thread_main(LPVOID param);
static void         audio_device_close(AudioDevice* dev);

/* =========================================================================
 * Endpoint enumeration
 * ====================================================================== */
static BOOL audio_device_enumerate(AudioDevice* dev)
{
    HRESULT hr;
    BOOL comHere;
    IMMDeviceCollection* collection = NULL;
    UINT count, i;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    comHere = SUCCEEDED(hr) || hr == S_FALSE;
    if (FAILED(hr) && hr != (HRESULT)RPC_E_CHANGED_MODE) {
        _mm_errno = MMERR_OPENING_AUDIO;
        return FALSE;
    }

    if (!dev->enumerator) {
        hr = CoCreateInstance(
            &MIKMOD_CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
            &MIKMOD_IID_IMMDeviceEnumerator, (void**)&dev->enumerator);
        if (FAILED(hr) || !dev->enumerator) {
            if (comHere) CoUninitialize();
            return FALSE;
        }
    }

    dev->endPointCount = 0;

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(
        dev->enumerator, eRender, DEVICE_STATE_ACTIVE, &collection);
    if (FAILED(hr) || !collection) {
        if (comHere) CoUninitialize();
        return FALSE;
    }

    hr = IMMDeviceCollection_GetCount(collection, &count);
    if (FAILED(hr)) {
        IMMDeviceCollection_Release(collection);
        if (comHere) CoUninitialize();
        return FALSE;
    }

    for (i = 0; i < count && dev->endPointCount < WASAPI_MAX_ENDPOINTS; ++i) {
        IMMDevice* d = NULL;
        IPropertyStore* props = NULL;
        IAudioClient* client = NULL;
        LPWSTR          idW = NULL;
        AudioDeviceEndpoint ep;

        hr = IMMDeviceCollection_Item(collection, i, &d);
        if (FAILED(hr) || !d) continue;

        memset(&ep, 0, sizeof(ep));
        ep.isOutput = TRUE;
        ep.nominalGranularity = 4096;

        /* Stable id */
        if (SUCCEEDED(IMMDevice_GetId(d, &idW)) && idW) {
            ep.id = fnv1a64_w(idW);
            CoTaskMemFree(idW);
        }

        /* Friendly name */
        if (SUCCEEDED(IMMDevice_OpenPropertyStore(d, STGM_READ, &props)) && props) {
            PROPVARIANT v;
            PropVariantInit(&v);
            if (SUCCEEDED(IPropertyStore_GetValue(props, &MIKMOD_PKEY_Device_FriendlyName, &v))
                && v.vt == VT_LPWSTR && v.pwszVal)
            {
                wide_to_utf8(v.pwszVal, ep.name, (int)sizeof(ep.name));
            }
            PropVariantClear(&v);
            IPropertyStore_Release(props);
        }

        /* Mix format – channels + sample rate */
        hr = IMMDevice_Activate(d, &MIKMOD_IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&client);
        if (SUCCEEDED(hr) && client) {
            WAVEFORMATEX* mix = NULL;
            if (SUCCEEDED(IAudioClient_GetMixFormat(client, &mix)) && mix) {
                ep.nominalSampleRate = mix->nSamplesPerSec;
                ep.maxOutputChannels = mix->nChannels;
                CoTaskMemFree(mix);
            }
            IAudioClient_Release(client);
        }
        if (ep.nominalSampleRate == 0) ep.nominalSampleRate = 44100;
        if (ep.maxOutputChannels == 0) ep.maxOutputChannels = 2;

        dev->endPoints[dev->endPointCount++] = ep;
        IMMDevice_Release(d);
    }

    IMMDeviceCollection_Release(collection);
    return TRUE;
}

/* =========================================================================
 * Constructor / destructor helpers
 * ====================================================================== */
static AudioDevice* audio_device_create(void)
{
    AudioDevice* dev = (AudioDevice*)calloc(1, sizeof(AudioDevice));
    if (!dev) return NULL;
    /* gainBits = 1.0f */
    {
        float one = 1.0f;
        LONG bits;
        memcpy(&bits, &one, sizeof(bits));
        dev->gainBits = bits;
    }
    audio_device_enumerate(dev);
    return dev;
}

static void audio_device_destroy(AudioDevice* dev)
{
    if (!dev) return;
    audio_device_close(dev);
    if (dev->enumerator) {
        IMMDeviceEnumerator_Release(dev->enumerator);
        dev->enumerator = NULL;
    }
    free(dev->tmpS16);
    free(dev);
}

/* =========================================================================
 * open()
 * ====================================================================== */
static BOOL audio_device_open(
    AudioDevice* dev,
    int             deviceIndex,
    uint32_t        sampleRate,
    uint32_t        channels,
    BOOL            wantFloat,
    uint32_t        framesPerCallback,
    RenderCallback  renderCb,
    void* user)
{
    HRESULT hr;
    DWORD   streamFlags;
    BOOL    initialized = FALSE;

    (void)deviceIndex; /* reserved – currently always uses default endpoint */

    if (!dev->enumerator) return FALSE;

    audio_device_close(dev);

    dev->reqSampleRate = sampleRate;
    dev->reqChannels = channels ? channels : 2;
    dev->reqFloat = wantFloat;
    dev->reqFramesPerCallback = framesPerCallback;
    dev->renderCb = renderCb;
    dev->user = user;

    /* ---- Get default render endpoint ---------------------------------- */
    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(
        dev->enumerator, eRender, eConsole, &dev->device);
    if (FAILED(hr)) return FALSE;

    /* ---- Activate IAudioClient ---------------------------------------- */
    hr = IMMDevice_Activate(
        dev->device, &MIKMOD_IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&dev->audioClient);
    if (FAILED(hr)) return FALSE;

#ifdef __IAudioClient3_INTERFACE_DEFINED__
    /* Try to also get IAudioClient3 (Win10+) */
    (void)IUnknown_QueryInterface(
        (IUnknown*)dev->audioClient, &MIKMOD_IID_IAudioClient3, (void**)&dev->audioClient3);
#endif

    /* ---- Mix format --------------------------------------------------- */
    hr = IAudioClient_GetMixFormat(dev->audioClient, &dev->mixFormat);
    if (FAILED(hr) || !dev->mixFormat) return FALSE;

    /* ---- Event handle ------------------------------------------------- */
    dev->event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!dev->event) return FALSE;

    streamFlags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;

#ifdef __IAudioClient3_INTERFACE_DEFINED__
    /* ---- Optional low-latency shared (IAudioClient3) ------------------ */
    if (dev->useDirect && dev->audioClient3) {
        UINT32 defPeriod = 0, fundamental = 0, minPeriod = 0, maxPeriod = 0;
        hr = IAudioClient3_GetSharedModeEnginePeriod(
            dev->audioClient3, dev->mixFormat,
            &defPeriod, &fundamental, &minPeriod, &maxPeriod);
        if (SUCCEEDED(hr)) {
            UINT32 desired = minPeriod;
            if (framesPerCallback != 0) {
                desired = framesPerCallback;
                if (desired < minPeriod) desired = minPeriod;
                if (desired > maxPeriod) desired = maxPeriod;
            }
            hr = IAudioClient3_InitializeSharedAudioStream(
                dev->audioClient3, streamFlags, desired, dev->mixFormat, NULL);
            if (SUCCEEDED(hr))
                initialized = TRUE;
            /* on failure fall through to classic Initialize */
        }
    }
#endif

    /* ---- Classic shared-mode Initialize ------------------------------- */
    if (!initialized) {
        hr = IAudioClient_Initialize(
            dev->audioClient,
            AUDCLNT_SHAREMODE_SHARED,
            streamFlags,
            WASAPI_BUF_DURATION,
            0,                  /* periodicity must be 0 in shared mode */
            dev->mixFormat,
            NULL);
        if (FAILED(hr)) return FALSE;
    }

    /* ---- Bind event --------------------------------------------------- */
    hr = IAudioClient_SetEventHandle(dev->audioClient, dev->event);
    if (FAILED(hr)) return FALSE;

    /* ---- Buffer size -------------------------------------------------- */
    hr = IAudioClient_GetBufferSize(dev->audioClient, &dev->bufferFrames);
    if (FAILED(hr)) return FALSE;

    /* ---- Render client ------------------------------------------------ */
    hr = IAudioClient_GetService(
        dev->audioClient, &MIKMOD_IID_IAudioRenderClient, (void**)&dev->renderClient);
    if (FAILED(hr)) return FALSE;

    return TRUE;
}

/* =========================================================================
 * close()
 * ====================================================================== */
static void audio_device_close(AudioDevice* dev)
{
    /* stop() first (joins thread, stops client) */
    if (dev->running) {
        InterlockedExchange(&dev->running, 0);
        if (dev->event) SetEvent(dev->event);   /* wake render thread */
        if (dev->thread) {
            WaitForSingleObject(dev->thread, INFINITE);
            CloseHandle(dev->thread);
            dev->thread = NULL;
        }
        if (dev->audioClient)
            IAudioClient_Stop(dev->audioClient);
    }

    if (dev->mixFormat) {
        CoTaskMemFree(dev->mixFormat);
        dev->mixFormat = NULL;
    }

    safe_close_handle(&dev->event);

    if (dev->renderClient) { IAudioRenderClient_Release(dev->renderClient); dev->renderClient = NULL; }
#ifdef __IAudioClient3_INTERFACE_DEFINED__
    if (dev->audioClient3) { IAudioClient3_Release(dev->audioClient3);      dev->audioClient3 = NULL; }
#endif
    if (dev->audioClient) { IAudioClient_Release(dev->audioClient);        dev->audioClient = NULL; }
    if (dev->device) { IMMDevice_Release(dev->device);                dev->device = NULL; }

    dev->user = NULL;
    dev->renderCb = NULL;
    dev->bufferFrames = 0;
}

/* =========================================================================
 * start() – prime buffer, launch thread
 * ====================================================================== */
static BOOL audio_device_render(AudioDevice* dev, BYTE* pData, UINT32 framesAvail);
static BOOL audio_device_start(AudioDevice* dev)
{
    HRESULT hr;
    UINT32  padding, framesAvail;
    BYTE* pData;

    if (!dev->audioClient || !dev->renderClient || !dev->event) return FALSE;
    if (dev->running) return FALSE;

    /* Prime: fill one buffer to avoid an immediate underrun */
    hr = IAudioClient_GetCurrentPadding(dev->audioClient, &padding);
    if (FAILED(hr)) return FALSE;

    framesAvail = dev->bufferFrames - padding;
    if (framesAvail > 0) {
        hr = IAudioRenderClient_GetBuffer(dev->renderClient, framesAvail, &pData);
        if (FAILED(hr)) return FALSE;
        if (pData) {
            BOOL produced = audio_device_render(dev, pData, framesAvail);
            if (!produced)
                memset(pData, 0, (size_t)framesAvail * bytes_per_frame(dev->mixFormat));
        }
        IAudioRenderClient_ReleaseBuffer(dev->renderClient, framesAvail, 0);
    }

    hr = IAudioClient_Start(dev->audioClient);
    if (FAILED(hr)) return FALSE;

    InterlockedExchange(&dev->running, 1);
    dev->thread = CreateThread(NULL, 0, render_thread_main, dev, 0, NULL);
    if (!dev->thread) {
        InterlockedExchange(&dev->running, 0);
        IAudioClient_Stop(dev->audioClient);
        return FALSE;
    }
    return TRUE;
}

/* =========================================================================
 * stop()
 * ====================================================================== */
static void audio_device_stop(AudioDevice* dev)
{
    if (!InterlockedCompareExchange(&dev->running, 0, 1))
        return; /* was already 0 */

    if (dev->event) SetEvent(dev->event);
    if (dev->thread) {
        WaitForSingleObject(dev->thread, INFINITE);
        CloseHandle(dev->thread);
        dev->thread = NULL;
    }
    if (dev->audioClient)
        IAudioClient_Stop(dev->audioClient);
}

#if 0 /* not used yet. */
static void audio_device_suspend(AudioDevice* dev)
{
    if (dev->audioClient) IAudioClient_Stop(dev->audioClient);
}

static void audio_device_resume(AudioDevice* dev)
{
    if (dev->audioClient) IAudioClient_Start(dev->audioClient);
}
#endif

/* =========================================================================
 * Render thread
 * ====================================================================== */

 /* Convert signed 16-bit samples to 32-bit float in-place (out can == in if
  * the output buffer is large enough).  Separate src/dst here for clarity. */
static void convert_i16_to_f32(float* dst, const int16_t* src, size_t count)
{
    size_t n;
    for (n = 0; n < count; ++n)
        dst[n] = (float)src[n] * (1.0f / 32768.0f);
}

static BOOL audio_device_render(AudioDevice* dev, BYTE* pData, UINT32 framesAvail)
{
    BOOL mixFloat;
    uint32_t ch;

    if (!dev || !pData || !framesAvail)
        return FALSE;

    ch = dev->mixFormat ? dev->mixFormat->nChannels : dev->reqChannels;
    mixFloat = is_float32_mix_format(dev->mixFormat);

    if (ch != dev->reqChannels || (ch != 1u && ch != 2u))
        return FALSE;

    if (!dev->reqFloat && mixFloat) {
        size_t need = (size_t)framesAvail * ch;
        if (need > dev->tmpS16Cap) {
            free(dev->tmpS16);
            dev->tmpS16 = (int16_t*)malloc(need * sizeof(int16_t));
            dev->tmpS16Cap = dev->tmpS16 ? need : 0;
        }
        if (dev->tmpS16 && dev->renderCb) {
            BOOL produced = dev->renderCb(dev->user, (BYTE*)dev->tmpS16, framesAvail);
            if (produced)
                convert_i16_to_f32((float*)pData, dev->tmpS16, need);
            return produced;
        }
    }
    else if (!dev->reqFloat && !mixFloat) {
        if (dev->renderCb)
            return dev->renderCb(dev->user, pData, framesAvail);
    }

    return FALSE;
}

static DWORD WINAPI render_thread_main(LPVOID param)
{
    AudioDevice* dev = (AudioDevice*)param;

    while (InterlockedCompareExchange(&dev->running, 1, 1)) {
        HRESULT hr;
        UINT32  padding, framesAvail;
        BYTE* pData;
        BOOL    produced;

        if (WaitForSingleObject(dev->event, WASAPI_WAIT_MS) != WAIT_OBJECT_0)
            continue;
        if (!InterlockedCompareExchange(&dev->running, 1, 1))
            break;

        hr = IAudioClient_GetCurrentPadding(dev->audioClient, &padding);
        if (FAILED(hr)) continue;

        framesAvail = dev->bufferFrames - padding;
        if (framesAvail == 0) continue;

        hr = IAudioRenderClient_GetBuffer(dev->renderClient, framesAvail, &pData);
        if (FAILED(hr) || !pData) continue;

        produced = audio_device_render(dev, pData, framesAvail);

        if (!produced)
            memset(pData, 0, (size_t)framesAvail * bytes_per_frame(dev->mixFormat));

        IAudioRenderClient_ReleaseBuffer(dev->renderClient, framesAvail, 0);
    }
    return 0;
}

/* =========================================================================
 * MikMod driver glue
 * ====================================================================== */

static AudioDevice* g_wasapi = NULL;
static ULONG        g_frames_per_cb = 0;
static ULONG        g_req_buffer_ms = 10;
static BOOL         g_started = FALSE;

/* MikMod callback: fill the WASAPI render buffer */
static BOOL wasapi_render_cb(void* user, BYTE* dst, uint32_t framesAvail)
{
    ULONG bytes;
    (void)user;

    /* MikMod side for WASAPI currently always renders int16 PCM. */
    bytes = framesAvail
        * 2u
        * ((md_mode & DMODE_STEREO) ? 2u : 1u);

    if (Player_Paused_internal()) {
        VC_SilenceBytes((SBYTE*)dst, bytes);
        return TRUE;
    }
    return VC_WriteBytes((SBYTE*)dst, bytes) == (int)bytes;
}

static void WASAPI_CommandLine(const CHAR* cmdline)
{
    CHAR* ptr = MD_GetAtom("buffer", cmdline, 0);
    if (ptr) {
        int ms = atoi(ptr);
        if (ms < 2) ms = 2;
        if (ms > 100) ms = 100;
        g_req_buffer_ms = (ULONG)ms;
        MikMod_free(ptr);
    }
}

static BOOL WASAPI_IsPresent(void)
{
    AudioDevice* dev = audio_device_create();
    BOOL present = dev && dev->endPointCount > 0;
    audio_device_destroy(dev);
    return present ? 1 : 0;
}

static int WASAPI_Init(void)
{
    uint32_t channels, framesPerCb;
    BOOL     wantFloat;

    g_wasapi = audio_device_create();
    if (!g_wasapi) {
        _mm_errno = MMERR_OPENING_AUDIO;
        return 1;
    }

    channels = (md_mode & DMODE_STEREO) ? 2u : 1u;

    /* For now, keep MikMod output in int16 and convert to float if WASAPI mix requires it. */
    md_mode &= ~DMODE_FLOAT;
    md_mode |= DMODE_16BITS;

    wantFloat = FALSE;
    framesPerCb = (uint32_t)((md_mixfreq * g_req_buffer_ms) / 1000u);
    if (!framesPerCb) framesPerCb = 256;

    if (!audio_device_open(
        g_wasapi,
        0,
        (uint32_t)md_mixfreq,
        channels,
        wantFloat,
        framesPerCb,
        wasapi_render_cb,
        NULL))
    {
        audio_device_destroy(g_wasapi);
        g_wasapi = NULL;
        _mm_errno = MMERR_OPENING_AUDIO;
        return 1;
    }

    /* WASAPI shared mode runs at the endpoint mix rate. Since this driver
       currently does no resampling, MikMod must mix at that same rate. */
    if (g_wasapi->mixFormat) {
        md_mixfreq = g_wasapi->mixFormat->nSamplesPerSec;
    }

    md_mode |= DMODE_SOFT_MUSIC | DMODE_SOFT_SNDFX;

#if defined(MIKMOD_SIMD)
    if (IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE))
        md_mode |= DMODE_SIMDMIXER;
#endif

    if (VC_Init()) return 1;

    g_frames_per_cb = framesPerCb;
    g_started = FALSE;
    return 0;
}

static void WASAPI_Exit(void)
{
    VC_Exit();
    if (g_wasapi) {
        audio_device_destroy(g_wasapi);
        g_wasapi = NULL;
    }
    g_started = FALSE;
}

static int WASAPI_PlayStart(void)
{
    if (VC_PlayStart()) return 1;

    if (g_wasapi && !audio_device_start(g_wasapi)) {
        VC_PlayStop();
        _mm_errno = MMERR_OPENING_AUDIO;
        return 1;
    }

    g_started = TRUE;
    return 0;
}

static void WASAPI_PlayStop(void)
{
    if (g_wasapi && g_started)
        audio_device_stop(g_wasapi);
    g_started = FALSE;
    VC_PlayStop();
}

/* Event-driven backend: no polling needed */
static void WASAPI_Update(void) {}

MIKMODAPI MDRIVER drv_wasapi = {
    NULL,
    "WASAPI",
    "Windows WASAPI driver v0.1",
    0, 255,
    "wasapi",
    "buffer:r:2,100,10:Audio buffer size in milliseconds\n",
    WASAPI_CommandLine,
    WASAPI_IsPresent,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    WASAPI_Init,
    WASAPI_Exit,
    NULL,
    VC_SetNumVoices,
    WASAPI_PlayStart,
    WASAPI_PlayStop,
    WASAPI_Update,
    NULL,
    VC_VoiceSetVolume,
    VC_VoiceGetVolume,
    VC_VoiceSetFrequency,
    VC_VoiceGetFrequency,
    VC_VoiceSetPanning,
    VC_VoiceGetPanning,
    VC_VoicePlay,
    VC_VoiceStop,
    VC_VoiceStopped,
    VC_VoiceGetPosition,
    VC_VoiceRealVolume
};

#else

MISSING(drv_wasapi);

#endif /* DRV_WASAPI */
