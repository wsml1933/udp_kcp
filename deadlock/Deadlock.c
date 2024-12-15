#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>


//�����������>��������ͼ����ϵ��

//����ǰ������ж���Դ�Ƿ������߳�ռ��
//��ռ�ã�������ͼ�й���һ����ǰ�߳�ָ��Ŀ����Դ��ռ���̵߳ı�
void before_lock(pthread_t tid, pthread_mutex_t* mtx) {

}

//�����󣬹�ϵ��������һ����ϵ
//����ͼ��ɾ��һ����
void after_lock(pthread_t tid, pthread_mutex_t* mtx) {

}

//�����󣬹�ϵ����ɾ��һ����ϵ
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

//����
#if 1  //debug

pthread_mutex_t mtx1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx4 = PTHREAD_MUTEX_INITIALIZER;

//�߳�1��ӵ����Դ1ռ����Դ2
void* t1_cb(void* arg) {

	pthread_mutex_lock(&mtx1);

	sleep(1); //�ȴ�1������̶߳�ִ�У��Ӷ���������

	pthread_mutex_lock(&mtx2);

	printf("t1_cb\n");

	pthread_mutex_unlock(&mtx2);

	pthread_mutex_unlock(&mtx1);

}

//�߳�2��ӵ����Դ2ռ����Դ3
void* t2_cb(void* arg) {
	pthread_mutex_lock(&mtx2);

	sleep(1); 


	pthread_mutex_lock(&mtx3);

	printf("t2_cb\n");

	pthread_mutex_unlock(&mtx3);

	pthread_mutex_unlock(&mtx2);
}

//�߳�3��ӵ����Դ3ռ����Դ4
void* t3_cb(void* arg) {
	pthread_mutex_lock(&mtx3);

	sleep(1);

	pthread_mutex_lock(&mtx4);

	printf("t3_cb\n");

	pthread_mutex_unlock(&mtx4);

	pthread_mutex_unlock(&mtx3);
}

//�߳�4��ӵ����Դ4ռ����Դ1
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