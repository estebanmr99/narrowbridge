#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

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

    //printf("%d", vehiculeQueueR);

    while(ambulanceL || !isSafeR){ //se maneja por la funcion que administra el puente, 0 no es seguro, 1 es seguro
        isSafeR = 0;
        if(vehiculesOnBridge == 0){
            isSafeR = 1;
            printf("Ya puede salir");
        }
        //printf("No es seguro para %lu porque hay una ambulancia del otro lado\n", pthread_self());
        //pthread_cond_wait(&isSafeRcon, &bridge[bridgeLen - 1]);
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
    
    --vehiculesOnBridge;

    --vehiculeQueueR;
    
    printf("Vehiculo %lu salio.... derecha\n",pthread_self());
    printf("vehiculos en el puente %d\n", vehiculesOnBridge);

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
            printf("Ya puede salir");
        }
        //printf("%d", ambulanceR);
        //printf("No es seguro para %lu porque hay una ambulancia del otro lado\n", pthread_self());
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
    
    --vehiculesOnBridge;

    --vehiculeQueueL;
    
    printf("Vehiculo %lu salio....\n",pthread_self());
    printf("vehiculos en el puente %d\n", vehiculesOnBridge);

    ambulanceL = 0;

    pthread_exit(0);
}

void *createVehiculeR(void *args){
    for(int i=0;i<4;i++){
        pthread_create(&vehiclesPR[i],NULL,vehicleRtoL,NULL);
        sleep(1);
    }
    
    for(int i=0;i<4;i++){
      pthread_join(vehiclesPR[i],NULL);
    }

    pthread_exit(0);
}

void *createVehiculeL(){
    for(int i=0;i<5;i++){
        pthread_create(&vehiclesPL[i],NULL,vehicleLtoR,NULL);
        sleep(1);
    }

    for(int i=0;i<5;i++){
        pthread_join(vehiclesPL[i],NULL);
    }

    pthread_exit(0);
}

void *carnage(void *args){

    pthread_t createVehiculeRThread;
    pthread_t createVehiculeLThread;
    
    pthread_create(&createVehiculeLThread,NULL,createVehiculeL,NULL);
    pthread_create(&createVehiculeRThread,NULL,createVehiculeR,NULL);

    pthread_join(createVehiculeRThread,NULL);
    pthread_join(createVehiculeLThread,NULL);

    //unlock(&safeR);
    //sleep(1);
    //isSafeR = 1;
    //pthread_cond_signal(&isSafeRcon);
    //sleep(1);
    //pthread_cond_signal(&isSafeRcon);


    pthread_exit(0);
}

int main(){
    srand(time(NULL));   // Initialization, should only be called once.
    scanf("%d",&bridgeLen);
  
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

    expAverageL = 4;
    expAverageR = 4;
    vehicleVelocityL = 4;
    vehicleVelocityR = 3;//segundos
    K1 =  6;
    K2 =  5;
    greenLightTimeL = 5;
    greenLightTimeR = 4;
    vehiclesL = 5;
    vehiclesR = 4;

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
