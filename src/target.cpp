
#include <stdio.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include "auxfunc.h"
#include <signal.h>
#include "target.hpp"
#include "targ_publisher.hpp"
#include "cjson/cJSON.h"


#define PERIODT 10

FILE *settingsfile = NULL;
FILE *targFile = NULL;

MyTargets targets;

int pid;

int main(int argc, char *argv[]) {
    
    // Opening log file
    targFile = fopen("log/target.log", "a");
    if (targFile == NULL) {
        perror("Error in opening the file");
        exit(1);
    }

    pid = writePid("log/passParam.txt", 'a','t');

    //Defining signals
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    //Open config file
    settingsfile = fopen("appsettings.json", "r");
    if (settingsfile == NULL) {
        perror("Error opening the file");
        return EXIT_FAILURE;
    }

    readConfig();

    for(int i = 0; i < MAX_TARGET; i++){
    targets.x[i] = 0;
    targets.y[i] = 0;
    }

    // Create the publisher
    TargetPublisher targPub;

    // Initialize the publisher
    if (!targPub.init()){      
        LOGERRORINIT();
        return 1;
    }

    while (1) {

        createTargets();
        LOGNEWMAP(targets);
        targPub.publish(targets);
        sleep(PERIODT);
    }
}

/*********************************************************************************************************************/
/**************************************FUNCTIONS TO CREATE NEW TARGETS************************************************/
/*********************************************************************************************************************/

int canSpawnPrev(int x_pos, int y_pos) {
    for (int i = 0; i < targets.number; i++) {
        if (abs(x_pos - targets.x[i]) <= NO_SPAWN_DIST && abs(y_pos - targets.y[i]) <= NO_SPAWN_DIST) return 0;
    }
    return 1;
}

void createTargets() {
    // This function creates new targets ensuring that they do not overlap with the old targets.

    int x_pos, y_pos;

    for (int i = 0; i < targets.number; i++){
        do {
            x_pos = rand() % (WINDOW_LENGTH - 10);
            y_pos = rand() % (WINDOW_WIDTH - 10);
        } while (canSpawnPrev(x_pos, y_pos) == 0);

        targets.x[i] = x_pos + 5;
        targets.y[i] = y_pos + 5;
        }
}

/*********************************************************************************************************************/
/***********************************************SIGNAL HANDLER********************************************************/
/*********************************************************************************************************************/

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler('t');
    } else if(signo == SIGTERM){
        LOGPROCESSDIED(); 
        fclose(targFile);
        exit(EXIT_SUCCESS);
    }
}

/*********************************************************************************************************************/
/********************************************READING CONFIGURATION****************************************************/
/*********************************************************************************************************************/

void readConfig() {

    int len = fread(jsonBuffer, 1, sizeof(jsonBuffer), settingsfile); 
    if (len <= 0) {
        perror("Error reading the file");
        return;
    }
    fclose(settingsfile);

    cJSON *json = cJSON_Parse(jsonBuffer); 

    if (json == NULL) {
        perror("Error parsing the file");
    }

    targets.number = cJSON_GetObjectItemCaseSensitive(json, "TargetNumber")->valueint;

    cJSON *numbersArray = cJSON_GetObjectItemCaseSensitive(json, "DefaultBTN"); // questo è un array

    cJSON_Delete(json);
}
