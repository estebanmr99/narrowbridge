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
pthread_mutex_t *bridge;
pthread_cond_t *condtitions;
pthread_cond_t isSafeLcon;
pthread_cond_t isSafeRcon;
int isSafeL;
int isSafeR;
pthread_mutex_t safeL;
pthread_mutex_t safeR;


void *vehicleRtoL(void *args){

    lock(&safeR);
    while(!isSafeR)//se maneja por la funcion que administra el puente, 0 no es seguro, 1 es seguro
        pthread_cond_wait(&isSafeRcon,&safeR);
    unlock(&safeR);   

    for(int i=bridgeLen-1;i>=0;i--){
        lock(&bridge[i]);
        //while(!isSafeR)//se maneja por la funcion que administra el puente, 0 no es seguro, 1 es seguro
        pthread_cond_wait(&condtitions[i],&bridge[i]);
        sleep(vehicleVelocityR);
        unlock(&bridge[i]);

    }
}


void *vehicleLtoR(int *canMove){

   
   
}







int main(){

    scanf("%d",&bridgeLen);
    bridge = (pthread_mutex_t*) malloc(bridgeLen*sizeof(pthread_mutex_t));
    condtitions = (pthread_cond_t*) malloc(bridgeLen*sizeof(pthread_cond_t));
    isSafeR = 1;
    isSafeL = 0;
    pthread_mutex_init(&safeR,NULL);
    pthread_mutex_init(&safeL,NULL);
    //---------------------------------//
    pthread_cond_init(&isSafeLcon,NULL);
    pthread_cond_init(&isSafeRcon,NULL);

    for(int i=0;i<bridgeLen;i++){
        pthread_mutex_init(&bridge[i],NULL);
    }

    for(int i=0;i<bridgeLen;i++){
        pthread_cond_init(&condtitions[i],NULL);
    }


    expAverageL = 4;
    expAverageR = 4;
    vehicleVelocityL = 80;
    vehicleVelocityR = 120;
    K1 =  6;
    K2 =  5;
    greenLightTimeL = 5;
    greenLightTimeR = 4;
    vehiclesL = 10;
    vehiclesR = 12;




    return 0;
}