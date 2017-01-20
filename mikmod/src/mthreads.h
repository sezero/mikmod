/*  MikMod module player
	(c) 1998 - 2000 Miodrag Vallat and others - see file AUTHORS for
	complete list.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id: mthreads.h,v 1.1.1.1 2004/01/16 02:07:43 raph Exp $

  More or less portable thread functions

==============================================================================*/

#ifndef MTHREADS_H
#define MTHREADS_H

#ifdef HAVE_USLEEP
#ifndef HAVE_USLEEP_PROTO
void usleep(unsigned long);
#endif
#else
int usleep_new(unsigned long);
#endif

#if defined(__OS2__)||defined(__EMX__)
#define SLEEP(n)  DosSleep(n)
#elif defined(_WIN32)
#define SLEEP(n)  Sleep(n*10)
#elif !defined(HAVE_USLEEP)
#define SLEEP(n)  usleep_new(n*1000)
#else
#define SLEEP(n)  usleep(n*1000)
#endif

typedef enum {
	MTH_NORUN,			/* thread does not run */
	MTH_RUNNING,		/* thread runs */
	MTH_QUITTING		/* thread is scheduled for quitting */
} MTH_MODE;

#define USE_THREADS

#ifdef HAVE_PTHREAD

#if defined(__OpenBSD__) && !defined(_POSIX_THREADS)
#define _POSIX_THREADS
#endif
#include <pthread.h>
#define DECLARE_MUTEX(name) \
	extern pthread_mutex_t _mm_mutex_##name
#define DEFINE_MUTEX(name) \
	pthread_mutex_t _mm_mutex_##name = PTHREAD_MUTEX_INITIALIZER
#define INIT_MUTEX(name) \
	(1)
#define MUTEX_LOCK(name) \
	pthread_mutex_lock(&_mm_mutex_##name)
#define MUTEX_UNLOCK(name) \
	pthread_mutex_unlock(&_mm_mutex_##name)

#define DEFINE_THREAD(name,modevar) \
	MTH_MODE modevar = MTH_NORUN; \
	pthread_t _mm_thread_##name
#define THREAD_START(name,fkt,arg) \
	(pthread_create(&_mm_thread_##name, NULL, &fkt, arg)==0)
#define THREAD_JOIN(name,modevar) \
	{	modevar = MTH_QUITTING; \
		pthread_join (_mm_thread_##name, NULL); \
	}
#elif defined(__OS2__)||defined(__EMX__)
#include <process.h>

#define DECLARE_MUTEX(name) \
	extern HMTX _mm_mutex_##name
#define DEFINE_MUTEX(name) \
	HMTX _mm_mutex_##name = NULLHANDLE
#define INIT_MUTEX(name) \
	(!DosCreateMutexSem((PSZ) NULL, &_mm_mutex_##name, 0, 0))
#define MUTEX_LOCK(name)  \
	if (_mm_mutex_##name) \
		DosRequestMutexSem(_mm_mutex_##name, SEM_INDEFINITE_WAIT)
#define MUTEX_UNLOCK(name) \
	if (_mm_mutex_##name)  \
		DosReleaseMutexSem(_mm_mutex_##name)

#define DEFINE_THREAD(name,modevar) \
	int modevar = MTH_NORUN
#define THREAD_START(name,fkt,arg) \
	(_beginthread(fkt, NULL, 4096, arg) != -1)
#define THREAD_JOIN(name,modevar) \
	{	modevar = MTH_QUITTING; \
		while (modevar==MTH_QUITTING) SLEEP(1); \
	}

#elif defined(_WIN32)

#include <windows.h>
#include <process.h>

#define DECLARE_MUTEX(name) \
	extern HANDLE _mm_mutex_##name
#define DEFINE_MUTEX(name) \
	HANDLE _mm_mutex_##name
#define INIT_MUTEX(name) \
	(_mm_mutex_##name = CreateMutex(NULL, FALSE, "mm_mutex("#name")"))
#define MUTEX_LOCK(name) \
	if (_mm_mutex_##name) WaitForSingleObject(_mm_mutex_##name, INFINITE)
#define MUTEX_UNLOCK(name) \
	if (_mm_mutex_##name) ReleaseMutex(_mm_mutex_##name)

#define DEFINE_THREAD(name,modevar) \
	int modevar = MTH_NORUN
#define THREAD_START(name,fkt,arg) \
	(_beginthread(fkt, 4096, arg) != -1)
#define THREAD_JOIN(name,modevar) \
	{	modevar = MTH_QUITTING; \
		while (modevar==MTH_QUITTING) SLEEP(1); \
	}
#else

#undef USE_THREADS

#define DECLARE_MUTEX(name)
#define DEFINE_MUTEX(name)
#define INIT_MUTEX(name)			(0)
#define MUTEX_LOCK(name)
#define MUTEX_UNLOCK(name)

#define DEFINE_THREAD(name,modevar) \
	int modevar = MTH_NORUN
#define THREAD_START(name,fkt,arg)	(0)
#define THREAD_JOIN(name,modevar)

#endif

#endif /* MTHREADS_H */
