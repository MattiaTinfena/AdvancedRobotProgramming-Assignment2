
#include <stdio.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include "auxfunc2.hpp"
#include <signal.h>
#include "target.hpp"
#include "targ_publisher.hpp"
#include "cjson/cJSON.h"


// management target
#define PERIODT 10

FILE *settingsfile = NULL;
FILE *targFile = NULL;

MyTargets targets;

int pid;

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(TARGET);
    } else if(signo == SIGTERM){
        LOGPROCESSDIED(); 
        fclose(targFile);
        exit(EXIT_SUCCESS);
    }
}

int canSpawnPrev(int x_pos, int y_pos) {
    for (int i = 0; i < targets.number; i++) {
        if (abs(x_pos - targets.x[i]) <= NO_SPAWN_DIST && abs(y_pos - targets.y[i]) <= NO_SPAWN_DIST) return 0;
    }
    return 1;
}

void createTargets() {
    int x_pos, y_pos;

    for (int i = 0; i < targets.number; i++){
        do {
            x_pos = rand() % (WINDOW_LENGTH - 1);
            y_pos = rand() % (WINDOW_WIDTH - 1);
        } while (canSpawnPrev(x_pos, y_pos) == 0);

        targets.x[i] = x_pos;
        targets.y[i] = y_pos;
        }
}

void readConfig() {

    int len = fread(jsonBuffer, 1, sizeof(jsonBuffer), settingsfile); 
    if (len <= 0) {
        perror("Error reading the file");
        return;
    }
    fclose(settingsfile);

    cJSON *json = cJSON_Parse(jsonBuffer); // parse the text to json object

    if (json == NULL) {
        perror("Error parsing the file");
    }


    // Aggiorna le variabili globali
    targets.number = cJSON_GetObjectItemCaseSensitive(json, "TargetNumber")->valueint;

    // Per array
    cJSON *numbersArray = cJSON_GetObjectItemCaseSensitive(json, "DefaultBTN"); // questo è un array

    cJSON_Delete(json); // pulisci
}

int main(int argc, char *argv[]) {
    
    // Opening log file
    targFile = fopen("log/target.log", "a");
    if (targFile == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }

    pid = writePid("log/passParam.txt", 'a', 1, 't');

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
        return EXIT_FAILURE;//1
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
