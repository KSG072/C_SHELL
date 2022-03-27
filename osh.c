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

//for Convenient pipe operation
#define WRITE 1
#define READ  0

#define MAX_LINE 80 /*The maximum length command*/

typedef struct operator{
    bool write;
    bool read;
    bool ampersand;
    bool pipe_oper;
}operator;

void initialize(char* input, char** args, char** pipe_args, operator* opers, int* i, int* file);
void normalize(char* input, char** args, char** pipe_args, operator* opers, char** file_name);
bool check_oper(char* sep, operator* opers, char** file_name);

int main(void){
    char *args[MAX_LINE/2 + 1] = { NULL, };//command array
    char *pipe_args[MAX_LINE/2 + 1] = { NULL, };//command array to the right of '|'
    char input[MAX_LINE] = { NULL, };//initial input
    char *file_name;//file name in commands
    char *sep;//seperator for parsing
    int should_run = 1;
    int i;// i for parsing
    int status;
    int file; // iterator of open(), close()
    operator *opers = malloc(sizeof(operator));
    pid_t pid;

    while(should_run){
        //initialization
        initialize(input, args, pipe_args, opers, &i, &file);
        //end initialization

        //input
        fgets(input, MAX_LINE, stdin);
        input[strlen(input)-1] = '\0';

        //normalize input
        normalize(input, args, pipe_args, opers, &file_name);

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
                    if (opers->read) {// < operator
                        file = open(file_name, O_CREAT | O_RDONLY);
                        if (file == -1) {
                            fprintf(stderr, "FILE_OPEN_FAILED");
                            return 2;
                        }
                        dup2(file, STDIN_FILENO);
                    }//end < operator
                    else if (opers->write) {// > operator
                        file = open(file_name, O_CREAT | O_WRONLY, 0755);
                        if (file == -1) {
                            fprintf(stderr, "FILE_OPEN_FAILED");
                            return 2;
                        }
                        dup2(file, STDOUT_FILENO);
                    }//end > operator
                    else if (opers->pipe_oper) {// using pipe
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
                    if(opers->ampersand){
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

void initialize(char* input, char** args, char** pipe_args, operator* opers, int* i, int* file){
    memset(args, NULL, (MAX_LINE/2+1)*sizeof(char*));
    memset(pipe_args, NULL, (MAX_LINE/2+1)*sizeof(char*));
    memset(input, NULL, (MAX_LINE)*sizeof(char));
    *i = 0;
    *file = 0;
    opers->write = false;
    opers->read = false;
    opers->ampersand = false;
    opers->pipe_oper = false;
    printf("osh>");
    fflush(stdout);
}

void normalize(char* input, char** args, char** pipe_args, operator* opers, char** file_name) {
    char* sep;
    int i = 0;
    sep = strtok(input, " ");
    while (sep != NULL) {
        if (check_oper(sep, opers, file_name)){
            break;
        }
        if (strchr(sep, '|')) {
            i = 0;
            opers->pipe_oper = true;
            sep = strtok(NULL, " ");//'|'날리기
            while (sep != NULL) {
                if (check_oper(sep, opers, file_name)){
                    break;
                }
                pipe_args[i++] = sep;
                sep = strtok(NULL, " ");
            }
            break;
        }
        args[i++] = sep;
        sep = strtok(NULL, " ");
    }
}

bool check_oper(char* sep, operator* opers, char** file_name){
    bool result = false;
    if (strchr(sep,'<')) {
        opers->read = true;
        *file_name = strtok(NULL," ");
        result = true;
    }
    else if (strchr(sep, '>')) {
        opers->write = true;
        *file_name = strtok(NULL, " ");
        result = true;
    }
    else if (strchr(sep, '&')) {
        opers->ampersand = true;
        result = true;
    }
    return result;
}
