#ifndef OBSTACLE_HPP
#define OBSTACLE_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "auxfunc.h"

#define USE_DEBUG 1  // 0 for release, 1 for debug
extern FILE *obstFile; //Global variable

//Functions definition
int canSpawnPrev(int x_pos, int y_pos);
void createObstacles();
void sig_handler(int signo);
void readConfig();

/*********************************************************************************************************************/
/********************************************FUNCTIONS TO LOG*********************************************************/
/*********************************************************************************************************************/

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

#endif // OBSTACLE_HPP
