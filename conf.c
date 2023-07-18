#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<unistd.h>
#include"conf.h"
#ifndef CONFIG
#define CONFIG "/etc/mupe.conf"
#endif
#define USER "/etc/passwd"
#define GROUP "/etc/group"
#define CACHE "/tmp/MicroPrivilegeEscalator"

char lscontain(const char *restrict haystack, const char *restrict needle)
{
    char exists = 0;
    for(size_t len = strlen(haystack); len > 0 && !exists; len = strlen(haystack))
    {
        if(strcmp(haystack, needle) == 0)
            exists = 1;
        else
            haystack += len + 1;
    }
    return exists;
}

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
            if(conf->user[1] == '\0')
                succ = getnamebyid(getuid(), conf->user) * -1;
            else
            {
                userenv = getenv(conf->user + 1);
                len = strlen(userenv);
                if(len < sizeof(conf->user))
                    strcpy(conf->user, userenv);
                else
                    succ = 13;
            }
        }
        strls(conf->upermit);
        strls(conf->gpermit);
        fclose(fh);
    }
    return succ;
}

char permitted(const struct config *conf)
{
    char name[361];
    FILE *fh;
    char perm = 0;
    size_t ind = 0;
    unsigned colon = 0, uid = getuid();
    if(getnamebyid(uid, name))
        perror("Could not get user name");
    else
    {
        perm = lscontain(conf->upermit, name);
        if(!perm)
        {
            char group[361], member[361];
            fh = fopen(GROUP, "r");
            if(fh == NULL)
                perror("Could not read group information");
            else
            {
                for(int ch = fgetc(fh); !perm && ch > 0; ch = fgetc(fh))
                {
                    switch(ch)
                    {
                        case',':
                            if(colon == 3)
                            {
                                member[ind] = '\0';
                                if(strcmp(member, name) == 0)
                                    perm = lscontain(conf->gpermit, group);
                            }
                            break;
                        case':':
                            if(++colon == 1)
                                group[ind] = '\0';
                            ind = 0;
                            break;
                        case'\n':
                            member[ind] = '\0';
                            if(strcmp(member, name) == 0)
                                perm = lscontain(conf->gpermit, group);
                            colon = 0;
                            break;
                        default:
                            if(colon == 0)
                            {
                                if(ind < 360)
                                    group[ind++] = ch;
                            }
                            else if(colon == 3)
                            {
                                if(ind < 360)
                                    member[ind++] = ch;
                            }
                    }
                }
                fclose(fh);
            }
        }
    }
    return perm;
}

long loadtime(const char *tty)
{
    char path[2601];
    FILE *fh;
    long t = 0;
    size_t len = strlen(tty);
    if(len + 29 < sizeof(path))
    {
        strcpy(path, CACHE);
        path[28] = '/';
        strcpy(path + 29, tty);
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
    if(len + 29 < sizeof(path))
    {
        strcpy(path, CACHE);
        path[28] = '/';
        strcpy(path + 29, tty);
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

int getnamebyid(unsigned uid, char *name)
{
    int succ = 0;
    size_t ind = 0;
    unsigned colon = 0, entryid = 0;
    FILE *fh = fopen(USER, "r");
    if(fh == NULL)
        succ = -1;
    else
    {
        for(int ch = fgetc(fh); ch > 0; ch = fgetc(fh))
        {
            switch(ch)
            {
                case':':
                    ++colon;
                    break;
                case'\n':
                    if(entryid == uid)
                    {
                        name[ind] = '\0';
                        fseek(fh, 0, SEEK_END);
                    }
                    else
                    {
                        colon = 0;
                        ind = 0;
                        entryid = 0;
                    }
                    break;
                default:
                    if(colon == 0)
                    {
                        if(ind < 360)
                            name[ind++] = ch;
                    }
                    else if(colon == 2)
                        entryid = entryid * 10 + ch - '0';
            }
        }
    }
    return succ;
}
