bool ParseFileName(char* name,char**& dirs)
{
    if(name[0]=='.' && name[1]=='/')
    {
        dirs=new char*[10];
        int DirLevInd=0;
        for(int i=1;name[i]!='\0';)
        { //check the '/'
            if(name[i]=='/')
            {
                i++;
                int index=0;
                dirs[DirLevInd]=new char[10];
                // get the name of directory[DirLevInd-1]
                for(;name[i]!='\0' && name[i]!='/';i++)
                {
                    dirs[DirLevInd][index++]=name[i];
                }
                dirs[DirLevInd][index]='\0';
                DirLevInd++;
            }
        }
        dirs[DirLevInd]=nullptr;
        // DirLevInd is cnt of FileLevel
        return true;
    }
    else
    {
        return false;
    }
}