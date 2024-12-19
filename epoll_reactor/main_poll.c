#include<sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/poll.h>
//一个服务端对应多个客户端

//tcp
int main()
{
	// 迎宾服务员
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(struct sockaddr_in));

	//绑定本地的，
	// ens33网卡，就是只和他进行连接。
	// lo（回环地址）是本地应用进行通信。
	// any意思是只要有个网卡就行，都可以通信
	serveraddr.sin_family = AF_INET;
	//网卡地址
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//端口，1024之前的都是系统在用的
	serveraddr.sin_port = htons(2048);

	//绑定
	if(-1 == bind(sockfd,(struct sockaddr*)&serveraddr,sizeof(struct sockaddr))){
		perror("bind");
		return -1;
	}

	//进行监听
	listen(sockfd,10);




	// poll
	// 传三个参数，三个集合被合成一个参数了，底层实现是一样的copy
	/*
	struct pollfd {
		int fd;          // 表示是哪个io
		short events;    // 传入的事件
		short revents;   // 返回的事件
	};
	*/


	struct pollfd fds[1024] = { 0 };

	fds[sockfd].fd = sockfd;
	fds[sockfd].events = POLLIN;

	int maxfd = sockfd;

	while (1) {

		int nready = poll(fds, maxfd + 1, -1); // 使用maxfd + 1不使用1024，原因是可以避免无效的访问

		if (fds[sockfd].revents & POLLIN) {
			// 客户端进行接收
			struct sockaddr_in clientaddr;
			socklen_t len = sizeof(clientaddr);
			// accept阻塞函数，客户端没有连接就一直等
			// 点餐服务员 ，后续一切都有它处理，与客户端对应
			int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
			printf("sockfd: %d \n", clientfd);
			fds[clientfd].fd = clientfd;
			fds[clientfd].events = POLLIN;

			maxfd = clientfd;
		}
		int i = 0;
		for (i = sockfd + 1; i <= maxfd; i++) { 
			if (fds[i].revents & POLLIN) { // 如果有事件，则读出来
				char buffer[128] = { 0 };
				// 当客户端断开的时候recv返回count=0,根据count=0判断客户端是否断开连接
				int count = recv(i, buffer, 128, 0);
				// 客户端断开则退出
				if (count == 0) {

					printf("disconnect\n");
					
					fds[i].fd = -1;
					fds[i].events = 0;

					close(i); // 关闭连接

					break;
				}
				send(i, buffer, count /*128*/, 0);
				printf("clientfd:%d, count:%d, buffer:%s\n", i, count, buffer);
			}
		}	
	}
	getchar(); // 防止执行完直接退出

}

