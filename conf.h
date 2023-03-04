#ifndef Included_conf_h
#define Included_conf_h

struct config
{
    long persist;
    char user[361];
    char upermit[2601], gpermit[2601];
};

int loadconf(struct config *conf);
char permitted(const struct config *conf);
long loadtime(const char *tty);
int savetime(const char *tty, long t);

#endif
