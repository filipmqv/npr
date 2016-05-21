// kompilacja: mpicxx mylockmain.cpp -std=c++11 -lpthread -o mylockmain -profile=myprof
// uruchomienie: mpiexec -np 5 ./mylockmain

#include <stdlib.h>
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
      ar( dataTmstmp, a, b, c );
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

void sendToPrinterFromMain(std::initializer_list<std::string> texts) {
    std::ostringstream stringStream;
    stringStream << "{" << rankMain << "}\t" << std::chrono::system_clock::now().time_since_epoch().count() << "\t";
    for(auto i = texts.begin(); i != texts.end(); i++) {
        stringStream << *i << "\t";
    }
    stringStream << " ";
    std::string copyOfStr = stringStream.str();
    MPI_Send(copyOfStr.c_str(), copyOfStr.size(), MPI_CHAR, /*0*/ worldSizeMain, MSG_PRINT, MPI_COMM_WORLD);
}

void broadcastLoop() {

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
    else {

        broadcastLoop();
    }

///////////////////////////////////////////////////////////////////////////////////////////
    finishMPI();
    return 0;
}

