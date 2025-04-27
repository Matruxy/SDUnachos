/*
 * @Author: yydsys123 1093956823@qq.com
 * @Date: 2018-09-05 09:56:44
 * @LastEditors: yydsys123 1093956823@qq.com
 * @LastEditTime: 2025-04-07 16:30:01
 * @FilePath: \code\filesys\directory.cc
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
// directory.cc 
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
	   {
        table[i].inUse = FALSE;
        table[i].dir=FALSE;
       }
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{ 
    delete [] table;
} 

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file)
{
    (void) file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file)
{
    (void) file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::FindIndex(char *name,bool isdir)
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen)&&(table[i].dir==isdir))
	    return i;
    return -1;		// name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::Find(char *name,bool isdir)
{
    int i = FindIndex(name,isdir);

    if (i != -1)
	return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool
Directory::Add(char *name, int newSector)
{ 
    if (FindIndex(name) != -1)
	return FALSE;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse) {
            table[i].inUse = TRUE;
            strncpy(table[i].name, name, FileNameMaxLen); 
            table[i].sector = newSector;
        return TRUE;
	}
    return FALSE;	// no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *name,bool dir)
{ 
    int i = FindIndex(name,dir);

    if (i == -1)
	return FALSE; 		// name not in directory
    table[i].inUse = FALSE;
    return TRUE;	
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------

void
Directory::List()
{
   for (int i = 0; i < tableSize; i++)
	if (table[i].inUse)
	    printf("%s\n", table[i].name);
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print()
{ 
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
	if (table[i].inUse) {
	    printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
	    hdr->FetchFrom(table[i].sector);
	    hdr->Print();
	}
    //printf("sasas\n");
    delete hdr;
}



//////////////////////////////////////////////////
bool 
Directory::Add(char* name,int newSector,bool isdir){
    if(isdir){
        int index=FindIndex(name,isdir);
        if(index!=-1){
            printf("has the same name dir \n");
            return FALSE;
        }
        else{
            for(int i=0;i<tableSize;i++){
                if(!table[i].inUse){
                    DEBUG('f',"~~~~~~~~~~~~%d\n",newSector);
                    strncpy(table[i].name,name,FileNameMaxLen);
                    table[i].inUse=TRUE;
                    table[i].dir=isdir;
                    table[i].sector=newSector;
                    return TRUE;
                }
            }
            return FALSE;
        }
    }else{
        printf("The function was invoked incorrectly\n");
        return FALSE;
    }
}


bool Directory::Clear(OpenFile* freeMapFile,OpenFile* curFile,int n)
{
    FileHeader* fileHdr=new FileHeader;
    
    BitMap* freeMap=new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);
    
    OpenFile* delFile;
    Directory* delDir=new Directory(n);

    for(int i=0;i<n;i++)
    {
        if(table[i].inUse)
        {
            int sector=table[i].sector;
            if(table[i].sector<0) return FALSE;
            DEBUG('f',"------1----------%d\n",sector);
            if(table[i].dir)
            {//delete a directory
                delFile=new OpenFile(table[i].sector);
                delDir->FetchFrom(delFile);
                delDir->Clear(freeMapFile,delFile,n);
            }
            DEBUG('f',"------2----------%d\n",sector);
            fileHdr->FetchFrom(table[i].sector);
            freeMap->FetchFrom(freeMapFile);
            DEBUG('f',"------3----------%d\n",sector);
            fileHdr->Deallocate(freeMap);
            DEBUG('f',"------4----------%d\n",sector);
            freeMap->Clear(table[i].sector);
            freeMap->WriteBack(freeMapFile);
            Remove(table[i].name);
            table[i].inUse=false;
        }
    }

    freeMap->WriteBack(freeMapFile);
    this->WriteBack(curFile);

    delete fileHdr;
    delete freeMap;
    return true;
}
