// JSON响应处理器
#pragma once

#include "HandlerRouter.h"
#include "../../third_party/nlohmann/json.hpp"

class JsonHandler : public ResponseHandler {
private:
    using json = nlohmann::json;
    
public:
    void handle(const HttpResponse& response) override;
};