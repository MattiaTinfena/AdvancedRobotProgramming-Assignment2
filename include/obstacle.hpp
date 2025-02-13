#ifndef OBSTACLE_HPP
#define OBSTACLE_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "auxfunc2.hpp"

// Macro di configurazione
#define MAX_LINE_LENGTH 100
#define USE_DEBUG 1

// Variabili globali
extern FILE *obstFile;

// Macro per il logging
#define LOGNEWMAP(obstacles) {\
    if (!obstFile) { \
        perror("Log file not initialized.\n"); \
        raise(SIGTERM); \
    } \
    char date[50]; \
    getFormattedTime(date, sizeof(date)); \
    fprintf(obstFile, "%s New obstacle generated.\n", date); \
    for (int t = 0; t < MAX_OBSTACLES; t++) { \
        fprintf(obstFile, "(%d, %d) ", obstacles.x[t], obstacles.y[t]); \
    } \
    fprintf(obstFile, "\n"); \
    fflush(obstFile); \
}
    

#define LOGPROCESSDIED() {\
    if (!obstFile) { \
        perror("Log file not initialized.\n"); \
        raise(SIGTERM); \
    } \
    char date[50]; \
    getFormattedTime(date, sizeof(date)); \
    fprintf(obstFile, "%s Process dead. Obstacle is quitting\n", date); \
    fflush(obstFile); \
}

#define LOGERRORINIT() {    \
    if (!obstFile) { \
        perror("Log file not initialized.\n"); \
        raise(SIGTERM); \
    } \
    char date[50]; \
    getFormattedTime(date, sizeof(date)); \
    fprintf(obstFile, "%s Error in initialization.\n", date); \
    fflush(obstFile); \
}


// #define LOGSUBSCRIPTION(current_count_change) \
//     if (!obstFile) { \
//         perror("Log file not initialized.\n"); \
//         raise(SIGTERM); \
//     } \
//     char date[50]; \
//     getFormattedTime(date, sizeof(date)); \
//     if (current_count_change == 1) { \
//         fprintf(obstFile, "%s Subscription matched\n", date); \
//     } else if (current_count_change == -1) { \
//         fprintf(obstFile, "%s Subscription un-matched\n", date); \
//     } else { \
//         fprintf(obstFile, "%s %d is not a valid value\n", date, current_count_change); \
//     } \
//     fflush(obstFile); \

// #define LOGPUBLISHERMATCHING(current_count_change) \
//     if (!obstFile) { \
//         perror("Log file not initialized.\n"); \
//     } \
//     char date[50]; \
//     getFormattedTime(date, sizeof(date)); \
//     if (current_count_change == 1) { \
//         fprintf(obstFile, "%s Obstacle Publisher matched\n", date); \
//     } else if (current_count_change == -1) { \
//         fprintf(obstFile, "%s Obstacle Publisher unmatched.\n", date); \
//     } else { \
//         fprintf(obstFile, "%s %d is not a valid value\n", date, current_count_change); \
//     } \
//     fflush(obstFile); \

// #if USE_DEBUG
// #define LOGPUBLISHNEWTARGET(obstacles) {\
//     if (!obstFile) { \
//         perror("Log file not initialized.\n"); \
//     } \
//     char date[50]; \
//     getFormattedTime(date, sizeof(date)); \
//     fprintf(obstFile, "%s Obstacle published correctly:\n", date); \
//     for (int t = 0; t < obstacles.obstacles_number(); t++) { \
//         fprintf(obstFile, "(%d, %d) ", obstacles.x()[t], obstacles.y()[t]); \
//     } \
//     fprintf(obstFile, "\n"); \
//     fflush(obstFile); \
// }
// #else
// #define LOGPUBLISHNEWTARGET(obstacles) 
// #endif

#endif // OBSTACLE_HPP
