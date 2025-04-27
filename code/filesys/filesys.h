// filesys.h 
//	Data structures to represent the Nachos file system.
//
//	A file system is a set of files stored on disk, organized
//	into directories.  Operations on the file system have to
//	do with "naming" -- creating, opening, and deleting files,
//	given a textual file name.  Operations on an individual
//	"open" file (read, write, close) are to be found in the OpenFile
//	class (openfile.h).
//
//	We define two separate implementations of the file system. 
//	The "STUB" version just re-defines the Nachos file system 
//	operations as operations on the native UNIX file system on the machine
//	running the Nachos simulation.  This is provided in case the
//	multiprogramming and virtual memory assignments (which make use
//	of the file system) are done before the file system assignment.
//
//	The other version is a "real" file system, built on top of 
//	a disk simulator.  The disk is simulated using the native UNIX 
//	file system (in a file named "DISK"). 
//
//	In the "real" implementation, there are two key data structures used 
//	in the file system.  There is a single "root" directory, listing
//	all of the files in the file system; unlike UNIX, the baseline
//	system does not provide a hierarchical directory structure.  
//	In addition, there is a bitmap for allocating
//	disk sectors.  Both the root directory and the bitmap are themselves
//	stored as files in the Nachos file system -- this causes an interesting
//	bootstrap problem when the simulated disk is initialized. 
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef FS_H
#define FS_H

#include "copyright.h"
#include "openfile.h"
#include "parse.h"
#include "directory.h"
#include "bitmap.h"
#ifdef FILESYS_STUB 		// Temporarily implement file system calls as 
				// calls to UNIX, until the real file system
				// implementation is available
class FileSystem {
  public:
    FileSystem(bool format) {}

    bool Create(char *name, int initialSize) { 
	int fileDescriptor = OpenForWrite(name);

	if (fileDescriptor == -1) return FALSE;
	Close(fileDescriptor); 
	return TRUE; 
	}

    OpenFile* Open(char *name) {
	  int fileDescriptor = OpenForReadWrite(name, FALSE);
		//printf("66666^^^^^^^^^^^^^^^^^^^^^^^%d\n",fileDescriptor);
	  if (fileDescriptor == -1) return NULL;
	  return new OpenFile(fileDescriptor);
      }

    bool Remove(char *name) { return (bool)(Unlink(name) == 0); }

};

#else // FILESYS
class FileSystem {
  public:
    FileSystem(bool format);		// Initialize the file system.
					// Must be called *after* "synchDisk" 
					// has been initialized.
    					// If "format", there is nothing on
					// the disk, so initialize the directory
    					// and the bitmap of free blocks.

    bool Create(char *name, int initialSize);  	
					// Create a file (UNIX creat)
	bool createFile(char* name,Directory *in,int fileLength,OpenFile* in_file);
    OpenFile* Open(char *name); 	// Open a file (UNIX open)
	OpenFile* Open(char **name);//多级目录打开方式
    bool Remove(char *name);  		// Delete a file (UNIX unlink)
	bool RemoveDir(char*name);
    void List();			// List all the files in the file system
	void ListDir(char* name);
    void Print();			// List all the files and their contents
	bool CreateDir(char**dirs,int filelength);//这个是总的处理目录的函数
	Directory*createdir(char*dir,Directory*fr,OpenFile*&in_file);//这个函数是被总函数根据路径循环调用，创建每一级目录，fr是父目录
  
	BitMap* getBitMap() {
		//numSector: DISK 上总扇区数（共有 32*32=1024 个扇区）
		BitMap *freeBitMap = new BitMap(1024); 
		freeBitMap->FetchFrom(freeMapFile);
		return freeBitMap;
	}
	void setBitMap(BitMap* freeMap) {
		freeMap->WriteBack(freeMapFile);
	}
	
	private:
   OpenFile* freeMapFile;		// Bit map of free disk blocks,
					// represented as a file
   OpenFile* directoryFile;		// "Root" directory -- list of 
					// file names, represented as a file
};

#endif // FILESYS

#endif // FS_H
