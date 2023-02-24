#include<stdio.h>
#include<time.h>
#include<termios.h>
#include<unistd.h>
#include"conf.h"
#define SECRET "/etc/shadow"
void verify(int fd)
{
    char cbuf[121];
    struct termios old, curr;
    tcgetattr(fd, &old);
    curr = old;
    curr.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(fd, TCSANOW, &curr);
    tcsetattr(fd, TCSANOW, &old);
}
int main(int argl, char *argv[])
{
    struct config conf;
    long timeout, curr = 0;
    char user[361], tty[361];
    char *ttybase;
    char ask = 1;
    int succ = 0;
    int ttyfd = 0;
    for(; ttyfd < 3 && !isatty(ttyfd); ++ttyfd);
    if(ttyfd == 3)
    {
        fputs("Fatal error: No interactive terminal device found.\n", stderr);
        succ = 1;
    }
    else
    {
        succ = loadconf(&conf);
        if(succ == 13)
            fputs("Environmental variable used for username is too long.\n", stderr);
        else if(succ == 1)
            perror("Reading configuration failed");
        else if(permitted(&conf))
        {
            if(conf.persist > 0)
            {
                ttyname_r(ttyfd, tty, sizeof tty);
                ttybase = tty;
                for(char *it = tty; *it != '\0'; ++it)
                {
                    if(*it == '/')
                        ttybase = it + 1;
                }
                timeout = loadtime(ttybase);
                curr = time(NULL);
                ask = curr > timeout;
            }
            if(ask)
            {
                timeout = curr + conf.persist;
                verify(ttyfd);
            }
            if(conf.persist > 0)
            {
                succ = savetime(ttybase, timeout);
                if(succ == 1)
                    perror("Saving timeout failed");
                else if(succ == 13)
                    fputs("TTY name too long.\n", stderr);
            }
        }
        else
        {
            succ = 1;
            fputs("You do not have permission to use superuser commands.\n", stderr);
        }
    }
    return succ;
}
