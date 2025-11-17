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
static void cleanup(arraylist_t *args, arraylist_t *a2, arraylist_t *a3);

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
            char c = buf[pos];

            if (c == '#')
            {
                while (pos < bytes && buf[pos] != '\n')
                    pos++;
                if (pos == bytes)
                    break;
                c = buf[pos];
            }

            if (c == '\n')
            {
                if (wordlen > 0)
                {
                    word = realloc(word, wordlen + 1);
                    word[wordlen] = '\0';
                    al_push(&line, word);
                    word = NULL;
                    wordlen = 0;
                }
                newLine = 1;
                break;
            }

            if (isspace((unsigned char)c))
            {
                if (wordlen > 0)
                {
                    word = realloc(word, wordlen + 1);
                    word[wordlen] = '\0';
                    al_push(&line, word);
                    word = NULL;
                    wordlen = 0;
                }
                continue;
            }

            word = realloc(word, wordlen + 2);
            word[wordlen++] = c;
            word[wordlen] = '\0';
        }
        if (!newLine && wordlen > 0)
        {
            continue;
        }

        if (newLine)
        {
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
        fprintf(stderr, "mysh: cd: %s: No such file or directory\n", pathname);
    }
}

char *pwd()
{
    char *currentdir = malloc(4096);
    getcwd(currentdir, 4096);
    return currentdir;
}

char *which(char *program)
{
    char *paths[] = {"/usr/local/bin", "/usr/bin", "/bin", NULL};

    for (int i = 0; paths[i] != NULL; i++)
    {
        int plen = strlen(program);
        int dlen = strlen(paths[i]);
        int total = dlen + 1 + plen + 1; 

        char *res = malloc(total);
        if (!res)
            return NULL;

        strcpy(res, paths[i]);
        strcat(res, "/");
        strcat(res, program);

        if (access(res, F_OK) == 0)
            return res;

        free(res);
    }

    return NULL; 
}

void exitShell()
{
    puts("mysh: exiting");
    exit(EXIT_SUCCESS);
}

char *killShell(arraylist_t args)
{
    if (args.len <= 2)
        return NULL;

    int total = 1;
    for (int i = 1; i < args.len - 1; i++)
        total += strlen(args.data[i]) + 1;

    char *res = malloc(total);
    res[0] = '\0';

    for (int i = 1; i < args.len - 1; i++)
    {
        strcat(res, args.data[i]);
        if (i < args.len - 2)
            strcat(res, " ");
    }

    return res;
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
        if (args.len < 2)
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
        if (res)
        {
            puts(res);
            free(res);
        }
        else
        {
            fprintf(stderr, "Program not found\n");
        }
    }
    else if (strcmp(cmd, "exit") == 0)
    {
        exitShell();
    }
    else
    {
        int redirectOut = 0;
        int redirectIn = 0;
        int conditional = 0;
        arraylist_t args2;
        al_init(&args2, 10);
        char *fileName;
        int fd2;
        arraylist_t newArgs;
        al_init(&newArgs, 10);
        for (int i = 0; i < args.len - 1; i++)
        {
            char *s = args.data[i];
            if (strcmp(s, ">") == 0)
            {
                if (i + 1 >= args.len - 1)
                {
                    fprintf(stderr, "mysh: syntax error near unexpected token `newline'\n");
                    cleanup(&args, &args2, &newArgs);
                    return 0;
                }

                fileName = args.data[i + 1];
                redirectOut = 1;

                al_remove(&args, i);
                al_remove(&args, i);
                // al_push(&args, NULL);

                break;
            }

            if (strcmp(s, "<") == 0)
            {
                if (i + 1 >= args.len - 1)
                {
                    fprintf(stderr, "mysh: syntax error near unexpected token `newline'\n");
                    cleanup(&args, &args2, &newArgs);
                    return 0;
                }

                fileName = args.data[i + 1];
                redirectIn = 1;

                al_remove(&args, i);
                al_remove(&args, i);
                // al_push(&args, NULL);

                break;
            }

            if (strcmp(s, "|") == 0)
            {

                for (int j = i + 1; j < args.len - 1; j++)
                {
                    al_push(&args2, args.data[j]);
                }
                al_push(&args2, NULL);

                while (args.len - 1 > i)
                {
                    char *tmp;
                    al_pop(&args, &tmp);
                }
                args.data[i] = NULL;
                char *cmd1 = args.data[0];
                char *cmd2 = args2.data[0];
                char *path1 = which(cmd1);
                char *path2 = which(cmd2);
                if (!path1 || !path2)
                {
                    if (!path1)
                    {
                        fprintf(stderr, "%s: Program not found\n", cmd1);
                    }
                    if (!path2)
                    {
                        fprintf(stderr, "%s: Program not found\n", cmd2);
                    }
                    cleanup(&args, &args2, &newArgs);
                    return 0;
                }

                int pfd[2];
                pipe(pfd);

                if (fork() == 0)
                {
                    dup2(pfd[1], STDOUT_FILENO);
                    close(pfd[1]);
                    close(pfd[0]);
                    execv(path1, args.data);
                    perror(path1);
                    exit(1);
                }

                if (fork() == 0)
                {
                    dup2(pfd[0], STDIN_FILENO);
                    close(pfd[0]);
                    close(pfd[1]);
                    execv(path2, args2.data);
                    perror(path2);
                    exit(1);
                }

                close(pfd[0]);
                close(pfd[1]);

                wait(NULL);
                wait(NULL);
                free(path1);
                free(path2);

                cleanup(&args, &args2, &newArgs);
                return 0;
            }

            if (strcmp(s, "or") == 0)
            {
                for (int j = i + 1; j < args.len - 1; j++)
                {
                    al_push(&newArgs, args.data[j]);
                }
                al_push(&newArgs, NULL);

                while (args.len - 1 > i)
                {
                    char *tmp;
                    al_pop(&args, &tmp);
                }
                args.data[i] = NULL;

                char *cmd1 = args.data[0];
                char *cmd2 = newArgs.data[0];
                char *path1 = which(cmd1);
                char *path2 = which(cmd2);
                if (!path1 || !path2)
                {
                    if (!path1)
                    {
                        fprintf(stderr, "%s: Program not found\n", cmd1);
                    }
                    if (!path2)
                    {
                        fprintf(stderr, "%s: Program not found\n", cmd2);
                    }
                    cleanup(&args, &args2, &newArgs);
                    return 0;
                }

                pid_t c = fork();
                if (c == 0)
                {
                    execv(path1, args.data);
                    perror(path1);
                    exit(1);
                }

                int status;
                wait(&status);

                if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
                {
                    pid_t c2 = fork();
                    if (c2 == 0)
                    {
                        execv(path2, newArgs.data);
                        perror(path2);
                        exit(1);
                    }
                    wait(NULL);
                }
                cleanup(&args, &args2, &newArgs);
                return 0;
            }

            else if (strcmp(s, "and") == 0)
            {
                for (int j = i + 1; j < args.len - 1; j++)
                    al_push(&newArgs, args.data[j]);
                al_push(&newArgs, NULL);

                while (args.len - 1 > i)
                {
                    char *tmp;
                    al_pop(&args, &tmp);
                }
                args.data[i] = NULL;

                char *cmd1 = args.data[0];
                char *cmd2 = newArgs.data[0];
                char *path1 = which(cmd1);
                char *path2 = which(cmd2);
                if (!path1 || !path2)
                {
                    if (!path1)
                    {
                        fprintf(stderr, "%s: Program not found\n", cmd1);
                    }
                    if (!path2)
                    {
                        fprintf(stderr, "%s: Program not found\n", cmd2);
                    }
                    cleanup(&args, &args2, &newArgs);
                    return 0;
                }

                pid_t c = fork();
                if (c == 0)
                {
                    execv(path1, args.data);
                    perror(path1);
                    exit(1);
                }

                int status;
                wait(&status);

                if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                {
                    pid_t c2 = fork();
                    if (c2 == 0)
                    {
                        execv(path2, newArgs.data);
                        perror(path2);
                        exit(1);
                    }
                    wait(NULL);
                }
                free(path1);
                free(path2);
                cleanup(&args, &args2, &newArgs);
                return 0;
            }
        }

        char *cmdName = args.data[0];
        char *pathName = which(cmdName);
        if (!pathName)
        {
            fprintf(stderr, "%s: Program not found\n", cmdName);
            cleanup(&args, &args2, &newArgs);
            return 0;
        }
        pid_t child = fork();
        if (child == 0)
        {
            if (redirectOut == 1)
            {
                fd2 = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, 0644);
                if (dup2(fd2, STDOUT_FILENO) == -1)
                {
                    perror(fileName);
                    exit(1);
                }
                close(fd2);
            }
            else if (redirectIn == 1)
            {
                fd2 = open(fileName, O_RDONLY);
                if (fd2 < 0)
                {
                    perror(fileName);
                    exit(1);
                }
                dup2(fd2, STDIN_FILENO);
                close(fd2);
            }
            execv(pathName, args.data);
            perror(pathName);
            exit(EXIT_FAILURE);
        }
        int status;
        child = wait(&status);

        free(pathName);
        cleanup(&args, &args2, &newArgs);
        return 0;
    }
}

static void cleanup(arraylist_t *args, arraylist_t *a2, arraylist_t *a3)
{
    if (a2 && a2->data)
    {
        free(a2->data);
        a2->data = NULL;
        a2->len = 0;
        a2->cap = 0;
    }

    if (a3 && a3->data)
    {
        free(a3->data);
        a3->data = NULL;
        a3->len = 0;
        a3->cap = 0;
    }

    if (args && args->data)
    {
        for (int i = 0; i < (int)args->len - 1; ++i)
        {
            free(args->data[i]);
        }

        al_destroy(args);
        args->data = NULL;
        args->len = 0;
        args->cap = 0;
    }
}
