#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "arraylist.h"
#include <ctype.h>

#ifndef BUFSIZE
#define BUFSIZE 256
#endif

void cd(char *pathname);
char *pwd();
char *which(char *program);
void exitShell();
char *killShell(arraylist_t args);
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
            if (args.len == 0)
            {
                exit(1);
            }

            char *cmd = args.data[0];
            if (strcmp(cmd, "#") == 0)
            {
                puts("comment");
                continue;
            }
            else if (strcmp(cmd, "die") == 0)
            {
                char *res = killShell(args);
                if (res != NULL)
                {
                    puts(res);
                    free(res);
                }
                exit(EXIT_FAILURE);
            }
            else if (strcmp(cmd, "cd") == 0)
            {
                if (args.len != 2)
                {
                    fprintf(stderr, "Invalid arguments\n");
                }
                else
                {
                    cd(args.data[1]);
                }
            }
            else if (strcmp(cmd, "pwd") == 0)
            {
                if (args.len > 2)
                {
                    fprintf(stderr, "Invalid arguments\n");
                }
                else
                {
                    char *res = pwd();
                    puts(res);
                    free(res);
                }
            }
            else if (strcmp(cmd, "which") == 0)
            {
                char *res = which(args.data[1]);
                puts(res);
                free(res);
            }

            for (int i = 0; i < args.len; i++)
            {
                char *word = args.data[i];
                if (strcmp(word, "exit") == 0)
                {
                    exitShell();
                }
                else if (strcmp(word, "<") == 0 || strcmp(word, ">") == 0)
                {
                    puts("redirect");
                    break;
                }
                else if (strcmp(word, "|") == 0)
                {
                    puts("pipe");
                    break;
                }
            }

            for (int i = 0; i < args.len; i++)
            {
                free(args.data[i]);
            }
            al_destroy(&args);
        }
    }
    else
    {
        // Non-interactive mode not implemented yet
    }

    return 0;
}

arraylist_t readLine(int fd)
{
    char buf[BUFSIZE];
    char *word = NULL;
    int bytes = 0;
    int wordlen = 0;
    int newLine = 0;
    arraylist_t line;
    al_init(&line, 10);

    while ((bytes = read(fd, buf, BUFSIZE)) > 0)
    {
        int segstart = 0;
        int pos;

        for (pos = 0; pos < bytes; pos++)
        {
            if (buf[pos] == '\n')
            {
                int seglen = pos - segstart;
                word = realloc(word, wordlen + seglen + 1);
                if (word == NULL)
                {
                    exit(1);
                }
                memcpy(word + wordlen, buf + segstart, seglen);
                word[wordlen + seglen] = '\0';
                al_push(&line, word);
                wordlen = 0;
                word = NULL;
                newLine = 1;
                break;
            }

            if (isspace(buf[pos]) && buf[pos] != '\n')
            {
                int seglen = pos - segstart;
                word = realloc(word, wordlen + seglen + 1);
                if (word == NULL)
                {
                    exit(1);
                }
                memcpy(word + wordlen, buf + segstart, seglen);
                word[wordlen + seglen] = '\0';
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
        if(newLine == 1) break;

    }
    return line;
}

void cd(char *pathname)
{
    int check = chdir(pathname);
    if (check < 0)
    {
        fprintf(stderr, "Failed to change directory\n");
        exit(1);
    }
}

char *pwd()
{
    char *currentdir = malloc(64);
    getcwd(currentdir, 64);
    return currentdir;
}

char *which(char *program)
{
    char *path1 = "/usr/local/bin";
    char *path2 = "/usr/bin";
    char *path3 = "/bin";
    int plen = strlen(program);
    int len = (int)strlen(path1);

    char *res = malloc(plen + len + 1);
    res = strcpy(res, path1);
    res = strcat(res, "/");
    res = strcat(res, program);

    if (access(res, F_OK) == 0)
    {
        return res;
    }

    free(res);
    len = strlen(path2);
    res = malloc(plen + len + 1);
    res = strcpy(res, path2);
    res = strcat(res, "/");
    res = strcat(res, program);

    if (access(res, F_OK) == 0)
    {
        return res;
    }

    free(res);
    len = strlen(path3);
    res = malloc(plen + len + 1);
    res = strcpy(res, path3);
    res = strcat(res, "/");
    res = strcat(res, program);

    if (access(res, F_OK) == 0)
    {
        return res;
    }

    fprintf(stderr, "Program not found\n");
    exit(1);
}

void exitShell()
{
    puts("mysh: exiting");
    exit(0);
}

char *killShell(arraylist_t args)
{
    int len = strlen(args.data[0]);
    char *res = malloc(len + 1);
    if (args.len > 1)
    {
        for (int i = 1; i < args.len; i++)
        {
            res = realloc(res, len + strlen(args.data[i]));
            res = strcat(res, args.data[i]);
        }
        return res;
    }
    return NULL;
}
