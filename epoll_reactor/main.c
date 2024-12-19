#include<sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//一个服务端对应一个客户端

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

	// 客户端进行接收
	struct sockaddr_in clientaddr;
	socklen_t len = sizeof(clientaddr);
	// accept阻塞函数，客户端没有连接就一直等
	// 点餐服务员 ，后续一切都有它处理，与客户端对应
	int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
	printf("accept\n");

#if 0
	// 接收一次数据
	char buffer[128] = {0};
	// recv阻塞函数，数据没有来就一直等
	int count = recv(clientfd, buffer, 128, 0);
	//填count，就是发多大你收多大，比如发32，你就收32的数据
	//填128，就是收128数据，你发32，它能接收，但是接收大小为128，发送数据与返回数据大小不匹配
	send(clientfd, buffer, count /*128*/, 0); 
	printf("sockfd:%d, clientfd:%d, count:%d, buffer:%s\n", sockfd, clientfd, count, buffer);

#else
	//一直接收数据
	while(1){
		char buffer[128] = {0};
		// 当客户端断开的时候recv返回count=0,根据count=0判断客户端是否断开连接
		int count = recv(clientfd, buffer, 128, 0);
		// 客户端断开则退出
		if(count == 0){
			break;
		}
		send(clientfd, buffer, count /*128*/, 0); 
		printf("sockfd:%d, clientfd:%d, count:%d, buffer:%s\n", sockfd, clientfd, count, buffer);
	}
#endif

	getchar(); // 防止执行完直接退出
	// 关闭客户端连接
	close(clientfd);
	return 0;
}