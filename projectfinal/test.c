#include <unistd.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/wait.h>

#define TIME_INTERVAL 4000000

void update_watch(int pid);

int main()
{
    int pid;
    remove("watch_result");
    FILE *filep=fopen("watch_result","w");
    fclose(filep);
    pid=fork();

    if(pid==0)
    {
    #ifdef THREAD
        char *argv[ ]={"/usr/bin/sysbench","--test=threads","--num-threads=17800","--thread-yields=5000","--thread-locks=4","run",NULL};
    #endif
    #ifdef MUTEX    
        char *argv[ ]={"/usr/bin/sysbench","--test=mutex","--num-threads=17800","--mutex-num=1000","--mutex-locks=100000","--mutex-loops=10000","run",NULL};
    #endif
    #ifdef FILEIO
        char *argv[ ]={"/usr/bin/sysbench","--test=fileio","--num-threads=17800","--file-total-size=5G","--file-test-mode=rndrw","run",NULL};
    #endif
        execve("/usr/bin/sysbench",argv,NULL);
    }
    else
    {
        FILE* fp = fopen("/proc/watch", "w");
        fprintf(fp,"%d",pid);
        fclose(fp);
        while(!waitpid(pid,NULL,WNOHANG|WUNTRACED))
        {
            update_watch(pid);
            usleep(TIME_INTERVAL);
        }
        return 0;
    }
}
void update_watch(int pid)
{
    int utime;
    int stime;
    int page_access;
    int sum_time;
    
    FILE *fp = fopen("/proc/watch", "r");
    fscanf(fp,"utime: %d ,stime: %d ,pages access: %d\n",&utime,&stime,&page_access);
    fclose(fp);
    sum_time=utime+stime;
    fp = fopen("watch_result", "a");
    fprintf(fp,"cpu rate: %d%%,mem rate: %dtimes/sec\n",100000*sum_time/TIME_INTERVAL,1000000*page_access/TIME_INTERVAL);
    fclose(fp);
}