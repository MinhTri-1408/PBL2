#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

// Máy chủ HTTP nhỏ (chỉ nghe ở 127.0.0.1), không cần thư viện ngoài.
// Đủ dùng cho API JSON + phục vụ file tĩnh của giao diện web.
namespace web {

struct Request {
    std::string method;                          // GET, POST, PUT, DELETE
    std::string path;                            // /api/courts/3
    std::string body;                            // nội dung gửi lên (JSON)
    std::map<std::string, std::string> query;    // ?date=2026-10-15
    std::map<std::string, std::string> params;   // :id trong đường dẫn

    std::string q(const std::string& key, const std::string& def = "") const;
};

struct Response {
    int status = 200;
    std::string body;
    std::string contentType = "application/json; charset=utf-8";
};

using Handler = std::function<void(const Request&, Response&)>;

class HttpServer {
public:
    // pattern dạng "/api/courts/:id"
    void route(const std::string& method, const std::string& pattern, Handler handler);

    // Thư mục chứa file giao diện (index.html, ...).
    void serveStatic(const std::string& dir) { staticDir_ = dir; }

    // Mở cổng (chỉ localhost). Trả về false nếu cổng đang bị chiếm.
    bool start(int port);

    // Vòng lặp nhận kết nối, chạy mãi mãi.
    void run();

private:
    struct Route {
        std::string method;
        std::vector<std::string> segments;
        Handler handler;
    };

    Response dispatch(Request& req) const;
    Response serveFile(const std::string& urlPath) const;
    void handleClient(unsigned long long sock) const;

    std::vector<Route> routes_;
    std::string staticDir_;
    unsigned long long listenSock_ = 0;
    bool listening_ = false;
};

}  // namespace web
