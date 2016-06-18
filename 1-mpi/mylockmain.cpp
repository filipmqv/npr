// kompilacja: mpicxx mylockmain.cpp -std=c++11 -lpthread -o mylockmain -profile=myprof
// uruchomienie: mpiexec -np 5 ./mylockmain

#include <stdlib.h>
#include <cstdlib> //rand
#include <stdio.h>
#include <mpi.h>
#include <stdbool.h>
#include <pthread.h>
#include <chrono> // time
#include <unistd.h>
#include <signal.h>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <sstream>

#include <my_lock.cpp>

#define MSG_PRINT 103

int worldSizeMain, rankMain;

class MyData: public ISerializableClass {
public:
    int a, b;
    std::string c;

    template <class Archive>
    void serialize( Archive & ar )
    {
      ar( dataTmstmp, a, b, c );
    }
};

class BufData: public ISerializableClass {
public:
    int buf[10];
    int bufSingle = 0;
    int bufUsage = 0;

    template <class Archive>
    void serialize( Archive & ar )
    {
      ar( dataTmstmp, buf, bufSingle, bufUsage );
    }
};

void initMPI(int argc, char **argv) {
    int provided; 
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided); 
    MPI_Comm_size(MPI_COMM_WORLD, &worldSizeMain);
    worldSizeMain--; // last MPI process is only for printing
    MPI_Comm_rank(MPI_COMM_WORLD, &rankMain);
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

/*void broadcastLoop() {

    MyData md;
    md.a = 0;
    md.b = 45;
    md.c = "";
    MyLock<MyData>* ml = new MyLock<MyData>(md);

    ml->acquire();
    ml->data.a += rankMain;
    ml->data.c += std::to_string(rankMain) + " ";
    ml->release();
    
    delete ml;
}*/

/*void waitSignalTestLoop() {
    MyData fake; 
    MyLock<MyData>* ml = new MyLock<MyData>(fake);
    if (rankMain == 0) {
        printf("(%d) waiter 0\n", rankMain);
        ml->acquire();
        printf("(%d) waiter 1\n", rankMain);
        ml->wait(5); 
        printf("(%d) waiter 2\n", rankMain);
        sleep(2);
        ml->release();
        printf("(%d) waiter 3\n", rankMain);
    }

    if (rankMain == 1) {
        printf("(%d) signaler 1\n", rankMain);
        sleep(1);
        printf("(%d) signaler 2\n", rankMain);
        ml->signal(5);
    }
    delete ml;
}*/

void producerLoop() {
    printf("(%d) producerLoop\n", rankMain);
    BufData md;
    for (int i=0; i<10; i++)
        md.buf[i] = 0;
    md.bufUsage = 0;
    md.bufSingle = 0;
    MyLock<BufData>* ml = new MyLock<BufData>(md);

    while (1) {
        ml->acquire();
        //printf("(%d) prod acquired \n", rankMain);
        while (ml->data.bufUsage == 1) {
            printf("(%d) prod before wait \n", rankMain);
            ml->wait(0);
            printf("(%d) prod after wait \n", rankMain);
        }
        ml->data.bufSingle++;
        ml->data.bufUsage++;
        ml->sendToPrinter("++ produced \t" + std::to_string(ml->data.bufSingle) + " " + std::to_string(ml->data.bufUsage));
        //printf("(%d) produced\t%d\n", rankMain, ml->data.bufSingle);
        
        ml->signal(0);
        ml->release();
        //ml->signal(0);
        usleep(300000 * (rand()%10));
    }
}

void consumerLoop() {
    printf("(%d) consumerLoop\n", rankMain);
    BufData md;
    for (int i=0; i<10; i++)
        md.buf[i] = 0;
    md.bufUsage = 0;
    md.bufSingle = 0;
    MyLock<BufData>* ml = new MyLock<BufData>(md);

    while (1) {
        ml->acquire();
        //printf("(%d) cons acquired \n", rankMain);
        while (ml->data.bufUsage == 0) {
            //printf("(%d) cons before wait \n", rankMain);
            ml->wait(0);
            //printf("(%d) cons after wait \n", rankMain);
        }
        ml->data.bufUsage--;

        ml->sendToPrinter("-- consumed \t" + std::to_string(ml->data.bufSingle) + " " + std::to_string(ml->data.bufUsage));
        //printf("(%d) consumed\t%d\n", rankMain, ml->data.bufSingle);
        
        ml->signal(0);
        ml->release();
        //ml->signal(0);
        usleep(300000 * (rand()%10));
    }
}

void printerLoop() {
    MyData fake; 
    MyLock<MyData>* ml = new MyLock<MyData>(fake);
    printf("printer %d\n", rankMain);
    delete ml; // wait here on barier for other MPI processes and only print msgs
}

int main(int argc, char **argv) {
    atexit(cleanup);
    signal(SIGINT, stop);

    initMPI(argc, argv);
    
//////////////////////////////////// insert your code here: ///////////////////////////////

    if (rankMain == worldSizeMain) 
        printerLoop();
    else if(rankMain%2 == 0)
        producerLoop();
    else
        consumerLoop();
    //else {
        //waitSignalTestLoop();
        //broadcastLoop();
    //}

///////////////////////////////////////////////////////////////////////////////////////////

    finishMPI();
    return 0;
}

