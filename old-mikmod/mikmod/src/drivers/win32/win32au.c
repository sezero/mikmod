#include <windows.h>
#include <process.h>
#include "win32au.h"
#include "mikmod.h"
#include "virtch.h"

int config_whichdevice=WAVE_MAPPER;
int config_altvolset=0;
char config_waveoutdir[] = "C:\\";

#define THREAD_STACK_SIZE 0
#define PLAY_DELAY 6           // was 20
#define DECODE_DELAY 20         // was 50

int audio_threadid=0;

typedef struct a_buffer {
  unsigned char *data;
  struct a_buffer *next;
  WAVEHDR header;
} a_buffer;

static int a_v=34, a_p=0;

HANDLE audio_thread_handle;
static HWAVEOUT devhandle;
static int devusing;
static int bufsize = 32768/2; // must be multiple of 1024
static int numbufs = 6;     // at least 4
static a_buffer *EmptyBuffers, *Buffers, *PlayingBuffers, *FullBuffers;
static int bps = 16, srate, numchan;
static int rec_offs, pb, flushed,flushing, do_pb;
static volatile int thread_active, active;
static volatile int a_play=0;
static CRITICAL_SECTION cs;
static int t_offset;
static int writtentime, w_offset;
static int _pbas;
static int interp_lasttime, interp_lastuclock;

static void AddToEmpty(a_buffer *p)
{
    a_buffer *a;
    p->next = 0;
    if (!EmptyBuffers)
        EmptyBuffers = p;
    else
    {   for (a = EmptyBuffers; a->next; a=a->next);
        a->next = p;
    }
}


/*static void AddToFull(a_buffer *p)
{   a_buffer *a;
    p->next = 0;
    if (!FullBuffers)
        FullBuffers = p;
    else
    {   for (a = FullBuffers; a->next; a=a->next);
        a->next = p;
    }
}*/


static void AddToPlay(a_buffer *p) {
  p->next = PlayingBuffers;
  PlayingBuffers = p;
}

a_buffer *GetFromPlay(int remove) {
  a_buffer *p, *lp;
  if (!PlayingBuffers) return 0;
  if (!PlayingBuffers->next) {
    p = PlayingBuffers;
    if (remove) PlayingBuffers = 0;
    return p;
  }
  lp = 0;
  for (p = PlayingBuffers; p->next; lp = p, p=p->next);
  if (remove) lp->next = 0;
  return p;
}


static DWORD WINAPI audioThread(void *params) {
    while (thread_active) {
        /*if (do_pb)
        {
            a_play=1;
            if (pb) while (thread_active && !flushed) 
            {
                int x;
                a_buffer *p;
                EnterCriticalSection(&cs);
                for (x = 0, p = FullBuffers; p; p=p->next, x++);
                LeaveCriticalSection(&cs);
                if (((x * 100) / (numbufs-1)) >= pb) break;
                Sleep(PLAY_DELAY);    
            }
            do_pb = 0;
            a_play=0;
        }*/
        Sleep(PLAY_DELAY);
        //if (!thread_active) continue;


        if (flushing)
        {   flushing=0;
            continue;
        }
        if (PlayingBuffers) 
        {
            EnterCriticalSection(&cs);
            while (1) 
            {
                a_buffer *p;
                p=GetFromPlay(0);
                if (!p || !(p->header.dwFlags & WHDR_DONE)) break;
                p=GetFromPlay(1);
                AddToEmpty(p);
            }
            LeaveCriticalSection(&cs);
        }
    }
    return 0;
}

// returns:
//   success: 0
//   not enough memory: -1
//   can't init audio, MMSYSERR_*
//   audio already active: -3
int audioOpen(int samplerate, int numchannels, int bitspersamp,
              int buffersize, int numbuffers, int prebuf, int pb_as) {
  int x;
  WAVEFORMATEX l;
  if (active) return -3;
  _pbas = pb_as;

  bufsize = buffersize;
  numbufs = numbuffers;
  if (numbufs < 3) numbufs = 3;
  do_pb = 1;
  pb = prebuf;
  devusing = config_whichdevice;

  l.wFormatTag = WAVE_FORMAT_PCM;
  numchan = l.nChannels = numchannels;
  srate = l.nSamplesPerSec = samplerate;
  bps = l.wBitsPerSample = bitspersamp;
  l.nBlockAlign = (bitspersamp*numchannels)/8;
  l.nAvgBytesPerSec = samplerate*l.nBlockAlign;
  l.cbSize = 0;

  if ((x = waveOutOpen(&devhandle,config_whichdevice,&l,0,0,0)) != MMSYSERR_NOERROR) 
  {
#if 0
      if (x == WAVERR_BADFORMAT) // fixme! (I shouldn't be here)
      {
          char str[1204];
          wsprintf(str,
              "Error returned: WAVERR_BADFORMAT\n"
              "Format information:\n"
              "\twFormatTag:\t\t%d\n"
              "\tnChannels:\t\t%d\n"
              "\tnSamplesPerSec:\t\t%d\n"
              "\twBitsPerSample:\t\t%d\n"
              "\tnAvgBytesPerSec:\t\t%d\n"
              "\tnBlockAlign:\t\t%d\n"
                ,
                l.wFormatTag,l.nChannels,l.nSamplesPerSec,l.wBitsPerSample,l.nAvgBytesPerSec,l.nBlockAlign);
          MessageBox(NULL,str,"WaveOutOpen error\n",MB_OK|MB_ICONEXCLAMATION);
      }
#endif
      return x;
  }
    Buffers = (a_buffer *) malloc((sizeof(a_buffer)+bufsize)*numbufs);
    if (!Buffers) return MMSYSERR_NOMEM;
    memset(Buffers,0,(sizeof(a_buffer)+bufsize)*numbufs);
    for (x = 0; x < numbufs; x ++) {
        Buffers[x].data = (char *) Buffers + sizeof(a_buffer)*numbufs+bufsize*x;
        Buffers[x].header.lpData = (void *) Buffers[x].data;
        Buffers[x].header.dwBufferLength=bufsize;
        Buffers[x].next = Buffers + x + 1;
        waveOutPrepareHeader(devhandle,&Buffers[x].header,sizeof(WAVEHDR));
    }
    Buffers[numbufs-1].next = 0;
    EmptyBuffers = Buffers;
    FullBuffers = 0;
    PlayingBuffers = 0;
  rec_offs = 0;
  thread_active = active = 1;
  flushed=0;
  flushing=0;
  t_offset=0;
  w_offset = writtentime = 0;
    InitializeCriticalSection(&cs); 
        audio_thread_handle = (HANDLE)
            _beginthreadex(NULL,THREAD_STACK_SIZE,(LPTHREAD_START_ROUTINE) audioThread,NULL,0,&audio_threadid);
  interp_lasttime=0;
  interp_lastuclock=GetTickCount(); 
  SetThreadPriority(audio_thread_handle,THREAD_PRIORITY_HIGHEST);
  SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);

  return 0; 
}

// returns
// 0: success
// -1: unable to kill thread
// -2: audio not active

int audioClose() {
    int x;
    flushing=1;
    if (!active) return -2;
    active=0;

    Sleep(90);
    waveOutReset(devhandle);
        thread_active = 0;
        WaitForSingleObject(audio_thread_handle,2000);
        CloseHandle(audio_thread_handle);
        for (x = 0; x < numbufs; x ++) {
            if (Buffers[x].header.dwFlags & WHDR_PREPARED)
                while (waveOutUnprepareHeader(devhandle,&Buffers[x].header,sizeof(WAVEHDR)) == WAVERR_STILLPLAYING) 
                    Sleep(10);  
        }
        free(Buffers);
        DeleteCriticalSection(&cs);
    //audioSetVolume(-666);
    while (waveOutClose(devhandle) == WAVERR_STILLPLAYING) Sleep(10);

    audio_threadid = 0;
    audio_thread_handle = 0;
}

// returns:
// 0 on success
// -1 if audio is not active
// -2 if operation would block and block==0
//       or, if block!=0, operation blocked too long (10s)
int audioWrite(int len) {
      a_buffer *k;

  if (!active) return -1;

  /*if (!len)
  {
    flushed=1;
    if (rec_offs) {
      a_buffer *k;
      EnterCriticalSection(&cs);
      k = EmptyBuffers;
      k->header.dwBufferLength = rec_offs;
      EmptyBuffers = EmptyBuffers->next;
      AddToFull(k);
      LeaveCriticalSection(&cs);
      rec_offs = 0;
    }
    return 0;
  }

*/
    if (!EmptyBuffers) return -2;
    if (!active) return -2;

    /*if (rec_offs+len > bufsize)
    {
      len2 = rec_offs+len-bufsize;
      len = bufsize-rec_offs;
    }*/

      VC_WriteBytes(md, EmptyBuffers->data,len);
      rec_offs += len;

      EnterCriticalSection(&cs);
      k = EmptyBuffers;
      k->header.dwBufferLength = bufsize;
      EmptyBuffers = EmptyBuffers->next;
      //AddToFull(k);

      if (waveOutWrite(devhandle,&k->header,sizeof(WAVEHDR)) == MMSYSERR_NOERROR) 
      {   //FullBuffers=FullBuffers->next;
          AddToPlay(k);
      }

      LeaveCriticalSection(&cs);


  writtentime += len;


  return 0;
}

int audioCanWrite() {
  if (!active) return 0;
  return (EmptyBuffers ? 1 : 0);
}

// returns:
// -1 if audio is not active
// 1 if audio is still playing
// 0 if audio is not playing
int audioIsPlaying() {
  if (!active) return 0;
  return (a_play || (FullBuffers || PlayingBuffers) ? 1 : 0);
}

static int is_paused;

int audioPause(int pause) {
  if (!active) return -1;

    if (pause) waveOutPause(devhandle);
    else waveOutRestart(devhandle);

  is_paused=pause;
  return 0;
}

static void _audioSetVolume() {
  int vol, v2;
  if (!active) return;
  if (a_v < 0) a_v = 0;
  if (a_v > 51) a_v = 51;
  v2 = vol = (a_v*65535) / 51;
  if (a_p < -12) a_p = -12;
  if (a_p > 12) a_p = 12;
  if (a_p > 0)
  {
      vol *= (12-a_p);
      vol /= 12;
  }
  else if (a_p < 0)
  {
      v2 *= (a_p+12);
      v2 /= 12;
  }
  vol |= v2<<16;

  waveOutSetVolume(devhandle,vol);

}


void audioSetVolume(int volume)
{
    if (volume != -666) a_v = volume;
    _audioSetVolume();
}

void audioSetPan(int pan)
{
    a_p = pan;
    _audioSetVolume();
}


void audioFlush(int time) {
  int x;
  if (!active) return;

  flushing=1;
  EnterCriticalSection(&cs);
  waveOutReset(devhandle);
    for (x = 0; x < numbufs; x ++) Buffers[x].next = Buffers + x + 1;
    Buffers[numbufs-1].next = 0;
    EmptyBuffers = Buffers;
    FullBuffers = 0;
    PlayingBuffers = 0;
    rec_offs = 0;
  t_offset=w_offset=0;
  t_offset = time - audioGetOutputTime() + 200;
  w_offset = time - audioGetWrittenTime();
  flushed=0;
  if (_pbas) do_pb = 1;
  LeaveCriticalSection(&cs);
  //audioSetVolume(-666);
  interp_lasttime=interp_lastuclock=0;
}

int audioGetOutputTime() { // returns milliseconds
    MMTIME m;
    int ms;
    if (!active) return 0;

    m.wType = TIME_BYTES;
    waveOutGetPosition(devhandle,&m,sizeof(m));
    if (m.wType == TIME_BYTES)
    {
        int l;
        ms = m.u.cb / numchan / (bps/8);
        l = ms%srate;
        ms /= srate;
        ms *= 1000;
        ms += (l*1000)/srate;
    }
    else if (m.wType == TIME_MS) ms = m.u.ms;
    else if (m.wType == TIME_SAMPLES)
    {
        int l;
        ms = m.u.sample;
        l = ms%srate;
        ms /= srate;
        ms *= 1000;
        ms += (l*1000)/srate;
    }
    else 
        {
        ms = 0;
        }
    ms += t_offset;

    return ms;
}

int audioGetWrittenTime() {
    int ms = (writtentime * 500.0 / (double) numchan) / (double) srate;
    if (bps == 8) ms*=2;
    return ms+w_offset;
}
