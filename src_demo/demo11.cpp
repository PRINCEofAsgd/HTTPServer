/*
 多进程文件传输客户端程序
*/

#include <iostream>
#include <fstream>
#include <cstdio>

#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

class tcpclient {
private:
	int clientfd;		// 客户端通信的文件描述符
	string serverip;	// 服务端ip或域名
	unsigned short port;	// 通信端口

public:
	tcpclient(): clientfd(-1) {}

	bool initclient(const string& in_serverip, const unsigned short in_port) {
		
		if(clientfd != -1) return false;

		// 把传入的服务端ip和端口参数赋值保存到成员变量
		serverip = in_serverip;
		port = in_port;

		if( (clientfd = socket(PF_INET, SOCK_STREAM, 0)) == -1 ) {
			cerr << "Fail to init sock." << endl;
			return false; // 创建客户端通信套接字描述符
		}

		// 创建结构体存储服务端地址信息
		struct sockaddr_in servaddr_in;
		memset(&servaddr_in, 0, sizeof(servaddr_in));
		servaddr_in.sin_family = PF_INET;
		servaddr_in.sin_port = htons(port);
		
		// 通过系统调用获取服务器主机的映射信息
		struct hostent* server_h;
		if ( (server_h = gethostbyname(serverip.c_str())) == nullptr) { 
			close(clientfd);
			clientfd = -1;
			cerr << "Fail to get serverip." << endl;
			return false;
		}
		memcpy(&servaddr_in.sin_addr, server_h->h_addr, server_h->h_length);

		// 连接到服务端套接字等待队列
		if (connect(clientfd, (struct sockaddr*)&servaddr_in, sizeof(servaddr_in)) == -1) {
			close(clientfd);
			clientfd = -1;
			cerr << "Fail to connect to server." << endl;
			return false;
		}

		return true;

	}

	// 向对端发送控制信息(字符串缓冲)
	bool cltsend_ctrl(const string& buffer) {
		if (clientfd == -1) return false;
		if ( (send(clientfd, &buffer[0], sizeof(buffer), 0)) <= 0 ) return false;
		return true;
	}

	// 向对端发送二进制数据(分批次,以精确字节数发送文件的一部分)
	bool cltsend_data(const void* buffer, const size_t bsize) {
		if (clientfd == -1) return false;
		if ( (send(clientfd, buffer, bsize, 0)) <= 0 ) return false;
		return true;
	}

	// 向对端发送文件(分批次传输,调用cltsend_data())
	bool cltsend_file(const string& filename, const size_t filesize) {

		if (clientfd == -1) return false;
		ifstream fin;
		fin.open(filename, ios::binary);
		if (!fin.is_open()) {
			cerr << "Fail to open file." << endl;
			return false;
		}

		size_t sended = 0;
		size_t sendonce = 0;
		char buffer[4096];
		while (true) {
			if (filesize - sended > 4096) sendonce = 4096;
			else sendonce = filesize - sended;
			fin.read(buffer, sendonce);
			if (cltsend_data(&buffer[0], sendonce) == false) {
				cerr << "Fail to send file." << endl;
			}
			sended += sendonce;
			if (sended == filesize) break;
		}

		return true;
	}

	// 接收控制信息
	bool cltrecv_ctrl(string& buffer, const size_t maxlen) {
		if (clientfd == -1) return false;
		buffer.clear();
		buffer.resize(maxlen);
		int bsize = recv(clientfd, &buffer[0], sizeof(buffer), 0);
		if (bsize <= 0) {
			buffer.clear();
			return false;
		}
		buffer.resize(bsize);
		return true;
	}

	// 关闭客户端描述符
	bool closeclient() {
		if (clientfd == -1) return false;
		close(clientfd);
		clientfd = -1;
		return true;
	}

	~tcpclient() {
		closeclient();
	}

};



int main(int argc, char* argv[])
{

	if (argc != 6) {
		cout << "Using:./demo11 服务端ip/域名 通信端口 发送文件名 发送文件大小 发送文件路径\n"
		     << "Example:./demo11 192.168.60.101 5005 aaa.txt 753 /home/loki/zhuom\n";
		return -1;
	}

	tcpclient democlient;
	if (democlient.initclient(argv[1], atoi(argv[2])) == false) {
		perror("initclient()");
		return -1;
	}
	cout << "成功连接服务端" << endl;
	
	struct fileinfo {
		char filename[256];
		int filesize;
	} demofile;
	memset(&demofile, 0, sizeof(demofile));
	memcpy(&demofile.filename, argv[3], strlen(argv[3]));
	demofile.filesize = atoi(argv[4]);

	// 向服务端发送文件元信息结构体
	if (democlient.cltsend_data(&demofile, sizeof(demofile)) == false) {
		perror("cltsend_data()");
		return -1;
	}
	cout << "发送文件信息结构体：" << demofile.filename << " 大小：" << demofile.filesize << endl;

	// 接收服务端回复的确认信息
	string ctrlbuffer;
	if (democlient.cltrecv_ctrl(ctrlbuffer, 4) == false) {
		perror("cltrecv_ctrl()");
		return -1;
	}
	if (ctrlbuffer != "ok. ") cout << "服务端没有回复:ok. " << endl;
	else cout << "服务端回复:ok. " << endl;

	// 向服务端发送文件
	if (democlient.cltsend_file(string(argv[5]) + '/' + demofile.filename, demofile.filesize) == false) {
		perror("cltsend_file");
		return -1;
	}
	if (democlient.cltrecv_ctrl(ctrlbuffer, 4) == false) {
		perror("cltrecv_ctrl()");
		return -1;
	}
	if (ctrlbuffer != "ok. ") cout << "传送文件失败. " << endl;
	else cout << "传送文件成功. " << endl;

	return 0;

}















