#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "auxfunc.h"
#include <signal.h>
#include <math.h>
#include "obstacle.hpp"
#include "obst_publisher.hpp"
#include "cjson/cJSON.h"


#define PERIODO 5

FILE *settingsfile = NULL;
FILE *obstFile = nullptr;

MyObstacles obstacles;

int pid;

int main(int argc, char *argv[]) {

    // Opening log file
    obstFile = fopen("log/obstacle.log", "a");
    if (obstFile == NULL) {
        perror("Errore nell'apertura del obstFile");
        exit(1);
    }

    pid = writePid("log/passParam.txt", 'a', 1, 'o');

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

    for( int i = 0; i < MAX_OBSTACLES; i++){
        obstacles.x[i] = 0;
        obstacles.y[i] = 0;
    }

    // Create the publisher
    ObstaclePublisher obstPub;

    // Initialize the publisher
    if (!obstPub.init()){      
        LOGERRORINIT();
        return 1;
    }

    while (1) {

        createObstacles();
        LOGNEWMAP(obstacles);
        obstPub.publish(obstacles);
        sleep(PERIODO);
    }
}

/*********************************************************************************************************************/
/************************************FUNCTIONS TO CREATE NEW OBSTACLES************************************************/
/*********************************************************************************************************************/


int canSpawnPrev(int x_pos, int y_pos) {
    for (int i = 0; i < obstacles.number; i++) {
        if (abs(x_pos - obstacles.x[i]) <= NO_SPAWN_DIST && abs(y_pos - obstacles.y[i]) <= NO_SPAWN_DIST) return 0;
    }
    return 1;
}

void createObstacles() {
    // This function creates new obstacles ensuring that they do not overlap with the old obstacles.

    int x_pos, y_pos;

    for (int i = 0; i < obstacles.number; i++){
        do {
            x_pos = rand() % (WINDOW_LENGTH - 1);
            y_pos = rand() % (WINDOW_WIDTH - 1);
        } while (canSpawnPrev(x_pos, y_pos) == 0);

        obstacles.x[i] = x_pos;
        obstacles.y[i] = y_pos;
    }
}

/*********************************************************************************************************************/
/***********************************************SIGNAL HANDLER********************************************************/
/*********************************************************************************************************************/

void sig_handler(int signo) {
    if (signo == SIGUSR1){
        handler(OBSTACLE);
    } else if(signo == SIGTERM){
        LOGPROCESSDIED();
        fclose(obstFile);
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

    obstacles.number = cJSON_GetObjectItemCaseSensitive(json, "ObstacleNumber")->valueint;

    cJSON *numbersArray = cJSON_GetObjectItemCaseSensitive(json, "DefaultBTN");

    cJSON_Delete(json);
}
