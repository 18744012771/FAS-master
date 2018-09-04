#include <errno.h>

#include <Condition.h>
#include <Mutex.h>
//pthread_cond_init用来创建一个条件变量
fas::Condition::Condition(Mutex& mutex) :
    mutex_(mutex) {
    ::pthread_cond_init(&cond_, NULL);
}
//用于释放一个条件变量的资源
fas::Condition::~Condition() {
    ::pthread_cond_destroy(&cond_);
}
//pthread_cond_wait和pthread_cond_timedwait用来等待条件变量被设置，
//值得注意的是这两个等待调用需要一个已经上锁的互斥体mutex，这是为了
//防止在真正进入等待状态之前别的线程有可能设置该条件变量而产生竞争。
void fas::Condition::wait() {
    ::pthread_cond_wait(&cond_, mutex_.getMutex());
}

bool fas::Condition::waitForSeconds(int seconds) {
    struct timespec abstime;
    ::clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += seconds;
    return ETIMEDOUT == pthread_cond_timedwait(&cond_, mutex_.getMutex(), &abstime);
}
//用于解除某一个等待线程的阻塞状态
void fas::Condition::notify() {
    ::pthread_cond_signal(&cond_);
}
//pthread_cond_broadcast用于设置条件变量，即使得事件发生，这样等待该事件的线程将不再阻塞
void fas::Condition::notifyAll() {
    ::pthread_cond_broadcast(&cond_);
}



