#include <serializable_class.cpp>
#include <type_traits>

template<class T>
class MyLock {
private:
	int worldSize, rank;
	long timestampGlobal;
	bool isRequesting, allowed; //allowed - set before signal in case signal goes before wait
	int numOfAcquireReplys;
	pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
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
	//std::string serialize();
	//struct deserialize();
public:
	T data;
	MyLock(T* input);
	virtual ~MyLock();
	void acquire();
	void release();
};