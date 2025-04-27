// filesys.cc 
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk 
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them 
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "disk.h"
#include "bitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known 
// sectors, so that they can be located on boot-up.
#define FreeMapSector 		0
#define DirectorySector 	1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number 
// of files that can be loaded onto the disk.
#define FreeMapFileSize 	(NumSectors / BitsInByte)
#define NumDirEntries 		10
#define DirectoryFileSize 	(sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).  
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{ 
    DEBUG('f', "Initializing the file system.\n");
    if (format) {
        BitMap *freeMap = new BitMap(NumSectors);    //free sector map
        Directory *directory = new Directory(NumDirEntries); //initialize the directory table
	FileHeader *mapHdr = new FileHeader;
	FileHeader *dirHdr = new FileHeader;

        DEBUG('f', "Formatting the file system.\n");

    // First, allocate space for FileHeaders for the directory and bitmap
    // (make sure no one else grabs these!)
	freeMap->Mark(FreeMapSector);	    
	freeMap->Mark(DirectorySector);

    // Second, allocate space for the data blocks containing the contents
    // of the directory and bitmap files.  There better be enough space!

	ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
	ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));

    // Flush the bitmap and directory FileHeaders back to disk
    // We need to do this before we can "Open" the file, since open
    // reads the file header off of disk (and currently the disk has garbage
    // on it!).

        DEBUG('f', "Writing headers back to disk.\n");
	mapHdr->WriteBack(FreeMapSector);    
	dirHdr->WriteBack(DirectorySector);

    // OK to open the bitmap and directory files now
    // The file system operations assume these two files are left open
    // while Nachos is running.

        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
     
    // Once we have the files "open", we can write the initial version
    // of each file back to disk.  The directory at this point is completely
    // empty; but the bitmap has been changed to reflect the fact that
    // sectors on the disk have been allocated for the file headers and
    // to hold the file data for the directory and bitmap.

        DEBUG('f', "Writing bitmap and directory back to disk.\n");
	freeMap->WriteBack(freeMapFile);	 // flush changes to disk
	directory->WriteBack(directoryFile);

	if (DebugIsEnabled('f')) {
	    freeMap->Print();
	    directory->Print();

        delete freeMap; 
	delete directory; 
	delete mapHdr; 
	delete dirHdr;
	}
    } else {
    // if we are not formatting the disk, just open the files representing
    // the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
    }
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk 
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file 
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------

bool
FileSystem::Create(char *name, int initialSize)
{
    Directory *directory;
    BitMap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;
    printf("Creating file\n");
    DEBUG('f', "Creating file %s, size %d\n", name, initialSize);

    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);
    printf("Creating file %s, size %d\n", name, initialSize);
    if (directory->Find(name) != -1){
      success = FALSE;			// file is already in directory
      printf("Creating file\n");
    }
    else {	
        freeMap = new BitMap(NumSectors);
        freeMap->FetchFrom(freeMapFile);
        sector = freeMap->Find();	// find a sector to hold the file header
    	if (sector == -1) 		
            success = FALSE;		// no free block for file header 
        else if (!directory->Add(name, sector))
            success = FALSE;	// no space in directory
	else {
    	    hdr = new FileHeader;
	    if (!hdr->Allocate(freeMap, initialSize))
            	success = FALSE;	// no space on disk for data
	    else {	
	    	success = TRUE;
		// everthing worked, flush all changes back to disk
    	    	hdr->WriteBack(sector); 		
    	    	directory->WriteBack(directoryFile);
    	    	freeMap->WriteBack(freeMapFile);
	    }
            delete hdr;
	}
        delete freeMap;
    }
    delete directory;
    return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.  
//	To open a file:
//	  Find the location of the file's header, using the directory 
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFile *
FileSystem::Open(char *name)
{ 
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = NULL;
    int sector;

    DEBUG('f', "Opening file %s\n", name);
    directory->FetchFrom(directoryFile);
    sector = directory->Find(name); 
    if (sector >= 0) 		
	openFile = new OpenFile(sector);	// name was found in directory 
    printf("%d\n",sector);
    delete directory;
    return openFile;				// return NULL if not found
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------
bool
FileSystem::Remove(char *name)
{ 
    DEBUG('f', "Opening filesss %s\n", name);
    Directory *directory;
    BitMap *freeMap;
    FileHeader *fileHdr;
    int sector;
    OpenFile*dirFile=directoryFile;
    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);
    bool re=false;

    char** dirs;
    if(ParseFileName(name,dirs))
    {
        re=true;
        DEBUG('f', "Opening filesss %s\n", name);
        int i=0;
        for(i=0;dirs[i+1]!=NULL;i++)
        {
            DEBUG('f', "Opening filesss %s\n", dirs[i]);
            sector=directory->Find(dirs[i],true);
            DEBUG('f', "Opening filesss %d\n", sector);
            if(sector==-1){
                delete dirFile;
                delete directory;
                return FALSE;
            }
            DEBUG('f', "Opening filesss %d\n", sector);
            dirFile=new OpenFile(sector);
            directory->FetchFrom(dirFile);
        }
    // current directory is the last level of directory
    // and dirs[i] is the name of the file
    name=dirs[i];
}
    /*directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);
    sector = directory->Find(name);
    if (sector == -1) {
       delete directory;
       return FALSE;			 // file not found 
    }*/
    sector = directory->Find(name);
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);

    freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);

    fileHdr->Deallocate(freeMap);  		// remove data blocks
    freeMap->Clear(sector);			// remove header block
    directory->Remove(name);
    freeMap->WriteBack(freeMapFile);		// flush to disk
    directory->WriteBack(dirFile);        // flush to disk

    delete fileHdr;
    delete directory;
    delete freeMap;

    return TRUE;
} 

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void
FileSystem::List()
{
    Directory *directory = new Directory(NumDirEntries);

    directory->FetchFrom(directoryFile);
    directory->List();
    delete directory;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void
FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    BitMap *freeMap = new BitMap(NumSectors);
    Directory *directory = new Directory(NumDirEntries);

    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("Directory file header:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    freeMap->FetchFrom(freeMapFile);
    freeMap->Print();

    directory->FetchFrom(directoryFile);
    directory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
} 

bool
FileSystem::CreateDir(char**dirs,int filelength){
    Directory*fr=new Directory(NumDirEntries);
    fr->FetchFrom(directoryFile);//已经获取根目录了
    OpenFile*cur=directoryFile;
    int i=0;
    while(1){
        DEBUG('f',"!!!!!!!!!!!!!!!!!\n\n");
        if(dirs[i+1]==NULL)break;
        fr=createdir(dirs[i],fr,cur);
        if(fr==NULL){
            printf("Directory creation failed\n");
            return FALSE;
        }
        i++;
    }
    //已经到达要创建的文件
    Directory*frr=new Directory(NumDirEntries);
        frr->FetchFrom(directoryFile);
        int index=frr->FindIndex(dirs[0],true);
        DEBUG('f', "$$ppppp$$$1%d\n",index);
    bool re= createFile(dirs[i],fr,filelength,cur);
    frr=new Directory(NumDirEntries);
        frr->FetchFrom(directoryFile);
        index=frr->FindIndex(dirs[0],true);
        DEBUG('f', "$$ppppp$$$1%d\n",index);
    return re;
}
Directory*
FileSystem::createdir(char*name,Directory*fr,OpenFile*&in_file){
    //先判断此目录下是否存在同名目录
    int find;
    find=fr->Find(name,true);
    if(find!=-1){
        //已存在同名目录
        printf("Directory already exists%s\n",name);
        in_file=new OpenFile(find);
        fr=new Directory(NumDirEntries);
        fr->FetchFrom(in_file);
        return fr;
    }
    //要创建新目录了
    DEBUG('f',"Createing Directory %s \n",name);
    BitMap* map=new BitMap(NumSectors);
    DEBUG('f', "1\n");
    map->FetchFrom(freeMapFile);

    int sector=map->Find();//获取一个空的块
    Directory*ans=NULL;
    DEBUG('f', "1   %d\n",sector);

    if(sector==-1){
        delete map;
        return ans;
    }
    else{
        DEBUG('f', "2\n");
        bool pk=fr->Add(name,sector,true);
        if(!pk){
            delete map;
            return ans;
        }

        int index=fr->FindIndex(name,true);
        DEBUG('f', "$$$$$%d\n",index);
        FileHeader*pg=new FileHeader;
        pk=pg->Allocate(map,DirectoryFileSize);
        DEBUG('f', "2\n");
        if(!pk){
            delete map;
            return ans;
        }
        DEBUG('f', "123456789");
        pg->WriteBack(sector);
        fr->WriteBack(in_file);

        map->WriteBack(freeMapFile);
        ans=new Directory(NumDirEntries);
        in_file=new OpenFile(sector);
        ans->FetchFrom(in_file);
        


        delete map;
        delete pg;
        return ans;
    }
}
bool FileSystem::createFile(char* name,Directory *in,int fileLength,OpenFile* in_file)
{
    Directory *directory;
    BitMap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;

    DEBUG('f', "Creating file %s, size %d\n", name, fileLength);

    // make file in the directory `in`
    directory = in;
    DEBUG('f', "Creating filedwdwdwdwdwdwdw%d\n",directory->Find(name));
    if (directory->Find(name) != -1){
      success = FALSE;			// file is already in directory
      DEBUG('f', "Creating filedwdwdwdwdwdwdw\n");}
    else {
        freeMap = new BitMap(NumSectors);
        freeMap->FetchFrom(freeMapFile);
        sector = freeMap->Find();	// find a sector to hold the file header
        DEBUG('f', "Creating filedwdwdwdwdwdwdw\n");

    	if (sector == -1) 		
            success = FALSE;		// no free block for file header 
        else if (!directory->Add(name, sector))
            success = FALSE;	// no space in directory
	else {
    	    hdr = new FileHeader;
	    if (!hdr->Allocate(freeMap, fileLength))
            	success = FALSE;	// no space on disk for data
	    else {	
	    	success = TRUE;
		// everthing worked, flush all changes back to disk
    	    	hdr->WriteBack(sector); 		
    	    	directory->WriteBack(in_file);
    	    	freeMap->WriteBack(freeMapFile);
	    }
            delete hdr;
	}
        delete freeMap;
    }
    delete directory;
    return success;
}


OpenFile *
FileSystem::Open(char **name)
{ 
    
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = directoryFile;
    int sector;
    int i=0;
    while(1){
        if(name[i+1]==NULL){
            DEBUG('f', "psk %s\n", name[i]);
            break;
        }
        directory->FetchFrom(openFile);
        sector = directory->Find(name[i],true);
        DEBUG('f', "Opening directory %s\n  %d", name[i],sector);
        if(sector<0)return NULL;
        openFile=new OpenFile(sector);
        i++;
    }
    DEBUG('f', "Opening filesss %s\n", name[i]);
    directory->FetchFrom(openFile);
    sector = directory->Find(name[i]); 
    DEBUG('f', "Opening filesss %d\n", sector);
    if (sector >= 0) 		
	openFile = new OpenFile(sector);	// name was found in directory 
else openFile=NULL;
    delete directory;
    return openFile;				// return NULL if not found
}
bool
FileSystem::RemoveDir(char*name){
    char**dirs;
    if(!ParseFileName(name,dirs)){
        printf("you should give a directory!\n");
        return false;
    }
    // printf("Entry\n");
    OpenFile *dirFile=directoryFile;
    Directory *dir=new Directory(NumDirEntries);
    dir->FetchFrom(dirFile);

    int i=0;
    int sector=0;
    if(!ParseFileName(name,dirs))
    {
        printf("You Should give a directory but not a file name\n");
        return false;
    }

    for(i=0;dirs[i+1]!=NULL;i++)
    {
        sector=dir->Find(dirs[i]);
        if(sector<0){
            printf("Cann't Find Directory %s\n",dirs[i]);
            return false;
        }
        dirFile=new OpenFile(sector);
        dir->FetchFrom(dirFile);
    }

    // printf("Ready to CLear!\n");

    sector=dir->Find(dirs[i],true);
    if(sector<0){
        printf("RemoveDir: Unable to Find the directory %s\n",dirs[i]);
    }
    printf("%d#############",sector);
    OpenFile* delFile=new OpenFile(sector);
    Directory* delDir=new Directory(NumDirEntries);

    delDir->FetchFrom(delFile);
    // clear the content of directory dirs[i]
    delDir->Clear(freeMapFile,delFile,NumDirEntries);

    FileHeader* fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);

    BitMap* freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);

    fileHdr->Deallocate(freeMap);  		// remove data blocks
    freeMap->Clear(sector);			// remove header block
    if(!dir->Remove(dirs[i],true))
        printf("RemoveDir: Unable to Remove directory %s\n",dirs[i]);

    freeMap->WriteBack(freeMapFile);		// flush to disk
    dir->WriteBack(dirFile);        // flush to disk

    delete dir;
    delete fileHdr;
    delete freeMap;
    delete delDir;
    delete delFile;
    delete dirFile;
}

void FileSystem::ListDir(char* name)
{
    DEBUG('f',"starting\n");
    char** dirs;
    if(ParseFileName(name,dirs))
    {
        DEBUG('f',"starting\n");
        DEBUG('f',"starting\n");
        OpenFile* curFile=directoryFile;
        Directory* dir=new Directory(NumDirEntries);
        dir->FetchFrom(curFile);
        int sector;
        for(int i=0;dirs[i]!=NULL;i++)
        {
            if((sector=dir->Find(dirs[i],true))<0)
            {
                printf("ListDir: Unable to find directory:%s\n",dirs[i]);
                return;
            }
            DEBUG('f',"starting    %d\n",sector);
            curFile=new OpenFile(sector);
            dir->FetchFrom(curFile);
        }
        dir->List();
        delete curFile;
        delete dir;
        return;
    }
    else
    {
        delete dirs;
        printf("Unable to List file \n");
    }
}

