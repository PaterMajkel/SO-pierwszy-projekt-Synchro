#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h> 
#include <string.h>
#include <utime.h>
#include <sys/mman.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>

volatile int breakingNews=0;

void delete(const char *dir,struct dirent **namelist,int j, int count, int recursive );
void adding(char* dir1, char* dir2,unsigned int copysize);
void HardLink(char* dir1, char* dir2, int recursive,unsigned int copysize);

void differ(char* path1, char* path2, char* name, struct stat stats, unsigned int copysize, int recursive,int file1, int file2)
{
    //printf("jestem w differ\n");
    syslog(LOG_NOTICE,"DIFFER: Launching a function for %s, %s",path1,path2);
    struct utimbuf buff;
    buff.actime=0;
    buff.modtime=stats.st_mtime;
    if(file1!=4)
    {
        if(file2==4)
        {
            if(recursive==1)
                HardLink(path2,path2, recursive,copysize);
            else
                return;
        }
        if(remove(path2)<0)
        {
            syslog(LOG_ERR, "DIFFER: Failed to delete %s", path2);
            exit(EXIT_FAILURE);
        }

        int plik_wyjsciowy = open(path2,O_CREAT | O_WRONLY | O_TRUNC, 0777);
        if(plik_wyjsciowy==-1)
            {
                syslog(LOG_ERR, "DIFFER: Failed to create %s",path2);
                exit(EXIT_FAILURE);
            }

        if(stats.st_size<=copysize)
        {
            //read write
            //printf("jestem w read write\n");
            syslog(LOG_NOTICE,"DIFFER: Copying file contents using READ/WRITE");
            int plik_wejsciowy=open(path1,O_RDONLY);

            if(plik_wejsciowy==-1)
            {
                syslog(LOG_ERR, "DIFFER: Failed to open %s", path1);
                exit(EXIT_FAILURE);
            }

            char bufor[2024];
            int wej,wyj; 
            while((wej=read(plik_wejsciowy,bufor,sizeof(bufor)))>0)
            {
                wyj=write(plik_wyjsciowy,bufor,(ssize_t)wej);
                if(wyj!=wej)
                {
                    syslog(LOG_ERR, "DIFFER: Failed to make a complete write");
                    exit(EXIT_FAILURE);
                }
            }
                //printf("\n");

            if(wej<0)
            {
                syslog(LOG_ERR, "DIFFER: Failed to read from %s", path1);
                exit(EXIT_FAILURE);
            }
                
            if(close(plik_wejsciowy)<0)
                {
                    syslog(LOG_ERR, "DIFFER: Failed to close %s", path1);
                    exit(EXIT_FAILURE);
                }

        }
        else
        {
            //printf("jestem w mmap");
            syslog(LOG_NOTICE,"DIFFER: Copying file contents using MMAP/WRITE");
            int rozmiar=stats.st_size;
            int plik_wejsciowy=open(path1,O_RDONLY);
            //int plik_wyjsciowy=open(wyjscie,O_RDWR | O_TRUNC, 0644);
            printf("kopiuje zawartosc mmap\n");
            if(plik_wejsciowy==-1)
            {
                syslog(LOG_ERR, "DIFFER: Failed to open %s", path1);
                exit(EXIT_FAILURE);
            }
            
            char *mapa= mmap(0,rozmiar,PROT_READ,MAP_SHARED | MAP_FILE,plik_wejsciowy,0);
            if((void*)mapa==(void*)-1)
            {
                syslog(LOG_ERR, "DIFFER: Failed to map %s", path1);
                exit(EXIT_FAILURE);
            }
            if(write(plik_wyjsciowy, mapa, rozmiar)<0)
            {
                syslog(LOG_ERR, "DIFFER: Failed to read from %s", path1);
                exit(EXIT_FAILURE);
            }
            
            if(munmap(mapa, rozmiar)<0)
            {
                syslog(LOG_ERR, "DIFFER: Failed to munmap");
                    exit(EXIT_FAILURE);
            }

            if(close(plik_wejsciowy)<0)
                {
                    syslog(LOG_ERR, "DIFFER: Failed to close %s", path1);
                    exit(EXIT_FAILURE);
                }
            //nowy_czas_modyfikacji_i_chmode(wejscie,wyjscie);
            
        }
        

        //printf("zamykam plik\n");

        if(close(plik_wyjsciowy)<0)
        {
            syslog(LOG_ERR, "DIFFER: Failed to close %s", path2);
                    exit(EXIT_FAILURE);
        }
        syslog(LOG_NOTICE, "DIFFER: Changing permissions of %s", path2);
        if(chmod(path2,stats.st_mode)<0)
        {
            syslog(LOG_ERR, "DIFFER: Failed to change permissions of %s", path2);
                    exit(EXIT_FAILURE);
        }
        syslog(LOG_NOTICE, "DIFFER: Changing modification time of %s", path2);

        if(utime(path2,&buff)<0)
        {
            syslog(LOG_ERR, "DIFFER: Failed to change modification time of %s", path2);
                    exit(EXIT_FAILURE);
        }
    }
    else if(recursive==1)
    {
        syslog(LOG_NOTICE, "DIFFER: Encountered a folder. Launching HardLink to delete its contents and Adding");
        HardLink(path2,path2, recursive,copysize);
        adding(path1,path2,copysize);
    }


}

void adding(char* dir1, char* dir2,unsigned int copysize)
{
    syslog(LOG_NOTICE, "ADDING: Started a function");
    struct dirent **namelist1;
    struct dirent **namelist2;
    int count1=0;
    int count2=0;
    count1=scandir(dir1, &namelist1, NULL, alphasort);
    count2=scandir(dir2, &namelist2, NULL, alphasort);
    char *file=(char*)malloc(strlen(dir2)*sizeof(char*));
    char* file2=(char*)malloc(strlen(dir1)*sizeof(char*));
    char slash='/';
    //printf("mam %d plikow do skopiowania\n",count1);

    for(int i=2; i<count1; i++)
    {

        syslog(LOG_NOTICE,"ADDING: Adding %s in %s",namelist1[i]->d_name, dir2);
                if(namelist1[i]->d_type==4)
                {
                    strcpy(file,dir2);
                    strncat(file,&slash,1);
                    strncat(file,namelist1[i]->d_name,strlen(namelist1[i]->d_name));
                    strcpy(file2,dir1);
                    strncat(file2,&slash,1);
                    strncat(file2,namelist1[i]->d_name,strlen(namelist1[i]->d_name));
                    struct stat stata;
                    if(stat(file2,&stata)==-1)
                    {
                        syslog(LOG_ERR,"ADDING: Getting stats of %s failed", file2);
                        exit(EXIT_FAILURE);
                    }

                    if(mkdir(file,stata.st_mode)!=0)
                    {
                        syslog(LOG_ERR,"ADDING: Couldn't create %s folder",file);
                    }
                    //i++;

                        syslog(LOG_NOTICE, "ADDING: Launching Adding function to recursively add contents of a folder");
                        adding(file2,file,copysize);
                    //break;
                    //sleep(3);
                }
                else
                {
                    //sleep(3);
                    //printf("jestem w dodawaniu plikow\n");
                    strcpy(file,dir2);
                    strncat(file,&slash,1);
                    strncat(file,namelist1[i]->d_name,strlen(namelist1[i]->d_name));
                    strcpy(file2,dir1);
                    strncat(file2,&slash,1);
                    strncat(file2,namelist1[i]->d_name,strlen(namelist1[i]->d_name));
                    struct stat stata2;
                    if(stat(file2,&stata2)==-1)
                    {
                        syslog(LOG_ERR,"ADDING: Getting stats of %s failed", file2);
                        exit(EXIT_FAILURE);
                    }
                    struct utimbuf buff;
                    //stata2.st_mode
                    if(open(file,O_CREAT,0777)<0)
                    {
                        syslog(LOG_ERR,"ADDING: Creation of %s failed", file);
                        exit(EXIT_FAILURE);
                    }
                    buff.modtime=stata2.st_mtime;
                    buff.actime=0;
                    if(chmod(file,stata2.st_mode)<0)
                    {
                        syslog(LOG_ERR,"ADDING: Changing permissions of %s failed", file);
                        exit(EXIT_FAILURE);
                    }
                    //if(stat(file,&stata1)==-1)
                        //kod bledu
                    //    printf("blad\n");
                    if(utime(file,&buff)<0)
                    {
                        syslog(LOG_ERR,"ADDING: Changing modification time of %s failed", file);
                        exit(EXIT_FAILURE);
                    }

                    if(stata2.st_size>0)
                    {
                        syslog(LOG_NOTICE,"ADDING: %s has some content inside. Launching Differ function",file2);
                       differ(file2, file, namelist1[i]->d_name, stata2, copysize, 1,namelist1[i]->d_type, namelist1[i]->d_type);
                    }
                    //i++;
                    //break;


                }
    }
    free(file);
    free(file2);
}

void HardLink(char* dir1, char* dir2, int recursive,unsigned int copysize)
{
    //printf("jestem w hardlink\n");
    syslog(LOG_NOTICE,"HARDLINK: function has begun");
    struct dirent **namelist1;
    struct dirent **namelist2;
    int count1=0;
    int count2=0;
    count1=scandir(dir1, &namelist1, NULL, alphasort);
    count2=scandir(dir2, &namelist2, NULL, alphasort);
    int help=0;
    if(strcmp(dir1,dir2)==0)
    {
        syslog(LOG_NOTICE,"HARDLINK: deleting contents of %s folder",dir1);
        help=1;
    }


    //if(count2==0)
    //{
        //dodawanie do pustego folderu
    //}
    //printf("jestem za DIR\n");
   // for(int i=0; i<count1; i++)
     //   printf("%s \n", namelist1[i]->d_name);
    

        //printf("drugi plik\n");
    //for(int i=0; i<count2; i++)
      //  printf("%s \n", namelist2[i]->d_name);
    

    int i=2,j=2;

    char *file=(char*)malloc(strlen(dir2)*sizeof(char*));
    char* file2=(char*)malloc(strlen(dir1)*sizeof(char*));
    char slash='/';
    int equals=0;

    for(j; j<count2; j++)
    {
        
        while(i<count1 && help==0)
        {
            //printf("%s/%s while  %s/%s ", dir1,namelist1[i]->d_name,dir2,namelist2[j]->d_name);
            int compare=strcmp(namelist1[i]->d_name,namelist2[j]->d_name);
            //printf("%d help: %d  equals: %d\n", compare, help,equals);
            //sleep(2);
            syslog(LOG_NOTICE,"HARDLINK: Comparing %s and %s", namelist1[i]->d_name, namelist2[j]->d_name);
            if(compare>0 || help==1)
            {
                //del
                syslog(LOG_NOTICE,"HARDLINK: Deleting %s in %s", namelist2[j]->d_name, dir2);
                //printf("%s/%s compare  %s/%s\n", dir1,namelist1[i]->d_name,dir2,namelist2[j]->d_name);

                strcpy(file,dir2);
                strncat(file,&slash,1);
                strncat(file,namelist2[j]->d_name,strlen(namelist2[j]->d_name));
                //printf("%s\n", file);
                //sleep(1);
                if(namelist2[j]->d_type==4 && recursive==1 && help==0)
                {
                    HardLink(file,file, recursive,copysize);
                    //help=1;
                }
                if(remove(file)<0)
                {
                    syslog(LOG_ERR, "HARDLINK: Failed to delete %s", file);
                }
                break;
            }
            else if(compare==0)
            {
                //compare
                printf("takie same\n");
                syslog(LOG_NOTICE,"Hardlink: Filenames are the same, comparing upon other data");
                struct stat stata1;
                struct stat stata2;

                struct utimbuf buff1;
                struct utimbuf buff2;

                strcpy(file,dir2);
                strncat(file,&slash,1);
                strncat(file,namelist1[i]->d_name,strlen(namelist1[i]->d_name));
                strcpy(file2,dir1);
                strncat(file2,&slash,1);
                strncat(file2,namelist1[i]->d_name,strlen(namelist1[i]->d_name));

                if(stat(file2,&stata1)==-1)
                    {
                        syslog(LOG_ERR,"HARDLINK: Getting stats of %s failed", file2);
                        exit(EXIT_FAILURE);
                    }

                if(stat(file,&stata2)==-1)
                    {
                        syslog(LOG_ERR,"HARDLINK: Getting stats of %s failed", file);
                        exit(EXIT_FAILURE);
                    }
                //printf("%ld %ld\n",stata1.st_size,stata2.st_size);
                if(stata1.st_mtime!=stata2.st_mtime || stata1.st_size!=stata2.st_size || stata1.st_mode!=stata2.st_mode || namelist1[i]->d_type!=namelist2[j]->d_type)
                {
                    syslog(LOG_NOTICE,"HARDLINK: Files have different stats. Lunching function differ");
                    differ(file2,file,namelist1[i]->d_name, stata1,copysize, recursive,namelist1[i]->d_type, namelist2[j]->d_type);
                }

                i++;
                equals=1;
                break;

            }

            else
            {
                //printf("jestem w dodawaniu\n");
                //add
                syslog(LOG_NOTICE,"HARDLINK: Adding %s in %s",namelist1[i]->d_name, dir2);
                if(namelist1[i]->d_type==4)
                {
                    strcpy(file,dir2);
                    strncat(file,&slash,1);
                    strncat(file,namelist1[i]->d_name,strlen(namelist1[i]->d_name));
                    strcpy(file2,dir1);
                    strncat(file2,&slash,1);
                    strncat(file2,namelist1[i]->d_name,strlen(namelist1[i]->d_name));
                    struct stat stata;
                    if(stat(file2,&stata)==-1)
                    {
                        syslog(LOG_ERR,"HARDLINK: Getting stats of %s failed", file2);
                        exit(EXIT_FAILURE);
                    }

                    if(mkdir(file,stata.st_mode)!=0)
                    {
                        syslog(LOG_ERR,"HARDLINK: Couldn't create %s folder",file);
                    }
                    //i++;
                    if(recursive==1)
                    {
                        syslog(LOG_NOTICE, "HARDLINK: Launching Adding function to recursively add contents of a folder");
                        adding(file2,file,copysize);
                    }
                    //break;
                    //sleep(3);
                }
                else
                {
                    //sleep(3);
                    //printf("jestem w dodawaniu plikow\n");
                    strcpy(file,dir2);
                    strncat(file,&slash,1);
                    strncat(file,namelist1[i]->d_name,strlen(namelist1[i]->d_name));
                    strcpy(file2,dir1);
                    strncat(file2,&slash,1);
                    strncat(file2,namelist1[i]->d_name,strlen(namelist1[i]->d_name));
                    struct stat stata2;
                    if(stat(file2,&stata2)==-1)
                    {
                        syslog(LOG_ERR,"HARDLINK: Getting stats of %s failed", file2);
                        exit(EXIT_FAILURE);
                    }
                    struct utimbuf buff;
                    //stata2.st_mode
                    if(open(file,O_CREAT,0777)<0)
                    {
                        syslog(LOG_ERR,"HARDLINK: Creation of %s failed", file);
                        exit(EXIT_FAILURE);
                    }
                    buff.modtime=stata2.st_mtime;
                    buff.actime=0;
                    if(chmod(file,stata2.st_mode)<0)
                    {
                        syslog(LOG_ERR,"HARDLINK: Changing permissions of %s failed", file);
                        exit(EXIT_FAILURE);
                    }
                    //if(stat(file,&stata1)==-1)
                        //kod bledu
                    //    printf("blad\n");
                    if(utime(file,&buff)<0)
                    {
                        syslog(LOG_ERR,"HARDLINK: Changing modification time of %s failed", file);
                        exit(EXIT_FAILURE);
                    }

                    if(stata2.st_size>0)
                    {
                        syslog(LOG_NOTICE,"HARDLINK: %s has some content inside. Launching Differ function",file2);
                       differ(file2, file, namelist1[i]->d_name, stata2, copysize, recursive,namelist1[i]->d_type, namelist2[j]->d_type);
                    }
                    //i++;
                    //break;


                }
                
            }
            i++;

        }       if(equals==0)
                {
                    syslog(LOG_NOTICE,"HARDLINK: Deleting %s in %s", namelist2[j]->d_name, dir2);
                    //printf("%s/%s poza while  %s/%s\n", dir1,namelist1[i]->d_name,dir2,namelist2[j]->d_name);
                    strcpy(file,dir2);
                    strncat(file,&slash,1);
                    strncat(file,namelist2[j]->d_name,strlen(namelist2[j]->d_name));
                    //printf("%s\n", file);
                    //sleep(1);
                    if(namelist2[j]->d_type==4 && recursive==1 && help==0)
                    {
                        HardLink(file,file, recursive,copysize);
                        //help=1;
                    }
                    //help=0;
                    if(remove(file)<0)
                    {
                        syslog(LOG_ERR, "HARDLINK: Failed to delete %s", file);
                    }
                }
                //help=0;
                equals=0;
                

    }

    
    if(help==0)
    while(i<count1)
    {
                //add
                syslog(LOG_NOTICE,"HARDLINK: Adding %s in %s",namelist1[i]->d_name, dir2);
                if(namelist1[i]->d_type==4)
                {
                    strcpy(file,dir2);
                    strncat(file,&slash,1);
                    strncat(file,namelist1[i]->d_name,strlen(namelist1[i]->d_name));
                    strcpy(file2,dir1);
                    strncat(file2,&slash,1);
                    strncat(file2,namelist1[i]->d_name,strlen(namelist1[i]->d_name));
                    struct stat stata;
                    if(stat(file2,&stata)==-1)
                    {
                        syslog(LOG_ERR,"HARDLINK: Getting stats of %s failed", file2);
                        exit(EXIT_FAILURE);
                    }

                    if(mkdir(file,stata.st_mode)!=0)
                    {
                        syslog(LOG_ERR,"HARDLINK: Couldn't create %s folder",file);
                    }
                    //i++;
                    if(recursive==1)
                    {
                        syslog(LOG_NOTICE, "HARDLINK: Launching Adding function to recursively add contents of a folder");
                        adding(file2,file,copysize);
                    }
                    //break;
                    //sleep(3);
                }
                else
                {
                    //sleep(3);
                    //printf("jestem w dodawaniu plikow\n");
                    strcpy(file,dir2);
                    strncat(file,&slash,1);
                    strncat(file,namelist1[i]->d_name,strlen(namelist1[i]->d_name));
                    strcpy(file2,dir1);
                    strncat(file2,&slash,1);
                    strncat(file2,namelist1[i]->d_name,strlen(namelist1[i]->d_name));
                    struct stat stata2;
                    if(stat(file2,&stata2)==-1)
                    {
                        syslog(LOG_ERR,"HARDLINK: Getting stats of %s failed", file2);
                        exit(EXIT_FAILURE);
                    }
                    struct utimbuf buff;
                    //stata2.st_mode
                    if(open(file,O_CREAT,0777)<0)
                    {
                        syslog(LOG_ERR,"HARDLINK: Creation of %s failed", file);
                        exit(EXIT_FAILURE);
                    }
                    buff.modtime=stata2.st_mtime;
                    buff.actime=0;
                    
                    //if(stat(file,&stata1)==-1)
                        //kod bledu
                    //    printf("blad\n");
                    if(utime(file,&buff)<0)
                    {
                        syslog(LOG_ERR,"HARDLINK: Changing modification time of %s failed", file);
                        exit(EXIT_FAILURE);
                    }

                    if(stata2.st_size>0)
                    {
                        syslog(LOG_NOTICE,"HARDLINK: %s has some content inside. Launching Differ function %s",file2, file);
                       differ(file2, file, namelist1[i]->d_name, stata2, copysize, recursive,namelist1[i]->d_type, namelist1[i]->d_type);
                    }
                    if(chmod(file,stata2.st_mode)<0)
                    {
                        syslog(LOG_ERR,"HARDLINK: Changing permissions of %s failed", file);
                        exit(EXIT_FAILURE);
                    }
                    //i++;
                    //break;


                }
            i++;
    }
    free(file);
    free(file2);
    return;
}

void handler(int signum){
    if(signum==SIGUSR1)
    {
        breakingNews=1;
        syslog(LOG_NOTICE, "HANDLER: Program has fastforwarded timer and woke up the demon");
    }
}

void sleeper(int time)
{
        int i=0;
    while(i<time)
        {
            if(breakingNews==1)
                return;
            sleep(1);
            i++;
        }
}
int main(int argc, char* argv[]) {

    if(signal(SIGUSR1, handler)==SIG_ERR)
    {
        syslog(LOG_ERR,"MAIN: Signal was wrongfully received");
    }

    if(argc<3)
    {
        syslog(LOG_ERR,"MAIN: Too few arguments");
        exit(EXIT_FAILURE);
    }

    int recursive=0;
    unsigned int copysize=0;
    int time=0;
    int iftime=0;

    

        struct stat sta1;
        struct stat sta2;
        char *wej=argv[1];
        char *wyj=argv[2];
        
        syslog(LOG_NOTICE,"MAIN: Task has begun");
        if(stat(wej, &sta1)==0)
            if(!S_ISDIR(sta1.st_mode))
                {
                    syslog(LOG_ERR, "MAIN: First argument is not a folder");
                    exit(EXIT_FAILURE);
                }

           
        if(stat(wyj, &sta2)==0)
            if(!S_ISDIR(sta2.st_mode))
                {
                    syslog(LOG_ERR, "MAIN: Second argument is not a folder");
                    exit(EXIT_FAILURE);
                }
        int opt=0;
        while((opt=getopt(argc,argv,"crs:t:"))!=-1)
        {
            switch (opt){

            case 'c':
                iftime=1;
                break;
            case 'r':
                recursive=1;
                break;
            case 't':
                time=atoi(optarg);
                break;
            case 's':
                copysize=atoi(optarg);
                break;
            default:
                break;
            }


        }

        pid_t pid, sid;
        
        pid = fork();

        if (pid < 0) {
            syslog(LOG_ERR,"MAIN: fork has failed");
                exit(EXIT_FAILURE);
        }
        if (pid > 0) {
                exit(EXIT_SUCCESS);
        }

      
        umask(0);
                
            
                
        
        sid = setsid();
        if (sid < 0) {
                syslog(LOG_ERR,"MAIN: Setting sid has failed");
                exit(EXIT_FAILURE);
        }
        

        
        
        if ((chdir("/")) < 0) {
                syslog(LOG_ERR,"MAIN: chdir has failed");
                exit(EXIT_FAILURE);
        }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        clock_t start;
        clock_t end;
        clock_t total;
        double _time=0;
        while(1)
        {
            sleeper(time);
            syslog(LOG_NOTICE, "MAIN: Waking up deamon");
            
            start=clock();
            HardLink(wej,wyj,recursive,copysize);

            end=clock();
            total=((end-start)/(CLOCKS_PER_SEC/1000));
            if(iftime==1)
            {
                _time=(double)total;
                syslog(LOG_NOTICE, "MAIN: Czas pracy funkcji: %f",_time/1000);
            }
            breakingNews=0;
            syslog(LOG_NOTICE, "MAIN: Putting deamon to sleep");

        }
        
        
}