#include "web/http_server.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
using sock_t = SOCKET;
static const sock_t BAD_SOCK = INVALID_SOCKET;
static void closeSock(sock_t s) { closesocket(s); }
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <csignal>
using sock_t = int;
static const sock_t BAD_SOCK = -1;
static void closeSock(sock_t s) { ::close(s); }
#endif

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <thread>

#ifdef MSG_NOSIGNAL
static const int SEND_FLAGS = MSG_NOSIGNAL;
#else
static const int SEND_FLAGS = 0;
#endif

namespace fs = std::filesystem;

namespace web {

std::string Request::q(const std::string& key, const std::string& def) const {
    auto it = query.find(key);
    return it == query.end() ? def : it->second;
}

namespace {

constexpr size_t MAX_HEADER_BYTES = 64 * 1024;
constexpr size_t MAX_BODY_BYTES = 1024 * 1024;

int hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string urlDecode(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size() && hexVal(s[i + 1]) >= 0 && hexVal(s[i + 2]) >= 0) {
            out += static_cast<char>(hexVal(s[i + 1]) * 16 + hexVal(s[i + 2]));
            i += 2;
        } else if (s[i] == '+') {
            out += ' ';
        } else {
            out += s[i];
        }
    }
    return out;
}

std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : path) {
        if (c == '/') {
            if (!cur.empty()) parts.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) parts.push_back(cur);
    return parts;
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

const char* statusText(int status) {
    switch (status) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default: return "OK";
    }
}

std::string contentTypeFor(const std::string& ext) {
    std::string e = toLower(ext);
    if (e == ".html" || e == ".htm") return "text/html; charset=utf-8";
    if (e == ".css") return "text/css; charset=utf-8";
    if (e == ".js") return "text/javascript; charset=utf-8";
    if (e == ".json") return "application/json; charset=utf-8";
    if (e == ".svg") return "image/svg+xml";
    if (e == ".png") return "image/png";
    if (e == ".jpg" || e == ".jpeg") return "image/jpeg";
    if (e == ".ico") return "image/x-icon";
    if (e == ".txt") return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

bool matchRoute(const std::vector<std::string>& pattern, const std::vector<std::string>& actual,
                std::map<std::string, std::string>& params) {
    if (pattern.size() != actual.size()) return false;
    std::map<std::string, std::string> found;
    for (size_t i = 0; i < pattern.size(); ++i) {
        if (!pattern[i].empty() && pattern[i][0] == ':') {
            found[pattern[i].substr(1)] = urlDecode(actual[i]);
        } else if (pattern[i] != actual[i]) {
            return false;
        }
    }
    params = std::move(found);
    return true;
}

void sendAll(sock_t s, const std::string& data) {
    size_t off = 0;
    while (off < data.size()) {
        int n = static_cast<int>(
            send(s, data.data() + off, static_cast<int>(data.size() - off), SEND_FLAGS));
        if (n <= 0) return;
        off += static_cast<size_t>(n);
    }
}

// Đọc 1 request HTTP từ socket. Trả về false nếu lỗi / quá hạn / quá lớn.
bool readRequest(sock_t s, Request& req) {
    std::string data;
    char buf[4096];
    size_t headerEnd = std::string::npos;
    while (headerEnd == std::string::npos) {
        int n = static_cast<int>(recv(s, buf, sizeof(buf), 0));
        if (n <= 0) return false;
        data.append(buf, static_cast<size_t>(n));
        headerEnd = data.find("\r\n\r\n");
        if (headerEnd == std::string::npos && data.size() > MAX_HEADER_BYTES) return false;
    }

    std::string head = data.substr(0, headerEnd);
    std::string rest = data.substr(headerEnd + 4);

    size_t eol = head.find("\r\n");
    std::string line = head.substr(0, eol);  // npos -> lấy hết

    size_t sp1 = line.find(' ');
    if (sp1 == std::string::npos) return false;
    size_t sp2 = line.find(' ', sp1 + 1);
    req.method = line.substr(0, sp1);
    std::string target =
        line.substr(sp1 + 1, sp2 == std::string::npos ? std::string::npos : sp2 - sp1 - 1);

    size_t qm = target.find('?');
    req.path = urlDecode(target.substr(0, qm));
    if (qm != std::string::npos) {
        std::string qs = target.substr(qm + 1);
        size_t start = 0;
        while (start <= qs.size()) {
            size_t amp = qs.find('&', start);
            std::string pair = qs.substr(start, amp == std::string::npos ? std::string::npos : amp - start);
            if (!pair.empty()) {
                size_t eq = pair.find('=');
                std::string key = urlDecode(pair.substr(0, eq));
                std::string val = eq == std::string::npos ? "" : urlDecode(pair.substr(eq + 1));
                req.query[key] = val;
            }
            if (amp == std::string::npos) break;
            start = amp + 1;
        }
    }

    size_t contentLength = 0;
    size_t pos = eol;
    while (pos != std::string::npos && pos < head.size()) {
        size_t next = head.find("\r\n", pos + 2);
        std::string h = head.substr(pos + 2, next == std::string::npos ? std::string::npos : next - pos - 2);
        if (toLower(h).rfind("content-length:", 0) == 0) {
            contentLength = static_cast<size_t>(std::strtoull(h.c_str() + 15, nullptr, 10));
        }
        pos = next;
    }
    if (contentLength > MAX_BODY_BYTES) return false;

    while (rest.size() < contentLength) {
        int n = static_cast<int>(recv(s, buf, sizeof(buf), 0));
        if (n <= 0) return false;
        rest.append(buf, static_cast<size_t>(n));
    }
    req.body = rest.substr(0, contentLength);
    return true;
}

}  // namespace

void HttpServer::route(const std::string& method, const std::string& pattern, Handler handler) {
    routes_.push_back({method, splitPath(pattern), std::move(handler)});
}

Response HttpServer::serveFile(const std::string& urlPath) const {
    Response res;
    res.status = 404;
    res.contentType = "text/plain; charset=utf-8";
    res.body = "Not found";

    // Chặn đường dẫn thoát ra ngoài thư mục web/.
    if (urlPath.find("..") != std::string::npos || urlPath.find(':') != std::string::npos ||
        urlPath.find('\\') != std::string::npos) {
        return res;
    }
    std::string p = (urlPath.empty() || urlPath == "/") ? "/index.html" : urlPath;
    fs::path full = fs::path(staticDir_) / p.substr(1);
    std::error_code ec;
    if (fs::is_directory(full, ec)) full /= "index.html";

    std::ifstream in(full, std::ios::binary);
    if (!in) return res;
    res.status = 200;
    res.contentType = contentTypeFor(full.extension().string());
    res.body.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return res;
}

Response HttpServer::dispatch(Request& req) const {
    std::vector<std::string> actual = splitPath(req.path);
    for (const auto& r : routes_) {
        if (r.method != req.method) continue;
        if (!matchRoute(r.segments, actual, req.params)) continue;
        Response res;
        try {
            r.handler(req, res);
        } catch (const std::exception&) {
            res.status = 500;
            res.body = R"({"ok":false,"message":"Loi may chu."})";
        }
        return res;
    }
    bool isApi = req.path.rfind("/api/", 0) == 0;
    if (req.method == "GET" && !isApi && !staticDir_.empty()) return serveFile(req.path);

    Response res;
    res.status = 404;
    res.body = R"({"ok":false,"message":"Khong tim thay."})";
    return res;
}

void HttpServer::handleClient(unsigned long long rawSock) const {
    sock_t s = static_cast<sock_t>(rawSock);
    try {
        // Quá 5 giây không gửi gì thì bỏ kết nối (trình duyệt hay mở sẵn kết nối rỗng).
#ifdef _WIN32
        DWORD timeoutMs = 5000;
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
#else
        timeval tv{};
        tv.tv_sec = 5;
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
        Request req;
        if (readRequest(s, req)) {
            Response res = dispatch(req);
            std::string out = "HTTP/1.1 " + std::to_string(res.status) + " " + statusText(res.status) +
                              "\r\nContent-Type: " + res.contentType +
                              "\r\nContent-Length: " + std::to_string(res.body.size()) +
                              "\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n" + res.body;
            sendAll(s, out);
        }
    } catch (const std::exception&) {
    }
    closeSock(s);
}

bool HttpServer::start(int port) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
#else
    std::signal(SIGPIPE, SIG_IGN);
#endif
    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == BAD_SOCK) return false;
#ifndef _WIN32
    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<unsigned short>(port));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // chỉ máy này truy cập được
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 || listen(s, 32) != 0) {
        closeSock(s);
        return false;
    }
    listenSock_ = static_cast<unsigned long long>(s);
    listening_ = true;
    return true;
}

void HttpServer::run() {
    if (!listening_) return;
    sock_t ls = static_cast<sock_t>(listenSock_);
    while (true) {
        sock_t c = accept(ls, nullptr, nullptr);
        if (c == BAD_SOCK) continue;
        unsigned long long raw = static_cast<unsigned long long>(c);
        std::thread([this, raw] { handleClient(raw); }).detach();
    }
}

}  // namespace web
