// HandlerFactory类实现
#include "../../include/client/HandlerFactory.h"
#include "../../include/client/Utils.h"
#include "../../include/client/ClientConn.h"
#include "../../include/client/ClientLoop.h"
#include "../../include/core/Buffer.h"
#include "../../include/http/HttpContext.h"

#include "../../include/client/HandlerRouter.h"
#include "../../include/client/JsonHandler.h"
#include "../../include/client/FileHandler.h"
#include "../../include/client/HeartBeatHandler.h"

HandlerFactory::HandlerFactory() : 
client_loop_(std::make_unique<ClientLoop>(HEARTBEAT_INTERVAL)), client_conn_(std::make_shared<ClientConn>(client_loop_.get())),
context_(std::make_unique<HttpContext>(false)), router_(std::make_unique<HandlerRouter>()), 
jsonHandler_(std::make_unique<JsonHandler>()), fileHandler_(std::make_unique<FileHandler>()), heartBeatHandler_(std::make_unique<HeartBeatHandler>()) {
    // 初始化连接和循环
    client_conn_->set_on_response([this](const std::string& message) { handleResponse(message); });
    client_loop_->handle_newconn(client_conn_);
    client_loop_->set_register_callback([this](int requestId, ReqType reqType) { RegisterRouter(requestId, reqType); });
    loop_thread_ = std::thread([this]() { client_loop_->run(); });
    // handler_thread_ = std::thread([this]() { router_->run(); }); // 启动路由线程，将来实现 IO 和业务分离
}

HandlerFactory::~HandlerFactory() {
    client_loop_->stop();
    if (loop_thread_.joinable()) loop_thread_.join();
}

// 处理HTTP响应
bool HandlerFactory::handleResponse(const std::string& message) {
    // 解析响应
    Buffer temp_buffer(2); // 使用HTTP分隔符
    temp_buffer.append(message.data(), message.size());
    
    if (context_->parse(temp_buffer)) { // 解析响应成功
        const HttpResponse& response = context_->getResponse();
        router_->handleResponse(response); // 调用HandlerRouter处理响应
        context_->reset(); // 重置上下文，准备处理下一个响应
        return true;
    }
    return false;
}

void HandlerFactory::handleRequest(const HttpRequest& request) { client_conn_->send_request(request); }

// 注册路由
void HandlerFactory::RegisterRouter(int requestId, ReqType reqType) {
    RequestContext context;
    context.reqType = reqType;
    
    // 根据ReqType设置回调函数
    switch (reqType) {
        case ReqType::HEARTBEAT:
            context.callback = [this](const HttpResponse& response) { heartBeatHandler_->handle(response); };
            break;
        case ReqType::LOGIN:
        case ReqType::REGISTER:
            context.callback = [this](const HttpResponse& response) { jsonHandler_->handle(response); };
            break;
        case ReqType::GET_STATIC:
            context.callback = [this](const HttpResponse& response) { fileHandler_->handle(response); };
            break;
        case ReqType::DOWNLOAD_FILE:
        case ReqType::UPLOAD_FILE:
            context.callback = [this](const HttpResponse& response) { fileHandler_->handle(response); };
            break;
        default: // 默认使用jsonHandler
            context.callback = [this](const HttpResponse& response) { jsonHandler_->handle(response); };
            break;
    }
    
    router_->RegisterRouter(requestId, context); // 调用HandlerRouter的RegisterRouter方法
}