#ifndef TARGET_HPP
#define TARGET_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "auxfunc2.hpp"

// Macro di configurazione
#define MAX_LINE_LENGTH 100
#define USE_DEBUG 1

// Variabili globali
extern FILE *targFile;

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