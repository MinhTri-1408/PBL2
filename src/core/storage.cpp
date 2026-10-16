#include "core/storage.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

Storage::Storage(std::string dataDir) : dataDir_(std::move(dataDir)) {
    std::filesystem::create_directories(dataDir_);
}

std::string Storage::courtsPath() const {
    return (std::filesystem::path(dataDir_) / "courts.json").string();
}

std::string Storage::bookingsPath() const {
    return (std::filesystem::path(dataDir_) / "bookings.json").string();
}

// File chưa có hoặc hỏng -> trả về mảng rỗng (không làm sập chương trình).
static json readArray(const std::string& path) {
    std::ifstream in(path);
    if (!in) return json::array();
    try {
        json j = json::parse(in);
        return j.is_array() ? j : json::array();
    } catch (const std::exception& e) {
        std::cerr << "Canh bao: khong doc duoc " << path << " (" << e.what() << ")\n";
        return json::array();
    }
}

static void writeJson(const std::string& path, const json& j) {
    std::ofstream out(path);
    if (!out) {
        std::cerr << "Loi: khong ghi duoc file " << path << "\n";
        return;
    }
    // error_handler_t::replace: ten khach nhap sai ma hoa van khong lam chuong trinh bi loi.
    out << j.dump(2, ' ', false, json::error_handler_t::replace) << "\n";
}

std::vector<Court> Storage::loadCourts() const {
    std::vector<Court> result;
    for (const auto& item : readArray(courtsPath())) {
        try {
            std::string name = item.at("name").get<std::string>();
            std::string type = item.value("type", "");
            if (type.empty()) {
                std::string lower = name;
                for (char& ch : lower) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                type = (lower.find("vip") != std::string::npos) ? "vip" : "standard";
            }
            result.emplace_back(item.at("id").get<int>(),
                                std::move(name),
                                item.at("pricePerHour").get<long long>(),
                                std::move(type));
        } catch (const std::exception&) {
            // bỏ qua bản ghi sai định dạng
        }
    }
    return result;
}

std::vector<Booking> Storage::loadBookings() const {
    std::vector<Booking> result;
    for (const auto& item : readArray(bookingsPath())) {
        try {
            result.emplace_back(item.at("id").get<int>(),
                                item.at("customerName").get<std::string>(),
                                item.at("phone").get<std::string>(),
                                item.at("courtId").get<int>(),
                                item.at("date").get<std::string>(),
                                item.at("startMin").get<int>(),
                                item.at("endMin").get<int>(),
                                item.value("paid", false));
        } catch (const std::exception&) {
        }
    }
    return result;
}

void Storage::saveCourts(const std::vector<Court>& courts) const {
    json arr = json::array();
    for (const auto& c : courts) {
        arr.push_back({{"id", c.id()},
                       {"name", c.name()},
                       {"pricePerHour", c.pricePerHour()},
                       {"type", c.type()}});
    }
    writeJson(courtsPath(), arr);
}

void Storage::saveBookings(const std::vector<Booking>& bookings) const {
    json arr = json::array();
    for (const auto& b : bookings) {
        arr.push_back({{"id", b.id()},
                       {"customerName", b.customerName()},
                       {"phone", b.phone()},
                       {"courtId", b.courtId()},
                       {"date", b.date()},
                       {"startMin", b.startMin()},
                       {"endMin", b.endMin()},
                       {"paid", b.paid()}});
    }
    writeJson(bookingsPath(), arr);
}
