#pragma once
#include "HttpResponse.h"
#include "HttpRequest.h"
#include "UserManager.h"

class RegisterController {
private:
    UserManager* userManager_;
    std::string staticFilesPath_;

public:
    RegisterController(UserManager* userManager, const std::string& staticFilesPath);
    ~RegisterController();

    bool createDirectory(const std::string& dirPath);
    HttpResponse handleRegister(const HttpRequest& request);
};
