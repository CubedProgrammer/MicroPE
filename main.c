#include<dirent.h>
#include<errno.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<time.h>
#include<termios.h>
#include<unistd.h>
#include"conf.h"
#define SECRET "/etc/shadow"
#define ring putchar('\a')
const char superuserhome[] = "/root";
unsigned controlling_tty(void)
{
    int ch;
    const char l = '(';
    const char r = ')';
    unsigned space = 0, open = 0;
    unsigned tty = 0;
    FILE *fh = fopen("/proc/self/stat", "r");
    while(space < 6)
    {
        ch = fgetc(fh);
        if(open == 0 && ch == ' ')
            ++space;
        else if(ch == l)
            ++open;
        else if(ch == r)
            --open;
    }
    fscanf(fh, "%u", &tty);
    fclose(fh);
    return tty;
}
int findtty(unsigned dev, char *buf, size_t bsz)
{
    size_t len;
    struct stat fdat;
    DIR *dh;
    int succ = 1;
    strcpy(buf, "/dev/");
    for(int m = 2; m; --m)
    {
        len = strlen(buf);
        dh = opendir(buf);
        for(struct dirent *en = readdir(dh); m && en != NULL; en = readdir(dh))
        {
            if(en->d_type == DT_CHR)
            {
                for(unsigned i = len; (buf[i] = en->d_name[i - len]) != '\0' && i + 1 < bsz; ++i);
                stat(buf, &fdat);
                if(fdat.st_rdev == dev)
                {
                    m = 0;
                    succ = 0;
                }
            }
        }
        if(m)
            strcpy(buf, "/dev/pts/");
        m += m == 0;
    }
    return succ;
}
int fdgetc(int fd)
{
    char unsigned ch;
    int cnt = read(fd, &ch, 1);
    return cnt == 1 ? ch : -1;
}
char verify(int fd, const char *user)
{
    const char lf = '\n';
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
                    pass[passind] = loopuser[ind] = '\0';
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
        for(int ch = fdgetc(fd); ch != '\n'; ch = fdgetc(fd))
        {
            if(ch == 033)
            {
                ch = (fdgetc(fd), fdgetc(fd));
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
        write(fd, &lf, 1);
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
    int cnt;
    pipe(pfd);
    flag = fcntl(pfd[1], F_GETFD);
    flag |= FD_CLOEXEC;
    fcntl(pfd[1], F_SETFD, flag);
    pid = fork();
    if(pid > 0)
    {
        close(pfd[1]);
        cnt = read(pfd[0], &flag, sizeof flag);
        if(cnt > 0)
        {
            pid = 0;
            errno = flag;
        }
        close(pfd[0]);
    }
    else
    {
        close(pfd[0]);
        setreuid(0, 0);
        setregid(0, 0);
        setenv("HOME", superuserhome, 1);
        setenv("LOGNAME", superuserhome + 1, 1);
        setenv("USER", superuserhome + 1, 1);
        execvp(cmd, args);
        flag = errno;
        write(pfd[1], &flag, sizeof flag);
        close(pfd[1]);
        exit(1);
    }
    return pid;
}
int main(int argl, char *argv[])
{
    struct config conf;
    struct stat fdat;
    long timeout, curr = 0;
    char user[361], tty[361];
    char promptmsg[361];
    char *ttybase, *promptptr;
    int pid, status;
    size_t promptlen;
    char auth, ask = 1;
    int succ = 0;
    int ttyfd = findtty(controlling_tty(), tty, sizeof tty);
    if(argl == 1)
        printf("%s version 1.0.3, Micro Privilege Escalator\n", *argv);
    else if(ttyfd)
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
                ttybase = tty;
                for(char *it = tty; *it != '\0'; ++it)
                {
                    if(*it == '/')
                        ttybase = it + 1;
                }
                timeout = loadtime(ttybase);
                curr = time(NULL);
                stat(tty, &fdat);
                ask = curr > timeout || fdat.st_ctime + conf.persist > timeout;
            }
            if(ask)
            {
                timeout = curr + conf.persist;
                ttyfd = open(tty, O_RDWR);
                if(ttyfd == -1)
                {
                    fprintf(stderr, "Opening %s", tty);
                    perror(" failed");
                }
                else
                {
                    promptlen = snprintf(promptmsg, sizeof(promptmsg), "%s: password for %s: ", *argv, conf.user);
                    if(promptlen >= sizeof promptmsg)
                    {
                        promptptr = malloc(promptlen + 1);
                        snprintf(promptptr, promptlen + 1, "%s: password for %s: ", *argv, conf.user);
                        write(ttyfd, promptptr, promptlen);
                    }
                    else
                        write(ttyfd, promptmsg, promptlen);
                    auth = verify(ttyfd, conf.user);
                    close(ttyfd);
                }
            }
            else
                auth = 1;
            if(auth)
            {
                pid = execmd(argv[1], argv + 1);
                if(conf.persist > 0)
                {
                    succ = savetime(ttybase, timeout);
                    if(succ == 1)
                        perror("Saving timeout failed");
                    else if(succ == 13)
                        fputs("TTY name too long.\n", stderr);
                }
                if(pid == 0)
                {
                    fprintf(stderr, "Executing command %s", argv[1]);
                    perror(" failed");
                    fputs("Some commands are part of your shell and not executable programs.\n", stderr);
                }
                else
                    waitpid(pid, &status, 0);
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
