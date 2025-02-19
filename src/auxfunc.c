#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "auxfunc.h"
#include <time.h>

const char *moves[] = {"upleft", "up", "upright", "left", "center", "right", "downleft", "down", "downright"};
char jsonBuffer[MAX_FILE_SIZE];

int numTarget = 4;
int numObstacle = 9;

void handleLogFailure() {
    printf("Logging failed. Cleaning up resources...\n");
   
    exit(EXIT_FAILURE);
}

int writeSecure(const char* filename, const char* data, char mode) {
    int fd;
    
    if (mode == 'w') {
        // Apri il file solo per troncarlo, poi chiudilo immediatamente
        fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0666);
        if (fd == -1) {
            perror("Errore nell'apertura per troncamento");
            return -1;
        }
        close(fd);

        // Ora riapri il file normalmente
        fd = open(filename, O_WRONLY | O_CREAT, 0666);
    } else { 
        fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0666);
    }

    if (fd == -1) {
        perror("Errore nell'apertura del file");
        return -1;
    }

    // Scrive i dati nel file
    ssize_t len = write(fd, data, strlen(data));
    if (len < (ssize_t)strlen(data)) {
        perror("Errore nella scrittura del file");
        close(fd);
        return -1;
    }

    fsync(fd);
    close(fd);

    return 0;
}

int readSecure(const char* filename, char* data, size_t dataSize) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        return -1;
    }

    int fd = fileno(file);
    if (fd == -1) {
        perror("Errore nel recupero del file descriptor");
        fclose(file);
        return -1;
    }

    // Blocca il file per lettura condivisa
    while (flock(fd, LOCK_SH) == -1) {
        if (errno == EWOULDBLOCK) {
            usleep(100000);  // Pausa di 100 ms
        } else {
            perror("Errore nel blocco del file");
            fclose(file);
            return -1;
        }
    }

    // Legge i dati dal file
    size_t len = fread(data, sizeof(char), dataSize - 1, file);
    if (len == 0 && ferror(file)) {
        perror("Errore nella lettura del file");
        flock(fd, LOCK_UN);
        fclose(file);
        return -1;
    }
    data[len] = '\0';  // Assicura che la stringa sia null-terminata

    // Sblocca il file
    if (flock(fd, LOCK_UN) == -1) {
        perror("Errore nello sblocco del file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}


void writeMsg(int pipeFds, Message* msg, const char* error, FILE* file){
    if (write(pipeFds, msg, sizeof(Message)) == -1) {
        fprintf(file,"Error: %s\n", error);
        fflush(file);
        perror(error);
        exit(EXIT_FAILURE);
    }  
}

void readMsg(int pipeFds, Message* msgOut, const char* error, FILE* file){   
    if (read(pipeFds, msgOut, sizeof(Message)) == -1){
        fprintf(file, "Error: %s\n", error);
        fflush(file);
        perror(error);
        exit(EXIT_FAILURE);
    }
}

void writeInputMsg(int pipeFds, inputMessage* msg, const char* error, FILE* file){
    if (write(pipeFds, msg, sizeof(inputMessage)) == -1) {
        fprintf(file,"Error: %s\n", error);
        fflush(file);
        perror(error);
        exit(EXIT_FAILURE);
    }  
}

void readInputMsg(int pipeFds, inputMessage* msgOut, const char* error, FILE* file){
    if (read(pipeFds, msgOut, sizeof(inputMessage)) == -1){
        fprintf(file, "Error: %s\n", error);
        fflush(file);
        perror(error);
        exit(EXIT_FAILURE);
    }
}

void fdsRead (int argc, char* argv[], int* fds){
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <fd_str>\n", argv[0]);
        exit(1);
    }

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
}

int writePid(const char* file, char mode, char id) {
    int pid = (int)getpid();
    char dataWrite[80];
    snprintf(dataWrite, sizeof(dataWrite), "%c%d,", id, pid);

    if (writeSecure(file, dataWrite,mode) == -1) {
        perror("Error in writing in passParam.txt");
        exit(1);
    }

    return pid;
}


void printInputMessageToFile(FILE *file, inputMessage* msg) {
    fprintf(file, "\n");
    fprintf(file, "msg: %c\n", msg->msg);
    fprintf(file, "name: %s\n", msg->name);
    fprintf(file, "input: %s\n", msg->input);
    fprintf(file,"Score: %d\n", msg->score);
    fprintf(file, "droneInfo:\n");
    fprintf(file, "  x: %d\n", msg->droneInfo.x);
    fprintf(file, "  y: %d\n", msg->droneInfo.y);
    fprintf(file, "  speedX: %.2f\n", msg->droneInfo.speedX);
    fprintf(file, "  speedY: %.2f\n", msg->droneInfo.speedY);
    fprintf(file, "  forceX: %.2f\n", msg->droneInfo.forceX);
    fprintf(file, "  forceY: %.2f\n", msg->droneInfo.forceY);
    fflush(file);
}


void msgInit(Message* status){
    status->msg = 'R';
    strcpy(status->input,"Reset");
    status->drone.x = 0;
    status->drone.y = 0;
    status->drone.speedX = 0;
    status->drone.speedY = 0;
    status->drone.forceX = 0;
    status->drone.forceY = 0;
    
    for(int i = 0; i < MAX_TARGET; i++){
        status->targets.x[i] = 0;
        status->targets.y[i] = 0; 
        status->hit[i] = 0;
    }
    status->targets.number = 0;
    for(int i = 0; i < MAX_OBSTACLES; i++){
        status->obstacles.x[i] = 0;
        status->obstacles.y[i] = 0;
    }
}

void inputMsgInit(inputMessage* status){
    status->msg = 'R';
    strcpy(status->name,"Default");
    strcpy(status->input,"Reset");
    status->score = 0;
    status->droneInfo.x = 0;
    status->droneInfo.y = 0;
    status->droneInfo.speedX = 0;
    status->droneInfo.speedY = 0;
    status->droneInfo.forceX = 0;
    status->droneInfo.forceY = 0;
}

void handler(char id) {
    char log_entry[256];
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    snprintf(log_entry, sizeof(log_entry), "%c%02d:%02d:%02d,", id, 
             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    writeSecure("log/passParam.txt", log_entry, 'a');
}


// Funzione helper per ottenere il timestamp formattato
void getFormattedTime(char *buffer, unsigned long size) {
    time_t currentTime = time(NULL);
    snprintf(buffer, size, "%.*s", (int)(strlen(ctime(&currentTime)) - 1), ctime(&currentTime));
}