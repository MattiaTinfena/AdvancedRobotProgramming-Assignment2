#ifndef TARGET_HPP
#define TARGET_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "auxfunc.h"

#define USE_DEBUG 1 // 0 for release, 1 for debug
extern FILE *targFile; //Global variable

//Functions definition
int canSpawnPrev(int x_pos, int y_pos);
void createTargets();
void sig_handler(int signo);
void readConfig();

/*********************************************************************************************************************/
/********************************************FUNCTIONS TO LOG*********************************************************/
/*********************************************************************************************************************/

#define LOGNEWMAP(target) {                                                      \
    if (!targFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
                                                                                 \
    fprintf(targFile, "%s New target generated.\n", date);                             \
    for (int t = 0; t < target.number; t++) {                                       \
        fprintf(targFile, "(%d, %d) ",                                  \
                targets.x[t], targets.y[t]); \
    }                                                                            \
    fprintf(targFile, "\n");                                                      \
    fflush(targFile);                                                             \
}


#define LOGPROCESSDIED() { \
    if (!targFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                           \
    fprintf(targFile, "%s Process dead. Obstacle is quitting\n", date);                              \
    fflush(targFile);                                                             \
}

#define LOGERRORINIT() {    \
    if (!targFile) { \
        perror("Log file not initialized.\n"); \
        raise(SIGTERM); \
    } \
    char date[50]; \
    getFormattedTime(date, sizeof(date)); \
    fprintf(targFile, "%s Error in initialization.\n", date); \
    fflush(targFile); \
}


#endif // TARGET_HPP