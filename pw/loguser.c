#include <fcntl.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

extern int read(int, void *, size_t);
extern uid_t getuid(void);
extern int write(int, const void *, size_t);
extern int close(int);

void loguser(char *log, char *text)
{
    int r, u, logf, textf;
    char buffy[BUFSIZ];

    if ((logf = open(log, O_RDWR)) >= 0) {
        while ((r = read(logf, &u, sizeof(u))) != 0)
            if (u == getuid())
                break;
        if (r == 0) {
            u = getuid();
            (void)write(logf, &u, sizeof(u));

            if ((textf = open(text, O_RDONLY)) >= 0) {
                while ((r = read(textf, buffy, sizeof(buffy))) != 0)
                    write(2, buffy, r);
                /*
                                fprintf(stderr, "\nhit return to continue\n");
                                while (getchar() != '\n')
                                    ;
                */
                (void)close(textf);
            }
            (void)close(logf);
        }
    }
}
