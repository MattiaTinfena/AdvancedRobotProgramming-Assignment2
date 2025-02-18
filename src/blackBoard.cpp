#include <ncurses.h> // Invece di <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#include "auxfunc2.hpp"
#include <signal.h>
#include "targ_subscriber.hpp"
#include "obst_subscriber.hpp"
#include "cjson/cJSON.h"
#include "log.h"
// process to whom that asked or received
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

#define nfds 19

#define HMARGIN 5
#define WMARGIN 5
#define BMARGIN 2
#define SCALERATIO 1

#define PERIODBB 10000  // [us]

int nh, nw;
float scaleh = 1.0, scalew = 1.0;

float second = 1000000;

int pid;

int fds[4][4] = {0};
int mode = PLAY;

WINDOW * win;
WINDOW * map;

// FILE *file = NULL;
FILE *logFile = NULL;
FILE *settingsfile = NULL;

Drone_bb prevDrone = {0, 0, 0, 0, 0, 0};

Message status;
Message msg;
inputMessage inputMsg;
inputMessage inputStatus;

TargetSubscriber targSub;
ObstacleSubscriber obstSub;

int pids[6] = {0, 0, 0, 0, 0, 0 };  // Initialize PIDs to 0

int collision = 0;
int targetsHit = 0;

void storePreviousPosition(Drone_bb *drone) {

    prevDrone.x = drone ->x;
    prevDrone.y = drone ->y;
}

void resizeHandler(){
    getmaxyx(stdscr, nh, nw);  /* get the new screen size */
    scaleh = ((float)(nh - 2) / (float)WINDOW_LENGTH);
    scalew = (float)nw / (float)WINDOW_WIDTH;
    endwin();
    initscr();
    start_color();
    curs_set(0);
    noecho();
    win = newwin(nh, nw, 0, 0);
    map = newwin(nh - 2, nw, 2, 0); 
}

void mapInit(FILE *file){


    fprintf(file,"-------------------------\n");
    fprintf(file," MAP INITIALIZAION       \n");
    fprintf(file,"-------------------------\n");
    fflush(file);

    // read initial drone position
    readMsg(fds[DRONE][askrd], &status,
                "[BB] Error reading drone position\n", file);
    
    while (!targSub.hasNewData() || !obstSub.hasNewData()){
        usleep(100000);
    }

    status.targets = targSub.getMyTargets();
    status.obstacles = obstSub.getMyObstacles();
    //Update drone position
    writeMsg(fds[DRONE][recwr], &status, 
            "[BB] Error sending updated map\n", file);
    
    inputStatus.droneInfo = status.drone;
    inputStatus.msg = 'B';
    writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                "Error sending ack", file);

    fprintf(file,"-------------------------\n");
    fprintf(file,"MAP INITIALIZAION FINISHED\n");
    fprintf(file,"-------------------------\n");
    fflush(file);
}

void drawDrone(WINDOW * win){
    int row = (int)(status.drone.y * scaleh);
    int col = (int)(status.drone.x * scalew);
    wattron(win, A_BOLD); // Attiva il grassetto
    wattron(win, COLOR_PAIR(1));   
    std::string m = "+";
    mvwprintw(win, row - 1, col, "%s", "|");     
    mvwprintw(win, row, col + 1, "%s", "--");
    mvwprintw(win, row, col, "%s", "+");
    mvwprintw(win, row + 1, col, "%s", "|");     
    mvwprintw(win, row , col -2, "%s", "--");
    wattroff(win, COLOR_PAIR(1)); 
    wattroff(win, A_BOLD); // Attiva il grassetto 
}

void drawObstacle(WINDOW * win){
    wattron(win, A_BOLD); // Attiva il grassetto
    wattron(win, COLOR_PAIR(2)); 
    for(int i = 0; i < status.obstacles.number; i++){
        mvwprintw(win, (int)(status.obstacles.y[i]*scaleh), (int)(status.obstacles.x[i]*scalew), "%s", "0");
    }
    wattroff(win, COLOR_PAIR(2)); 
    wattroff(win, A_BOLD); // Attiva il grassetto 
}

void drawTarget(WINDOW * win) {
    wattron(win, A_BOLD); // Attiva il grassetto
    wattron(win, COLOR_PAIR(3)); 
    for (int i = 0; i < status.targets.number; i++) {
        if (status.hit[i]) continue;
        std::string val_str = std::to_string(i + 1); // Converte il valore in stringa
        mvwprintw(win, (int)(status.targets.y[i] * scaleh), (int)(status.targets.x[i] * scalew), "%s", val_str.c_str()); // Usa un formato esplicito
    } 
    wattroff(win, COLOR_PAIR(3)); 
    wattroff(win, A_BOLD); // Disattiva il grassetto
}


void drawMenu(WINDOW* win) {
    wattron(win, A_BOLD); // Attiva il grassetto

    // Preparazione delle stringhe
    char score_str[10];
    sprintf(score_str, "%d", inputStatus.score);

    // Array con le etichette e i valori corrispondenti
    const char* labels[] = { "Score: ", "Player: " };
    const char* values[] = { score_str, inputStatus.name };

    int num_elements = 2; // Numero di elementi nel menu

    // Calcola la lunghezza totale occupata dalle stringhe
    int total_length = 0;
    for (int i = 0; i < num_elements; i++) {
        total_length += strlen(labels[i]) + strlen(values[i]);
    }

    // Ottieni la larghezza della finestra
    int nw;
    getmaxyx(win, nw, nw); // Ignora l'altezza, usa solo la larghezza

    // Calcola lo spazio rimanente e lo spazio tra gli elementi
    int remaining_space = nw - total_length; // Spazio non occupato dalle stringhe
    int spacing = remaining_space / (num_elements + 1); // Spaziatura uniforme

    // Stampa gli elementi equidistanti
    int current_position = spacing; // Posizione iniziale
    for (int i = 0; i < num_elements; i++) {
        // Stampa l'etichetta e il valore
        mvwprintw(win, 0, current_position, "%s%s", labels[i], values[i]);

        // Aggiorna la posizione corrente
        current_position += strlen(labels[i]) + strlen(values[i]) + spacing;
    }

    wattroff(win, A_BOLD); // Disattiva il grassetto
    // Aggiorna la finestra per mostrare i cambiamenti
    wrefresh(win);
}


int randomSelect(int n) {
    unsigned int random_number;
    int random_fd = open("/dev/urandom", O_RDONLY);
    
    if (random_fd == -1) {
        perror("Error opening /dev/urandom");
        return -1;  // Indicate failure
    }

    if (read(random_fd, &random_number, sizeof(random_number)) == -1) {
        perror("Error reading from /dev/urandom");
        close(random_fd);
        return -1;  // Indicate failure
    }
    
    close(random_fd);  // Close file after successful read
    
    return random_number % n;
}

void detectCollision(Message* status, Drone_bb * prev) {
    
    for (int i = 0; i < status->targets.number; i++) {
        
        if (!status->hit[i] && (((prev->x <= status->targets.x[i] + 2 && status->targets.x[i] - 2 <= status->drone.x)  &&
            (prev->y <= status->targets.y[i] + 2 && status->targets.y[i]- 2 <= status->drone.y) )||
            ((prev->x >= status->targets.x[i] - 2 && status->targets.x[i] >= status->drone.x + 2) &&
            (prev->y >= status->targets.y[i] - 2 && status->targets.y[i] >= status->drone.y + 2) ))){
                inputStatus.score += i + 1;
                status->hit[i] = 1;
                collision = 1;
                targetsHit++;   
        }
    }
}

void createNewMap(FILE *file){

    storePreviousPosition(&status.drone);

    readMsg(fds[DRONE][askrd],  &msg,
        "[BB] Reading ready from [DRONE]", file);

    if(msg.msg == 'R'){

        status.msg = 'M';

        writeMsg(fds[DRONE][recwr], &status, 
                "[BB] Error asking drone position", file);

        storePreviousPosition(&status.drone);

        readMsg(fds[DRONE][askrd], &status,
                "[BB] Error reading drone position", file);
    }
}

void closeAll(){

    for(int j = 0; j < 6; j++){
        if (j != BLACKBOARD && pids[j] != 0){
            fprintf(logFile, "sending kill to %d\n",pids[j]);
            fflush(logFile); 
            if (kill(pids[j], SIGTERM) == -1) {
            fprintf(logFile,"Process %d is not responding or has terminated\n", pids[j]);
            fflush(logFile);
            }
        }
    }
    fprintf(logFile,"closing blackboard\n");
    fflush(logFile);
    fclose(logFile);
    exit(EXIT_SUCCESS);
}

void quit(){

    readInputMsg(fds[INPUT][askrd], &inputMsg, 
                "[BB] Error reading input", logFile);

    while(inputMsg.msg != 'q'){
                
        fprintf(logFile, "Waiting for quit\n");
        fflush(logFile);

        readInputMsg(fds[INPUT][askrd], &inputMsg, 
                "[BB] Error reading input", logFile);

    }
    
    inputStatus.msg = 'S';
    
    writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                "[BB] Error sending ack", logFile);
    
    readInputMsg(fds[INPUT][askrd], &inputMsg, 
                "[BB] Error reading input", logFile);
    
    if(inputMsg.msg == 'R'){    //input ready, all data are saved
        closeAll();
    }          
}

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(BLACKBOARD);
    } else if (signo == SIGTERM) {
        close(fds[DRONE][recwr]);
        close(fds[DRONE][askrd]);
        close(fds[INPUT][recwr]);
        close(fds[INPUT][askrd]);
        fprintf(logFile,"Closing Blackboard");
        fflush(logFile);
        fclose(logFile);
        exit(EXIT_SUCCESS);
    } else if( signo == SIGWINCH){
        resizeHandler();
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
    status.targets.number = cJSON_GetObjectItemCaseSensitive(json, "TargetNumber")->valueint;
    status.obstacles.number = cJSON_GetObjectItemCaseSensitive(json, "ObstacleNumber")->valueint;

    cJSON_Delete(json); // pulisci
}

int main(int argc, char *argv[]) {

    logFile = fopen("log/logfile.log", "w");
    if (logFile == NULL) {
        perror("Errore nell'aprire il file di log");
        return 1;
    }
    
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <fd_str[0]> <fd_str[1]> <fd_str[2]> <fd_str[3]>\n", argv[0]);
        exit(1);
    }

   
    char *fd_str0 = argv[1];

    int index0 = 0;

    // Tokenizzazione di ogni valore e scarto della ","
    char *token0 = strtok(fd_str0, ",");
    token0 = strtok(NULL, ",");

    // Estrazione dei file descriptor
    while (token0 != NULL && index0 < 4) {
        fds[INPUT][index0] = atoi(token0);
        index0++;
        token0 = strtok(NULL, ",");
    }

    char *fd_str2 = argv[2];

    int index2 = 0;

    // Tokenizzazione di ogni valore e scarto della ","
    char *token2 = strtok(fd_str2, ",");
    token2 = strtok(NULL, ",");

    // Estrazione dei file descriptor
    while (token2 != NULL && index2 < 4) {
        fds[DRONE][index2] = atoi(token2);
        index2++;
        token2 = strtok(NULL, ",");
    }

    for(int i = 0; i < MAX_TARGET; i++){
        status.hit[i] = 0;
    }

    if (!targSub.init()){

        fprintf(logFile,"Problem targ");
        fflush(logFile);
        //--------------------
        // LOG
        //----------------------
        ;
    } 
    

    if (!obstSub.init()){

        fprintf(logFile,"Problem Obst");
        fflush(logFile);
        //--------------------
        // LOG
        //----------------------
        ;
    } 
    
    status.targets.number = 10;
    pid = (int)getpid();
    char dataWrite [80] ;
    snprintf(dataWrite, sizeof(dataWrite), "b%d,", pid);

    if(writeSecure("log/passParam.txt", dataWrite,1,'a') == -1){
        perror("Error in writing in passParan.txt");
        exit(1);
    }
    
    //Open config file
    settingsfile = fopen("appsettings.json", "r");
    if (settingsfile == NULL) {
        perror("Error opening the file");
        return EXIT_FAILURE;//1
    }

    // closing the unused fds to avoid deadlock
    close(fds[DRONE][askwr]);
    close(fds[DRONE][recrd]);
    close(fds[INPUT][askwr]);
    close(fds[INPUT][recrd]);

    fd_set readfds;
    struct timespec ts;
    
    //Setting select timeout
    ts.tv_sec = 0;
    ts.tv_nsec = 1000 * 1000;

    
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Error while setting sigaction for SIGTERM");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGWINCH, &sa, NULL) == -1) {
        perror("Error while setting sigaction for SIGWINCH");
        exit(EXIT_FAILURE);
    }

    initscr();
    start_color();
    curs_set(0);
    noecho();
    cbreak();
    getmaxyx(stdscr, nh, nw);
    win = newwin(nh, nw, 0, 0);
    map = newwin(nh - 2, nw, 2, 0); 
    scaleh = (float)(nh - 2) / (float)WINDOW_LENGTH;
    scalew = (float)nw / (float)WINDOW_WIDTH;

    // Definizione delle coppie di colori
    init_pair(1, COLOR_BLUE, COLOR_BLACK);     // Testo blu su sfondo nero
    init_pair(2, COLOR_RED , COLOR_BLACK);  // Testo arancione su sfondo nero
    init_pair(3, COLOR_GREEN, COLOR_BLACK);    // Testo verde su sfondo nero

    usleep(500000);

    char datareaded[200];
    if (readSecure("log/passParam.txt", datareaded,1) == -1) {
        perror("[BB] Error reading the passParam file");
        exit(1);
    }

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
        }else if (type == 'w') {
            pids[WATCHDOG] = number;
        }
        token = strtok(NULL, ",");
    }

    // Write the PID values to the output file
    for (int i = 0; i < 6; i++) {
        fprintf(logFile, "pid[%d] = %d\n", i, pids[i]);
        fflush(logFile);
    }

    inputStatus.score = 0;

    readInputMsg(fds[INPUT][askrd], &inputStatus, 
                "Error reading input", logFile);

    inputStatus.msg = 'A';
    writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                "Error sending ack", logFile);

    readConfig();

    mapInit(logFile);

    while (1) {
        
        obstSub.run();
        targSub.run();

        
        
        if (targetsHit >= status.targets.number) {

            int y_center = (nh - 2) / 2;
            int x_center = nw / 2;

            const char *msg1 = "Congratulations! You Won!";
            const char *msg2 = "Press q to quit";

            mvwprintw(map, y_center, x_center - strlen(msg1) / 2, "%s", msg1);
            mvwprintw(map, y_center + 1, x_center - strlen(msg2) / 2, "%s", msg2);
            wrefresh(map);

            quit();
        }

        if (targSub.hasNewData() || obstSub.hasNewData()){

            if(targSub.hasNewData()){
                status.targets = targSub.getMyTargets();
            }

            if(obstSub.hasNewData()){
                status.obstacles = obstSub.getMyObstacles();
            }
            createNewMap(logFile);

            LOGNEWMAP(status);
        }

        // // Update the main window
        werase(win);
        werase(map);
        box(map, 0, 0);

        drawMenu(win);
        drawDrone(map);
        drawObstacle(map);
        drawTarget(map);
        wrefresh(win);
        wrefresh(map);

        sigset_t mask;
        sigemptyset(&mask);
        // sigaddset(&mask, SIGUSR1);
        // sigaddset(&mask, SIGTERM);
        // sigaddset(&mask, SIGWINCH);

        //FDs setting for select
        FD_ZERO(&readfds);
        FD_SET(fds[DRONE][askrd], &readfds);
        FD_SET(fds[INPUT][askrd], &readfds);

        int fdsQueue [4];
        int ready = 0;

        int sel = pselect(nfds, &readfds, NULL, NULL, &ts, &mask);
        
        if (sel == -1) {
            perror("Select error");
            break;
        } 

        if (FD_ISSET(fds[DRONE][askrd], &readfds)) {
            fdsQueue[ready] = fds[DRONE][askrd];
            ready++;
        }
        if (FD_ISSET(fds[INPUT][askrd], &readfds)) {
            fdsQueue[ready] = fds[INPUT][askrd];
            ready++;
        }
        if (FD_ISSET(fds[OBSTACLE][askrd], &readfds)) {
            fdsQueue[ready] = fds[OBSTACLE][askrd];
            ready++;
        }
        if (FD_ISSET(fds[TARGET][askrd], &readfds)) {
            fdsQueue[ready] = fds[TARGET][askrd];
            ready++;
        }

        if(ready > 0){
            unsigned int rand = randomSelect(ready);
            int selected = fdsQueue[rand];
            detectCollision(&status, &prevDrone);

            if (selected == fds[DRONE][askrd]){
         
                readMsg(fds[DRONE][askrd], &msg,
                                "[BB] Error reading drone position", logFile);


                if(msg.msg == 'R'){
                    if (collision) {
 
                        collision = 0;
                        
                        writeMsg(fds[DRONE][recwr], &status, 
                                "[BB] Error asking drone position", logFile);

                        storePreviousPosition(&status.drone);

                        readMsg(fds[DRONE][askrd], &status,
                                "[BB] Error reading drone position", logFile);

                    }else{
                        
                        // fprintf(file, "[BB] Asking drone position to [DRONE]\n");
                        // fflush(file);
                        
                        status.msg = 'A';

                        writeMsg(fds[DRONE][recwr], &status, 
                                "[BB] Error asking drone position", logFile);

                        status.msg = '\0';

                        storePreviousPosition(&status.drone);

                        readMsg(fds[DRONE][askrd], &status,
                                "[BB] Error reading drone position", logFile);

                        // fprintf(file, "Drone updated position: %d,%d\n", status.drone.x, status.drone.y);
                        // fflush(file);
                    }
                }
                  
            } else if (selected == fds[INPUT][askrd]){
                
                
                readInputMsg(fds[INPUT][askrd], &inputMsg, 
                                "[BB] Error reading input", logFile);


                if(inputMsg.msg == 'P'){
                    mode = PAUSE;

                    inputStatus.msg = 'A';

                    writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                                "[BB] Error sending ack", logFile);

                    // fprintf(logFile, "Sended ack\n");
                    // fflush(logFile);
                  
                    inputMsg.msg = 'A';

                    while(inputMsg.msg != 'P' && inputMsg.msg != 'q'){
                        
                        fprintf(logFile, "Waiting for play\n");
                        fflush(logFile);

                        readInputMsg(fds[INPUT][askrd], &inputMsg, 
                                "[BB] Error reading input", logFile);

                    }
                    if(inputMsg.msg == 'P'){
                        mode = PLAY;
                    }else if(inputMsg.msg == 'q'){
                        fprintf(logFile, "Quit\n");
                        fflush(logFile);
                        
                        inputStatus.msg = 'S';
                        
                        writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                                    "[BB] Error sending ack", logFile);
                        
                        readInputMsg(fds[INPUT][askrd], &inputMsg, 
                                    "[BB] Error reading input", logFile);
                        
                        if(inputMsg.msg == 'R'){    //input ready, all data are saved

                            closeAll();
                        }           
                    }
                    continue;

                }else if (inputMsg.msg == 'q'){
                    inputStatus.msg = 'S';
                    writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                                "[BB] Error sending ack", logFile);
                    
                    readInputMsg(fds[INPUT][askrd], &inputMsg, 
                                "[BB] Error reading input", logFile);
                    
                    if(inputMsg.msg == 'R'){    //input ready, all data are saved

                        closeAll();
                    }           
                }

                readMsg(fds[DRONE][askrd], &msg,
                                "[BB] Error reading drone ready", logFile);

                if(msg.msg == 'R'){

                    // fprintf(file, "drone ready\n"); 
                    // fflush(file);

                    status.msg = 'I';
                    strncpy(status.input, inputMsg.input, sizeof(inputStatus.input));
                    
                    writeMsg(fds[DRONE][recwr], &status, 
                                "[BB] Error asking drone position", logFile);     
                }

                storePreviousPosition(&status.drone);

                readMsg(fds[DRONE][askrd], &status,
                                "[BB] Error reading drone position", logFile);

                inputStatus.droneInfo = status.drone;
                
                writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                                "[BB] Error asking drone position", logFile); 
            }else{
            }
        }
        usleep(PERIODBB);
    }

    return 0;
}