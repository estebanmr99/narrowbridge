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
int K1Temp,K2Temp; // Variables pasan cambiando
int greenLightTimeL,greenLightTimeR;
int vehiclesL,vehiclesR;
int totalVehicles;  // Variable pasa cambiando
int vehiclesLCounter,vehiclesRCounter;  // Variables pasan cambiando
int ambulanceL, ambulanceR; // Variables pasan cambiando
int vehiclesOnBridge; // Variable pasa cambiando
int vehicleQuantityL, vehicleQuantityR; // Variables pasan cambiando
int bridgeDir; // Variable pasa cambiando
int mode;
int bridgeDirReal; // Variable pasa cambiando
int firstCar;
pthread_t *vehiclesPL;
pthread_t *vehiclesPR;
pthread_mutex_t *bridge;
pthread_mutex_t policeLeftMutex, policeRightMutex;
pthread_mutex_t K1TempMutex, K2TempMutex;
pthread_mutex_t totalVehiclesMutex;
pthread_mutex_t vehiclesLCounterMutex, vehiclesRCounterMutex;
pthread_mutex_t ambulanceLMutex, ambulanceRMutex;
pthread_mutex_t vehiclesOnBridgeMutex;
pthread_mutex_t vehicleQuantityLMutex, vehicleQuantityRMutex;
pthread_mutex_t bridgeDirMutex;
pthread_mutex_t bridgeDirRealMutex;
pthread_cond_t isSafeLcon, isSafeRcon;
pthread_cond_t policeRightCond, policeLeftCond;

void crossBridge(int dir, int vel, unsigned long int id){
    int start = dir == 1 ? 0 : bridgeLen - 1;
    int end = dir == -1 ? -1 : bridgeLen;

    lock(&bridge[start]);
    lock(&vehiclesOnBridgeMutex);
    ++vehiclesOnBridge;
    unlock(&vehiclesOnBridgeMutex);
    printf("V %lu en pos %d en la direccion: %d\n", id, start, dir);
    for(int i = start + dir; i != end; i += dir){
        printf("V %lu en pos %d en la direccion: %d\n", id, i, dir);
        sleep(vel);
        lock(&bridge[i]);
        unlock(&bridge[i - dir]);
    }
    unlock(&bridge[end - dir]);
    lock(&vehiclesOnBridgeMutex);
    --vehiclesOnBridge;
    unlock(&vehiclesOnBridgeMutex);

    if(dir == 1 && mode == 3){
        lock(&vehiclesLCounterMutex);
        --vehiclesLCounter;
        unlock(&vehiclesLCounterMutex);
    } else if (dir == -1 && mode == 3){
        lock(&vehiclesRCounterMutex);
        --vehiclesRCounter;
        unlock(&vehiclesRCounterMutex);
    } else if (mode == 2) {
        lock(&totalVehiclesMutex);
        --totalVehicles;
        unlock(&totalVehiclesMutex);
    }

    printf("Vehiculo %lu salio.... %d\n --------------- Vehiculos en el puente %d\n", pthread_self(), dir, vehiclesOnBridge);
}

void leavingBridgeCarnage(int dir){
    if(((dir == -1) || vehicleQuantityR == 0) && vehiclesOnBridge == 0){
        if (ambulanceR){
            lock(&ambulanceRMutex);
            ambulanceR = 0;
            unlock(&ambulanceRMutex);
        }
        pthread_cond_broadcast(&isSafeLcon);
    }else if(((dir == 1) || vehicleQuantityL == 0) && vehiclesOnBridge == 0){
        if (ambulanceL){
            lock(&ambulanceLMutex);
            ambulanceL = 0;
            unlock(&ambulanceLMutex);
        }
        pthread_cond_broadcast(&isSafeRcon);
    }
}

void leavingBridgeTrafficPolice(int dir){
    if((dir == -1) && vehiclesOnBridge == 0){
        printf("\nleaving bridge R\n\n");
        lock(&bridgeDirRealMutex);
        bridgeDirReal = 1;
        unlock(&bridgeDirRealMutex);
        pthread_cond_signal(&policeLeftCond);
    }else if((dir == 1) && vehiclesOnBridge == 0){
        printf("\nleaving bridge L\n\n");
        lock(&bridgeDirRealMutex);
        bridgeDirReal = -1;
        unlock(&bridgeDirRealMutex);
        pthread_cond_signal(&policeRightCond);
    }
}
// Funcion para el ultimo vehiculo que deja el puente
void leavingBridgeSemaphore(int dir) {
    if(dir == -1 && vehiclesOnBridge == 0){ // Si soy el ultimo vehiculo hacia la izquierda
        if (ambulanceR){                    // y soy una ambulancia
            lock(&ambulanceRMutex);
            ambulanceR = 0;                 // Apago bandera de ambulancia
            unlock(&ambulanceRMutex);
            if(bridgeDir == dir){           // Doy paso al lado que este en verde
                pthread_cond_broadcast(&isSafeRcon);
            } else{
                lock(&bridgeDirRealMutex);
                bridgeDirReal *= -1;
                unlock(&bridgeDirRealMutex);
                pthread_cond_broadcast(&isSafeLcon);
            }
            return;
        }
        lock(&bridgeDirRealMutex);          // Si no soy una ambulancia
        bridgeDirReal *= -1;                // doy paso al otro lado del puente
        unlock(&bridgeDirRealMutex);
        pthread_cond_broadcast(&isSafeLcon);
    }else if(dir == 1 && vehiclesOnBridge == 0){ // Si soy el ultimo vehiculo hacia la derecha
        if (ambulanceL){                         // y soy una ambulancia
            lock(&ambulanceLMutex);
            ambulanceL = 0;                      // Apago bandera de ambulancia
            unlock(&ambulanceLMutex);
            if(bridgeDir == dir){                // Doy paso al lado que este en verde
                pthread_cond_broadcast(&isSafeLcon);
            } else{
                lock(&bridgeDirRealMutex);
                bridgeDirReal *= -1;
                unlock(&bridgeDirRealMutex);
                pthread_cond_broadcast(&isSafeRcon);
            }
            return;
        }
        lock(&bridgeDirRealMutex);            // Si no soy una ambulancia
        bridgeDirReal *= -1;                  // doy paso al otro lado del puente
        unlock(&bridgeDirRealMutex);
        pthread_cond_broadcast(&isSafeRcon);
    }
}

int isSafeCarnage(int dir){
    if(vehiclesOnBridge == 0){
        lock(&bridgeDirMutex);
        bridgeDir = dir;
        unlock(&bridgeDirMutex);
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


int isSafeTrafficPolice(int dir, int ambulance, int position){
    if(dir == bridgeDirReal){
        if((dir == 1 && K1Temp > 0)){
            lock(&K1TempMutex);
            --K1Temp;
            unlock(&K1TempMutex);
            return 1;
        }else if (dir == -1 && K2Temp > 0){
            lock(&K2TempMutex);
            --K2Temp;
            unlock(&K2TempMutex);
            return 1;
        }else if(ambulance && position == 1){
            if((dir == 1 && K1Temp == 0) || (dir == -1 && K2Temp == 0)){
                return 1;
            }
        }
        return 0;
    }else{
        return 0;
    }
}

int isSafeSemaphore(int dir, int ambulance){
    if(dir == bridgeDirReal){   // Si no vienen carros en la direccion contraria
        if(dir == bridgeDir){   // Si el semaforo esta en verde
            if((dir == 1) && !ambulanceR){ // Si no hay ambulancias del otro lado
                return 1;   // Puedo pasar
            }else if((dir == -1) && !ambulanceL){ // Si no hay ambulancias del otro lado
                return 1;   // Puedo pasar
            }
        } else if(ambulance){   // Si soy una ambulancia
            return 1;   // Puedo pasar
        }

        return 0;
    }else{
        return 0;
    }
}

void *vehicle(void *direction){
    int dir = *(int *)direction;
    printf("Vehiculo %lu creado.... %d\n", pthread_self(), dir);

    if(!firstCar && mode == 3){
        firstCar = 1;
        lock(&bridgeDirRealMutex);
        bridgeDirReal = dir;
        unlock(&bridgeDirRealMutex);
        lock(&bridgeDirMutex);
        bridgeDir = dir;
        unlock(&bridgeDirMutex);
    }

    if (dir == 1){
        lock(&vehicleQuantityLMutex);
        ++vehicleQuantityL;
        unlock(&vehicleQuantityLMutex);
    }else{
        lock(&vehicleQuantityRMutex);
        ++vehicleQuantityR;
        unlock(&vehicleQuantityRMutex);
    }
    int position = dir == 1 ? vehicleQuantityL : vehicleQuantityR;
    int isAmb = rand() % 100;
    int velocity = 0;
    if (dir == -1){
        velocity = rand() % ((vehicleVelocitySupR - vehicleVelocityInfR) + 1) + vehicleVelocityInfR;
    }else{
        velocity = rand() % ((vehicleVelocitySupL - vehicleVelocityInfL) + 1) + vehicleVelocityInfL;
    }
    int ambulance = 0;
    velocity = T / velocity;

    if (isAmb <= 5){
        ambulance = 1;
        printf("Vehiculo %lu es una ambulancia de lado: %d\n", pthread_self(), dir);
        if (position == 1){
            lock(&ambulanceLMutex);
            ambulanceL = dir == 1 ? 1 : ambulanceL;
            unlock(&ambulanceLMutex);
            lock(&ambulanceRMutex);
            ambulanceR = dir == -1 ? 1 : ambulanceR;
            unlock(&ambulanceRMutex);
        }
    }

    int isSafe = 0;
    if (mode == 1){
        isSafe = isSafeCarnage(dir);
    }else if (mode == 2){
        isSafe = isSafeSemaphore(dir, ambulance);
    }else if (mode == 3){
        isSafe = 0;
        if(position == 1){
            dir == 1 ? pthread_cond_signal(&policeLeftCond) : pthread_cond_signal(&policeRightCond);
        }
    }

    while (!isSafe){
        int posLock = dir == 1 ? 0 : bridgeLen - 1;

        lock(&bridge[posLock]);
        if (dir == 1){
            pthread_cond_wait(&isSafeLcon, &bridge[posLock]);
            position = vehicleQuantityL;
        }else{
            pthread_cond_wait(&isSafeRcon, &bridge[posLock]);
            position = vehicleQuantityR;
        }
        unlock(&bridge[posLock]);

        if (mode == 1){
            isSafe = isSafeCarnage(dir);
        }else if (mode == 2){
            isSafe = isSafeSemaphore(dir, ambulance);
        }else if (mode == 3){
            isSafe = isSafeTrafficPolice(dir, ambulance, position);
        }
    }

    if (dir == 1){
        lock(&vehicleQuantityLMutex);
        --vehicleQuantityL;
        unlock(&vehicleQuantityLMutex);
    }else{
        lock(&vehicleQuantityRMutex);
        --vehicleQuantityR;
        unlock(&vehicleQuantityRMutex);
    }

    crossBridge(dir, velocity, pthread_self());

    if (mode == 1){
        leavingBridgeCarnage(dir);
    }else if (mode == 2){
        leavingBridgeSemaphore(dir);
    }else if (mode == 3){
        leavingBridgeTrafficPolice(dir);
    }
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
void *policeLeft(void *args){
    while (vehiclesLCounter) {
        printf("Se durmio policia IZQUIERDO\n");
        pthread_cond_wait(&policeLeftCond, &policeLeftMutex);
        lock(&K1TempMutex);
        K1Temp = K1;
        unlock(&K1TempMutex);
        printf("Se desperto policia IZQUIERDO\n");
        if(vehiclesOnBridge == 0 && bridgeDir == 1 && bridgeDirReal == 1){
            if(vehicleQuantityL > 0){
                for(int i = 0; i < K1 + 1; i++){
                    pthread_cond_signal(&isSafeLcon);
                }
                lock(&bridgeDirMutex);
                bridgeDir = -1;
                unlock(&bridgeDirMutex);
            }else{
                lock(&bridgeDirMutex);
                bridgeDir = -1;
                unlock(&bridgeDirMutex);
                lock(&bridgeDirRealMutex);
                bridgeDirReal = -1;
                unlock(&bridgeDirRealMutex);
                pthread_cond_signal(&policeRightCond);
            }
        }
    }

    pthread_exit(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
void *policeRight(void *args){
    while (vehiclesRCounter) {
        printf("Se durmio policia DERECHO\n");
        pthread_cond_wait(&policeRightCond, &policeRightMutex);
        lock(&K2TempMutex);
        K2Temp = K2;
        unlock(&K2TempMutex);
        printf("Se desperto policia DERECHO\n");
        if(vehiclesOnBridge == 0 && bridgeDir == -1 && bridgeDirReal == -1){
            if(vehicleQuantityR > 0){
                for(int i = 0; i < K2 + 1; i++){
                    pthread_cond_signal(&isSafeRcon);
                }
                lock(&bridgeDirMutex);
                bridgeDir = 1;
                unlock(&bridgeDirMutex);
            }else{
                lock(&bridgeDirMutex);
                bridgeDir = 1;
                unlock(&bridgeDirMutex);
                lock(&bridgeDirRealMutex);
                bridgeDirReal = 1;
                unlock(&bridgeDirRealMutex);
                pthread_cond_signal(&policeLeftCond);
            }
        }
    }

    pthread_exit(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
void *trafficPolice(void *args){

    pthread_t createVehiculeRThread;
    pthread_t createVehiculeLThread;

    pthread_t policeRightThread;
    pthread_t policeLeftThread;

    K1Temp = K1;
    K2Temp = K2;

    vehiclesLCounter = vehiclesL;
    vehiclesRCounter = vehiclesR;

    pthread_create(&policeLeftThread,NULL,policeLeft,NULL);
    pthread_create(&policeRightThread,NULL,policeRight,NULL);

    pthread_create(&createVehiculeLThread,NULL,createVehiculeL,NULL);
    pthread_create(&createVehiculeRThread,NULL,createVehiculeR,NULL);

    pthread_join(createVehiculeLThread,NULL);
    pthread_join(createVehiculeRThread,NULL);

    pthread_exit(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Hilo para la funcionalidad de cambio de semaforo
void *semaphoreChange(void *args) {
    lock(&bridgeDirRealMutex);  // Comienza la direccion del puente hacia la derecha
    bridgeDirReal = 1;
    unlock(&bridgeDirRealMutex);

    while (totalVehicles) { // Mientras queden carros sin pasar
        lock(&bridgeDirMutex);  // Pone el semaforo izquierdo en verde
        bridgeDir = 1;
        unlock(&bridgeDirMutex);

        printf("\nSemaforo izquierdo en verde\n\n");
        if (vehiclesOnBridge == 0) {
            pthread_cond_broadcast(&isSafeLcon);    // Da paso si hay carros de este lado dormidos
        }
        sleep(greenLightTimeL);     // Espera el tiempo de luz verde izquierdo


        lock(&bridgeDirMutex);  // Pone el semaforo derecho en verde
        bridgeDir = -1;
        unlock(&bridgeDirMutex);

        printf("\nSemaforo derecho en verde\n\n");
        if (vehiclesOnBridge == 0) {
            pthread_cond_broadcast(&isSafeRcon);    // Da paso si hay carros de este lado dormidos
        }
        sleep(greenLightTimeR);     // Espera el tiempo de luz verde derecho
    }

    pthread_exit(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Hilo principal del modo semaforo
void *semaphore(void *args) {

    pthread_t createVehiculeRThread;
    pthread_t createVehiculeLThread;
    pthread_t createSemaphoreChangeThread;

    totalVehicles = vehiclesL + vehiclesR;

    pthread_create(&createSemaphoreChangeThread,NULL,semaphoreChange,NULL);    // Crea hilo de cambio de semaforos
    pthread_create(&createVehiculeRThread,NULL,createVehiculeR,NULL);   // Crea hilo de creacion de carros por la derecha
    pthread_create(&createVehiculeLThread,NULL,createVehiculeL,NULL);   // Crea hilo de creacion de carros por la izquierda

    pthread_join(createVehiculeRThread,NULL);
    pthread_join(createVehiculeLThread,NULL);

    pthread_exit(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Funcion que lee las variables del archivo de configuracion
// y las inicializa
void readConfiguration(){
    char const* const fileName = "config.txt";
    FILE* file = fopen(fileName, "r");  // Abre archivo de configuracion
    char line[256];
    const char s[4] = " = ";
    char *token;

    fgets(line, sizeof(line), file);    // Tamaño del puente
    strtok(line, s);
    token = strtok(NULL, s);
    bridgeLen = atoi(token);

    fgets(line, sizeof(line), file);    // Media exponencial izquierda
    strtok(line, s);
    token = strtok(NULL, s);
    expAverageL = atoi(token);

    fgets(line, sizeof(line), file);    // Media exponencial derecha
    strtok(line, s);
    token = strtok(NULL, s);
    expAverageR = atoi(token);

    fgets(line, sizeof(line), file);    // Velocidad inferior izquierda
    strtok(line, s);
    token = strtok(NULL, s);
    vehicleVelocityInfL = atoi(token);

    fgets(line, sizeof(line), file);    // Velocidad inferior derecha
    strtok(line, s);
    token = strtok(NULL, s);
    vehicleVelocityInfR = atoi(token);

    fgets(line, sizeof(line), file);    // Velocidad superior izquierda
    strtok(line, s);
    token = strtok(NULL, s);
    vehicleVelocitySupL = atoi(token);

    fgets(line, sizeof(line), file);    // Velocidad superior derecha
    strtok(line, s);
    token = strtok(NULL, s);
    vehicleVelocitySupR = atoi(token);

    fgets(line, sizeof(line), file);    // Cantidad K1 oficiales de transito
    strtok(line, s);
    token = strtok(NULL, s);
    K1 = atoi(token);

    fgets(line, sizeof(line), file);    // Cantidad K2 oficiales de transito
    strtok(line, s);
    token = strtok(NULL, s);
    K2 = atoi(token);

    fgets(line, sizeof(line), file);    // Tiempo de luz verde izquierda
    strtok(line, s);
    token = strtok(NULL, s);
    greenLightTimeL = atoi(token);

    fgets(line, sizeof(line), file);    // Tiempo de luz verde derecha
    strtok(line, s);
    token = strtok(NULL, s);
    greenLightTimeR = atoi(token);

    fgets(line, sizeof(line), file);    // Cantidad de vehiculos por la izquierda
    strtok(line, s);
    token = strtok(NULL, s);
    vehiclesL = atoi(token);

    fgets(line, sizeof(line), file);    // Cantidad de vehiculos por la derecha
    strtok(line, s);
    token = strtok(NULL, s);
    vehiclesR = atoi(token);

    fclose(file);
}

//---------------------------------------------------------------------------------------------------------------------------------------
int main(){
    readConfiguration(); // Lee el archivo de configuracion

    srand(time(NULL));   // Initialization, should only be called once.

    bridge = (pthread_mutex_t*) malloc(bridgeLen*sizeof(pthread_mutex_t));

    pthread_cond_init(&isSafeLcon,NULL);    // Inicializacion de mutex y condiciones para semaforos de SO
    pthread_cond_init(&isSafeRcon,NULL);
    pthread_mutex_init(&ambulanceLMutex, NULL);
    pthread_mutex_init(&ambulanceRMutex, NULL);
    pthread_mutex_init(&vehiclesOnBridgeMutex, NULL);
    pthread_mutex_init(&vehicleQuantityLMutex, NULL);
    pthread_mutex_init(&vehicleQuantityRMutex, NULL);
    pthread_mutex_init(&bridgeDirMutex, NULL);
    pthread_mutex_init(&bridgeDirMutex, NULL);
    pthread_mutex_init(&bridgeDirRealMutex, NULL);

    for(int i=0;i<bridgeLen;i++){   // Inicializacion del puente
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
    firstCar = 0;

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
    // Inicializa el modo semaforo
    else if (mode == 2){
        printf("\n");
        pthread_t semaphoreThread;

        pthread_mutex_init(&totalVehiclesMutex,NULL);

        pthread_create(&semaphoreThread,NULL,semaphore,NULL);

        pthread_join(semaphoreThread,NULL);
    }
    else if (mode == 3){
        printf("\n");
        pthread_t trafficPoliceThread;

        pthread_mutex_init(&policeLeftMutex, NULL);
        pthread_mutex_init(&policeRightMutex, NULL);

        pthread_cond_init(&policeLeftCond,NULL);
        pthread_cond_init(&policeRightCond,NULL);

        pthread_mutex_init(&K1TempMutex, NULL);
        pthread_mutex_init(&K2TempMutex, NULL);

        pthread_mutex_init(&vehiclesLCounterMutex, NULL);
        pthread_mutex_init(&vehiclesRCounterMutex, NULL);

        pthread_create(&trafficPoliceThread,NULL,trafficPolice,NULL);

        pthread_join(trafficPoliceThread,NULL);
    }
    else{
        printf("\nOpcion invalida\n");
    }

    //pthread_exit(0);

    return 0;
}