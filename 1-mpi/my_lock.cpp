#include "my_lock.hpp"

#define MSG_ACQUIRE_REQUEST 100
#define MSG_ACQUIRE_REPLY 101
#define MSG_RELEASE 102
#define MSG_PRINT 103
#define MSG_SIGNAL 104
#define MSG_FINISH 666

/*
inicjator broadcastuje swój timestamp i rank (request)
odbierający odpowiada (reply):
    od razu zatwierdza, jeśli sam nie chce się ubiegać lub ma wyższy timestamp (późniejszy)
            (jeśli timestampy są równe to bierze się pod uwagę rank - niższy ma pierwszeństwo)
    wpp po zakończeniu własnej sekcji krytycznej

do sekcji można wejść po otrzymaniu wszystkich potwierdzeń
po wyjściu z sekcji wysyła się zmiany (release) oraz opóźnione potwierdzenia (reply)
*/

template <class T>
MyLock<T>::MyLock(T input) {
	static_assert(std::is_base_of<ISerializableClass, T>::value, "T must extend ISerializableClass");
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    worldSize--; // last MPI process is only for printing
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	timestampGlobal = std::chrono::system_clock::now().time_since_epoch().count();
    isRequesting = false;
    allowed = false;
    data = input;
    for(int i = 0; i < NUM_OF_COND_VARS; i++)
        waitSignal[i] = PTHREAD_COND_INITIALIZER;
    printerCount = 0;

    initThreadReceiver();
}

template <class T>
void MyLock<T>::initThreadReceiver() {
	pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&thread, &attr, &MyLock::myThreadReceiverHelper, this);
}

template <class T>
void MyLock<T>::stopThreadReceiver() {
	MPI_Barrier(MPI_COMM_WORLD);
	
	int dummy = 1;
	MPI_Send(&dummy, 1, MPI_INT, rank, MSG_FINISH, MPI_COMM_WORLD);
	pthread_join(thread, NULL);
}

template <class T>
MyLock<T>::~MyLock() {
	stopThreadReceiver();
	pthread_mutex_destroy(&m);
    pthread_mutex_destroy(&dataLock);
    pthread_cond_destroy(&allResponses);	
    for(int i = 0; i < NUM_OF_COND_VARS; i++)
        pthread_cond_destroy(&waitSignal[i]);
}

template <class T>
void MyLock<T>::sendToPrinter(std::string texts) {
    std::ostringstream stringStream;
    stringStream << "{" << rank << "}\t" << std::chrono::system_clock::now().time_since_epoch().count() << "\t";
    stringStream << texts << " ";
    std::string copyOfStr = stringStream.str();
    MPI_Send(copyOfStr.c_str(), copyOfStr.size(), MPI_CHAR, /*0*/ worldSize, MSG_PRINT, MPI_COMM_WORLD);
}

template <class T>
void MyLock<T>::broadcastLockAcquireRequest() {
    pthread_mutex_lock(&m);
    acquireReplysRanks.clear();
    acquireReplysRanks.insert(rank); // one from myself
    timestampGlobal = std::chrono::system_clock::now().time_since_epoch().count();
    long timestamp = timestampGlobal;
    isRequesting = true;
    allowed = false;
    pthread_mutex_unlock(&m);

    for(int i = 0; i < worldSize; i++) {
        //printf("  (%d) Wysylam tmstmp %ld do %d\n", rank, timestamp, i);
        MPI_Send(&timestamp, 1, MPI_LONG, i, MSG_ACQUIRE_REQUEST, MPI_COMM_WORLD);
    }
}

template <class T>
void MyLock<T>::handleMsgAcquireRequestReceived() {
    MPI_Status status;
    long rcvdTimestamp;
    MPI_Recv(&rcvdTimestamp, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    //printf("(%d) MSG_ACQUIRE_REQUEST %ld from %d\n", rank, rcvdTimestamp, status.MPI_SOURCE);

    if (status.MPI_SOURCE == rank) {
        return;
    }

    pthread_mutex_lock(&m);
    long myTimestamp = timestampGlobal;
    bool amIRequestingNow = isRequesting;

    if ( !amIRequestingNow || rcvdTimestamp < myTimestamp || 
                (rcvdTimestamp == myTimestamp && status.MPI_SOURCE < rank) ) {
        pthread_mutex_unlock(&m);
        MPI_Send(&rcvdTimestamp, 1, MPI_LONG, status.MPI_SOURCE, MSG_ACQUIRE_REPLY, MPI_COMM_WORLD);
    } else {
        delayedReplys.push_back(status.MPI_SOURCE);
        delayedReplysTimestamps.push_back(rcvdTimestamp);
        pthread_mutex_unlock(&m);
    }
}

template <class T>
void MyLock<T>::handleMsgAcquireReplyReceived() {
    MPI_Status status;
    long rcvdTimestamp;
    MPI_Recv(&rcvdTimestamp, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    //printf("(%d) MSG_ACQUIRE_REPLY %ld from %d\n", rank, rcvdTimestamp, status.MPI_SOURCE);

    pthread_mutex_lock(&m);
    if (isRequesting && timestampGlobal == rcvdTimestamp) {
        acquireReplysRanks.insert(status.MPI_SOURCE);
        if (acquireReplysRanks.size() == worldSize) {
            allowed = true;
            pthread_cond_signal(&allResponses);
        }
    }
    pthread_mutex_unlock(&m);
}

template <class T>
void MyLock<T>::handleMsgReleaseReceived(int dataSize) {
    MPI_Status status;
    char rcvdData[dataSize];
    MPI_Recv(&rcvdData, dataSize, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

	if (status.MPI_SOURCE == rank) {
        return;
    }

    // deserialize and save obtained data (if it's newer than owned data)
    pthread_mutex_lock(&dataLock);
    std::stringstream ss;
    ss << std::string(rcvdData, dataSize);
    cereal::BinaryInputArchive iarchive(ss); // Create an input archive
    T rcvdDataDeserialized;
    iarchive(rcvdDataDeserialized); // Read the data from the archive

    if (data.dataTmstmp < rcvdDataDeserialized.dataTmstmp) {
        data = rcvdDataDeserialized;
    }
    pthread_mutex_unlock(&dataLock);
}

template <class T>
void MyLock<T>::handleMsgPrintReceived(int dataSize) {
    char rcvdData[dataSize];
    MPI_Recv(&rcvdData, dataSize, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    rcvdData[dataSize-1] = '\0';
    std::cout << printerCount++ << " " << rcvdData << std::endl;
}

template <class T>
void MyLock<T>::handleMsgSignalReceived() {
    MPI_Status status;
    int rcvdId;
    MPI_Recv(&rcvdId, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    pthread_mutex_lock(&m);
    signalCame = true;
    pthread_mutex_unlock(&m);
    pthread_cond_signal(&waitSignal[rcvdId]);
}

template <class T>
void* MyLock<T>::myThreadReceiver(void) {
	bool running = true;
    while (running) {
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
            	int dataSize;
            	MPI_Get_count(&status, MPI_CHAR, &dataSize);
                handleMsgReleaseReceived(dataSize);
            break;
            case MSG_PRINT:
                int dataSize2;
                MPI_Get_count(&status, MPI_CHAR, &dataSize2);
                handleMsgPrintReceived(dataSize2);
            break;
            case MSG_SIGNAL:
                handleMsgSignalReceived();
            break;
            case MSG_FINISH:
                int dummy;
                MPI_Recv(&dummy, 1, MPI_INT, rank, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                running = false;
            break;

            default:
                printf("unknown MPI_TAG\n");
            break;
        } 
    }
}

template <class T>
void MyLock<T>::broadcastLockRelease() {
    pthread_mutex_lock(&m);
    isRequesting = false;
    allowed = false;

    // serialize data
    data.dataTmstmp = std::chrono::system_clock::now().time_since_epoch().count();
    std::stringstream ss; // any stream can be used
    cereal::BinaryOutputArchive oarchive(ss); // Create an output archive
    oarchive(data); // Write the data to the archive
    std::string serializedData = ss.str();
    sendToPrinter("   release\t" + std::to_string(data.bufSingle) + " " + std::to_string(data.bufUsage) + "\t" + std::to_string(data.dataTmstmp) );

    for(int i = 0; i < worldSize; i++) {
        //printf("  (%d) Wysylam release %ld do %d\n", rank, timestamp, i);
        MPI_Send(serializedData.c_str(), serializedData.size(), MPI_CHAR, i, MSG_RELEASE, MPI_COMM_WORLD);
    }
    //sleep(1);
    for(int i = 0; i < delayedReplys.size(); i++) {
        //printf("(%d) delayed ack sent to %d\n", rank, delayedReplys[i]);
        MPI_Send(&delayedReplysTimestamps[i], 1, MPI_LONG, delayedReplys[i], MSG_ACQUIRE_REPLY, MPI_COMM_WORLD);
    }
    delayedReplys.clear();
    delayedReplysTimestamps.clear();
    pthread_mutex_unlock(&m);
}

template <class T>
void MyLock<T>::acquire() {
	broadcastLockAcquireRequest();
    pthread_mutex_lock(&m);
    if(!allowed) { // if allowed == true -- signal was sent before waiting for it
    	pthread_cond_wait(&allResponses, &m);
    }
    pthread_mutex_unlock(&m);

    pthread_mutex_lock(&dataLock); // unlocked after release ****
}

template <class T>
void MyLock<T>::release() {
	broadcastLockRelease();
    pthread_mutex_unlock(&dataLock); // locked after acquire ****
}

template <class T>
T* MyLock<T>::getData() {
    pthread_mutex_lock(&dataLock);
    T* res = &data;
    pthread_mutex_unlock(&dataLock);
    return res;
}

/////////////////////////// wait and signal ///////////////
template <class T>
void MyLock<T>::wait(int id) {
    if(id < 0 || id > NUM_OF_COND_VARS-1) 
        return;
    //pthread_mutex_lock(&m);
    //signalCame = false;
    //pthread_mutex_unlock(&m);
    sendToPrinter("before release");
    release();
    pthread_mutex_lock(&m);
    if(!signalCame) { // if signalCame == true -- signal was sent before waiting for it
        sendToPrinter("waiting");
        pthread_cond_wait(&waitSignal[id], &m);
    } else {
        sendToPrinter("signal came earlier");
    }
    signalCame = false;
    sendToPrinter("awaken");
    pthread_mutex_unlock(&m);
    acquire();
}

template <class T>
void MyLock<T>::signal(int id) {
    if(id < 0 || id > NUM_OF_COND_VARS-1) 
        return;
    for(int i = 0; i < worldSize; i++) {
        MPI_Send(&id, 1, MPI_INT, i, MSG_SIGNAL, MPI_COMM_WORLD);
    }
}