#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<unistd.h>
#include"conf.h"
#define USER "/etc/passwd"
#define GROUP "/etc/group"
#define CACHE "/tmp/MicroPrivilageEscalator"

void strls(char *ls)
{
    for(; *ls != '\0'; ++ls)
    {
        if(*ls == ',')
            *ls++ = '\0';
    }
    *++ls = '\0';
}

int loadconf(struct config *conf)
{
    char *userenv;
    size_t len;
    int succ = 0;
    FILE *fh = fopen(CONFIG, "r");
    if(fh == NULL)
        succ = 1;
    else
    {
        fscanf(fh, "%ld", &conf->persist);
        fscanf(fh, "%s", conf->user);
        fscanf(fh, "%s", conf->upermit);
        fscanf(fh, "%s", conf->gpermit);
        if(conf->user[0] == '$')
        {
            userenv = getenv(conf->user + 1);
            len = strlen(userenv);
            if(len < sizeof(conf->user))
                strcpy(conf->user, userenv);
            else
                succ = 13;
        }
        strls(conf->upermit);
        strls(conf->gpermit);
        fclose(fh);
    }
    return succ;
}

char permitted(const struct config *conf)
{
    char perm = 0;
    unsigned uid = getuid();
    return perm;
}

long loadtime(const char *tty)
{
    char path[2601];
    FILE *fh;
    long t = 0;
    size_t len = strlen(tty);
    if(len + 30 < sizeof(path))
    {
        fh = fopen(path, "r");
        if(fh != NULL)
        {
            fread(&t, 1, sizeof(t), fh);
            fclose(fh);
        }
    }
    return t;
}

int savetime(const char *tty, long t)
{
    char path[2601];
    size_t len;
    FILE *fh;
    int succ = 0;
    if(access(CACHE, F_OK))
    {
        if(mkdir(CACHE, 0755))
        {
            perror("Creating cache directory failed");
            succ = 1;
        }
    }
    len = strlen(tty);
    if(len + 30 < sizeof(path))
    {
        strcpy(path, CACHE);
        path[29] = '/';
        strcpy(path + 30, tty);
        fh = fopen(path, "w");
        if(fh == NULL)
            succ = 1;
        else
        {
            fwrite(&t, 1, sizeof(t), fh);
            fclose(fh);
        }
    }
    else
        succ = 13;
    return succ;
}
