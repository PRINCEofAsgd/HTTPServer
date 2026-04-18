# Linux 系统下基于 Reactor 模型的高并发服务器
## 1、并发性能压力测试
- 配置：四核虚拟机
### 1.1 少量连接并发请求：
- 50 并发连接 50 * 20000 请求 150k QPS；
### 1.2 简单并发回显业务：
- 50 并发连接 50 * 20000 回显业务 30k/s；
### 1.3 大量连接并发回显：
- 1000 并发连接 1000 * 1000 回显业务 20k/s，成功率 99.7%。


## 2、项目结构
### 2.1 底层核心模块 - core 目录
`core/`
├─ TcpServer: 服务器核心类，管理线程池、事件循环和连接生命周期
│
├─ ThreadPool: 线程池类，用于并发处理异步事件
│
├─ EventLoop: 事件循环类，负责循环监听和分发事件
│   ├─ Epoll: 封装 epoll 功能，负责事件监听
│   └─ Channel: 事件分发器，负责事件的注册和处理
│
├─ Acceptor: Channel 的上层类，连接接收器，负责接受新的连接
│   └─ Socket: 套接字类，封装网络连接
│   └─ InetAddress: 网络地址类，封装 IP 地址和端口号
│
└─ Connection: Channel 的上层类，连接管理类，负责处理与客户端的通信
    ├─ Buffer: 缓冲区类，处理数据读写
    ├─ Timestamp: 时间戳类，记录连接活动时间
    └─ SendTask: 发送任务类，管理发送任务队列


### 2.2 HTTP 协议模块 - http 目录
`http/`
├─ HttpServer: HTTP 服务器核心类，管理 HTTP 请求和响应
│   │
│   ├─ HttpMessage: HTTP 消息基类，定义 HTTP 消息的通用结构
│   │   ├─ HttpRequest: HTTP 请求类，解析和存储 HTTP 请求报文
│   │   └─ HttpResponse: HTTP 响应类，生成和存储 HTTP 响应报文
│   │
│   └─ HttpContext: HTTP 上下文类，管理 HTTP 连接的状态和数据
│
├─ HttpRouter: HTTP 路由器类，负责请求路径的路由和分发
│
├─ 业务处理类: 
│   ├─ HeartbeatController: 心跳控制器，处理心跳请求(由于过于简单，当场构造心跳包)
│   ├─ StaticFileController: 静态文件控制器，处理静态资源请求
│   ├─ RegisterController: 注册控制器，处理用户注册请求
│   ├─ LoginController: 登录控制器，处理用户登录请求
│   ├─ DownloadController: 下载控制器，处理文件下载请求
│   └─ UploadController: 上传控制器，处理文件上传请求
│
└─ 中间件和管理类: 
    ├─ AuthorMiddleWare: 认证中间件，处理用户认证
    ├─ TokenManager: 令牌管理器，管理用户令牌，主要使用 Redis
    └─ UserManager: 用户管理器，管理用户信息，主要使用 MySQL


### 2.3 客户端 HTTP 自动序列化模块 - client 目录
`client/`
├─ HandlerFactory: 处理器工厂类，创建不同类型的处理器，同时也管理组件生命周期，层次类似于服务端 HttpServer 和 TcpServer 的简化版
│
├─ HandlerRouter: 处理器路由器类，根据请求类型路由到相应的处理器
│
├─ ClientLoop: 客户端事件循环类，负责处理客户端事件
│
├─ ClientConn: 客户端连接类，管理与服务器的连接和通信
│
├─ 业务处理类: 
│   ├─ HeartBeatHandler: 心跳处理器，处理心跳请求
│   ├─ FileHandler: 文件处理器，处理文件上传下载
│   └─ JsonHandler: JSON 处理器，处理 JSON 数据
│
└─ Utils: 工具类，提供通用工具函数，如 HTTP 序列化、JSON 序列化等


### 2.4 压测脚本 - src_concurrent 目录
#### 2.4.1 `client_input.cpp`
- 功能: 向服务器发送逐个消息
- 说明：用于测试服务器的实时通信能力，服务器会实时回显客户端发送的消息。

#### 2.4.2 `client_test.cpp`
- 功能: 阻塞地接收并行地与服务器交互测试消息
- 说明：用于测试服务器的并发能力，客户端能精确稳定地发送和接收与服务器的交互消息，是实际业务场景中客户端的常用模式。

#### 2.4.3 `client_concurrent.cpp`
- 功能: 非阻塞地循环向服务器发送并发消息
- 说明：用于测试服务器的并发能力，客户端发送海量消息，将非阻塞地接收服务器的回显，能够测试服务器的最强并发处理能力，但实际业务不常用，需要给客户端添加防御性逻辑。


### 2.5 其他
- redis/RedisClient: Redis 客户端类，负责与 Redis 数据库交互
- mysql/MySQLClient: MySQL 客户端类，负责与 MySQL 数据库交互
- /static: 静态资源目录，包含用于测试的 HTML 等资源
- /third_party: 第三方库目录，包含项目依赖的第三方库 nlohmann/json



## 3、项目运行
### 3.1 编译项目
#### 3.1.1 编译回显服务器、Web 服务器、Web 客户端
- ./build
- cmake ../ && make
#### 3.1.2 编译回显客户端、压测脚本
- ./src_concurrent
- make

### 3.2 回显服务器
#### 3.2.1 服务端运行 - `echoserver`
- ./bin/echoserver `ip` `port`
- 输入服务器的 IP 地址和占用端口号
- 服务器会监听指定 IP 地址和端口号，等待客户端连接，当客户端连接成功后，服务器会回显客户端发送的消息

#### 3.2.2.1 客户端运行 - `client_input`
- ./bin/client_input `ip` `port`
- 输入服务器的 IP 地址和占用端口号
- 客户端会向服务器发送逐个消息，服务器会实时回显客户端发送的消息

#### 3.2.2.2 客户端运行 - `client_test`
- ./bin/client_test `ip` `port`
- 输入服务器的 IP 地址和占用端口号
- 客户端会向服务器阻塞地发送海量消息
- sh client_test.sh `conc` `n`
- 输入并发连接数和发送消息个数
- 启动 conc 个客户端，同时向服务器阻塞地发送 n 个并发消息，服务器会精确稳定地逐个接收并响应消息

#### 3.2.2.3 客户端运行 - `client_concurrent`
- ./bin/client_concurrent `ip` `port`
- 输入服务器的 IP 地址和占用端口号
- 客户端会向服务器非阻塞地发送海量并发消息
- sh client_concurrent.sh `conc` `n`
- 输入并发连接数和发送消息个数
- 启动 conc 个客户端，同时向服务器非阻塞地发送 n 个并发消息，服务器会极为高效地处理并响应消息


### 3.3 Web 服务器
#### 3.3.1 服务端运行 - `httpserver`
- ./bin/httpserver
- 为简便，服务端 IP 端口已在程序中绑定，可修改
- 服务器会监听指定 IP 地址和端口号，等待客户端连接，当客户端连接成功后，服务器会根据请求路径路由到相应的处理器处理请求。

#### 3.3.2 客户端运行 - `client`
- ./bin/client
- 为简便，客户端 IP 端口已在程序中绑定，可修改
- 客户端已自动序列化 HTTP 请求，按照循环的输出提示即可通过输入自动构造请求，执行业务


### 3.4 自定义参数设置

