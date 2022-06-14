#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
int num(char dirname[])
{
	int i,num=0;
	for(i=0;dirname[i]!='\0';i++)
    {
        if(dirname[i]>='0' && dirname[i]<='9')
		    num=num*10+(dirname[i]-'0');
        else
            return 0;
    }
	
	return num;
}

int main(void)
{
    /* TODO */
    char statname[30],cmdname[30];
    char c='\0';
    DIR *dirptr=opendir("/proc/");
    struct dirent *direntp;
    int pid;
    int length;
    FILE *f;char buf[5000];
    printf("  PID S CMD\n");
    while((direntp=readdir(dirptr)) != NULL)   
	{
		if((pid=num(direntp->d_name))!=0)         
		{
            printf("%5d ",pid);
            sprintf(statname,"/proc/%d/stat",pid);
            sprintf(cmdname,"/proc/%d/cmdline",pid);
            f=fopen(statname,"r");
            while (fgetc(f)!=')');
            while (fgetc(f)!=' ');
            printf("%c ",fgetc(f));
            fclose(f);
            f=fopen(cmdname,"r");
            if((c=fgetc(f))==EOF)
            {
                fclose(f);
                sprintf(cmdname,"/proc/%d/comm",pid);
                f=fopen(cmdname,"r");
            }
            else 
                printf("%c",c);
            while (fgets(buf,5000,f)!=NULL)
            {
                length=strlen(buf);
                if(buf[length-1]=='\n')buf[length-1]='\0';
                printf("%s  ",buf);
            }
            printf("\n");
	    }
	}
    return 0;
}
