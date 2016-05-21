#include "my_lock.hpp"

#define MSG_ACQUIRE_REQUEST 100
#define MSG_ACQUIRE_REPLY 101
#define MSG_RELEASE 102
#define MSG_FINISH 666
#define MSG_PRINT 103

/*
inicjator broadcastuje swój timestamp i rank
odbierający odpowiada:
    od razu zatwierdza, jeśli sam nie chce się ubiegać lub ma wyższy timestamp (późniejszy)
            (jeśli timestampy są równe to bierze się pod uwagę rank - niższy ma pierwszeństwo)
    po zakończeniu własnej sekcji krytycznej zatwierdza

do sekcji można wejść po otrzymaniu wszystkich potwierdzeń
po wyjściu z sekcji wysyła się zmiany, a sygnał release traktowany jest jako potwierdzenie
*/

template <class T>
MyLock<T>::MyLock(T* input) {
	static_assert(std::is_base_of<ISerializableClass, T>::value, "T must extend ISerializableClass");
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	timestampGlobal = std::chrono::system_clock::now().time_since_epoch().count();
    isRequesting = false;
    allowed = false;
    data = *input;

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
    pthread_cond_destroy(&allResponses);	
}

template <class T>
void MyLock<T>::sendToPrinter(std::initializer_list<std::string> texts) {
    std::ostringstream stringStream;
    stringStream << "{" << rank << "} " << std::chrono::system_clock::now().time_since_epoch().count() << " ";
    for(auto i = texts.begin(); i != texts.end(); i++) {
        stringStream << *i << " ";
    }
    stringStream << " ";
    std::string copyOfStr = stringStream.str();
    MPI_Send(copyOfStr.c_str(), copyOfStr.size(), MPI_CHAR, 0, MSG_PRINT, MPI_COMM_WORLD);
}

template <class T>
void MyLock<T>::broadcastLockAcquireRequest() {
    pthread_mutex_lock(&m);
    numOfAcquireReplys = 1; // one ack from myself
    timestampGlobal = std::chrono::system_clock::now().time_since_epoch().count();
    long timestamp = timestampGlobal;
    isRequesting = true;
    allowed = false;
    pthread_mutex_unlock(&m);
    //printf("(%d) tstmp %ld\n", rank, timestamp);
    sendToPrinter({"test", std::to_string(isRequesting)});

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
    pthread_mutex_unlock(&m);

    if (!amIRequestingNow || rcvdTimestamp < myTimestamp || 
                (rcvdTimestamp == myTimestamp && status.MPI_SOURCE < rank)) {
        MPI_Send(&rcvdTimestamp, 1, MPI_LONG, status.MPI_SOURCE, MSG_ACQUIRE_REPLY, MPI_COMM_WORLD);
    } // else don't send reply
}

template <class T>
void MyLock<T>::handleMsgAcquireReplyReceived() {
    MPI_Status status;
    long rcvdTimestamp;
    MPI_Recv(&rcvdTimestamp, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    //printf("(%d) MSG_ACQUIRE_REPLY %ld from %d\n", rank, rcvdTimestamp, status.MPI_SOURCE);

    pthread_mutex_lock(&m);
    if (isRequesting && timestampGlobal == rcvdTimestamp) {
        numOfAcquireReplys++;
        if (numOfAcquireReplys == worldSize) {
            //printf("(%d) signal\n", rank);
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
    //printf("(%d) MSG_RELEASE %ld from %d\n", rank, rcvdTimestamp, status.MPI_SOURCE);

	if (status.MPI_SOURCE == rank) {
        return;
    }

    std::stringstream ss;
    ss << std::string(rcvdData, dataSize);
    cereal::BinaryInputArchive iarchive(ss); // Create an input archive
    T rcvdDataDeserialized;
    iarchive(rcvdDataDeserialized); // Read the data from the archive

    data = rcvdDataDeserialized;

    pthread_mutex_lock(&m);
    if (isRequesting) {
        numOfAcquireReplys++;
        if (numOfAcquireReplys == worldSize) {
            allowed = true;
            pthread_cond_signal(&allResponses);
        }
    }
    pthread_mutex_unlock(&m);
}

template <class T>
void MyLock<T>::handleMsgPrintReceived(int dataSize) {
    char rcvdData[dataSize];
    MPI_Recv(&rcvdData, dataSize, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //printf("%s\n", rcvdData);
    rcvdData[dataSize-1] = '\0';
    std::cout << rcvdData << std::endl;
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
    pthread_mutex_unlock(&m);

    // serialize data
    std::stringstream ss; // any stream can be used
    cereal::BinaryOutputArchive oarchive(ss); // Create an output archive
    oarchive(data); // Write the data to the archive
    std::string serializedData = ss.str();

    for(int i = 0; i < worldSize; i++) {
        //printf("  (%d) Wysylam release %ld do %d\n", rank, timestamp, i);
        MPI_Send(serializedData.c_str(), serializedData.size(), MPI_CHAR, i, MSG_RELEASE, MPI_COMM_WORLD);
    }
}

template <class T>
void MyLock<T>::acquire() {
	broadcastLockAcquireRequest();
    pthread_mutex_lock(&m);
    if(!allowed) { // if allowed == true -- signal was sent before waiting for it
    	pthread_cond_wait(&allResponses, &m);
    }
    pthread_mutex_unlock(&m);
}

template <class T>
void MyLock<T>::release() {
	broadcastLockRelease();
}

