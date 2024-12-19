#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/epoll.h> 

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define	ENABLE_HTTP_RESPONSE	1


#define BUFFER_LENGTH 1024

typedef int (*RCALLBACK)(int fd);


// listenfd有可读事件，执行accept_cb回调函数
// listenfd触发EPOLLIN事件时执行accept_cb
int accept_cb(int fd);

// clientfd有可读事件，执行recv_cb回调函数。可写事件，执行send_cd回调函数
// clientfd触发EPOLLIN事件时执行，执行recv_cb。触发EPOLLOUT事件时执行，执行send_cd
int recv_cb(int fd); //buffer存放数据的指针，数据的长度
int send_cb(int fd);

// 全局变量
int epfd = 0;  // 蜂巢盒子

//解决数据读取不完，这样还能拼接  
// conn，fd，buffer，callback
// 一个连接应该有这三个fd，buffer，callback
struct conn_item {
	int fd;

	// 将读写分离出来
	char rbuffer[BUFFER_LENGTH];
	int rlen;
	char wbuffer[BUFFER_LENGTH];
	int wlen;

	char resource[BUFFER_LENGTH]; // index.html 

	union {
		RCALLBACK accept_callback;
		RCALLBACK recv_callback;
	} recv_t;
	RCALLBACK send_callback;
};

struct conn_item connlist[1024] = { 0 };

#if ENABLE_HTTP_RESPONSE

#define ROOT_DIR	"/root/sharedata/learn/2.1.1-multi-io"

typedef struct conn_item connection_t;
//http://192.168.243.129:2048/index.html
//GET / index.html HTTP / 1.1
int http_request(connection_t* conn) {
	//conn->rbuffer
	//conn->rlen

	return 0;
}

int http_response(connection_t* conn) {

#if 0
	conn->wlen = sprintf(conn->wbuffer,
		"HTTP/1.1 200 0k\r\n"
		"Accept-Ranges: bytes\r\n" 
		"Content-Length: 82\r\n"
		"Content-Type: text/html\r\n"
		"Date:sat, 06 Aug 2022 13:16:46 GMT\r\n\r\n"
		"<html><head><title>0voice.king</title></head><body><h1>King</h1></body></html>\r\n\r\n");
#else

	int filefd = open("index.html", O_RDONLY);

	struct stat stat_buf; //获取到文件的状态,比如文件的长度
	fstat(filefd, &stat_buf);

	conn->wlen = sprintf(conn->wbuffer,
		"HTTP/1.1 200 0k\r\n"
		"Accept-Ranges: bytes\r\n"
		"Content-Length: %ld\r\n"
		"Content-Type: text/html\r\n"
		"Date:sat, 06 Aug 2022 13:16:46 GMT\r\n\r\n", stat_buf.st_size); // stat_buf.st_size文件的长度

	int count = read(filefd, conn->wbuffer + conn->wlen, BUFFER_LENGTH - conn->wlen);
	conn->wlen += count; 

#endif
	return conn->wlen; 
}

#endif
//修改事件
int set_event(int fd, int event, int flag) {
	
	struct epoll_event ev;
	ev.events = event;
	ev.data.fd = fd;
	if (flag) { //1 add    0 mod(修改)

		epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
	}
	else {
		epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
	}
}

int accept_cb(int fd) {

	// 客户端进行接收
	struct sockaddr_in clientaddr;
	socklen_t len = sizeof(clientaddr);
	// accept阻塞函数，客户端没有连接就一直等
	// 点餐服务员 ，后续一切都有它处理，与客户端对应
	int clientfd = accept(fd, (struct sockaddr*)&clientaddr, &len);
	
	if (clientfd < 0) { //失败直接返回-1
		return -1;
	}
	
	set_event(clientfd, EPOLLIN, 1); //新增

	connlist[clientfd].fd = clientfd;
	memset(connlist[clientfd].rbuffer, 0, BUFFER_LENGTH); //清空buffer
	connlist[clientfd].rlen = 0;
	memset(connlist[clientfd].wbuffer, 0, BUFFER_LENGTH); //清空buffer
	connlist[clientfd].wlen = 0;
	connlist[clientfd].recv_t.recv_callback = recv_cb;
	connlist[clientfd].send_callback = send_cb;

	
	return clientfd;
}

// 读数据，并把对应的数据放到合适的位置
int recv_cb(int fd) {
	char* buffer = connlist[fd].rbuffer;
	int idx = connlist[fd].rlen;

	// buffer加上idx得到指针要新存数据的地址，同时最大存储量减去已经存的数据长度
	int count = recv(fd, buffer + idx, BUFFER_LENGTH - idx, 0);
	// 客户端断开则退出
	if (count == 0) {

		printf("disconnect\n");

		epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL); //构建了k-v存储，所以删除只需传k就行
		close(fd); // 关闭连接

		return -1;
	}
	// 存储长度加上此时传数据的长度
	connlist[fd].rlen += count;

	// http
	http_request(&connlist[fd]);
	http_response(&connlist[fd]);

	//修改事件，读已经处理完了，现在关注是否可写
	set_event(fd, EPOLLOUT, 0); //修改为可写

	return count;
}

int send_cb(int fd) {
	char* buffer = connlist[fd].wbuffer;
	int idx = connlist[fd].wlen;
	int count = send(fd, buffer, idx, 0);

	//修改事件,否则会一直等
	set_event(fd, EPOLLIN, 0); //修改为可读
	 
	return count;
}

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

	//加入accept回调函数
	connlist[sockfd].fd = sockfd;
	connlist[sockfd].recv_t.accept_callback = accept_cb;

	//reactor
	//epoll_create(); 安装一个蜂巢盒子
	//epoll_ctl(); 对io可读可写，进行添加
	//epoll_wait(); 快递员多长时间去蜂巢放取快递

	epfd = epoll_create(1); // int size   指盒子大小，后来转为了链表，
							 	// 为了兼容保留了参数，只要大于零就行

	set_event(sockfd, EPOLLIN, 1); // 放入了一个住户   

	struct epoll_event events[1024] = { 0 };
	while (1) { //mainloop(); 主要的循环
		int nready = epoll_wait(epfd, events, 1024, -1);

		int i = 0;
		for (i = 0; i < nready; i++) {
			
			int connfd = events[i].data.fd;
			if (events[i].events & EPOLLIN) {

				int count = connlist[connfd].recv_t.recv_callback(connfd); // 使用回调函数
				printf("recv <-- buffer:%s\n", connlist[connfd].rbuffer);
			}
			else if (events[i].events & EPOLLOUT) {
				printf("send --> buffer:%s\n", connlist[connfd].wbuffer);

				int count = connlist[connfd].send_callback(connfd); // 使用回调函数

			}  
		}
	}

	getchar(); // 防止执行完直接退出

}

