#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int flag = 0; //�Ƿ����ڴ�й©���

//mallocһ�λ����һ���ļ�,������֮��Ϳ���֪������ͬ�����С������ڴ�����й¶��
void* nMalloc(size_t size, const char* filename, int line) {
	void* p = malloc(size);

	if (flag) {
		//�����ļ�
		char buff[128] = { 0 };
		snprintf(buff, 128, "./mem/%p.mem", p);

		FILE* fp = fopen(buff, "w");
		if (!fp) {
			free(p);
			return NULL;
		}

		//�ļ�д������
		fprintf(fp, "[+]%s:%d, addr: %p, size: %ld\n", filename, line, p, size);
		fflush(fp);
		fclose(fp);
	}

	//printf("nMalloc: %p, size: %ld\n", p, size);
	return p;
} 

void nFree(void* ptr) {
	//printf("nFree: %p\n", ptr);

	if (flag) {
		//ɾ����Ӧ���ļ�
		char buff[128] = { 0 };
		snprintf(buff, 128, "./mem/%p.mem", ptr);

		if (unlink(buff) < 0) {
			printf("double free: %p", ptr);
			return;
		}
	}

	return free(ptr);
}

#define malloc(size) nMalloc(size, __FILE__, __LINE__) //__FILE__, __LINE__��ȡ�ļ�������һ�У��������Դ���
#define free(ptr) nFree(ptr)

int main() {
#if 1
	void* p1 = malloc(5);
	void* p2 = malloc(10);
	void* p3 = malloc(15);
	void* p4 = malloc(10);

	free(p1);
	free(p3);
	free(p4);
#else
	void* p1 = nMalloc(5);
	void* p2 = nMalloc(10);
	void* p3 = nMalloc(15);

	nFree(p1);
	nFree(p3);
#endif
	getchar();

	return 0;

}