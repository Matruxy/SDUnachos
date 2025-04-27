#include "syscall.h" 

int 
main() 
{ 
    OpenFileId fp;
    char buffer[50];
    int size;
    int c;
    Create("tests");
    fp=Open("tests");
    Write("hello world!!!!",14,fp);
    size=Read(buffer,10,fp);
    Close(fp);
    c=Exec("halt.noff");
    Join(c);
    Exit(0);
    //Halt();
    
}

