#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>


//解决死锁——>构建有向图，关系表

//加锁前，查表判断资源是否被其他线程占用
//若占用，则有向图中构建一条当前线程指向目标资源被占用线程的边
void before_lock(pthread_t tid, pthread_mutex_t* mtx) {

}

//加锁后，关系表中增加一条关系
//有向图中删除一条边
void after_lock(pthread_t tid, pthread_mutex_t* mtx) {

}

//解锁后，关系表中删除一条关系
void after_unlock(pthread_t tid, pthread_mutex_t* mtx) {

}

typedef int (*pthread_mutex_lock_t)(pthread_mutex_t* mtx);
pthread_mutex_lock_t phtread_mutex_lock_f = NULL;

typedef int (*pthread_mutex_unlock_t)(pthread_mutex_t* mtx);
pthread_mutex_unlock_t phtread_mutex_unlock_f = NULL;

int pthread_mutex_lock(pthread_mutex_t* mtx) {

	printf("before pthread_mutex_lock %ld, %p\n", pthread_self(), mtx);

	phtread_mutex_lock_f(mtx);

	printf("after pthread_mutex_lock\n");


}

int pthread_mutex_unlock(pthread_mutex_t* mtx) {

	phtread_mutex_unlock_f(mtx);

	printf("pthread_mutex_unlock %ld, %p\n", pthread_self(), mtx);
}

void init_hook(void) {

	if (!phtread_mutex_lock_f) {
		phtread_mutex_lock_f = dlsym(RTLD_NEXT, "phtread_mutex_lock");
	}

	if (!phtread_mutex_unlock_f) {
		phtread_mutex_unlock_f = dlsym(RTLD_NEXT, "phtread_mutex_unlock");
	}
}

//死锁
#if 1  //debug

pthread_mutex_t mtx1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx4 = PTHREAD_MUTEX_INITIALIZER;

//线程1中拥有资源1占用资源2
void* t1_cb(void* arg) {

	pthread_mutex_lock(&mtx1);

	sleep(1); //等待1秒好让线程二执行，从而发生死锁

	pthread_mutex_lock(&mtx2);

	printf("t1_cb\n");

	pthread_mutex_unlock(&mtx2);

	pthread_mutex_unlock(&mtx1);

}

//线程2中拥有资源2占用资源3
void* t2_cb(void* arg) {
	pthread_mutex_lock(&mtx2);

	sleep(1); 


	pthread_mutex_lock(&mtx3);

	printf("t2_cb\n");

	pthread_mutex_unlock(&mtx3);

	pthread_mutex_unlock(&mtx2);
}

//线程3中拥有资源3占用资源4
void* t3_cb(void* arg) {
	pthread_mutex_lock(&mtx3);

	sleep(1);

	pthread_mutex_lock(&mtx4);

	printf("t3_cb\n");

	pthread_mutex_unlock(&mtx4);

	pthread_mutex_unlock(&mtx3);
}

//线程4中拥有资源4占用资源1
void* t4_cb(void* arg) {
	pthread_mutex_lock(&mtx4);

	sleep(1);

	pthread_mutex_lock(&mtx1);

	printf("t4_cb\n");

	pthread_mutex_unlock(&mtx1);

	pthread_mutex_unlock(&mtx4);
}

int main() {

	init_hook();

	pthread_t t1, t2, t3, t4;

	pthread_create(&t1, NULL, t1_cb, NULL);
	pthread_create(&t2, NULL, t2_cb, NULL);
	pthread_create(&t3, NULL, t3_cb, NULL);
	pthread_create(&t4, NULL, t4_cb, NULL);

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	pthread_join(t3, NULL);
	pthread_join(t4, NULL);

	printf("complete\n");

	return 0;
}

#endif