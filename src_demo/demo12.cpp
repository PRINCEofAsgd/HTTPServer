/*
 多进程文件传输服务端程序
*/

#include <iostream>
#include <cstdio>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <signal.h>
#include <error.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

class tcpserver {

private:
	int listenfd;
	int clientfd;
	string clientip;
	unsigned short port;
	// 初始数据类型皆为终端传入参数的原始类型，便于存储和阅读，在初始化函数中进行转换

public:

	tcpserver(): listenfd(-1), clientfd(-1) {}

	bool initserver(const unsigned short in_port) {

		if ( (listenfd = socket(PF_INET, SOCK_STREAM, 0)) == -1 ) return false; // 创建套接字，失败返回

		port = in_port; // 端口号初始化为读取的参数
		
		// 存储所有地址信息的结构体初始化
		struct sockaddr_in servaddr_in;
		memset(&servaddr_in, 0, sizeof(servaddr_in));

		// 协议族、端口、ip结构体，三者初始化
		servaddr_in.sin_family = PF_INET;
		servaddr_in.sin_port = htons(port);
		servaddr_in.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY 可直接写0.0.0.0

		if (bind(listenfd, (struct sockaddr*)&servaddr_in, sizeof(servaddr_in)) == -1) { // 将地址信息绑定到套接字
			close(listenfd);
			listenfd = -1;
			return false; 
		}

		if(listen(listenfd, 5) == -1) { // 将套接字设置为可监听状态
			close(listenfd);
			listenfd = -1;
			return false;
		}

		return true;

	}

	bool servaccept() {
		struct sockaddr_in cltaddr_in;
		socklen_t cltaddr_len = sizeof(cltaddr_in);
		if ( (clientfd = accept(listenfd, (struct sockaddr*)&cltaddr_in, &cltaddr_len)) == -1) return false; // 从监听套接字的等候列表中取一个
		clientip = inet_ntoa(cltaddr_in.sin_addr);
		return true;
	}

	const string& getcltip() const { // 获取客户端ip
		return clientip;
	}

	bool servsend_ctrl(const string& buffer) { // 发送控制信息报文
		if (clientfd == -1) return false;
		if (send(clientfd, buffer.data(), buffer.size(), 0) <= 0) return false;
		return true;
	}

	bool servrecv_ctrl(string& buffer, const size_t maxlen) { //接收控制信息报文
		if (clientfd == -1) return false;
		buffer.clear();
		buffer.resize(maxlen);
		int realsize = recv(clientfd, &buffer[0], buffer.size(), 0);
		if (realsize <= 0) {
			buffer.clear();
			return false;
		}
		buffer.resize(realsize);
		return true;
	}

	bool servrecv_data(void* buffer, const size_t size) { // 接收单个数据报
		if (recv(clientfd, buffer, size, 0) <= 0) return false;
		return true;
	}

	bool servrecv_file(const string& filename, const size_t filesize) { // 接收文件(分为若干数据报发送)

		// 创建输入文件流
		ofstream fout;
		fout.open(filename, ios::binary);
		if (fout.is_open() == false) return false;

		// 分批次接收数据报判断信息
		size_t rcved = 0; // 已接收字节数
		size_t rcvonce = 0; // 单次接收的字节数
		char buffer[4096]; // 单次最大缓冲区

		while (true) { // 以4096为单位分批接收数据报至文件结束

			if (filesize - rcved > 4096) rcvonce = 4096;
			else rcvonce = filesize - rcved;

			if (servrecv_data(buffer, rcvonce) == false) return false;
			fout.write(buffer, rcvonce);

			rcved += rcvonce;
			if (rcved == filesize) break;
		}

		return true;
	}

	bool closeclt() {
		if (clientfd == -1) return false;
		close(clientfd);
		clientfd = -1;
		return true;
	}

	bool closelst() {
		if (listenfd == -1) return false;
		close(listenfd);
		listenfd = -1;
		return true;
	}

	~tcpserver() {
		closeclt();
		closelst();
	}

};

tcpserver demoserver;


void fatherexit(int sig) {
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	cout << "父进程退出, sig = "
	     << sig 
	     << endl;
	kill(0, SIGTERM);
	demoserver.closelst();
	exit(0);
}

void childexit(int sig) {
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	cout << "子进程退出, sig = "
	     << sig 
	     << endl;
	demoserver.closeclt();
	exit(0);
}


int main(int argc, char *argv[]) {

	if (argc != 3) {
		cout << "Using:./demo12 通信端口 文件存放目录\n"
		     << "Example:./demo12 5005 /home/loki/桌面\n"
		     << "注意：运行服务端程序的linux系统的防火墙必须开通输入的端口\n";
		return -1;
	}

	for (int i = 0; i < 64; ++i) signal(i, SIG_IGN);
	signal(SIGINT, fatherexit);
	signal(SIGTERM, fatherexit);

	if (demoserver.initserver(atoi(argv[1])) == false) {
		perror("initserver()");
		return -1;
	}
	cout << "套接字创建成功。" << endl;

	while (true) {

		if (demoserver.servaccept() == false) {
			perror("accept()");
			return -1;
		}

		int pid = fork(); // 创建子进程处理单个客户端连接socket, 父进程继续监听
		if (pid == -1 ) {
			perror("fork()");
			return -1;
		}
		if (pid > 0) { // 父进程, 循环监听
			demoserver.closeclt();
			continue;
		}

		// 子进程
		demoserver.closelst();
		signal(SIGINT, SIG_IGN);
		signal(SIGTERM, childexit);
		cout << "客户端已连接: "
		     << demoserver.getcltip()
		     << endl;

		struct fileinfo {
			char filename[256];
			int filesize;
		} demofile;
		memset(&demofile, 0, sizeof(demofile));

		// 接收文件名和文件大小
		if (demoserver.servrecv_data(&demofile, sizeof(demofile)) == false) {
			perror("servrecv_data()");
			return -1;
		}
		// 给客户端发送文件传输确认信息
		cout << "文件名: " << demofile.filename 
		     << ", 文件大小: " << demofile.filesize
		     << endl;
		if (demoserver.servsend_ctrl("ok. ") == false) {
			perror("servsend_ctrl()");
			return -1;
		}

		// 接收文件
		if (demoserver.servrecv_file(string(argv[2]) + '/' + demofile.filename, demofile.filesize) == false) {
			perror("servrecv_file()");
			return -1;
		}
		if (demoserver.servsend_ctrl("ok. ") == false) {
			perror("servsend_ctrl()");
			return -1;
		}
		cout << "接收文件成功" << endl;

		// 子进程退出
		return 0;

	}
}










