#ifndef _WIN32AU_H_
#define _WIN32AU_H_

extern HANDLE audio_thread_handle;
extern int audio_threadid;
// returns:
//   success: 0
//   not enough memory: -1
//   format not supported: -2
//   audio already active: -3
int audioOpen(int samplerate, int numchannels, int bitspersamp,
              int buffersize, int numbuffers, int prebuf, int pb_as);
  

// returns
// 0: success
// -1: unable to kill thread
// -2: audio not active
int audioClose();
  
// returns:
// 0 on success
// -1 if audio is not active
// -2 if operation would block and block==0
//       or, if block!=0, operation blocked too long (10s)
int audioWrite();

// returns:
// -1 if audio is not active
// 1 if data can be written
// 0 if data can't be written
int audioCanWrite();

// returns:
// -1 if audio is not active
// 1 if audio is still playing
// 0 if audio is not playing
int audioIsPlaying();

// returns:
// -1 if audio not active
// 0 if successful
int audioPause(int pause);

void audioSetVolume(int volume); // from 0 to 51
void audioSetPan(int pan);

void audioFlush(int); // flushes the buffers and starts over

int audioGetOutputTime(); // returns milliseconds
int audioGetWrittenTime(); // same


#endif

