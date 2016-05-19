// kompilacja: mpicxx mylockmain.cpp -std=c++11 -lpthread -o mylockmain -profile=myprof
// uruchomienie: mpiexec -np 5 ./mylockmain

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

#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <sstream>

#include <my_lock.cpp>

int worldSizeMain, rankMain;

class MyData: public ISerializableClass {
public:
    int a, b;
    std::string c;

    template <class Archive>
    void serialize( Archive & ar )
    {
      ar( a, b, c );
    }
};

void initMPI(int argc, char **argv) {
    int provided; 
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided); 
    MPI_Comm_size(MPI_COMM_WORLD, &worldSizeMain);
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


void broadcastLoop() {

    MyData* isc1 = new MyData();
    isc1->a = 44;
    isc1->b = 45;
    isc1->c = "ala ma kota";
    MyLock<MyData>* ml = new MyLock<MyData>(isc1);
    ml->acquire();
    printf("(%d) w sekcji krytycznej\n", rankMain);
    sleep(1);
    if (rankMain == 0)
        ml->data.a = 999;
    if (rankMain == 3)
        ml->data.c += ", a kot Alę";
    printf("(%d) po sekcji krytycznej\n", rankMain);

    
    ml->release();
    printf("(%d) po release %d, %d, %s\n", rankMain, ml->data.a, ml->data.b, ml->data.c.c_str());
    delete ml;

    printf("(%d) brLoop finished\n", rankMain);

}

int main(int argc, char **argv) {
    atexit(cleanup);
    signal(SIGINT, stop);

    initMPI(argc, argv);
    
    broadcastLoop();

    finishMPI();
    return 0;
}

