#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "arraylist.h"
#include <ctype.h>
#include <sys/wait.h>

#ifndef BUFSIZE
#define BUFSIZE 256
#endif

void cd(char *pathname);
char *pwd();
char *which(char *program);
void exitShell();
char *killShell(arraylist_t args);
arraylist_t readLine(int fd);
int generalCommands(arraylist_t args, int fd);

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
    arraylist_t args;
    if (isatty(fd))
    {
        puts("Welcome to my shell!");
        int ok = 1;
        while (ok == 1)
        {
            printf("mysh> ");
            fflush(stdout);
            generalCommands(args, fd);
        }
    }
    else
    {
        generalCommands(args, fd);
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
        if (newLine == 1)
        {
            word = NULL;
            break;
        }
    }

    if (wordlen > 0 && word != NULL)
    {
        word[wordlen] = '\0';
        al_push(&line, word);
    }
    al_push(&line, NULL);
    return line;
}

void cd(char *pathname)
{
    int check = chdir(pathname);
    if (check < 0)
    {
        fprintf(stderr, "Failed to change directory\n");
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
}

void exitShell()
{
    puts("mysh: exiting");
    exit(EXIT_SUCCESS);
}

char *killShell(arraylist_t args)
{
    int len = strlen(args.data[0]);
    char *res = malloc(len + 1);
    if (args.len - 1 > 1)
    {
        for (int i = 1; i < args.len - 1; i++)
        {
            res = realloc(res, len + strlen(args.data[i]));
            res = strcat(res, args.data[i]);
        }
        return res;
    }
    return NULL;
}

int generalCommands(arraylist_t args, int fd)
{
    args = readLine(fd);
    if (args.len - 1 == 0)
    {
        exit(1);
    }

    char *cmd = args.data[0];
    if (strcmp(cmd, "#") == 0)
    {
        puts("comment");
        return 1;
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
        if (args.len - 1 != 2)
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
    else if (strcmp(cmd, "exit") == 0)
    {
        exitShell();
    }
    else
    {
        int redirectOut = 0;
        int redirectIn = 0;
        int pipeCheck = 0;
        int conditional = 0;
        arraylist_t args2;
        al_init(&args2, 10);
        char *fileName;
        int fd2;
        for (int i = 0; i < args.len - 1; i++)
        {
            char *s = args.data[i];
            if (strcmp(s, ">") == 0)
            {
                fileName = args.data[i + 1];
                redirectOut = 1;
                for (int j = i - 2; j < args.len; j++)
                {
                    char **s = malloc(sizeof(char **));
                    al_pop(&args, s);
                    free(s);
                }
                al_push(&args, NULL);
                // break;
            }
            if (strcmp(s, "<") == 0)
            {
                fileName = args.data[i + 1];
                redirectIn = 1;
                for (int j = i - 2; j < args.len; j++)
                {
                    char **s = malloc(sizeof(char **));
                    al_pop(&args, s);
                    puts("popped");
                    free(s);
                }
                al_push(&args, NULL);
                // break;
            }
            if (strcmp(s, "|") == 0)
            {
                pipeCheck = 1;
                char **temp = malloc(sizeof(char **));
                al_pop(&args, temp);
                free(temp);
                for (int j = i + 1; j < args.len; j++)
                {
                    char *s = args.data[j];
                    al_push(&args2, s);
                }
                for (int i = args.len - 1; i > args2.len - 1; i--)
                {
                    temp = malloc(sizeof(char **));
                    al_pop(&args, temp);
                    free(temp);
                }
                al_push(&args2, NULL);
                al_push(&args, NULL);
                char *cmd1 = args.data[0];
                char *cmd2 = args2.data[0];
                char *path1 = which(cmd1);
                char *path2 = which(cmd2);
                int pfd[2];
                pipe(pfd);

                if (fork() == 0)
                {
                    dup2(pfd[1], STDOUT_FILENO);
                    close(pfd[1]);
                    close(pfd[0]);

                    execv(path1, args.data);
                    perror(path1);
                    exit(EXIT_FAILURE);
                }

                if (fork() == 0)
                {
                    dup2(pfd[0], STDIN_FILENO);
                    close(pfd[0]);
                    close(pfd[1]);

                    execv(path2, args2.data);
                    perror(path2);
                    exit(EXIT_FAILURE);
                }

                close(pfd[1]);
                close(pfd[0]);

                wait(NULL);
                wait(NULL);
                return 0;
            }
            if (strcmp(s, "or") == 0)
            {
                arraylist_t newArgs;
                al_init(&newArgs, 10);
                for (int j = i + 1; j < args.len - 1; j++)
                {
                    char *s = args.data[j];
                    al_push(&newArgs, s);
                }
                char **temp = malloc(sizeof(char **));
                al_pop(&args, temp);
                free(temp);
                for (int i = args.len - 1; i > newArgs.len - 1; i--)
                {
                    temp = malloc(sizeof(char **));
                    al_pop(&args, temp);
                    free(temp);
                }

                al_push(&newArgs, NULL);
                al_push(&args, NULL);
                char *cmd1 = args.data[0];
                char *cmd2 = newArgs.data[0];

                char *path1 = which(cmd1);
                char *path2 = which(cmd2);
                pid_t child = fork();
                if (child == 0)
                {
                    execv(path1, args.data);
                    perror(path1);
                    exit(EXIT_FAILURE);
                }

                int status;
                child = wait(&status);
                if (WIFSIGNALED(status))
                {
                    pid_t child = fork();
                    if (child == 0)
                    {
                        execv(path2, newArgs.data);
                        perror(path2);
                        exit(EXIT_FAILURE);
                    }
                }
                wait(NULL);
                return 0;
            }
            else if (strcmp(s, "and") == 0)
            {
                arraylist_t newArgs;
                al_init(&newArgs, 10);
                for (int j = i + 1; j < args.len - 1; j++)
                {
                    char *s = args.data[j];
                    al_push(&newArgs, s);
                }

                char **temp = malloc(sizeof(char **));
                al_pop(&args, temp);
                free(temp);
                for (int i = args.len - 1; i > newArgs.len - 1; i--)
                {
                    temp = malloc(sizeof(char **));
                    al_pop(&args, temp);
                    free(temp);
                }
                al_push(&newArgs, NULL);
                al_push(&args, NULL);

                char *cmd1 = args.data[0];
                char *cmd2 = newArgs.data[0];

                char *path1 = which(cmd1);
                char *path2 = which(cmd2);
                pid_t child = fork();
                if (child == 0)
                {
                    execv(path1, args.data);
                    perror(path1);
                    exit(EXIT_FAILURE);
                }

                int status;
                child = wait(&status);
                if (WIFEXITED(status))
                {
                    pid_t child = fork();
                    if (child == 0)
                    {
                        execv(path2, newArgs.data);
                        perror(path2);
                        exit(EXIT_FAILURE);
                    }
                }
                wait(NULL);
                return 0;
            }
        }

        char *cmdName = args.data[0];
        char *pathName = which(cmdName);
        pid_t child = fork();
        if (child == 0)
        {
            if (redirectOut == 1)
            {
                fd2 = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, 0640);
                if (dup2(fd2, STDOUT_FILENO) == -1)
                {
                    exit(1);
                }
                close(fd2);
            }
            else if (redirectIn == 1)
            {
                fd2 = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, 0640);
                if (dup2(fd2, STDIN_FILENO) == -1)
                {
                    exit(1);
                }
                close(fd2);
            }
            execv(pathName, args.data);
            perror(pathName);
            exit(EXIT_FAILURE);
        }
        int status;
        child = wait(&status);

        al_destroy(&args2);
    }
    if (args.len - 1 != 0)
    {
        for (int i = 0; i < args.len - 1; i++)
        {
            free(args.data[i]);
        }
    }
    al_destroy(&args);
}