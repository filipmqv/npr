#include <serializable_class.cpp>
#include <type_traits>
#include <vector>
#include <set>

template<class T>
class MyLock {
private:
	int worldSize, rank;
	long timestampGlobal;
	bool isRequesting, allowed; //allowed - set before signal in case signal goes before wait
	std::set<int> acquireReplysRanks; // collect unique ranks which replied
	std::vector<int> delayedReplys; // for replys that should be sent after critical section
	std::vector<long> delayedReplysTimestamps;
	pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER, dataLock = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t allResponses = PTHREAD_COND_INITIALIZER;
	pthread_t thread;

	static void *myThreadReceiverHelper(void *context) {
        return ((MyLock *)context)->myThreadReceiver();
    }
    void sendToPrinter(std::initializer_list<std::string> texts);
    void initThreadReceiver();
	void stopThreadReceiver();
	void broadcastLockAcquireRequest();
	void handleMsgAcquireRequestReceived();
	void handleMsgAcquireReplyReceived();
	void handleMsgReleaseReceived(int dataSize);
	void handleMsgPrintReceived(int dataSize);
	void* myThreadReceiver(void);
	void broadcastLockRelease();
public:
	T data;
	MyLock(T input);
	virtual ~MyLock();
	void acquire();
	void release();
	T* getData();
};