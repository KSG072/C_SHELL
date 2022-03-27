#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

//create bool type, without include <stdbool.h>
typedef int bool;
#define true  1
#define false 0

#define WRITE 1
#define READ  0

#define MAX_LINE 80 /*The maximum length command*/

int main(void){
    char *args[MAX_LINE/2 + 1] = { NULL, };//command array
    char *pipe_args[MAX_LINE/2 + 1] = { NULL, };//command array to the right of '|'
    char input[MAX_LINE] = { NULL, };//initial input
    char *file_name;//file name in commands
    char *sep;//seperator for parsing
    int i;// i for parsing
    int status;
    int file; // iterator of open(), close()
    bool should_run = true;
    bool write_operation; //if '>' serves, it will be true.
    bool read_operation; //if '<' serves, it will be true.
    bool ampersand; //if '&' serves, it is will be true.
    bool pipe_operation;//if '|' serves, it will be true.
    pid_t pid;

    while(should_run){
        //initialization
        memset(args, NULL, (MAX_LINE/2+1)*sizeof(char*));
        memset(pipe_args, NULL, (MAX_LINE/2+1)*sizeof(char*));
        memset(input, NULL, (MAX_LINE)*sizeof(char));
        i = 0;
        file = 0;
        write_operation = false;
        read_operation = false;
        ampersand = false;
        pipe_operation = false;
        printf("osh>");
        fflush(stdout);
        //end initialization

        //input
        fgets(input, MAX_LINE, stdin);
        input[strlen(input)-1] = '\0';

        //parsing and confirming input
        sep = strtok(input, " ");
        while (sep != NULL) {
            if (strchr(sep,'<')) {
                read_operation = true;
                file_name = strtok(NULL," ");
                break;
            }
            if (strchr(sep, '>')) {
                write_operation = true;
                file_name = strtok(NULL, " ");
                break;
            }
            if (strchr(sep, '&')) {
                ampersand = true;
                break;
            }
            if (strchr(sep, '|')) {
                i = 0;
                pipe_operation = true;
                sep = strtok(NULL, " ");//'|'날리기
                while (sep != NULL) {
                    if (strchr(sep,'<')) {
                        read_operation = true;
                        file_name = strtok(NULL," ");
                        break;
                    }
                    if (strchr(sep, '>')) {
                        write_operation = true;
                        file_name = strtok(NULL, " ");
                        break;
                    }
                    if (strchr(sep, '&')) {
                        ampersand = true;
                        break;
                    }
                    pipe_args[i++] = sep;
                    sep = strtok(NULL, " ");
                }
                break;
            }
            args[i++] = sep;
            sep = strtok(NULL, " ");
        }//end parsing and confirming

        //the end
        if (strcmp(args[0],"exit") == 0) {
            should_run = false;
        }
        //executing block
        else {
            if (strcmp(args[0], "cd") == 0) {//cd
                chdir(args[1]);
            }
            else {//except cd
                //make child process
                pid = fork();

                if (pid < 0) {
                    fprintf(stderr, "FORK ERROR");
                    return 1;
                } else if (pid == 0) {//child
                    if (read_operation) {// < operator
                        file = open(file_name, O_CREAT | O_RDONLY);
                        if (file == -1) {
                            fprintf(stderr, "FILE_OPEN_FAILED");
                            return 2;
                        }
                        dup2(file, STDIN_FILENO);
                    }//end < operator
                    else if (write_operation) {// > operator
                        file = open(file_name, O_CREAT | O_WRONLY, 0755);
                        if (file == -1) {
                            fprintf(stderr, "FILE_OPEN_FAILED");
                            return 2;
                        }
                        dup2(file, STDOUT_FILENO);
                    }//end > operator
                    else if (pipe_operation) {// using pipe
                        int fd[2];
                        if(pipe(fd) == -1) {
                            fprintf(stderr, "PIPE ERROR");
                            return 3;
                        }
                        pid = fork();
                        if (pid < 0) {
                            fprintf(stderr, "FORK ERROR IN CONDUCTING PIPE");
                            return 4;
                        }
                        else if (pid == 0) {//자식의 출력방향을 바꿈
                            close(fd[READ]);
                            dup2(fd[WRITE], STDOUT_FILENO);
                        }
                        else if (pid > 0) {//부모의 입력방향을 바꿈
                            close(fd[WRITE]);
                            dup2(fd[READ], STDIN_FILENO);
                            waitpid(pid, &status, 0);
                            execvp(pipe_args[0],pipe_args);
                            exit(1);
                        }
                    }// end using pipe
                    execvp(args[0], args);
                    exit(1);
                }//end child
                else {//parent
                    if(ampersand){
                        waitpid(pid, &status, WNOHANG);
                    }
                    else{
                        waitpid(pid, &status, 0);
                    }//end parent
                }//end parent
            }//end execept cd
        }//end execuing block
    }//end while
    return 0;
}