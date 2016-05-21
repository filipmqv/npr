#include <serializable_class.cpp>
#include <type_traits>
#include <vector>
#include <set>

#define NUM_OF_COND_VARS 10

template<class T>
class MyLock {
private:
	int worldSize, rank;
	long timestampGlobal;
	bool isRequesting, allowed, signalCame; //allowed - set before signal in case signal goes before wait
	std::set<int> acquireReplysRanks; // collect unique ranks which replied
	std::vector<int> delayedReplys; // for replys that should be sent after critical section
	std::vector<long> delayedReplysTimestamps;
	pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER, dataLock = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t allResponses = PTHREAD_COND_INITIALIZER;
	pthread_cond_t waitSignal[NUM_OF_COND_VARS]; // initialized in constructor; array for wait-signal cond variables
	pthread_t thread;

	static void *myThreadReceiverHelper(void *context) {
        return ((MyLock *)context)->myThreadReceiver();
    }
    void initThreadReceiver();
	void stopThreadReceiver();
	void broadcastLockAcquireRequest();
	void handleMsgAcquireRequestReceived();
	void handleMsgAcquireReplyReceived();
	void handleMsgReleaseReceived(int dataSize);
	void handleMsgPrintReceived(int dataSize);
	void handleMsgSignalReceived();
	void* myThreadReceiver(void);
	void broadcastLockRelease();
public:
	T data;
	MyLock(T input);
	virtual ~MyLock();
	void acquire();
	void release();
	T* getData();
	void wait(int id);
	void signal(int id);
	void sendToPrinter(std::string texts);
};