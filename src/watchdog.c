#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include "auxfunc.h"
#include <signal.h>
#include <time.h>
#include "watchdog.h"

#define PROCESSTOCONTROL 5

int pids[PROCESSTOCONTROL] = {0, 0, 0, 0, 0};
long times[PROCESSTOCONTROL] = {0, 0, 0, 0, 0};

FILE *wdFile;

int main() {

    wdFile = fopen("log/watchdog.log", "w");
    if (wdFile == NULL) {
        perror("Error opening the wdFile");
        exit(1);
    }

    int pid = (int)getpid();
    char dataWrite[80];
    snprintf(dataWrite, sizeof(dataWrite), "w%d,", pid);

    if(writeSecure("log/passParam.txt", dataWrite, 'a') == -1){
        perror("[WATCHDOG] Error in writing in passParam.txt");
        exit(1);
    }

    sleep(1);

    char datareaded[200];
    if (readSecure("log/passParam.txt", datareaded, sizeof(datareaded)) == -1) {
        perror("[WD1] Error reading the passParam wdFile");
        exit(1);
    }

    fprintf(wdFile, "%s\n", datareaded);
    fflush(wdFile);

    // Parse the data and assign roles
    char *token = strtok(datareaded, ",");
    while (token != NULL) {
        char type = token[0];          // Get the prefix
        int number = atoi(token + 1);  // Convert the number part to int

        if (type == 'i') {
            pids[INPUT] = number;
        } else if (type == 'd') {
            pids[DRONE] = number;
        } else if (type == 'o') {
            pids[OBSTACLE] = number;
        } else if (type == 't') {
            pids[TARGET] = number;
        } else if (type == 'b') {
            pids[BLACKBOARD] = number;
        }

        token = strtok(NULL, ",");
    }

    // Write the PID values to the output wdFile
    for (int i = 0; i < PROCESSTOCONTROL; i++) {
        fprintf(wdFile, "pid[%d] = %d\n", i, pids[i]);
        fflush(wdFile);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Error while setting sigaction for SIGTERM");
        exit(EXIT_FAILURE);
    }

    char reset[2] = "!\0";
    if(writeSecure("log/passParam.txt", reset, 'w') == -1){
        perror("[WATCHDOG] Error in writing in passParam.txt");
        exit(1);
    }

    for (int i = 0; i < PROCESSTOCONTROL; i++) {
        if (pids[i] != 0){
            if (kill(pids[i], SIGUSR1) == -1) {
                LOG_PROCESS_NOT_RESPONDING(pids[i]);
                closeAll(i);
            }
            usleep(10000);
        }
    }

    int interval = 0;

    while (1) {
        sleep(1);

        interval++;

        if(interval >= 4){
            interval = 0;

            char reset[2] = "!\0";
            if(writeSecure("log/passParam.txt", reset, 'w') == -1){
                perror("[WATCHDOG] Error in writing in passParam.txt");
                exit(1);
            }

            for (int i = 0; i < PROCESSTOCONTROL; i++) {
                if (pids[i] != 0){
                    if (kill(pids[i], SIGUSR1) == -1) {
                        LOG_PROCESS_NOT_RESPONDING(pids[i]);
                        closeAll(i);
                    }
                    usleep(10000);
                }
            }
        }

        usleep(10000);
        time_t rawtime;
        struct tm *timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);

        char timeReaded[200];

        if (readSecure("log/passParam.txt", timeReaded, sizeof(timeReaded)) == -1) {
            perror("[WD1] Error reading the passParam wdFile");
            exit(1);
        }

        memset(times, 0, sizeof(times));

        char *token = strtok(timeReaded, ",");

        while (token != NULL) {
            if (token[0] == '!') {
                token++;
            }

            char letter = token[0];
            int hh, mm, ss;


            sscanf(token + 1, "%d:%d:%d", &hh, &mm, &ss);
            long pidtime = convertToSeconds(hh, mm, ss);


            if (letter == 'i') {
                times[INPUT] = pidtime;
            } else if (letter == 'd') {
                times[DRONE] = pidtime;
            } else if (letter == 'o') {
                times[OBSTACLE] = pidtime;
            } else if (letter == 't') {
                times[TARGET] = pidtime;
            } else if (letter == 'b') {
                times[BLACKBOARD] = pidtime;
            }

            token = strtok(NULL, ",");
        }


        for(int i = 0; i < PROCESSTOCONTROL; i++){
            if (pids[i] != 0) {
                long currentTimeInSeconds = convertToSeconds(timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
                long timeDifference = currentTimeInSeconds - times[i];


                if (timeDifference > 5) {
                    closeAll(i);
                }
            }
        }
    }

    // Close the wdFile
    fclose(wdFile);

    return 0;
}


void sig_handler(int signo) {
    if(signo == SIGTERM){
        LOGWDDIED();
        fclose(wdFile);
        exit(EXIT_SUCCESS);
    }
}

void closeAll(int id){
    for(int i  = 0; i < PROCESSTOCONTROL; i++){
        if (pids[i] == 0) continue;
        if (i != id) {
            if (kill(pids[i], SIGTERM) == -1) {
                LOGPROCESSDIED(pids[i]);
            }
        }
    }
    LOGWDDIED();
    fclose(wdFile);
    exit(EXIT_SUCCESS);
}

long convertToSeconds(int hh, int mm, int ss) {
    return hh * 3600 + mm * 60 + ss;
}