#include "TcpServer.h"
#include "ThreadPool.h"

class EchoServer {
private:
    TcpServer* tcpserver_;
    ThreadPool threadpool_;

public:
    EchoServer(TcpServer* tcpserver, int threadnum_comp = 5);
    ~EchoServer();

    void start();
    void stop();

    void handle_newconn(spConnection conn);
    void handle_closeconn(spConnection conn);
    void handle_errorconn(spConnection conn);
    void handle_removeconn(spConnection conn);
    void handle_recv(spConnection conn, std::shared_ptr<std::string> message);
    void handle_send(spConnection conn);
    void handle_eptimeout(EventLoop* loop);
    void handle_comp(spConnection conn, std::shared_ptr<std::string> message);
};
