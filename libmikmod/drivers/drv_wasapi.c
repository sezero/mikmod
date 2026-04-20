/* drv_wasapi.c - Windows WASAPI output driver for MikMod
 *
 * Ported from C++ to C99/C11.  Requires linking against ole32.lib and
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
#include <functiondiscoverykeys_devpkey.h>  /* PKEY_Device_FriendlyName */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define INITGUID

#include <initguid.h>
#include <ksmedia.h>

static const GUID KSDATAFORMAT_SUBTYPE_IEEE_FLOAT =
{ 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };

DEFINE_GUID(CLSID_MMDeviceEnumerator,
    0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);

DEFINE_GUID(IID_IMMDeviceEnumerator,
    0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);

DEFINE_GUID(IID_IAudioClient,
    0x1CB9AD4C, 0xDBFA, 0x4C32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2);

DEFINE_GUID(IID_IAudioRenderClient,
    0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2);

DEFINE_GUID(IID_IAudioClient3,
    0x7ED4EE07, 0x8E67, 0x4CD4, 0x8C, 0x1A, 0x2B, 0x7A, 0x59, 0x87, 0xAD, 0x42);

 /* -------------------------------------------------------------------------
  * Forward-declare the GUID constants that <audioclient.h> only gives us when
  * INITGUID is defined before including the header *once*.  We use initguid.h
  * above which handles that, but keep explicit references here for clarity.
  * ---------------------------------------------------------------------- */
  /* KSDATAFORMAT_SUBTYPE_IEEE_FLOAT  – defined by initguid.h + ksmedia.h */
  /* KSDATAFORMAT_SUBTYPE_PCM         – same                               */

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
    const uint64_t FNV_OFFSET = 14695981039346656037ull;
    const uint64_t FNV_PRIME = 1099511628211ull;
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
    if (!out || outSize <= 0) return;
    out[0] = '\0';
    if (!ws) return;
    int n = WideCharToMultiByte(CP_UTF8, 0, ws, -1, out, outSize, NULL, NULL);
    if (n <= 0) out[0] = '\0';
    out[outSize - 1] = '\0';
}

static void safe_close_handle(HANDLE* h)
{
    if (*h) { CloseHandle(*h); *h = NULL; }
}

/* Is the mix format 32-bit IEEE float? */
static BOOL is_float32_mix_format(const WAVEFORMATEX* wf)
{
    if (!wf) return FALSE;
    if (wf->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) return TRUE;
    if (wf->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        const WAVEFORMATEXTENSIBLE* wfe = (const WAVEFORMATEXTENSIBLE*)wf;
        return IsEqualGUID(&wfe->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
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
    IAudioClient3* audioClient3;   /* Win10+, may be NULL */
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
            &CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
            &IID_IMMDeviceEnumerator, (void**)&dev->enumerator);
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
            if (SUCCEEDED(IPropertyStore_GetValue(props, &PKEY_Device_FriendlyName, &v))
                && v.vt == VT_LPWSTR && v.pwszVal)
            {
                wide_to_utf8(v.pwszVal, ep.name, (int)sizeof(ep.name));
            }
            PropVariantClear(&v);
            IPropertyStore_Release(props);
        }

        /* Mix format – channels + sample rate */
        hr = IMMDevice_Activate(d, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&client);
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
        dev->device, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&dev->audioClient);
    if (FAILED(hr)) return FALSE;

    /* Try to also get IAudioClient3 (Win10+) */
    (void)IUnknown_QueryInterface(
        (IUnknown*)dev->audioClient, &IID_IAudioClient3, (void**)&dev->audioClient3);

    /* ---- Mix format --------------------------------------------------- */
    hr = IAudioClient_GetMixFormat(dev->audioClient, &dev->mixFormat);
    if (FAILED(hr) || !dev->mixFormat) return FALSE;

    /* ---- Event handle ------------------------------------------------- */
    dev->event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!dev->event) return FALSE;

    streamFlags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;

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
        dev->audioClient, &IID_IAudioRenderClient, (void**)&dev->renderClient);
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
    if (dev->audioClient3) { IAudioClient3_Release(dev->audioClient3);      dev->audioClient3 = NULL; }
    if (dev->audioClient) { IAudioClient_Release(dev->audioClient);        dev->audioClient = NULL; }
    if (dev->device) { IMMDevice_Release(dev->device);                dev->device = NULL; }

    dev->user = NULL;
    dev->renderCb = NULL;
    dev->bufferFrames = 0;
}

/* =========================================================================
 * start() – prime buffer, launch thread
 * ====================================================================== */
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

static void audio_device_suspend(AudioDevice* dev)
{
    if (dev->audioClient) IAudioClient_Stop(dev->audioClient);
}

static void audio_device_resume(AudioDevice* dev)
{
    if (dev->audioClient) IAudioClient_Start(dev->audioClient);
}

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