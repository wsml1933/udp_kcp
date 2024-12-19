#include<sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
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

	// select 
	// 5个参数（最大的fd，可读的集合，可写的集合，出错的集合，超时）
	// fd代表连接，其中0（std::in），1(std::out)，2(标准错误)，3（listen）
	// 4是第一个连接，5第二个。。。。
	// maxfd是用来内部循环最大值，判断是否可读可写
	//int nready = select(maxfd, rset, wset, eset,timeout)

	//typedef struct {
	//	unsigned long fds_bits[1024 / (8 * sizeof(long))];
	//} __kernel_fd_set;

	fd_set rfds, rset;
	FD_ZERO(&rfds); // 全部清空
	FD_SET(sockfd, &rfds); //把sockfd这一位置为1

	int maxfd = sockfd; //刚进来就等于listen fd

	while (1) {

		rset = rfds;

		int nready = select(maxfd + 1, &rset, NULL, NULL, NULL); // nready事件的个数

		if (FD_ISSET(sockfd, &rset)) { // 判断是否置为1
			// 客户端进行接收
			struct sockaddr_in clientaddr;
			socklen_t len = sizeof(clientaddr);
			// accept阻塞函数，客户端没有连接就一直等
			// 点餐服务员 ，后续一切都有它处理，与客户端对应
			int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
			printf("sockfd: %d \n", clientfd);

			FD_SET(clientfd, &rfds); // 将clientfd在rfds中置为1

			maxfd = clientfd; // 最大值变为了新连接的fd
		}

		int i = 0;
		for (i = sockfd + 1; i <= maxfd; i++) { // sockfd + 1 代表listen后面的连接
			if (FD_ISSET(i, &rset)) { // 如果有事件，则读出来
				char buffer[128] = { 0 };
				// 当客户端断开的时候recv返回count=0,根据count=0判断客户端是否断开连接
				int count = recv(i, buffer, 128, 0);
				// 客户端断开则退出
				if (count == 0) {

					printf("disconnect\n");
					FD_CLR(i, &rfds); //清空io多路复用设置的这个
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

