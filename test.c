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
int vehicleVelocityL,vehicleVelocityR;
int K1,K2;
int greenLightTimeL,greenLightTimeR;
int vehiclesL,vehiclesR;
pthread_t *vehiclesPL;
pthread_t *vehiclesPR;
pthread_mutex_t *bridge;
pthread_cond_t isSafeLcon;
pthread_cond_t isSafeRcon;
int isSafeL, isSafeR;
int ambulanceL, ambulanceR;
int vehiculesOnBridge;
int vehiculeQueueL, vehiculeQueueR;
pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;


void *vehicleRtoL(void *args){
    printf("Vehiculo %lu creado.... derecha\n", pthread_self());
    
    ++vehiculeQueueR;
    int soynumero = vehiculeQueueR;

    int isAmb = rand() % 100;

    if (isAmb <= 5){
        printf("Vehiculo %lu es una ambulancia derecha\n", pthread_self());
        if (vehiculeQueueR == 1){
            ambulanceR = 1;
            isSafeL = 0;
        }
    }

    if(vehiculesOnBridge == 0)
        isSafeR = 1;

    while(ambulanceL || !isSafeR){ //se maneja por la funcion que administra el puente, 0 no es seguro, 1 es seguro
        isSafeR = 0;
        if(vehiculesOnBridge == 0){
            isSafeR = 1;
            printf("-------------------Ya puede salir---------------\n");
        }
        if(soynumero > 1){
            pthread_cond_wait(&isSafeRcon, &bridge[bridgeLen - 1]);
        }
    }

    pthread_cond_signal(&isSafeRcon);

    printf("avanza derecha\n");

    isSafeR = 1;

    ++vehiculesOnBridge;

    for(int i=bridgeLen-1;i>=0;i--){
        if (i != (bridgeLen - 1))
            lock(&bridge[i]);
        printf("V %lu en pos %d derecha\n",pthread_self(),i);
        sleep(vehicleVelocityR);
        unlock(&bridge[i]);
    }
    
    vehiculesOnBridge = vehiculesOnBridge - 1;

    --vehiculeQueueR;
    
    printf("Vehiculo %lu salio.... derecha\n",pthread_self());
    printf("--------------- Vehiculos en el puente %d\n", vehiculesOnBridge);

    ambulanceR = 0;

    pthread_exit(0);
}


void *vehicleLtoR(void *args){
    printf("Vehiculo %lu creado....\n", pthread_self());
    
    ++vehiculeQueueL;
    int soynumero = vehiculeQueueL;

    int isAmb = rand() % 100;

    if (isAmb <= 5){
        printf("Vehiculo %lu es una ambulancia\n", pthread_self());
        if (vehiculeQueueL == 1){
            ambulanceL = 1;
            isSafeR = 0;
        }
    }

    if(vehiculesOnBridge == 0)
        isSafeL = 1;

    //printf("%d", vehiculeQueueR);

    while(ambulanceR || !isSafeL){ //se maneja por la funcion que administra el puente, 0 no es seguro, 1 es seguro
        isSafeL = 0;
        if(vehiculesOnBridge == 0){
            isSafeL = 1;
            printf("-------------------Ya puede salir---------------\n");
        }
        if(soynumero > 1){
            pthread_cond_wait(&isSafeLcon, &bridge[0]);
        }
    }

    pthread_cond_signal(&isSafeLcon);

    printf("avanza\n");

    isSafeL = 1;

    ++vehiculesOnBridge;

    for(int i = 0; i < bridgeLen; i++){
        if (i != 0)
            lock(&bridge[i]);
        printf("V %lu en pos %d\n",pthread_self(),i);
        sleep(vehicleVelocityL);
        unlock(&bridge[i]);
    }
    
    vehiculesOnBridge = vehiculesOnBridge - 1;

    --vehiculeQueueL;
    
    printf("Vehiculo %lu salio....\n",pthread_self());
    printf("---------------Vehiculos en el puente %d\n", vehiculesOnBridge);

    ambulanceL = 0;

    pthread_exit(0);
}

void *createVehiculeR(void *args){
    for(int i=0;i<vehiclesR;i++){
        pthread_create(&vehiclesPR[i],NULL,vehicleRtoL,NULL);
        int time =  (-1 * expAverageR) * log(1 - (double)rand() / (double)RAND_MAX);
        sleep(1);
    }
    
    for(int i=0;i<vehiclesR;i++){
      pthread_join(vehiclesPR[i],NULL);
    }

    pthread_exit(0);
}

void *createVehiculeL(){
    for(int i=0;i<vehiclesL;i++){
        pthread_create(&vehiclesPL[i],NULL,vehicleLtoR,NULL);
        int time =  (-1 * expAverageL) * log(1 - (double)rand() / (double)RAND_MAX);
        sleep(1);
    }

    for(int i=0;i<vehiclesL;i++){
        pthread_join(vehiclesPL[i],NULL);
    }

    pthread_exit(0);
}

void *carnage(void *args){

    pthread_t createVehiculeRThread;
    pthread_t createVehiculeLThread;
    
    pthread_create(&createVehiculeRThread,NULL,createVehiculeR,NULL);
    pthread_create(&createVehiculeLThread,NULL,createVehiculeL,NULL);

    pthread_join(createVehiculeRThread,NULL);
    pthread_join(createVehiculeLThread,NULL);

    pthread_exit(0);
}

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
    vehicleVelocityL = atoi(token);

    fgets(line, sizeof(line), file);
    strtok(line, s);
    token = strtok(NULL, s);
    vehicleVelocityR = atoi(token);

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

int main(){

    readConfiguration();

    /*
    //////////// MENU QUE PODEMOS USAR ////////////

    int option;

    printf("\n                   NarrowBridge\n");
    printf("\n");
    printf("Escoja el modo a ejecutar:\n");
    printf("\n");
    printf("1) Carnage.\n");
    printf("2) Semaforos.\n");
    printf("3) Oficiales de transito.\n");
    printf("\n");
    printf("-> ");
    scanf("%d", &option);

    if (option == 1){
        //correr carnage
    }
    else if (option == 2){
        //correr semaforos
    }
    else if (option == 3){
        //correr oficiales de transito
    }
    else{
        printf("\nOpcion invalida\n");
    }
    //////////////////////////////////////////////
    */

    srand(time(NULL));   // Initialization, should only be called once.
  
    bridge = (pthread_mutex_t*) malloc(bridgeLen*sizeof(pthread_mutex_t));
    //condtitions = (pthread_cond_t*) malloc(bridgeLen*sizeof(pthread_cond_t));
    isSafeR = 0;
    isSafeL = 0;
    
    //pthread_mutex_init(&safeR,NULL);
    //pthread_mutex_init(&safeL,NULL);
    //---------------------------------//
    pthread_cond_init(&isSafeLcon,NULL);
    pthread_cond_init(&isSafeRcon,NULL);

    for(int i=0;i<bridgeLen;i++){
        pthread_mutex_init(&bridge[i],NULL);
    }

    vehiclesPL = (pthread_t*) malloc(vehiclesL*sizeof(pthread_t));
    vehiclesPR = (pthread_t*) malloc(vehiclesR*sizeof(pthread_t));

    ambulanceR = 0;
    ambulanceL = 0;

    vehiculeQueueL = 0;
    vehiculeQueueR = 0;

    vehiculesOnBridge = 0;

    pthread_t carnageThread;
    
    pthread_create(&carnageThread,NULL,carnage,NULL);

    pthread_join(carnageThread,NULL);

    pthread_exit(0);

    return 0;
}
