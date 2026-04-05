#include "../../include/http/HttpServer.h"
#include "../../include/http/HttpResponse.h"
#include "../../include/core/SendTask.h"
#include "../../include/core/Timestamp.h"
#include "../../third_party/nlohmann/json.hpp"
#include <sys/syscall.h>
#include <sstream>
#include <random>

using json = nlohmann::json;

HttpServer::HttpServer(TcpServer* tcpserver, int threadnum_comp) : tcpserver_(tcpserver), threadpool_(threadnum_comp, "COMP") {
    // 设置 TcpServer 不同通讯事件回调函数，事件有对应的业务可回调本层业务层处理，没有则注释掉业务即可忽略事件、不回调
    tcpserver_->set_newconncallback([this](spConnection conn) { this->handle_newconn(conn); });
    tcpserver_->set_closeconncallback([this](spConnection conn) { this->handle_closeconn(conn); });
    tcpserver_->set_errorconncallback([this](spConnection conn) { this->handle_errorconn(conn); });
    tcpserver_->set_removeconncallback([this](spConnection conn) { this->handle_removeconn(conn); });
    tcpserver_->set_recvcallback([this](spConnection conn, std::shared_ptr<std::string> msg) { this->handle_recv(conn, msg); });
    tcpserver_->set_sendcallback([this](spConnection conn) { this->handle_send (conn); });
    // tcpserver_->set_eptimeoutcallback([this](EventLoop* loop) { this->handle_eptimeout(loop); });
    
    // 初始化 RedisClient 和 MySQLClient
    redisClient_ = new RedisClient();
    redisClient_->connect(); 
    mysqlClient_ = new MySQLClient();
    mysqlClient_->connect(); 
    
    // 初始化 TokenManager, UserManager
    tokenManager_ = new TokenManager(redisClient_);
    userManager_ = new UserManager(redisClient_, mysqlClient_);
    
    // 初始化 RegisterController, LoginController, AuthorMiddleWare
    authorMiddleWare_ = new AuthorMiddleWare(tokenManager_);
    registerController_ = new RegisterController(userManager_, "/home/loki/桌面/HttpStaticFiles");
    loginController_ = new LoginController(userManager_, tokenManager_);
    
    // 初始化 DownloadController, UploadController
    downloadController_ = new DownloadController(authorMiddleWare_);
    uploadController_ = new UploadController(authorMiddleWare_);
    
    // 注册路由
    router_.registerRoute("GET", "/heartbeat", [](const HttpRequest& request) { 
        HttpResponse response(200);
        response.addHeader("Content-Type", "text/plain");
        response.setBody("OK");
        return response;
    });
    router_.registerRoute("POST", "/register", [this](const HttpRequest& request) { return registerController_->handleRegister(request); });
    router_.registerRoute("POST", "/login", [this](const HttpRequest& request) { return loginController_->handleLogin(request); });
    router_.registerRoute("GET", "/", [this](const HttpRequest& request) { return staticFileController_.handleStaticFile(request); });
    router_.registerRoute("GET", "/download", [this](const HttpRequest& request) { return downloadController_->handleFileDownload(request); });
    router_.registerRoute("POST", "/upload", [this](const HttpRequest& request) { return uploadController_->handleFileUpload(request); });
}

HttpServer::~HttpServer() {
    delete redisClient_;
    delete mysqlClient_;
    delete tokenManager_;
    delete userManager_;
    delete authorMiddleWare_;
    delete loginController_;
    delete registerController_;
    delete downloadController_;
    delete uploadController_;
}
void HttpServer::start() { tcpserver_->start(); }
void HttpServer::stop() {
    threadpool_.stop();
    printf("comp threads have been stopped.\n");
}

void HttpServer::handle_newconn(spConnection conn) { printf("new connection(fd = %d, ip = %s, port = %d) success in main thread %ld.\n", conn->get_fd(), conn->get_ip().c_str(), conn->get_port(), syscall(SYS_gettid)); }
void HttpServer::handle_closeconn(spConnection conn) { printf("connection(fd = %d) disconnected in main thread %ld.\n", conn->get_fd(), syscall(SYS_gettid)); }
void HttpServer::handle_errorconn(spConnection conn) { printf("connection(fd = %d) error in main thread %ld.\n", conn->get_fd(), syscall(SYS_gettid)); }
void HttpServer::handle_removeconn(spConnection conn) { printf("connection(fd = %d) removed in IO thread %ld.\n", conn->get_fd(), syscall(SYS_gettid)); }
void HttpServer::handle_send(spConnection conn) { /* printf("send message(fd = %d) success in IO thread %ld.\n\n", conn->get_fd(), syscall(SYS_gettid)); */ }
void HttpServer::handle_eptimeout(EventLoop* loop) { printf("epoll_wait() out of time %d seconds in IO thread %ld.\n", loop->get_timeout() / 1000, syscall(SYS_gettid)); }

void HttpServer::handle_recv(spConnection conn, std::shared_ptr<std::string> message) {
    printf("receive message(fd = %d) success in IO thread %ld.\n", conn->get_fd(), syscall(SYS_gettid));
    threadpool_.addtask([this, conn, message]() { this->handle_comp(conn, message); }); // 将 HTTP 报文解析任务提交到计算线程池
}

void HttpServer::handle_comp(spConnection conn, std::shared_ptr<std::string> message) {
    HttpContext context;
    Buffer buffer(2);
    buffer.append(message->data(), message->size());
    
    if (context.parse(buffer)) { // HttpContext 解析 recv 到的字符串流 HTTP 报文，转换为 HttpRequest 对象
        const HttpRequest& request = context.getRequest();
        printf("HTTP request parsed in comp thread %ld:\n", syscall(SYS_gettid));
        printf("Method: %s\n", request.getMethod().c_str());
        printf("Path: %s\n", request.getPath().c_str());
        printf("Version: %s\n\n", request.getVersion().c_str());
        
        HttpResponse response = router_.handleRequest(request); // 使用 HttpRouter 处理解析好的 HttpRequest 对象，构造 HttpResponse 对象
        
        // 添加X-Request字段，使用与请求相同的X-Request-ID值
        std::string xRequestId = request.getXRequestId();
        response.addHeader("X-Request-ID", xRequestId);
        
        // 根据请求的 Connection 头设置 Connection 的 keep-alive 状态，默认添加 keep-alive 头
        std::string connectionHeader = request.getConnectionHeader();
        if (!connectionHeader.empty() && connectionHeader == "close") { 
            response.addHeader("Connection", "close");
            conn->set_keep_alive(false);
        }
        
        // 首先发送 BUFFER 类型的任务(HTTP 响应头)
        std::shared_ptr<std::string> reply = std::make_shared<std::string>(response.toString());
        auto buffer_task = std::make_shared<SendTask>(TaskType::BUFFER);
        buffer_task->buffer_data = reply;
        printf("reply: %s\n", reply->c_str());
        conn->addtask_toIOthread(buffer_task);
        
        // 然后判断是否需要发送文件
        if (response.useSendfile()) {
            auto file_task = std::make_shared<SendTask>(TaskType::FILE);
            file_task->file_fd = response.getFileFd();
            file_task->file_offset = response.getFileOffset();
            file_task->file_size = response.getFileSize();
            conn->addtask_toIOthread(file_task);
        }
        
        conn->increment_request_count();                // 增加请求计数
        if (conn->should_close()) conn->close_conn();   // 检查是否应该关闭连接
    } 
    else { // 解析失败，返回400错误
        HttpResponse response(400);
        response.addHeader("Content-Type", "text/html");
        response.addHeader("Connection", "close"); // 解析失败时关闭连接
        // 添加X-Request-ID字段，使用默认值
        response.addHeader("X-Request-ID", "default-request-id");
        conn->set_keep_alive(false);
        response.setBody("<html><body><h1>400 Bad Request</h1></body></html>");
        
        std::shared_ptr<std::string> reply = std::make_shared<std::string>(response.toString());
        auto buffer_task = std::make_shared<SendTask>(TaskType::BUFFER);
        buffer_task->buffer_data = reply;
        conn->addtask_toIOthread(buffer_task);
        conn->close_conn(); // 解析失败时关闭连接
    }
}