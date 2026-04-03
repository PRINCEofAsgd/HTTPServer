/*
    epoll模型实现网络通讯的服务端
*/

#include <iostream>

#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>

using namespace std;

void setnonblocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        cout << "Using: ./tcpepoll ip port\n"
             << "Example: ./tcpepoll 192.168.60.101 4819\n";
        return -1;
    }

    int listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenfd == -1) {
        perror("socket() failed");
        return -1;
    }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, (socklen_t)sizeof(opt));
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &opt, (socklen_t)sizeof(opt));
    setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, &opt, (socklen_t)sizeof(opt));
    setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, &opt, (socklen_t)sizeof(opt));
    setnonblocking(listenfd);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    if (bind(listenfd, (sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind()");
        return -1;
    }

    if (listen(listenfd, 128) == -1) {
        perror("listen()");
        return -1;
    }

    int epollfd = epoll_create(1);
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);
    epoll_event evrtn[10];

    while (true) {
        
        int infds = epoll_wait(epollfd, evrtn, 10, 10000);
        if (infds == -1) {
            perror("epoll_wait(): failed");
            break;
        }
        else if (infds == 0) {
            cout << "epoll_wait(): out of time for 10 seconds." << endl;
            continue;
        }

        for (int i = 0; i < infds; ++i) {

            if (evrtn[i].data.fd == listenfd) {
                sockaddr_in cltaddr;
                socklen_t cltaddr_len = sizeof(sockaddr);
                int clientfd = accept(listenfd, (sockaddr*)&cltaddr, &cltaddr_len);
                setnonblocking(clientfd);
                if (clientfd == -1) {
                    perror("accept()");
                    continue;
                }
                printf("accept client(fd = %d, ip = %s, port = %d).\n", 
                        clientfd, inet_ntoa(cltaddr.sin_addr), ntohs(cltaddr.sin_port));
                ev.data.fd = clientfd;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev);
            }

            else {

                if (evrtn[i].events & EPOLLRDHUP) {
                    printf("clientfd: %d disconnected.\n", evrtn[i].data.fd);
                    close(evrtn[i].data.fd);
                }

                else if (evrtn[i].events & (EPOLLIN | EPOLLPRI)) {

                    char buffer[1024];
                    
                    while (true) {
                        memset(&buffer[0], 0, sizeof(buffer));
                        int recvnum = recv(evrtn[i].data.fd, buffer, sizeof(buffer), 0);

                        if (recvnum > 0 ) {
                            printf("recv(fd = %d, buffer = %s)\n", evrtn[i].data.fd, buffer);
                            send(evrtn[i].data.fd, buffer, sizeof(buffer), 0);
                        }

                        else if (recvnum == -1 && errno == EINTR) continue;

                        else if (recvnum == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;

                        else if (recvnum == 0) {
                            printf("clientfd: %d disconnected.\n", evrtn[i].data.fd);
                            close(evrtn[i].data.fd);
                            break;
                        }
                    }

                }

                else if (evrtn[i].events & EPOLLOUT) { }

                else {
                    perror("clientfd: %d error.\n");
                    close(evrtn[i].data.fd);
                }

            }

        }

    }

    return 0;
}