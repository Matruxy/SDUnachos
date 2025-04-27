// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
void AdvancePC() {
    machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
    machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
    machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}

void StartProcess(int spaceid){
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();
    machine->Run();
    printf("执行完毕\n");
    ASSERT(FALSE);
}
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException)) {
        switch(type){
            case SC_Halt:{
                DEBUG('a', "Shutdown, initiated by user program.\n");
                printf("Execute system call of halt\n");
                printf("finished\n");
                interrupt->Halt();
                
                break;
            }
            case SC_Exec:{
                printf("Execute system call of Exec()\n");
                char filename[128]; 
                int addr=machine->ReadRegister(4); 
                int i=0;
                printf("{{{{{{{{{{{}}}}}}}}}}}\n");
                do{
                    machine->ReadMem(addr+i,1,(int *)&filename[i]);
                }while(filename[i++]!='\0');
                OpenFile *executable = fileSystem->Open(filename); 
                if (executable == NULL) {
                    printf("Unable to open file %s\n", filename);
                    return;
                }
                printf("{{{{{{{{{{{}}}}}}}}}}}\n");
                //new address space
                AddrSpace *space = new AddrSpace(executable); 
                printf("{{{{{{{{{{{}}}}}}}}}}}\n");
                delete executable; // close file
                //new and fork thread
                char *forkedThreadName=filename;
                //
                printf("{{{{{{{{{{{}}}}}}}}}}}\n");
                currentThread->setWaitSpaceId(space->getSpaceID());
                Thread *thread = new Thread("forkedThreadName");
                thread->space = space;
                thread->Fork(StartProcess, space->getSpaceID());
                machine->WriteRegister(2,space->getSpaceID());
                AdvancePC(); 
                break;    
            }
            case SC_Exit:{
                printf("This is SC_Exit, CurrentThreadId: %d\n",(currentThread->space)->getSpaceID());
                int exitCode = machine->ReadRegister(4);
                machine->WriteRegister(2,exitCode);
                currentThread->setExitCode(exitCode);
                //delete currentThread->space;
                currentThread->Finish();
                AdvancePC();
                break;
            }
            case SC_Join:{
                printf("This is SC_Join, CurrentThreadId: %d\n",(currentThread->space)->getSpaceID());
                int SpaceId = machine->ReadRegister(4);
                currentThread->Join(SpaceId);
                // waitProcessExitCode —— 返回 Joinee 的退出码
                machine->WriteRegister(2, currentThread->waitExitCode());
                AdvancePC();
                break;
            }
            case SC_Yield:{
                printf("This is SC_Yield, CurrentThreadId: %d\n",(currentThread->space)->getSpaceID());
                currentThread->Yield();
                AdvancePC();
                break;
            }
            case SC_Create:{
                #ifdef FILESYS
                    printf("Execute system call of Create()\n");    
                    int base=machine->ReadRegister(4);
                    int value;
                    int count=0;
                    char *FileName= new char[128];
                    do{
                        machine->ReadMem(base+count,1,&value);
                        FileName[count]=*(char*)&value;
                        count++;
                    } while(*(char*)&value!='\0'&&count<128);
                    //when calling Create(), thread go to sleep, waked up when I/O finish
                    if(!fileSystem->Create(FileName,0)) //call Create() in FILESYS,see filesys.h
                        printf("create file %s failed!\n",FileName);
                    else
                        DEBUG('f',"create file %s succeed!\n",FileName); 
                        
                        AdvancePC();
                        break;
                    #else
                    int addr = machine->ReadRegister(4);
                    char filename[128];
                    for(int i = 0; i < 128; i++){
                        machine->ReadMem(addr+i,1,(int *)&filename[i]);
                        if(filename[i] == '\0') break;
                    }
                    int fileDescriptor = OpenForWrite(filename);
                    if(fileDescriptor == -1) printf("create file %s failed!\n",filename);
                    else printf("create file %s succeed, the file id is %d\n",filename,fileDescriptor);
                    Close(fileDescriptor);
                    AdvancePC();
                    break;
                    #endif
            }
            case SC_Open:{
            #ifdef FILESYS
                int base=machine->ReadRegister(4);
                int value;
                int count=0;
                char *FileName= new char[128];
                do{
                    machine->ReadMem(base+count,1,&value);
                    FileName[count]=*(char*)&value;
                    count++;
                }while(*(char*)&value!='\0'&&count<128);
                int fileid;
               //call Open() in FILESYS,see filesys.h,Nachos Open()
                OpenFile* openfile=fileSystem->Open(FileName); 
                if(openfile == NULL ) { //file not existes, not found
                    printf("File \"%s\" not Exists, could not open it.\n",FileName);
                    fileid = -1;
                }
                else { //file found
                //set the opened file id in AddrSpace, which wiil be used in Read() and Write()
                    fileid = currentThread->space->getfiledescriptor(openfile); 
                    if (fileid < 0)
                    printf("Too many files opened!\n");
                    else 
                    DEBUG('f',"file :%s open secceed! the file id is %d\n",FileName,fileid);
                }
                machine->WriteRegister(2,fileid);
                AdvancePC();
               break;
               #else
                    int addr = machine->ReadRegister(4);
                    char filename[128];
                    for(int i = 0; i < 128; i++){
                        machine->ReadMem(addr+i,1,(int *)&filename[i]);
                        if(filename[i] == '\0') break;
                    }
                    int fileDescriptor = OpenForWrite(filename);
                    if(fileDescriptor == -1) printf("Open file %s failed!\n",filename);
                    else printf("Open file %s succeed, the file id is %d\n",filename,fileDescriptor);                
                    machine->WriteRegister(2,fileDescriptor);
                    AdvancePC();
                    break;
                #endif
            }
            case SC_Write:{
            #ifdef FILESYS
                printf("This is SC_Write, CurrentThreadId: %d\n",(currentThread->space)->getSpaceID());
                int base =machine->ReadRegister(4); //buffer
                int size=machine->ReadRegister(5); //bytes written to file 
                int fileId=machine->ReadRegister(6); //fd 
                int value;
                int count=0;
                // printf("base=%d, size=%d, fileId=%d \n",base,size,fileId );
                OpenFile* openfile =new OpenFile (fileId);
                ASSERT(openfile != NULL);
                
                char* buffer= new char[128];
                do{
                    machine->ReadMem(base+count,1,&value);
                    buffer[count] = *(char*)&value; 
                    count++;
                } while((*(char*)&value!='\0') && (count<size));
                buffer[size]='\0';
                
                openfile = currentThread->space->getfileId(fileId); 
                //printf("$$$$$$$$$$$$$$$$\n");
                //printf("openfile =%d\n",openfile);
                if (openfile == NULL)
                {
                    printf("Failed to Open file \"%d\" .\n",fileId); 
                    AdvancePC();
                    break;
                } 
                //printf("$$$$$$$$$$$$$$$$\n");
                if (fileId ==1 || fileId ==2)
                {
                    //printf("$$$$$$$$$$$$$$$$\n");
                    //printf("$$$asasa$$$$$\n");
                    openfile->WriteStdout(buffer,size); 
                    //printf("$$$$$$$$$$$$$$$$\n");
                    delete [] buffer;
                    AdvancePC();
                    break;
                } 
                
                int WritePosition = openfile->Length();
                
                openfile->Seek(WritePosition); //append write
                //openfile->Seek(0); //write form begining
                
                int writtenBytes;
                //writtenBytes = openfile->AppendWriteAt(buffer,size,WritePosition);
                writtenBytes = openfile->Write(buffer,size);
                if((writtenBytes)==0)
                DEBUG('f',"\nWrite file failed!\n");
                else
                {
                if (fileId != 1 && fileId != 2)
                {
                    DEBUG('f',"\n\"%s\" has wrote in file %d succeed!\n",buffer,fileId);
                    DEBUG('H',"\n\"%s\" has wrote in file %d succeed!\n",buffer,fileId);
                    DEBUG('J',"\n\"%s\" has wrote in file %d succeed!\n",buffer,fileId);
                }
                //printf("\n\"%s\" has wrote in file %d succeed!\n",buffer,fileId); 
                }
                
                //delete openfile;
                delete [] buffer;
                AdvancePC();
                break;
                #else
                    printf("This is SC_Write, CurrentThreadId: %d\n",(currentThread->space)->getSpaceID());
                    int addr = machine->ReadRegister(4);
                    int size = machine->ReadRegister(5);       // 字节数
                    int fileId = machine->ReadRegister(6);      // fd
                    int value;
                    int count=0;
                    // 打开文件
                    OpenFile *openfile = new OpenFile(fileId);
                    ASSERT(openfile != NULL);
                    char* buffer= new char[128];
                    do{
                    machine->ReadMem(addr+count,1,&value);
                    buffer[count] = *(char*)&value; 
                    count++;
                    } while((*(char*)&value!='\0') && (count<size));
                    buffer[size]='\0';

                    // 写入数据
                    int writePos;
                    if(fileId == 1) writePos = 0;
                    else writePos = openfile->Length();
                    // 在 writePos 后面进行数据添加
                    int writtenBytes = openfile->WriteAt(buffer,size,writePos);
                    if(writtenBytes == 0) printf("write file failed!\n");
                    else printf("\"%s\" has wrote in file %d succeed!\n",buffer,fileId);
                    AdvancePC();
                    break;
                #endif
            }
            case SC_Read:{
            #ifdef FILESYS
                int base =machine->ReadRegister(4);
                int size = machine->ReadRegister(5);
                int fileId=machine->ReadRegister(6);
                OpenFile* openfile = currentThread->space->getfileId(fileId); 
                //printf("please input the program you want to run:");
                char buffer[size];
                int readnum=0;
                if (fileId == 0) //stdin
                readnum = openfile->ReadStdin(buffer,size);
                else
                readnum = openfile->Read(buffer,size);
                
                for(int i = 0;i < readnum; i++)
                machine->WriteMem(base,1,buffer[i]);
                buffer[readnum]='\0';
                
                for(int i = 0;i < readnum; i++)
                if (buffer[i] >=0 && buffer[i] <= 9)
                buffer[i] = buffer[i] + 0x30; 
                
                char *buf = buffer;
                if (readnum > 0)
                {
                    if (fileId != 0)
                    {
                        DEBUG('f',"Read file (%d) succeed! the content is \"%s\" , the length is %d\n",fileId,buf,readnum);
                        DEBUG('H',"Read file (%d) succeed! the content is \"%s\" , the length is %d\n",fileId,buf,readnum);
                        DEBUG('J',"Read file (%d) succeed! the content is \"%s\" , the length is %d\n",fileId,buf,readnum);
                        printf("the reading content is \"%s\" , the length is %d\n",buf,readnum);
                        
                        printf("\n");
                    } 
                }
                else
                printf("\nRead file failed!\n");
                
               machine->WriteRegister(2,readnum);
               AdvancePC();
               break;
            #else
                int addr = machine->ReadRegister(4);
                int size = machine->ReadRegister(5);       // 字节数
                int fileId = machine->ReadRegister(6);      // fd

                // 打开文件读取信息
                char buffer[size+1];
                OpenFile *openfile = new OpenFile(fileId);
                int readnum = openfile->Read(buffer,size);

                for(int i = 0; i < size; i++)
                    if(!machine->WriteMem(addr,1,buffer[i])) printf("This is something Wrong.\n");
                buffer[size] = '\0';
                printf("read succeed, the content is \"%s\", the length is %d\n",buffer,size);
                machine->WriteRegister(2,readnum);
                AdvancePC();
                break;
            #endif
            }
            case SC_Close:{
                #ifdef FILESYS
                int fileId =machine->ReadRegister(4);
                OpenFile* openfile = currentThread->space->getfileId(fileId);
                if (openfile != NULL)
                {
                openfile->WriteBack(); // write file header back to DISK
                delete openfile; // close file 
                currentThread->space->releasefiledescriptor(fileId);
                
                DEBUG('f',"File %d closed succeed!\n",fileId);
                DEBUG('H',"File %d closed succeed!\n",fileId);
                DEBUG('J',"File %d closed succeed!\n",fileId);
                }
                else
                printf("Failded to Close File %d.\n",fileId);
                AdvancePC();
                break;
                #else
                int fileId = machine->ReadRegister(4);
                Close(fileId);
                printf("File %d closed succeed!\n",fileId);
                AdvancePC();
                break;
                #endif
            }
            default:{
                printf("Unexpected user mode exception %d %d\n", which, type);
	            ASSERT(FALSE);
            }
        }
    } else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}