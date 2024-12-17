#define _GNU_SOURCE
#include <dlfcn.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <link.h>
//hook

typedef void* (*malloc_t)(size_t size);
malloc_t malloc_f = NULL;

typedef void (*free_t)(void* ptr);
free_t free_f = NULL;

int enable_malloc = 1; //Ϊ�˽��printf��malloc��ѭ���ݹ�
int enable_free = 1; //����ѭ���ͷ�


//�߰汾����caller��ַ�޷��鿴���������������������λ�ü�ȥmain������λ�õ�ƫ����ȷ������һ��
void* ConvertToELF(void* addr) {

	Dl_info info;
	struct link_map* link;

	dladdr1(addr, &info, (void**) & link, RTLD_DL_LINKMAP);

	return (void*)((size_t)addr - link->l_addr);
}


void* malloc(size_t size) {

	void* p = NULL;

	if (enable_malloc) {
		enable_malloc = 0;

		p = malloc_f(size);

		void* caller = __builtin_return_address(0);//gdb������������һ�����õ�λ�ã�0������һ����1�������ϼ�

		//�����ļ�
		char buff[128] = { 0 };
		snprintf(buff, 128, "./mem/%p.mem", p);

		FILE* fp = fopen(buff, "w");
		if (!fp) {
			free(p);
			return NULL;
		}

		//�ļ�д������
		//��    addr2line -f -e ./xxx -a ��ַ -g   ���鿴caller��ַ��λ����һ��
		//fprintf(fp, "[+]%p, addr: %p, size: %ld\n", caller, p, size);

		fprintf(fp, "[+]%p, addr: %p, size: %ld\n", ConvertToELF(caller), p, size);

		fflush(fp);
		fclose(fp);

		enable_malloc = 1;
	}
	else {
		p = malloc_f(size);
	}

	return p;
}

void free(void* ptr) {

	if (enable_free) {
		enable_free = 0;

		//printf(" % p\n", ptr);
		//ɾ����Ӧ���ļ�
		char buff[128] = { 0 };
		snprintf(buff, 128, "./mem/%p.mem", ptr);

		if (unlink(buff) < 0) {
			//printf("double free: %p\n", ptr);
			enable_free = 1;
			return;
		}

		free_f(ptr);

		enable_free = 1;
	}
	else {
		free_f(ptr);
	}

	return ;
}

void init_hook(void) {

	if (!malloc_f) {
		//���malloc_fδ������malloc��ڵ�ַת��malloc_f
		malloc_f = (malloc_t)dlsym(RTLD_NEXT, "malloc");  //����ʱgcc -o xx xx.c -ldl
	}

	if (!free_f) {
		free_f = (free_t)dlsym(RTLD_NEXT, "free");
	}
}

int main() {

	init_hook();
	void* p1 = malloc(5);
	void* p2 = malloc(10);
	void* p3 = malloc(15);
	void* p4 = malloc(10);

	free(p1);
	free(p3);
	free(p4);

	getchar();
	//return 0;

}