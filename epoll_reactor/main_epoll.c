#include<sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/epoll.h>
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




	//epoll
	// 
	//epoll_create(); 安装一个蜂巢盒子
	//epoll_ctl(); 对io可读可写，进行添加
	//epoll_wait(); 快递员多长时间去蜂巢放取快递

	int epfd = epoll_create(1); // int size   指盒子大小，后来转为了链表，
							 	// 为了兼容保留了参数，只要大于零就行
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = sockfd;

	epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev); // 放入了一个住户   

	struct epoll_event events[1024] = { 0 };
	while (1) {
		int nready = epoll_wait(epfd, events, 1024, -1);

		int i = 0;
		for (i = 0; i < nready; i++) {
			
			int connfd = events[i].data.fd;
			if (sockfd == connfd) {
				// 客户端进行接收
				struct sockaddr_in clientaddr;
				socklen_t len = sizeof(clientaddr);
				// accept阻塞函数，客户端没有连接就一直等
				// 点餐服务员 ，后续一切都有它处理，与客户端对应
				int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);

				ev.events = EPOLLIN | EPOLLET;  //边缘触发( | EPOLLET )
				ev.data.fd = clientfd;
				epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);
			}
			else if (events[i].events & EPOLLIN) {
				char buffer[10] = { 0 };
				// 当客户端断开的时候recv返回count=0,根据count=0判断客户端是否断开连接
				int count = recv(connfd, buffer, 10, 0);
				// 客户端断开则退出
				if (count == 0) {

					printf("disconnect\n");
				
					epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL); //构建了k-v存储，所以删除只需传k就行
					close(i); // 关闭连接

					continue;
				}
				send(connfd, buffer, count /*128*/, 0);
				printf("clientfd:%d, count:%d, buffer:%s\n", connfd, count, buffer);
			}
		}
	}

	getchar(); // 防止执行完直接退出

}

