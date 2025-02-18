#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "auxfunc.h"
#include <signal.h>
#include <cjson/cJSON.h>
#include "keyboardMap.h"
#include "input.h"



int nh, nw;
float scaleh = 1.0, scalew = 1.0;

int btnValues[9] ={0};
char *droneInfoText[6] = {"Position x: ", "Position y: ", "Force x: ", "Force y: ", "Speed x ", "Speed y: "};
char *menuBtn[2] = {"Press P to pause", "Press Q to quit"};

int pid;
int fds[4]; 

float droneInfo[6] = {0.0};
char droneInfo_str[40];

int mode = MENU;   
int disp = CHOOSENAME;

WINDOW * winBut[9]; 
WINDOW * win;
WINDOW* control;

FILE *settingsfile = NULL;
FILE *inputFile = NULL;

Drone_bb drone = {0, 0, 0, 0, 0, 0};
Force force = {0, 0};
Speed speed = {0, 0};

inputMessage inputMsg;
inputMessage inputStatus;
Message msg;
Message status;


void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(INPUT);
    }else if(signo == SIGTERM){
        LOGPROCESSDIED(); 
        fclose(inputFile);
        close(fds[recrd]);
        close(fds[askwr]);
        exit(EXIT_SUCCESS);
    }
}

void btnSetUp (int row, int col){

    for(int i = 0; i < BUTTONS; i++){
        int newRow = row + ((i/3)*BTNDISTR);
        int newCol = col + ((i%3)*BTNDISTC);
        winBut[i] = newwin(BTNSIZER, BTNSIZEC, newRow, newCol);
    }
}

void drawBtn(int b) {
    char btn[10] = ""; // Stringa per ogni pulsante

    for (int i = 0; i < BUTTONS; i++) {
        // Ottieni il carattere dal valore ASCII
        if (btnValues[i] != 0) { // Verifica che il valore ASCII non sia vuoto
            snprintf(btn, sizeof(btn), "%c", (char)btnValues[i]); // Converte in carattere
        } else {
            snprintf(btn, sizeof(btn), " "); // Se vuoto, usa uno spazio
        }

        werase(winBut[i]);

        // Calcolo della posizione centrata
        int r = 2; // Riga centrale
        int c = (BTNSIZEC - strlen(btn)) / 2; // Colonna centrata

        // Stampa del testo centrato nel pulsante
        if (i == b) {
            wattron(winBut[i], COLOR_PAIR(1));  
        }
        box(winBut[i], 0, 0); // Disegna il bordo del pulsante
        mvwprintw(winBut[i], r, c, "%s", btn);
        wrefresh(winBut[i]); // Aggiorna la finestra
        if (i == b) {
            wattroff(winBut[i], COLOR_PAIR(1));  
        }
    }
}

void drawName(){
    const int prompt_row = nh / 2 - 2; // Riga leggermente sopra il centro
    const int name_row = nh / 2;     // Riga per il nome
    const char *prompt = "Choose a name:";

    werase(stdscr);
    box(stdscr, 0, 0);
    mvwprintw(stdscr, prompt_row, (nw - strlen(prompt)) / 2, "%s", prompt);
    mvwprintw(stdscr, name_row, (nw - strlen(inputStatus.name)) / 2, "%s", inputStatus.name);
    wrefresh(stdscr);
}

void setName() {
    werase(stdscr);
    box(stdscr, 0, 0);

    const char *prompt = "Choose a name:";
    const int prompt_row = nh / 2 - 2;
    const int name_row = nh / 2; 
    const int error_row = nh / 2 + 2;

    mvwprintw(stdscr, prompt_row, (nw - strlen(prompt)) / 2, "%s", prompt);
    mvwprintw(stdscr, name_row, (nw - strlen(inputStatus.name)) / 2, "%s", inputStatus.name);
    wrefresh(stdscr);

    int ch = 0;
    unsigned long pos = strlen(inputStatus.name); // Inizializza la posizione in base alla lunghezza del nome esistente

    while (ch != MY_KEY_ENTER) {
        ch = getch(); // Legge il prossimo carattere premuto

        if (ch == MY_KEY_BACK) {
            // Backspace: rimuove l'ultimo carattere se possibile
            if (pos > 0) {
                pos--;
                inputStatus.name[pos] = '\0'; // Termina correttamente la stringa
            }
        } else if (ch >= 32 && ch <= 126) { // Solo caratteri stampabili
            if (pos < sizeof(inputStatus.name) - 1 && pos < MAX_LINE_LENGTH - 1) {
                inputStatus.name[pos++] = ch;
                inputStatus.name[pos] = '\0';
            } else if (pos >= MAX_LINE_LENGTH - 1) {
                // Mostra messaggio di errore per superamento lunghezza massima
                mvwprintw(stdscr, error_row, (nw - strlen("Name too long, try again.")) / 2, "%s", "Name too long, try again.");
            }
        } else {
            // Mostra messaggio di errore per tasto non valido
            mvwprintw(stdscr, error_row, (nw - strlen("Invalid key, try again.")) / 2, "%s", "Invalid key, try again.");
        }

        // Ridisegna lo schermo
        werase(stdscr);
        box(stdscr, 0, 0);
        mvwprintw(stdscr, prompt_row, (nw - strlen(prompt)) / 2, "%s", prompt);
        mvwprintw(stdscr, name_row, (nw - strlen(inputStatus.name)) / 2, "%s", inputStatus.name);
        wrefresh(stdscr);
    }

    // Mostra il nome inserito e termina
    werase(stdscr);
    box(stdscr, 0, 0);
    char confirmation[110];
    snprintf(confirmation, sizeof(confirmation), "Name set: %s", inputStatus.name);
    mvwprintw(stdscr, nh / 2, (nw - strlen(confirmation)) / 2, "%s", confirmation);
    wrefresh(stdscr);
}

int keyAlreadyUsed(int key, int index ){
    for(int i = 0; i < index + 1; i++){
        if(key == btnValues[i] || key == MY_KEY_p || key == MY_KEY_q || key == MY_KEY_Q || key == MY_KEY_P){
            mvwprintw(stdscr, 13, 17, "%s", "Key already used");
            wrefresh(stdscr);
            return 1;
        }
    }
    return 0;
}

void setBtns(){
    werase(stdscr);
    box(stdscr, 0, 0);
    const char *line1 = "Do you want to use the default key configuration?";
    const char *line2 = "y - yes";
    const char *line3 = "n - no";

    // Calcola le posizioni orizzontali centrate
    int col1 = (nw - strlen(line1)) / 2;
    int col2 = (nw - strlen(line2)) / 2;
    int col3 = (nw - strlen(line3)) / 2;

    // Stampa le scritte centrate
    mvwprintw(stdscr, 10, col1, "%s", line1);
    mvwprintw(stdscr, 11, col2, "%s", line2);
    mvwprintw(stdscr, 12, col3, "%s", line3);

    btnSetUp(14,(col1 + col2)/2);

    wrefresh(stdscr); 

    drawBtn(99); //to make all the buttons white
    usleep(10000);
    int ch = getch();
    if (ch == MY_KEY_N || ch == MY_KEY_n) {
        for(int i = 0; i < BUTTONS; i++){
            werase(stdscr);
            box(stdscr, 0, 0);   
            mvwprintw(stdscr, 10, 10, "%s", "Choose wich key do you want to use for the highlighted direction?"); 
            wrefresh(stdscr);
            drawBtn(i);
            while ((ch = getch()) == ERR || keyAlreadyUsed(ch, i)) {
                //nessun tasto premuto dall'utente
                usleep(100000);
            }
            
            btnValues[i] = ch;
            usleep(100000);
        }
        werase(stdscr);
    }else if(ch == 121){
        return;
    }else{
        setBtns();
    }
}

void pauseMenu(){
    werase(stdscr);
    box(stdscr, 0, 0);
    const char *prompt = "Press P to play";
    const char *prompt2 = "Press Q to quit";
    int prompt_row = nh / 2 - 2; // Riga leggermente sopra il centro

    mvwprintw(stdscr, prompt_row, (nw - strlen(prompt)) / 2, "%s", prompt);
    mvwprintw(stdscr, prompt_row + 1, (nw - strlen(prompt)) / 2, "%s", prompt2);

    wrefresh(stdscr);
}
 
void mainMenu(){
   
    mode = MENU;
    disp = CHOOSENAME;
    setName();
    disp = CHOOSEBUTTON;
    setBtns();
    disp = CHOOSEDIFF;

    
    werase(stdscr);
    wrefresh(stdscr);

    inputStatus.msg = 'A';
    strncpy(inputStatus.input, "left", 10);

    writeInputMsg(fds[askwr], &inputStatus, 
                "Error sending settings", inputFile);

    readInputMsg(fds[recrd], &inputStatus, 
                "Error reading ack", inputFile);

    LOGACK(inputStatus);

    readInputMsg(fds[recrd], &inputStatus, 
                "Error reading drone info", inputFile);
    LOGDRONEINFO(inputStatus.droneInfo);

}

void drawInfo() {

    //Menu part 1

    int initialrow = (int)((nh/3) - 6)/2 > 0 ? (int)((nh/3) - 6)/2 : 0;
    char droneInfoStr[6][20];

    for (int i = 0; i < 6; i++) {

        switch (i) {
            case 0:
                snprintf(droneInfoStr[i], sizeof(droneInfoStr[i]), "%d", inputStatus.droneInfo.x);
                break;
            case 1:
                snprintf(droneInfoStr[i], sizeof(droneInfoStr[i]), "%d", inputStatus.droneInfo.y);
                break;
            case 2:
                snprintf(droneInfoStr[i], sizeof(droneInfoStr[i]), "%.3f", inputStatus.droneInfo.speedX);
                break;
            case 3:
                snprintf(droneInfoStr[i], sizeof(droneInfoStr[i]), "%.3f", inputStatus.droneInfo.speedY);
                break;
            case 4:
                snprintf(droneInfoStr[i], sizeof(droneInfoStr[i]), "%.3f", inputStatus.droneInfo.forceX);
                break;
            case 5:
                snprintf(droneInfoStr[i], sizeof(droneInfoStr[i]), "%.3f", inputStatus.droneInfo.forceY);
                break;
        }
        // Calcola le lunghezze effettive delle stringhe
        int textLen = strlen(droneInfoText[i]);
        int valueLen = strlen(droneInfoStr[i]);

        // Larghezza totale della combinazione di testo e valore
        int totalLen = textLen + valueLen; // +1 per lo spazio tra le stringhe

        // Calcola la colonna per centrare l'intera combinazione
        int col = ((nw / 2) - totalLen) / 2;

        // Stampa la riga centrata
        mvwprintw(control, initialrow + i, col, "%s%s", droneInfoText[i], droneInfoStr[i]);
        wrefresh(control);
    }

     // Menu part 2 

    int initialrow2 = ((nh / 3) + ((nh / 3 - 2) / 2) > 0) ? (( nh / 3) + ((nh / 3 - 2) / 2)) : (nh / 3);
    

    for (int i = 0; i < 2; i++) {

        // Calcola le lunghezze effettive delle stringhe
        int textLen = strlen(menuBtn[i]);

        // Calcola la colonna per centrare l'intera combinazione
        int col = ((nw / 2) - textLen) / 2;

        // fprintf(inputFile,"Row: %d, Col: %d\n", initialrow + i, col);
        // fflush(inputFile);

        // Stampa la riga centrata
        mvwprintw(control, initialrow2 + i, col, "%s", menuBtn[i]);
        wrefresh(control);
    }
}

void resizeHandler(){
    if (mode == PLAY){
        getmaxyx(stdscr, nh, nw);  /* get the new screen size */
        scaleh = ((float)nh / (float)WINDOW_LENGTH);
        scalew = (float)nw / (float)WINDOW_WIDTH;
        endwin();
        initscr();
        start_color();
        curs_set(0);
        noecho();
        werase(win);
        werase(control);
        werase(stdscr);
        
        wrefresh(stdscr);
        win = newwin(nh, (int)(nw / 2) - 1, 0, 0); 
        control = newwin(nh, (int)(nw / 2) - 1, 0, (int)(nw / 2) + 1);
        box(win, 0, 0); 
        wrefresh(win);      
        box(control, 0, 0);
        wrefresh(control);
        btnSetUp((int)(((float)nh/2)/2),(int)((((float)nw / 2) - 35)/2));
        drawBtn(99); //to make all the buttons white
        drawInfo();
    }else if( mode == MENU && disp == CHOOSENAME){
        getmaxyx(stdscr, nh, nw);  /* get the new screen size */
        scaleh = ((float)nh / (float)WINDOW_LENGTH);
        scalew = (float)nw / (float)WINDOW_WIDTH;
        endwin();
        initscr();
        start_color();
        curs_set(0);
        noecho();
        werase(win);
        werase(control);
        werase(stdscr);
        win = newwin(nh, (int)(nw / 2) - 1, 0, 0); 
        control = newwin(nh, (int)(nw / 2) - 1, 0, (int)(nw / 2) + 1);

        drawName();
    }else if( mode == MENU && disp == CHOOSEBUTTON){
        getmaxyx(stdscr, nh, nw);  /* get the new screen size */
        scaleh = ((float)nh / (float)WINDOW_LENGTH);
        scalew = (float)nw / (float)WINDOW_WIDTH;
        endwin();
        initscr();
        start_color();
        curs_set(0);
        noecho();
        werase(win);
        werase(control);
        werase(stdscr);
        win = newwin(nh, (int)(nw / 2) - 1, 0, 0); 
        control = newwin(nh, (int)(nw / 2) - 1, 0, (int)(nw / 2) + 1);

        btnSetUp((int)(((float)nh/2)/2),(int)((((float)nw / 2) - 35)/2));
        drawBtn(99); //to make all the buttons white
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

    // Salva i dati direttamente in inputStatus
    strcpy(inputStatus.name, cJSON_GetObjectItemCaseSensitive(json, "PlayerName")->valuestring);

    // Aggiorna le variabili globali
    numTarget = cJSON_GetObjectItemCaseSensitive(json, "TargetNumber")->valueint;
    numObstacle = cJSON_GetObjectItemCaseSensitive(json, "ObstacleNumber")->valueint;

    // Per array
    cJSON *numbersArray = cJSON_GetObjectItemCaseSensitive(json, "DefaultBTN"); // questo è un array
    LOGINPUTCONFIGURATION(numbersArray);

    cJSON_Delete(json); // pulisci
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <fd_str>\n", argv[0]);
        exit(1);
    }
    
    // Opening log file
    inputFile = fopen("log/input.log", "a");
     
    if (inputFile == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }

    //Open config file
    settingsfile = fopen("appsettings.json", "r");
    if (settingsfile == NULL) {
        perror("Error opening the file");
        return EXIT_FAILURE;//1
    }

    readConfig();

    // FDs reading
    char *fd_str = argv[1];
    int index = 0;
    
    char *token = strtok(fd_str, ",");
    token = strtok(NULL, ","); 

    // FDs extraction
    while (token != NULL && index < 4) {
        fds[index] = atoi(token);
        index++;
        token = strtok(NULL, ",");
    }

    pid = (int)getpid();
    char dataWrite [80] ;
    snprintf(dataWrite, sizeof(dataWrite), "i%d,", pid);

    if(writeSecure("log/passParam.txt", dataWrite,1,'a') == -1){
        perror("[INPUT]Error in writing in passParam.txt");
        exit(1);
    }

    //Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    struct sigaction sa;
    //memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags = SA_RESTART;  // Riavvia read/write interrotte
    
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Error while setting sigaction for SIGTERM");
        exit(EXIT_FAILURE);
    }

    struct sigaction res;
    res.sa_handler = resizeHandler;
    sigemptyset(&res.sa_mask);
    res.sa_flags = SA_RESTART;

    if (sigaction(SIGWINCH, &res, NULL) == -1) {
        perror("Error while setting sigaction for SIGWINCH");
        exit(EXIT_FAILURE);
    }

    initscr();
    start_color();
    curs_set(0);
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);

    init_pair(1, COLOR_RED , COLOR_BLACK);  // Testo arancione su sfondo nero

    getmaxyx(stdscr, nh, nw);
    win = newwin(nh, (int)(nw / 2), 0, 0); 
    control = newwin(nh, (int)(nw / 2) - 1, 0, (int)(nw / 2) + 1);

    mainMenu();
    btnSetUp((int)(((float)nh/2)/2),(int)((((float)nw / 2) - 35)/2));
    mode = PLAY;

    inputMsgInit(&inputStatus);

    while (1) {

        int ch;

        if(mode == PLAY){
            if ((ch = getch()) == ERR) {
                //nessun tasto premuto dall'utente
                usleep(100000);
                werase(win);
                werase(control);
                box(win, 0, 0);
                wrefresh(win); 
                box(control, 0 ,0);               
                wrefresh(control);   
                drawBtn(99); //to make all the buttons white
                drawInfo();
            }else {             
                
                inputStatus.msg = 'I';
                strcpy(inputStatus.input, "reset");

                int btn;
                // int ch = 99;
                if (ch == btnValues[0]) {
                    btn = LEFTUP;
                    strcpy(inputStatus.input, moves[btn]);
                } else if (ch == btnValues[1]) {
                    btn = UP;
                    strcpy(inputStatus.input, moves[btn]);
                } else if (ch == btnValues[2]) {
                    btn = RIGHTUP;
                    strcpy(inputStatus.input, moves[btn]);
                } else if (ch == btnValues[3]) {
                    btn = LEFT;
                    strcpy(inputStatus.input, moves[btn]);
                } else if (ch == btnValues[4]) {
                    btn = CENTER;
                    strcpy(inputStatus.input, moves[btn]);
                } else if (ch == btnValues[5]) {
                    btn = RIGHT;
                    strcpy(inputStatus.input, moves[btn]);
                } else if (ch == btnValues[6]) {
                    btn = LEFTDOWN;
                    strcpy(inputStatus.input, moves[btn]);
                } else if (ch == btnValues[7]) {
                    btn = DOWN;
                    strcpy(inputStatus.input, moves[btn]);
                } else if (ch == btnValues[8]) {
                    btn = RIGHTDOWN;
                    strcpy(inputStatus.input, moves[btn]);
                }else if (ch == MY_KEY_p || ch == MY_KEY_P){
                    btn = 112; //Pause
                    inputStatus.msg = 'P';
                    mode = PAUSE;
                    LOGSTUATUS(mode);
                    LOGDRONEINFO(inputStatus.droneInfo);
                } else if (ch == MY_KEY_q || ch == MY_KEY_Q){
                    btn = 109; //Quit
                    inputStatus.msg = 'q';

                }else{
                    btn = 99;   //Any of the direction buttons pressed
                } 

                werase(win);
                box(win, 0, 0);       
                wrefresh(win);
                drawBtn(btn);
                usleep(100000);
                werase(win);
                box(win, 0, 0);       
                wrefresh(win);
                drawBtn(99); //to make all the buttons white

                LOGDIRECTION(inputStatus.input);

                // Send the message to the blackboard
                
                writeInputMsg(fds[askwr], &inputStatus, 
                            "[INPUT] Error sending message", inputFile);

                readInputMsg(fds[recrd], &inputStatus, 
                            "Error reading ack", inputFile);
                if(inputStatus.msg == 'A'){
                    LOGACK(inputStatus);
                }
                if(inputStatus.msg == 'S'){                    

                    inputStatus.msg = 'R';

                    writeInputMsg(fds[askwr], &inputStatus, 
                            "[INPUT] Error sending message", inputFile);
                    
                }

            }  
        }else if(mode == PAUSE){

            while ((ch = getch()) != MY_KEY_P && ch != MY_KEY_p && ch != MY_KEY_Q && ch != MY_KEY_q) {
                pauseMenu();
                usleep(10000);
            }
            if(ch == MY_KEY_P || ch == MY_KEY_p){
                
                wrefresh(stdscr);
                mode = PLAY;
                LOGSTUATUS(mode);
                inputStatus.msg = 'P';
                strcpy(inputStatus.input, "reset");

                writeInputMsg(fds[askwr], &inputStatus, 
                            "[INPUT] Error sending play", inputFile);

            }else if(ch == MY_KEY_Q || ch == MY_KEY_q){

                wrefresh(stdscr);
                inputStatus.msg = 'q';
                strcpy(inputStatus.input, "reset");

                writeInputMsg(fds[askwr], &inputStatus, 
                            "[INPUT] Error sending play", inputFile);

                readInputMsg(fds[recrd], &inputStatus, 
                            "Error reading ack", inputFile);

                if(inputStatus.msg == 'S'){

                    inputStatus.msg = 'R';
                    
                    writeInputMsg(fds[askwr], &inputStatus, 
                            "[INPUT] Error sending message", inputFile);
                    
                }
            }
        }

    }
    return 0;
}