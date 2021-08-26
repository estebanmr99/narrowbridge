#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#define T 500
#define lock pthread_mutex_lock
#define unlock pthread_mutex_unlock 

int bridgeLen;
int expAverageL,expAverageR;
int vehicleVelocityL,vehicleVelocityR;
int K1,K2;
int greenLightTimeL,greenLightTimeR;
int vehiclesL,vehiclesR;
pthread_t *vehicles;
pthread_mutex_t *bridge;
pthread_cond_t *condtitions;
pthread_cond_t isSafeLcon;
pthread_cond_t isSafeRcon;
int isSafeL;
int isSafeR;
pthread_mutex_t safeL;
pthread_mutex_t safeR;


void *vehicleRtoL(void *args){

    printf("Vehiculo %lu creado....\n",pthread_self());
    ////////////////////////////////////////////////////////
    lock(&safeR);
    while(!isSafeR){//se maneja por la funcion que administra el puente, 0 no es seguro, 1 es seguro
        printf("No es seguro para %lu\n",pthread_self());
        pthread_cond_wait(&isSafeRcon,&safeR);
    }   
    unlock(&safeR);   
    printf("avanza\n");


    for(int i=bridgeLen-1;i>=0;i--){
        lock(&bridge[i]);
        printf("V %lu en pos %d\n",pthread_self(),i);
        sleep(vehicleVelocityR);
        unlock(&bridge[i]);
    }

    printf("Vehiculo %lu salio....\n",pthread_self());
    pthread_exit(0);
}   


void *vehicleLtoR(int *canMove){

   
   
}

void *createVehicles(void *args){
    
    //lock(&safeR);
    for(int i=0;i<4;i++){
        isSafeR=0;
        pthread_create(&vehicles[i],NULL,vehicleRtoL,NULL);
        
        pthread_cond_signal(&isSafeRcon);
        isSafeR=1;
       
    }
    //unlock(&safeR);
    for(int i=0;i<4;i++){
      pthread_join(vehicles[i],NULL);
    }

    pthread_exit(0);
}

int main(){
   
    scanf("%d",&bridgeLen);
  
    bridge = (pthread_mutex_t*) malloc(bridgeLen*sizeof(pthread_mutex_t));
    condtitions = (pthread_cond_t*) malloc(bridgeLen*sizeof(pthread_cond_t));
    vehicles = (pthread_t*) malloc(3*sizeof(pthread_t));
    isSafeR = 0;
    isSafeL = 0;
    
    pthread_mutex_init(&safeR,NULL);
    pthread_mutex_init(&safeL,NULL);
    //---------------------------------//
    pthread_cond_init(&isSafeLcon,NULL);
    pthread_cond_init(&isSafeRcon,NULL);

    for(int i=0;i<bridgeLen;i++){
        pthread_mutex_init(&bridge[i],NULL);
    }

    expAverageL = 4;
    expAverageR = 4;
    vehicleVelocityL = 80;
    vehicleVelocityR = 3;//segundos
    K1 =  6;
    K2 =  5;
    greenLightTimeL = 5;
    greenLightTimeR = 4;
    vehiclesL = 10;
    vehiclesR = 12;

    //printf("oooo");
    pthread_t creatingCars;
    
    pthread_create(&creatingCars,NULL,createVehicles,NULL);



    pthread_join(creatingCars,NULL);

    return 0;
}
