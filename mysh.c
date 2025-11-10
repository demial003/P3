#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "arraylist.h"
#include <ctype.h>

#ifndef BUFSIZE
#define BUFSIZE 256;
#endif

arraylist_t readLine(int fd);
int main(int argc, char **argv)
{

    int fd = 0;
    if (argc == 1)
    {
        fd = STDIN_FILENO;
    }
    else
    {
        fd = open(argv[1], O_RDONLY);
        if (fd < 0)
        {
            exit(1);
        }
    }

    if (isatty(fd))
    {
        arraylist_t args;
        puts("Welcome to my shell!");
        int ok = 1;
        while (ok)
        {
            printf("mysh> ");
            fflush(stdout);
            args = readLine(fd);
            if (strcmp(args.data[0], "#") == 0)
            {
                puts("comment");
                continue;
            }
            else if (strcmp(args.data[0], "exit") == 0)
            {
                puts("mysh: exiting");
                exit(0);
            }
            else if (strcmp(args.data[0], "die") == 0)
            {
                if (args.len > 1)
                {
                    for (int i = 1; i < args.len; i++)
                    {
                        char *word = args.data[i];
                        puts(word);
                    }
                }
                exit(0);
            }
            for (int i = 0; i < args.len; i++)
            {
                char *word = args.data[i];

                if (strcmp(word, "<") == 0 || strcmp(word, ">") == 0)
                {
                    puts("redirect");
                    break;
                }
                else if (strcmp(word, "|") == 0)
                {
                    puts("pipe");
                    break;
                }
                puts(word);
            }
        }
        al_destroy(&args);
    }
    else
    {
    }
    return 0;
}

arraylist_t readLine(int fd)
{
    char buf[256];
    char *word = NULL;
    int bytes = 0;
    int wordlen = 0;
    arraylist_t line;
    al_init(&line, 10);
    while ((bytes = read(fd, buf, 256)) > 0)
    {
        int segstart = 0;
        int pos;
        for (pos = 0; pos < bytes; pos++)
        {
            if (isspace(buf[pos]) || buf[pos] == '\n')
            {
                int seglen = pos - segstart;
                word = realloc(word, wordlen + seglen + 1);
                if (word == NULL)
                {
                    exit(1);
                }
                memcpy(word + wordlen, buf + segstart, seglen);
                word[wordlen + seglen] = '\0';

                // puts(word);
                al_push(&line, word);
                wordlen = 0;
                word = NULL;
                segstart = pos + 1;
            }
        }
        if (segstart < pos)
        {
            int seglen = pos - segstart;
            word = realloc(word, wordlen + seglen + 1);
            memcpy(word + wordlen, buf + segstart, seglen);
            word[wordlen + seglen] = '\0';
            wordlen = wordlen + seglen;
        }
        free(word);
        return line;
    }
}

