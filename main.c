#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<termios.h>
#include<unistd.h>
#include"conf.h"
#define SECRET "/etc/shadow"
#define ring putchar('\a')
char verify(int fd, const char *user)
{
    char cbuf[121], pass[121];
    char loopuser[361];
    char *trypass;
    struct termios old, curr;
    char succ = 0;
    FILE *fh = fopen(SECRET, "r");
    if(fh == NULL)
        perror("failed to open shadow password file");
    else
    {
        unsigned colon = 0;
        unsigned ind = 0, passind = 0;
        for(int ch = fgetc(fh); !feof(fh); ch = fgetc(fh))
        {
            switch(ch)
            {
                case'\n':
                    loopuser[ind] = '\0';
                    if(strcmp(loopuser, user) == 0)
                        fseek(fh, 0, SEEK_END);
                    else
                    {
                        colon = 0;
                        ind = 0;
                        passind = 0;
                    }
                    break;
                case':':
                    ++colon;
                    break;
                default:
                    if(colon == 0 && ind < sizeof(loopuser) - 1)
                        loopuser[ind++] = ch;
                    else if(colon == 1 && passind < sizeof(pass) - 1)
                        pass[passind++] = ch;
            }
        }
        fclose(fh);
        tcgetattr(fd, &old);
        curr = old;
        curr.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(fd, TCSANOW, &curr);
        passind = ind = 0;
        for(int ch = getchar(); ch != '\n'; ch = getchar())
        {
            if(ch == 033)
            {
                ch = (getchar(), getchar());
                switch(ch)
                {
                    case 0103:
                        if(ind < passind)
                            ++ind;
                        else
                            ring;
                        break;
                    case 0104:
                        if(ind > 0)
                            --ind;
                        else
                            ring;
                        break;
                    case 0106:
                        ind = passind;
                        break;
                    case 0110:
                        ind = 0;
                        break;
                    default:
                        ring;
                }
            }
            else if(ch == 0177 && ind > 0)
            {
                --ind;
                --passind;
                if(ind < passind)
                    memmove(cbuf + ind, cbuf + ind + 1, passind - ind);
            }
            else if(passind < sizeof(cbuf) - 1)
            {
                if(ind < passind)
                    memmove(cbuf + ind + 1, cbuf + ind, passind - ind);
                cbuf[ind++] = ch;
                ++passind;
            }
        }
        tcsetattr(fd, TCSANOW, &old);
        cbuf[passind] = '\0';
        trypass = crypt(cbuf, pass);
        strcpy(cbuf, trypass);
        succ = strcmp(cbuf, pass) == 0;
    }
    return succ;
}
int execmd(const char *cmd, char *const* args)
{
    int pfd[2];
    int flag, pid;
    int succ = 0;
    pipe(pfd);
    flag = fcntl(pfd[1], F_GETFD);
    flag |= FD_CLOEXEC;
    fcntl(pfd[1], F_SETFD, flag);
    pid = fork();
    if(pid > 0)
    {
        close(pfd[1]);
        flag = read(pfd[1], &flag, sizeof flag);
        if(flag > 0)
            succ = 1;
        close(pfd[0]);
    }
    else
    {
        close(pfd[0]);
        execvp(cmd, args);
        write(pfd[1], &flag, sizeof flag);
        close(pfd[1]);
        exit(1);
    }
}
int main(int argl, char *argv[])
{
    struct config conf;
    long timeout, curr = 0;
    char user[361], tty[361];
    char *ttybase;
    char ask = 1, auth;
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
        setvbuf(stdout, NULL, _IONBF, 0);
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
                printf("%s: password for %s: ", *argv, conf.user);
                auth = verify(ttyfd, conf.user);
            }
            else
                auth = 1;
            if(auth)
            {
                execmd(argv[1], argv + 1);
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
                puts("Authorization failed.");
                succ = 1;
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
