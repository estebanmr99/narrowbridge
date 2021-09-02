#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#define T 500
#define lock pthread_mutex_lock
#define unlock pthread_mutex_unlock

int bridgeLen;
int expAverageL,expAverageR;
int vehicleVelocityInfL,vehicleVelocityInfR;
int vehicleVelocitySupL,vehicleVelocitySupR;
int K1,K2;
int greenLightTimeL,greenLightTimeR;
int vehiclesL,vehiclesR;
int isSafeL, isSafeR;
int ambulanceL, ambulanceR;
int vehiclesOnBridge;
int vehicleQuantityL, vehicleQuantityR;
int bridgeDir;
int mode;
pthread_t *vehiclesPL;
pthread_t *vehiclesPR;
pthread_mutex_t *bridge;
pthread_cond_t isSafeLcon;
pthread_cond_t isSafeRcon;

void crossBridge(int dir, int vel, unsigned long int id){
    ++vehiclesOnBridge;
    int start = dir == 1 ? 0 : bridgeLen - 1; 
    int end = dir == -1 ? -1 : bridgeLen;

    lock(&bridge[start]);
    printf("V %lu en pos %d en la direccion: %d\n", id, start, dir);
    for(int i = start + dir; i != end; i += dir){
        printf("V %lu en pos %d en la direccion: %d\n", id, i, dir);
        sleep(vel);
        lock(&bridge[i]);
        unlock(&bridge[i - dir]);
    }
    unlock(&bridge[end - dir]);
    --vehiclesOnBridge;
}

void leavingBridgeCarnage(int dir){
    if(((dir == 1) || vehicleQuantityR == 0) && vehiclesOnBridge == 0){
        if (ambulanceR){
            ambulanceR = 0;
        }
        bridgeDir = 1;
        pthread_cond_broadcast(&isSafeRcon);
    }

    if(((dir == -1) || vehicleQuantityL == 0) && vehiclesOnBridge == 0){
        if (ambulanceL){
            ambulanceL = 0;
        }
        bridgeDir = -1;
        pthread_cond_broadcast(&isSafeLcon);
    }
}

int isSafeCarnage(int dir){

    if(vehiclesOnBridge == 0){
        bridgeDir = dir;
    }

    if(dir == bridgeDir){
        if((dir == 1) && !ambulanceR){ //positivo es izquierda - > derecha, negativo es derecha - > izquerda
            return 1;
        }else if((dir == -1) && !ambulanceL){ //positivo es izquierda - > derecha, negativo es derecha - > izquerda
            return 1;
        }
        
        return 0;
    }else{
        return 0;
    }
}


void *vehicle(void *direction){
    int dir = *(int *)direction;

    printf("Vehiculo %lu creado.... %d\n", pthread_self(), dir);

    dir == 1 ? ++vehicleQuantityL : ++vehicleQuantityR;
    int position = dir == 1 ? vehicleQuantityL : vehicleQuantityR;
    int isAmb = rand() % 100;
    int velocity = rand() % ((vehicleVelocitySupR - vehicleVelocityInfR) + 1) + vehicleVelocityInfR; // validar la direccion
    int ambulance;
    velocity = T / velocity;

    if (isAmb <= 5){
        ambulance = 1;
        printf("Vehiculo %lu es una ambulancia de lado: %d\n", pthread_self(), dir);
        if (position){
            ambulanceL = dir == 1 ? 1 : ambulanceL;
            ambulanceR = dir == -1 ? 1 : ambulanceR;
            bridgeDir = dir;
        }
    }

    int isSafe = isSafeCarnage(dir);

    while (!isSafe){
        int posLock = dir == 1 ? 0 : bridgeLen - 1; 

        lock(&bridge[posLock]);
        if (dir == 1){
            pthread_cond_wait(&isSafeLcon, &bridge[posLock]);
        }else{
            pthread_cond_wait(&isSafeRcon, &bridge[posLock]);
        }
        unlock(&bridge[posLock]);

        isSafe = isSafeCarnage(dir);
    }

    dir == 1 ? --vehicleQuantityL : --vehicleQuantityR;

    printf("avanza %d\n", dir);

    crossBridge(dir, velocity, pthread_self());

    leavingBridgeCarnage(dir);

    printf("Vehiculo %lu salio.... %d\n",pthread_self(), dir);
    printf("--------------- Vehiculos en el puente %d\n", vehiclesOnBridge);

}

//---------------------------------------------------------------------------------------------------------------------------------------
void *createVehiculeR(void *args){
    int dir = -1;
    for(int i=0;i<vehiclesR;i++){
        int time =  (-1 * expAverageR) * log(1 - (double)rand() / (double)RAND_MAX);
        sleep(time);
        pthread_create(&vehiclesPR[i],NULL,vehicle,&dir);
    }

    for(int i=0;i<vehiclesR;i++){
      pthread_join(vehiclesPR[i],NULL);
    }

    pthread_exit(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
void *createVehiculeL(){
    for(int i=0;i<vehiclesL;i++){
        int dir = 1;
        int time =  (-1 * expAverageL) * log(1 - (double)rand() / (double)RAND_MAX);
        sleep(time);
        pthread_create(&vehiclesPL[i],NULL,vehicle,&dir);
    }

    for(int i=0;i<vehiclesL;i++){
        pthread_join(vehiclesPL[i],NULL);
    }

    pthread_exit(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
void *carnage(void *args){

    pthread_t createVehiculeRThread;
    pthread_t createVehiculeLThread;

    pthread_create(&createVehiculeRThread,NULL,createVehiculeR,NULL);
    pthread_create(&createVehiculeLThread,NULL,createVehiculeL,NULL);

    pthread_join(createVehiculeRThread,NULL);
    pthread_join(createVehiculeLThread,NULL);

    pthread_exit(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
void readConfiguration(){
    char const* const fileName = "config.txt";
    FILE* file = fopen(fileName, "r");  //abre archivo de configuracion
    char line[256];
    const char s[4] = " = ";
    char *token;

    fgets(line, sizeof(line), file);
    strtok(line, s);
    token = strtok(NULL, s);
    bridgeLen = atoi(token);

    fgets(line, sizeof(line), file);
    strtok(line, s);
    token = strtok(NULL, s);
    expAverageL = atoi(token);

    fgets(line, sizeof(line), file);
    strtok(line, s);
    token = strtok(NULL, s);
    expAverageR = atoi(token);

    fgets(line, sizeof(line), file);
    strtok(line, s);
    token = strtok(NULL, s);
    vehicleVelocityInfL = atoi(token);

    fgets(line, sizeof(line), file);
    strtok(line, s);
    token = strtok(NULL, s);
    vehicleVelocityInfR = atoi(token);

    fgets(line, sizeof(line), file);
    strtok(line, s);
    token = strtok(NULL, s);
    vehicleVelocitySupL = atoi(token);

    fgets(line, sizeof(line), file);
    strtok(line, s);
    token = strtok(NULL, s);
    vehicleVelocitySupR = atoi(token);

    fgets(line, sizeof(line), file);
    strtok(line, s);
    token = strtok(NULL, s);
    K1 = atoi(token);

    fgets(line, sizeof(line), file);
    strtok(line, s);
    token = strtok(NULL, s);
    K2 = atoi(token);

    fgets(line, sizeof(line), file);
    strtok(line, s);
    token = strtok(NULL, s);
    greenLightTimeL = atoi(token);

    fgets(line, sizeof(line), file);
    strtok(line, s);
    token = strtok(NULL, s);
    greenLightTimeR = atoi(token);

    fgets(line, sizeof(line), file);
    strtok(line, s);
    token = strtok(NULL, s);
    vehiclesL = atoi(token);

    fgets(line, sizeof(line), file);
    strtok(line, s);
    token = strtok(NULL, s);
    vehiclesR = atoi(token);

    fclose(file);
}

//---------------------------------------------------------------------------------------------------------------------------------------
int main(){
    readConfiguration();

    srand(time(NULL));   // Initialization, should only be called once.

    bridge = (pthread_mutex_t*) malloc(bridgeLen*sizeof(pthread_mutex_t));
    isSafeR = 0;
    isSafeL = 0;

    pthread_cond_init(&isSafeLcon,NULL);
    pthread_cond_init(&isSafeRcon,NULL);

    for(int i=0;i<bridgeLen;i++){
        pthread_mutex_init(&bridge[i],NULL);
    }

    vehiclesPL = (pthread_t*) malloc(vehiclesL*sizeof(pthread_t));
    vehiclesPR = (pthread_t*) malloc(vehiclesR*sizeof(pthread_t));

    ambulanceR = 0;
    ambulanceL = 0;

    vehicleQuantityL = 0;
    vehicleQuantityR = 0;

    vehiclesOnBridge = 0;

    bridgeDir = 0;

    printf("\n                   NarrowBridge\n");
    printf("\n");
    printf("Escoja el modo a ejecutar:\n");
    printf("\n");
    printf("1) Carnage.\n");
    printf("2) Semaforos.\n");
    printf("3) Oficiales de transito.\n");
    printf("\n");
    printf("-> ");
    scanf("%d", &mode);

    if (mode == 1){

      printf("\n");
      pthread_t carnageThread;

      pthread_create(&carnageThread,NULL,carnage,NULL);

      pthread_join(carnageThread,NULL);
    }
    else if (mode == 2){
      printf("\nProximamente...\n");
    }
    else if (mode == 3){
      printf("\nProximamente...\n");
    }
    else{
      printf("\nOpcion invalida\n");
    }

    pthread_exit(0);

    return 0;
}
