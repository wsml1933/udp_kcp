#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int flag = 0; //是否开启内存泄漏检测

//malloc一次会分配一个文件,加上行之后就可以知道在相同分配大小情况下内存在哪泄露的
void* nMalloc(size_t size, const char* filename, int line) {
	void* p = malloc(size);

	if (flag) {
		//创建文件
		char buff[128] = { 0 };
		snprintf(buff, 128, "./mem/%p.mem", p);

		FILE* fp = fopen(buff, "w");
		if (!fp) {
			free(p);
			return NULL;
		}

		//文件写入内容
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
		//删除对应的文件
		char buff[128] = { 0 };
		snprintf(buff, 128, "./mem/%p.mem", ptr);

		if (unlink(buff) < 0) {
			printf("double free: %p", ptr);
			return;
		}
	}

	return free(ptr);
}

#define malloc(size) nMalloc(size, __FILE__, __LINE__) //__FILE__, __LINE__获取文件名和哪一行（编译器自带）
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