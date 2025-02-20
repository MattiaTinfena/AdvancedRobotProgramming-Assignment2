#ifndef AUXFUNC_H
#define AUXFUNC_H

#include <signal.h>
#include <stdio.h>

#define DRONE 0        
#define INPUT 1        
#define OBSTACLE 2        
#define TARGET 3 
#define BLACKBOARD 4
#define WATCHDOG 5

#define WINDOW_WIDTH 100
#define WINDOW_LENGTH 100
#define MAX_DIRECTIONS 80

#define MAX_TARGET 20
#define MAX_OBSTACLES 20
#define NO_SPAWN_DIST 5

#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

#define MAX_LINE_LENGTH 100
#define MAX_FILE_SIZE 1024

#define PLAY 0
#define PAUSE 1

extern const char *moves[9];

extern int levelTime;
extern int numTarget;
extern int numObstacle;
extern int incTime;
extern int incTarget;
extern int incObstacle;

typedef struct {
    int x;
    int y;
    float speedX;
    float speedY;
    float forceX;
    float forceY;
} Drone_bb;

typedef struct {
    float x;
    float y;
} Force;

typedef struct {
    float x;
    float y;
} Speed;

typedef struct {
    int x[MAX_TARGET];
    int y[MAX_TARGET];
    int number;
} MyTargets;

typedef struct {
    int x[MAX_OBSTACLES];
    int y[MAX_OBSTACLES];
    int number;
} MyObstacles;

typedef struct {
    char msg;
    char input[10];
    int hit[MAX_TARGET];
    Drone_bb drone;
    MyTargets targets;
    MyObstacles obstacles;
} Message;

typedef struct {
    char msg;
    char name[MAX_LINE_LENGTH];
    char input[10];
    int score;
    Drone_bb droneInfo;
} inputMessage;

typedef struct {
    char name[MAX_LINE_LENGTH];
    int score;
    int level;
} Player;

extern char jsonBuffer[MAX_FILE_SIZE];

// Logging and error handling functions
void handleLogFailure();

// Secure read/write functions on files
int writeSecure(const char* filename, const char* data, char mode);
int readSecure(const char* filename, char* data, size_t datasize);

// Signal Management
void handler(char id);

// Pipe communication functions
void writeMsg(int pipeFds, Message* msg, const char* error, FILE* file);
void readMsg(int pipeFds, Message* msgOut, const char* error, FILE* file);
void writeInputMsg(int pipeFds, inputMessage* msg, const char* error, FILE* file);
void readInputMsg(int pipeFds, inputMessage* msgOut, const char* error, FILE* file);

// Function to read file descriptor from input
void fdsRead(int argc, char* argv[], int* fds);

// Function to write PID to file
int writePid(const char* file, char mode, char id);

// Functions for printing messages to file
void printInputMessageToFile(FILE *file, inputMessage* msg);
void printMessageToFile(FILE *file, Message* msg);

// Data structure initialization functions
void msgInit(Message* status);
void inputMsgInit(inputMessage* status);

// Function to get the time formatted
void getFormattedTime(char *buffer, size_t size);

#endif // AUXFUNC_H
