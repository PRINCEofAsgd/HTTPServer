#include "../core/TcpServer.h"
#include "../core/ThreadPool.h"
#include "HttpContext.h"
#include "HttpRouter.h"
#include "DownloadController.h"
#include "UploadController.h"
#include "StaticFileController.h"
#include "UserManager.h"
#include "RegisterController.h"
#include "TokenManager.h"
#include "LoginController.h"
#include "AuthorMiddleWare.h"
#include "../redis/RedisClient.h"

class HttpServer {
private:
    TcpServer* tcpserver_;
    ThreadPool threadpool_;
    HttpRouter router_;
    StaticFileController staticFileController_;
    DownloadController downloadController_;
    UploadController uploadController_;
    UserManager* userManager_;
    RegisterController* registerController_;
    RedisClient* redisClient_;
    TokenManager* tokenManager_;
    LoginController* loginController_;
    AuthorMiddleWare* authorMiddleWare_;

public:
    HttpServer(TcpServer* tcpserver, int threadnum_comp = 5);
    ~HttpServer();

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