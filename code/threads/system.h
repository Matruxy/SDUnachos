// system.h 
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"

/*新建头文件*/
//#include"bitmap.h"
//#include "machine.h"


/*#ifndef SYSTEM_Hs
#define SYSTEM_Hs
BitMap* GlobalFreeMap=new BitMap(NumPhysPages);//全局空闲块管理
BitMap* GlobalSpaceId=new BitMap(NumPhysPages);//管理全局空间标识
#endif
*/



// Initialization and cleanup routines

extern void Initialize(int argc, char **argv); 	// Initialization,
						// called before anything else
extern void Cleanup();				// Cleanup, called when
						// Nachos is done.

extern Thread *currentThread;			// the thread holding the CPU
extern Thread *threadToBeDestroyed;  		// the thread that just finished
extern Scheduler *scheduler;			// the ready list
extern Interrupt *interrupt;			// interrupt status
extern Statistics *stats;			// performance metrics
extern Timer *timer;				// the hardware alarm clock


/*#ifndef USER_PROGRAMs
#define USER_PROGRAMs
extern BitMap* GlobalFreeMap;;//全局空闲块管理
extern BitMap* GlobalSpaceId;;//管理全局空间标识
#endif*/



#ifdef USER_PROGRAM
#include "machine.h"
extern Machine* machine;	// user program memory and registers
extern BitMap* GlobalFreeMap;;//全局空闲块管理
extern BitMap* GlobalSpaceId;;//管理全局空间标识
extern int gh;
#endif

#ifdef FILESYS_NEEDED 		// FILESYS or FILESYS_STUB 
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
#endif

#endif // SYSTEM_H
