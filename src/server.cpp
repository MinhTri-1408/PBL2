// Chuong trinh quan ly dat san - giao dien web.
// Chay: app_web [cong] [thu_muc_data]   (mac dinh: cong 8080, data cua du an)
// Sau do mo trinh duyet: http://127.0.0.1:8080

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

#include "core/manager.hpp"
#include "core/paths.hpp"
#include "core/storage.hpp"
#include "core/utils.hpp"
#include "web/http_server.hpp"

using json = nlohmann::json;

// ======================= Tien ich JSON =======================

// Cac request chay tren nhieu luong, con Manager thi khong an toan da luong
// -> moi API deu khoa mutex nay.
static std::mutex gMutex;

static void reply(web::Response& res, int status, const json& j) {
    res.status = status;
    // replace: ky tu UTF-8 loi trong ten khach khong lam server bi loi.
    res.body = j.dump(-1, ' ', false, json::error_handler_t::replace);
}

static void fail(web::Response& res, int status, const std::string& message) {
    reply(res, status, {{"ok", false}, {"message", message}});
}

static void replyResult(web::Response& res, const Result& r, json extra = json::object()) {
    extra["ok"] = r.ok;
    extra["message"] = r.message;
    extra["id"] = r.id;
    reply(res, r.ok ? 200 : 400, extra);
}

static web::Handler guarded(web::Handler f) {
    return [f](const web::Request& req, web::Response& res) {
        std::lock_guard<std::mutex> lock(gMutex);
        try {
            f(req, res);
        } catch (const json::exception&) {
            fail(res, 400, "Du lieu gui len khong hop le.");
        } catch (const std::exception&) {
            fail(res, 500, "Loi may chu.");
        }
    };
}

static bool readBody(const web::Request& req, web::Response& res, json& body) {
    body = json::parse(req.body, nullptr, false);
    if (!body.is_object()) {
        fail(res, 400, "Du lieu gui len khong hop le.");
        return false;
    }
    return true;
}

static bool readDateParam(const web::Request& req, web::Response& res, std::string& date) {
    date = req.q("date");
    if (date.empty()) date = utils::today();
    if (!utils::isValidDate(date)) {
        fail(res, 400, "Ngay khong hop le (dinh dang YYYY-MM-DD).");
        return false;
    }
    return true;
}

static json bookingJson(const Manager& m, const Booking& b) {
    const Court* c = m.findCourt(b.courtId());
    return {{"id", b.id()},
            {"customerName", b.customerName()},
            {"phone", b.phone()},
            {"courtId", b.courtId()},
            {"courtName", c ? c->name() : std::string()},
            {"courtType", c ? c->type() : "standard"},
            {"pricePerHour", c ? c->pricePerHour() : 0LL},
            {"date", b.date()},
            {"start", utils::formatTime(b.startMin())},
            {"end", utils::formatTime(b.endMin())},
            {"startMin", b.startMin()},
            {"endMin", b.endMin()},
            {"durationMinutes", b.durationMinutes()},
            {"amount", m.calcAmount(b)},
            {"paid", b.paid()}};
}

// Mot san + lich dat + khung gio trong cua san do trong 1 ngay.
static json courtDayJson(const Manager& m, const Court& c, const std::string& date) {
    json bookings = json::array();
    auto list = m.bookingsOfCourt(c.id(), date);
    for (const auto& b : list) bookings.push_back(bookingJson(m, b));

    json slots = json::array();
    auto free = m.freeSlots(c.id(), date);
    for (const auto& s : free) {
        slots.push_back({{"startMin", s.startMin},
                         {"endMin", s.endMin},
                         {"start", utils::formatTime(s.startMin)},
                         {"end", utils::formatTime(s.endMin)}});
    }
    std::string status = list.empty() ? "free" : (free.empty() ? "full" : "partial");
    return {{"id", c.id()},
            {"name", c.name()},
            {"type", c.type()},
            {"pricePerHour", c.pricePerHour()},
            {"status", status},
            {"bookingCount", list.size()},
            {"bookings", bookings},
            {"freeSlots", slots}};
}

// ======================= main =======================

int main(int argc, char* argv[]) {
    int port = argc > 1 ? std::atoi(argv[1]) : 8080;
    if (port <= 0 || port > 65535) port = 8080;
    std::string dataDir = argc > 2 ? argv[2] : paths::dataDir();
    std::string webDir = paths::webDir();

    if (!std::filesystem::exists(std::filesystem::path(webDir) / "index.html")) {
        std::cerr << "Loi: khong tim thay web/index.html.\n"
                  << "Hay chay chuong trinh trong thu muc du an (noi co CMakeLists.txt).\n";
        return 1;
    }

    Storage storage(dataDir);
    Manager m(storage);
    web::HttpServer server;

    // ---- Bang lich: tat ca san + lich dat trong 1 ngay ----
    server.route("GET", "/api/board", guarded([&](const web::Request& req, web::Response& res) {
        std::string date;
        if (!readDateParam(req, res, date)) return;
        json courts = json::array();
        for (const auto& c : m.courts()) courts.push_back(courtDayJson(m, c, date));
        reply(res, 200,
              {{"ok", true},
               {"date", date},
               {"open", utils::OPEN_MIN},
               {"close", utils::CLOSE_MIN},
               {"courts", courts}});
    }));

    // ---- Quan ly san ----
    server.route("POST", "/api/courts", guarded([&](const web::Request& req, web::Response& res) {
        json body;
        if (!readBody(req, res, body)) return;
        std::string name = utils::trim(body.at("name").get<std::string>());
        long long price = body.at("pricePerHour").get<long long>();
        std::string type = body.value("type", "standard");
        replyResult(res, m.addCourt(name, price, type));
    }));

    server.route("PUT", "/api/courts/:id", guarded([&](const web::Request& req, web::Response& res) {
        json body;
        if (!readBody(req, res, body)) return;
        int id = std::atoi(req.params.at("id").c_str());
        std::string name = utils::trim(body.at("name").get<std::string>());
        long long price = body.at("pricePerHour").get<long long>();
        std::string type = body.value("type", "");
        replyResult(res, m.updateCourt(id, name, price, type));
    }));

    server.route("DELETE", "/api/courts/:id", guarded([&](const web::Request& req, web::Response& res) {
        int id = std::atoi(req.params.at("id").c_str());
        replyResult(res, m.removeCourt(id));
    }));

    // ---- Trang thai 1 san trong ngay ----
    server.route("GET", "/api/courts/:id/schedule",
                 guarded([&](const web::Request& req, web::Response& res) {
        int id = std::atoi(req.params.at("id").c_str());
        std::string date;
        if (!readDateParam(req, res, date)) return;
        const Court* c = m.findCourt(id);
        if (!c) return fail(res, 404, "Khong tim thay san.");
        json j = courtDayJson(m, *c, date);
        j["ok"] = true;
        j["date"] = date;
        j["open"] = utils::OPEN_MIN;
        j["close"] = utils::CLOSE_MIN;
        reply(res, 200, j);
    }));

    // ---- Dat san / huy / tim theo so dien thoai ----
    server.route("POST", "/api/bookings", guarded([&](const web::Request& req, web::Response& res) {
        json body;
        if (!readBody(req, res, body)) return;
        std::string name = utils::trim(body.at("customerName").get<std::string>());
        std::string phone = utils::trim(body.at("phone").get<std::string>());
        int courtId = body.at("courtId").get<int>();
        std::string date = body.at("date").get<std::string>();
        int startMin = 0, endMin = 0;
        if (!utils::parseTime(body.at("start").get<std::string>(), startMin) ||
            !utils::parseTime(body.at("end").get<std::string>(), endMin)) {
            return fail(res, 400, "Gio khong hop le (dinh dang HH:MM).");
        }
        Result r = m.book(name, phone, courtId, date, startMin, endMin);
        json extra = json::object();
        if (r.ok) extra["booking"] = bookingJson(m, *m.findBooking(r.id));
        replyResult(res, r, extra);
    }));

    server.route("GET", "/api/bookings", guarded([&](const web::Request& req, web::Response& res) {
        std::string phone = utils::trim(req.q("phone"));
        if (!utils::isValidPhone(phone)) {
            return fail(res, 400, "So dien thoai phai gom 9-11 chu so.");
        }
        json list = json::array();
        for (const auto& b : m.bookingsByPhone(phone)) list.push_back(bookingJson(m, b));
        reply(res, 200, {{"ok", true}, {"bookings", list}});
    }));

    server.route("DELETE", "/api/bookings/:id",
                 guarded([&](const web::Request& req, web::Response& res) {
        int id = std::atoi(req.params.at("id").c_str());
        replyResult(res, m.cancel(id));
    }));

    // ---- Thanh toan ----
    server.route("POST", "/api/bookings/:id/pay",
                 guarded([&](const web::Request& req, web::Response& res) {
        int id = std::atoi(req.params.at("id").c_str());
        Result r = m.pay(id);
        json extra = json::object();
        if (r.ok) extra["booking"] = bookingJson(m, *m.findBooking(id));
        replyResult(res, r, extra);
    }));

    // ---- Thong ke doanh thu theo tuan, thang, nam ----
    server.route("GET", "/api/stats", guarded([&](const web::Request& req, web::Response& res) {
        std::string period = req.q("period", "week");
        std::string date = req.q("date", utils::today());
        if (!utils::isValidDate(date)) date = utils::today();

        StatsReport r = m.getStats(period, date);
        json breakdown = json::array();
        for (const auto& item : r.breakdown) {
            breakdown.push_back({
                {"key", item.key},
                {"label", item.label},
                {"totalRevenue", item.totalRevenue},
                {"paidRevenue", item.paidRevenue},
                {"unpaidRevenue", item.unpaidRevenue},
                {"bookingCount", item.bookingCount},
                {"durationMinutes", item.durationMinutes},
                {"vipRevenue", item.vipRevenue},
                {"standardRevenue", item.standardRevenue}
            });
        }

        reply(res, 200, {
            {"ok", true},
            {"period", r.period},
            {"label", r.label},
            {"startDate", r.startDate},
            {"endDate", r.endDate},
            {"totalRevenue", r.totalRevenue},
            {"paidRevenue", r.paidRevenue},
            {"unpaidRevenue", r.unpaidRevenue},
            {"totalBookings", r.totalBookings},
            {"totalMinutes", r.totalMinutes},
            {"vip", {
                {"revenue", r.vipRevenue},
                {"bookings", r.vipBookings},
                {"minutes", r.vipMinutes}
            }},
            {"standard", {
                {"revenue", r.standardRevenue},
                {"bookings", r.standardBookings},
                {"minutes", r.standardMinutes}
            }},
            {"breakdown", breakdown}
        });
    }));

    server.serveStatic(webDir);

    if (!server.start(port)) {
        std::cerr << "Loi: khong mo duoc cong " << port << " (co the dang bi chuong trinh khac dung).\n"
                  << "Thu cong khac: app_web 8081\n";
        return 1;
    }

    std::string url = "http://127.0.0.1:" + std::to_string(port);
    std::cout << "Quan ly dat san - giao dien web\n"
              << "Mo trinh duyet tai: " << url << "\n"
              << "Du lieu: " << dataDir << "\n"
              << "Nhan Ctrl+C de thoat.\n";

    // Tu mo trinh duyet tren Windows va macOS.
#if defined(_WIN32)
    std::system(("start \"\" " + url).c_str());
#elif defined(__APPLE__)
    std::system(("open " + url).c_str());
#endif

    server.run();
    return 0;
}
