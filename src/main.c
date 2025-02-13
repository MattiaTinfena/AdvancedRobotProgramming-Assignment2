#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>       
#include "auxfunc.h"

#define ask 0       
#define receive 1

#define read 0
#define write 1 

#define PROCESSNUM 4
#define PIPEXPROCESS 2

#define DRONE 0        
#define INPUT 1        
#define OBSTACLE 2        
#define TARGET 3 
#define BLACKBOARD 4
#define WATCHDOG 5

int main() {

    // Creazione delle pipe
    int pipes[PROCESSNUM][PIPEXPROCESS][2];

    for (int i = 0; i < PROCESSNUM; i++) {
        for (int j = 0; j < 2; j++){
            if (pipe(pipes[i][j]) == -1) {          
                perror("Pipe creation error");      
                exit(1);
            } 
        }
    }

    char fd_str[PROCESSNUM][50];        
    char test[15];                      
    
    // Inizializzare fd_str a 0 per concatenare i file descriptor
    for (int i = 0; i < PROCESSNUM; i++) {    
        sprintf(fd_str[i], "%d", 0);
        strcat(fd_str[i], ",");
    }

    // Riempimento di fd_str con i valori nelle pipe (separati da virgole)
    for (int i = 0; i < PROCESSNUM; i++) {
        for (int j = 0; j < PIPEXPROCESS; j++) {
            for (int k = 0; k < 2; k++) {
                sprintf(test, "%d", pipes[i][j][k]);
                strcat(fd_str[i], test);
                strcat(fd_str[i], ",");
            }
        }
    }

    FILE *passParamFile = fopen("log/passParam.txt", "w");
    if (passParamFile == NULL) {
        perror("Errore nell'apertura del file passParam");
        exit(1);
    }

    pid_t pids[PROCESSNUM];   

    int choice;
    printf("Press 1 for the server and 2 for the clien: ");
    fflush(stdout);
    scanf("%d", &choice);

    if (choice == 1) {
        pids[BLACKBOARD] = fork();
        if (pids[BLACKBOARD] == 0) {
            char *argb[] = { "konsole", "-e", "./bin/blackBoard", fd_str[INPUT], fd_str[DRONE], NULL };
            execvp(argb[0], argb);
            perror("Error in execvp for blackBoard");
            exit(1);
        }

        pids[DRONE] = fork();
        if (pids[DRONE] == 0) {
            char *argd[] = { "./bin/drone", fd_str[DRONE], NULL };
            execvp(argd[0], argd);
            perror("Errore in execvp per drone");
            exit(1);
        }

        pids[INPUT] = fork();
        if (pids[INPUT] == 0) {
            char *argi[] = { "konsole", "-e", "./bin/input", fd_str[INPUT], NULL }; 
            execvp(argi[0], argi);
            perror("Errore in execvp per input");
            exit(1);
        }
    } else if (choice == 2) {
        pids[TARGET] = fork();
        if (pids[TARGET] == 0) {
            char *argt[] = { "./bin/target", fd_str[TARGET], NULL };
            execvp(argt[0], argt);
            perror("Errore in execvp per target");
            exit(1);
        }

        pids[OBSTACLE] = fork();
        if (pids[OBSTACLE] == 0) {
            char *argo[] = { "./bin/obstacle", fd_str[OBSTACLE], NULL };
            execvp(argo[0], argo);
            perror("Errore in execvp per obstacle");
            exit(1);
        }
    }

    // Fork for watchdog
    pid_t pidwd = fork();

    if (pidwd < 0) {
        perror("Watchdog fork error");
        exit(1);
    } else if (pidwd == 0) { 
        char *argw[] = {"./bin/watchdog", NULL };
        execvp(argw[0], argw);
        perror("Error in execvp for watchdog");
        exit(1);
    }

    if (choice == 1) {
        for (int i = 0; i < 4; i++) { 
            wait(NULL);
        }    
    } else if (choice == 2) {
        for (int i = 0; i < 3; i++) { 
            wait(NULL);
        }
    }

    fclose(passParamFile);

    return 0;
}
