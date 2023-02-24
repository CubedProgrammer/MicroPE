#include<stdio.h>
#include<unistd.h>
#include"conf.h"
#define SECRET "/etc/shadow"
int main(int argl, char *argv[])
{
    struct config conf;
    char user[361], tty[361];
    int succ = 0;
    int ttyfd = 0;
    for(; ttyfd < 3 && !isatty(ttyfd); ++ttyfd);
    if(ttyfd == 3)
    {
        fputs("Fatal error: No interactive terminal device found.\n", stderr);
        succ = 1;
    }
    else
        loadconf(&conf);
    return succ;
}
