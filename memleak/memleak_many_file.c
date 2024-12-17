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

int enable_malloc = 1; //为了解除printf与malloc的循环递归
int enable_free = 1; //避免循环释放


//高版本对于caller地址无法查看，所以用这个函数，触发位置减去main函数的位置的偏移来确定是哪一行
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

		void* caller = __builtin_return_address(0);//gdb函数，返回上一级调用的位置，0代表上一级，1代表上上级

		//创建文件
		char buff[128] = { 0 };
		snprintf(buff, 128, "./mem/%p.mem", p);

		FILE* fp = fopen(buff, "w");
		if (!fp) {
			free(p);
			return NULL;
		}

		//文件写入内容
		//用    addr2line -f -e ./xxx -a 地址 -g   ，查看caller地址定位到哪一行
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
		//删除对应的文件
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
		//如果malloc_f未分配则将malloc入口地址转给malloc_f
		malloc_f = (malloc_t)dlsym(RTLD_NEXT, "malloc");  //编译时gcc -o xx xx.c -ldl
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