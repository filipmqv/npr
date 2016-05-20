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
#include <initializer_list> // for multiple args passed to function

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

void sendToPrinterFromMain(std::initializer_list<std::string> texts) {
    std::ostringstream stringStream;
    stringStream << "{" << rankMain << "} " << std::chrono::system_clock::now().time_since_epoch().count() << " ";
    for(auto i = texts.begin(); i != texts.end(); i++) {
        stringStream << *i << " ";
    }
    stringStream << " ";
    std::string copyOfStr = stringStream.str();
    MPI_Send(copyOfStr.c_str(), copyOfStr.size(), MPI_CHAR, 0, MSG_PRINT, MPI_COMM_WORLD);
}

void broadcastLoop() {

    MyData* md = new MyData();
    md->a = 44;
    md->b = 45;
    md->c = "ala ma kota";
    MyLock<MyData>* ml = new MyLock<MyData>(md);

    ml->acquire();

    sendToPrinterFromMain( {"w sekcji krytycznej"} );
    //sleep(1);
    if (rankMain == 0)
        ml->data.a = 999;
    if (rankMain == 1)
        ml->data.c += ", a kot Alę";
    //printf("(%d) po sekcji krytycznej\n", rankMain);
    sendToPrinterFromMain( {"po sekcji krytycznej"} );
    
    
    ml->release();
    //printf("(%d) po release %d, %d, %s\n", rankMain, ml->data.a, ml->data.b, ml->data.c.c_str());
    sendToPrinterFromMain({std::to_string(ml->data.a), std::to_string(ml->data.b), ml->data.c});
    delete ml;

    printf("(%d) broadcastLoop finished\n", rankMain);

}

int main(int argc, char **argv) {
    atexit(cleanup);
    signal(SIGINT, stop);

    initMPI(argc, argv);
    
    broadcastLoop();

    finishMPI();
    return 0;
}

