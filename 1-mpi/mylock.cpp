// kompilacja: mpicxx mylock.cpp -std=c++11 -lpthread -o mylock
// uruchomienie: mpiexec -np 5 ./mylock 

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
//#include <math.h>
#include <stdbool.h>
#include <pthread.h>
#include <chrono> // time
#include <unistd.h>
//#include <string.h>
#include <signal.h>
#include <stddef.h> // offsetof
#define ROOT 0
#define MSG_ACQUIRE_REQUEST 100
#define MSG_ACQUIRE_REPLY 101
#define MSG_RELEASE 102

////////////////////////// global structs /////////////////////
/*typedef struct acquireRequestStruct {
        int rank;
        long timestamp;
} asr;
MPI_Datatype MPI_ASR_TYPE;*/

//////////////////// globals ///////////////////////////////
int worldSize, rank;
long timestampGlobal;
bool isRequesting;
int numOfAcquireReplys;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t allResponses = PTHREAD_COND_INITIALIZER;


///////////////////// methods ///////////////////////////////
/*void createTypeAcquireRequestStruct() {
    const int nitems = 2;
    int blocklengths[nitems] = {1,1};
    MPI_Datatype types[nitems] = {MPI_INT, MPI_LONG};
    MPI_Aint offsets[nitems];

    offsets[0] = offsetof(asr, rank);
    offsets[1] = offsetof(asr, timestamp);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &MPI_ASR_TYPE);
    MPI_Type_commit(&MPI_ASR_TYPE);
}*/

void initMPI(int argc, char **argv) {
    int provided; 
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided); 
    
    //createTypeAcquireRequestStruct();

    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
}

void otherInits() {
    timestampGlobal = std::chrono::system_clock::now().time_since_epoch().count();
    isRequesting = false;
}

void finishMPI() {
    MPI_Finalize();
}

void cleanup() {
    pthread_exit(NULL);
    finishMPI();
}

void stop(int signo) {
    exit(EXIT_SUCCESS);
}


/*
inicjator broadcastuje swój timestamp i rank
odbierający odpowiada:
    od razu zatwierdza, jeśli sam nie chce się ubiegać lub ma wyższy timestamp (późniejszy)
            (jeśli timestampy są równe to bierze się pod uwagę rank - niższy ma pierwszeństwo)
    po zakończeniu własnej sekcji krytycznej zatwierdza

do sekcji można wejść po otrzymaniu wszystkich potwierdzeń
po wyjściu z sekcji wysyła się zmiany i odłożone zatwierdzenia
TODO zatwierdzenia - kolejka?
*/

void broadcastLockAcquireRequest() {
    pthread_mutex_lock(&m);
    numOfAcquireReplys = 1; // one from myself
    timestampGlobal = std::chrono::system_clock::now().time_since_epoch().count();
    long timestamp = timestampGlobal;
    isRequesting = true;
    pthread_mutex_unlock(&m);
    printf("(%d) tstmp %ld\n", rank, timestamp);

    //acquireRequestStruct sendAsr;
    //sendAsr.rank = rank;
    //sendAsr.timestamp = timestampGlobal;

    int i;
    //usleep((rank+1)*1000000);
    for(i = 0; i < worldSize; i++) {
        //printf("  (%d) Wysylam tmstmp %ld do %d\n", rank, timestamp, i);
        MPI_Send(&timestamp, 1, MPI_LONG, i, MSG_ACQUIRE_REQUEST, MPI_COMM_WORLD);
    }
    //sleep(5);
}

void handleMsgAcquireRequestReceived() {
    MPI_Status status;
    long rcvdTimestamp;
    MPI_Recv(&rcvdTimestamp, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    //printf("(%d) MSG_ACQUIRE_REQUEST %ld from %d\n", rank, rcvdTimestamp, status.MPI_SOURCE);

    if (status.MPI_SOURCE == rank) {
        // ignore
        //printf("(%d) ignored\n", rank);
        return;
    }

    pthread_mutex_lock(&m);
    long myTimestamp = timestampGlobal;
    bool amIRequestingNow = isRequesting;
    pthread_mutex_unlock(&m);

    if (!amIRequestingNow || rcvdTimestamp < myTimestamp || 
                (rcvdTimestamp == myTimestamp && status.MPI_SOURCE < rank)) {
        MPI_Send(&rcvdTimestamp, 1, MPI_LONG, status.MPI_SOURCE, MSG_ACQUIRE_REPLY, MPI_COMM_WORLD);
    } else {
        // TODO
        //printf("(%d) do kolejki\n", rank);
        // kolejka potwierdzeń do wysłania
        //
        //
        //
        // z 2 strony sam broadcast po wyjściu można traktować jako potwierdzenie 
        // tylko trzeba to specjalnie obsłużyć - że dodatkowe potwierdzenie juz nie przyjdzie
    }
}

void handleMsgAcquireReplyReceived() {
    MPI_Status status;
    long rcvdTimestamp;
    MPI_Recv(&rcvdTimestamp, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    //printf("(%d) MSG_ACQUIRE_REPLY %ld from %d\n", rank, rcvdTimestamp, status.MPI_SOURCE);

    pthread_mutex_lock(&m);
    if (isRequesting && timestampGlobal == rcvdTimestamp) {
        numOfAcquireReplys++;
        if (numOfAcquireReplys == worldSize) {
            //printf("(%d) signal\n", rank);
            pthread_cond_signal(&allResponses);
        }
    }
    pthread_mutex_unlock(&m);
}

void handleMsgReleaseReceived() {
    MPI_Status status;
    long rcvdTimestamp;
    MPI_Recv(&rcvdTimestamp, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    //printf("(%d) MSG_RELEASE %ld from %d\n", rank, rcvdTimestamp, status.MPI_SOURCE);

// TODO zamienić bufor na otrzymany

    pthread_mutex_lock(&m);
    if (isRequesting) {
        numOfAcquireReplys++;
        if (numOfAcquireReplys == worldSize) {
            //printf("(%d) signal z release'a\n", rank);
            pthread_cond_signal(&allResponses);
        }
    }
    pthread_mutex_unlock(&m);
}

void *myThreadReceiver(void *id) { 0;
    while (1) {
        MPI_Status status;
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        switch (status.MPI_TAG) {
            case MSG_ACQUIRE_REQUEST:
                handleMsgAcquireRequestReceived();
            break;
            case MSG_ACQUIRE_REPLY:
                handleMsgAcquireReplyReceived();
            break;
            case MSG_RELEASE:
                handleMsgReleaseReceived();
            break;

            default:
                printf("unknown MPI_TAG\n");
            break;
        } 
    }
    return NULL;
}

void broadcastLockRelease() {
    pthread_mutex_lock(&m);
    isRequesting = false;
    long timestamp = timestampGlobal;
    pthread_mutex_unlock(&m);

    int i;
    //usleep((rank+1)*1000000);
    for(i = 0; i < worldSize; i++) {
        //printf("  (%d) Wysylam release %ld do %d\n", rank, timestamp, i);
        MPI_Send(&timestamp, 1, MPI_LONG, i, MSG_RELEASE, MPI_COMM_WORLD);
    }
    //sleep(5);
}

void broadcastLoop() {
    broadcastLockAcquireRequest();
    pthread_mutex_lock(&m);
    pthread_cond_wait(&allResponses, &m);
    pthread_mutex_unlock(&m);

    // sekcja krytyczna
    printf("(%d) w sekcji krytycznej\n", rank);
    sleep(1);
    printf("(%d) po sekcji krytycznej\n", rank);

    broadcastLockRelease(); // TODO with changed buff!!!!!!!!

    printf("(%d) brLoop finished\n", rank);

}

int main(int argc, char **argv) {
    atexit(cleanup);
    signal(SIGINT, stop);

    initMPI(argc, argv);
    otherInits();
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
    pthread_t thread;
    pthread_create(&thread, &attr, myThreadReceiver, NULL);
    //pthread_create(&threads[1], &attr, myThreadReceiver, NULL);
    
    broadcastLoop();

    pthread_join(thread, NULL);
    //pthread_join(threads[1], NULL);

    pthread_exit(NULL);
    
    finishMPI();
}

