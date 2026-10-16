#include "core/manager.hpp"

#include <algorithm>

#include "core/utils.hpp"

Manager::Manager(Storage& storage)
    : storage_(storage), courts_(storage.loadCourts()), bookings_(storage.loadBookings()) {}

// ======================= Quản lý sân =======================

const Court* Manager::findCourt(int id) const {
    auto it = std::find_if(courts_.begin(), courts_.end(),
                           [id](const Court& c) { return c.id() == id; });
    return it == courts_.end() ? nullptr : &*it;
}

int Manager::nextCourtId() const {
    int maxId = 0;
    for (const auto& c : courts_) maxId = std::max(maxId, c.id());
    return maxId + 1;
}

int Manager::nextBookingId() const {
    int maxId = 0;
    for (const auto& b : bookings_) maxId = std::max(maxId, b.id());
    return maxId + 1;
}

Result Manager::addCourt(const std::string& name, long long pricePerHour, const std::string& type) {
    if (name.empty()) return {false, "Ten san khong duoc de trong.", 0};
    if (pricePerHour <= 0) return {false, "Gia san phai lon hon 0.", 0};
    int id = nextCourtId();
    courts_.emplace_back(id, name, pricePerHour, type == "vip" ? "vip" : "standard");
    saveCourts();
    return {true, "Da them san.", id};
}

Result Manager::updateCourt(int id, const std::string& name, long long pricePerHour, const std::string& type) {
    auto it = std::find_if(courts_.begin(), courts_.end(),
                           [id](const Court& c) { return c.id() == id; });
    if (it == courts_.end()) return {false, "Khong tim thay san.", 0};
    if (name.empty()) return {false, "Ten san khong duoc de trong.", 0};
    if (pricePerHour <= 0) return {false, "Gia san phai lon hon 0.", 0};
    it->setName(name);
    it->setPricePerHour(pricePerHour);
    if (!type.empty()) {
        it->setType(type);
    }
    saveCourts();
    return {true, "Da cap nhat san.", id};
}

Result Manager::removeCourt(int id) {
    auto it = std::find_if(courts_.begin(), courts_.end(),
                           [id](const Court& c) { return c.id() == id; });
    if (it == courts_.end()) return {false, "Khong tim thay san.", 0};

    // Không cho xóa sân còn lượt đặt chưa thanh toán.
    bool hasActive = std::any_of(bookings_.begin(), bookings_.end(), [id](const Booking& b) {
        return b.courtId() == id && !b.paid();
    });
    if (hasActive) {
        return {false, "San con luot dat chua thanh toan, hay huy hoac thanh toan truoc.", id};
    }
    courts_.erase(it);
    saveCourts();
    return {true, "Da xoa san.", id};
}

// ======================= Đặt sân =======================

const Booking* Manager::findBooking(int id) const {
    auto it = std::find_if(bookings_.begin(), bookings_.end(),
                           [id](const Booking& b) { return b.id() == id; });
    return it == bookings_.end() ? nullptr : &*it;
}

bool Manager::isFree(int courtId, const std::string& date, int startMin, int endMin) const {
    return std::none_of(bookings_.begin(), bookings_.end(), [&](const Booking& b) {
        return b.courtId() == courtId && b.date() == date && b.overlaps(startMin, endMin);
    });
}

Result Manager::book(const std::string& customerName, const std::string& phone, int courtId,
                     const std::string& date, int startMin, int endMin) {
    if (customerName.empty()) return {false, "Ten khach khong duoc de trong.", 0};
    if (!utils::isValidPhone(phone)) return {false, "So dien thoai phai gom 9-11 chu so.", 0};
    if (!findCourt(courtId)) return {false, "Khong tim thay san.", 0};
    if (!utils::isValidDate(date)) return {false, "Ngay khong hop le (dinh dang YYYY-MM-DD).", 0};
    if (endMin <= startMin) return {false, "Gio ket thuc phai sau gio bat dau.", 0};
    if (startMin < utils::OPEN_MIN || endMin > utils::CLOSE_MIN) {
        return {false,
                "San chi mo cua tu " + utils::formatTime(utils::OPEN_MIN) + " den " +
                    utils::formatTime(utils::CLOSE_MIN) + ".",
                0};
    }
    if (!isFree(courtId, date, startMin, endMin)) {
        return {false, "Khung gio nay da co nguoi dat.", 0};
    }
    int id = nextBookingId();
    bookings_.emplace_back(id, customerName, phone, courtId, date, startMin, endMin);
    saveBookings();
    return {true, "Dat san thanh cong.", id};
}

Result Manager::cancel(int bookingId) {
    auto it = std::find_if(bookings_.begin(), bookings_.end(),
                           [bookingId](const Booking& b) { return b.id() == bookingId; });
    if (it == bookings_.end()) return {false, "Khong tim thay luot dat.", 0};
    if (it->paid()) return {false, "Luot dat da thanh toan, khong the huy.", bookingId};
    bookings_.erase(it);
    saveBookings();
    return {true, "Da huy luot dat.", bookingId};
}

std::vector<Booking> Manager::bookingsOfCourt(int courtId, const std::string& date) const {
    std::vector<Booking> result;
    for (const auto& b : bookings_) {
        if (b.courtId() == courtId && b.date() == date) result.push_back(b);
    }
    std::sort(result.begin(), result.end(),
              [](const Booking& a, const Booking& b) { return a.startMin() < b.startMin(); });
    return result;
}

std::vector<Booking> Manager::bookingsByPhone(const std::string& phone) const {
    std::vector<Booking> result;
    for (const auto& b : bookings_) {
        if (b.phone() == phone && !b.paid()) result.push_back(b);
    }
    std::sort(result.begin(), result.end(), [](const Booking& a, const Booking& b) {
        return a.date() != b.date() ? a.date() < b.date() : a.startMin() < b.startMin();
    });
    return result;
}

std::vector<TimeSlot> Manager::freeSlots(int courtId, const std::string& date) const {
    std::vector<TimeSlot> slots;
    int cursor = utils::OPEN_MIN;
    for (const auto& b : bookingsOfCourt(courtId, date)) {
        if (b.startMin() > cursor) slots.push_back({cursor, b.startMin()});
        cursor = std::max(cursor, b.endMin());
    }
    if (cursor < utils::CLOSE_MIN) slots.push_back({cursor, utils::CLOSE_MIN});
    return slots;
}

// ======================= Thanh toán =======================

long long Manager::calcAmount(const Booking& b) const {
    const Court* court = findCourt(b.courtId());
    if (!court) return 0;
    // (end - start) tính theo phút nên chia 60 để ra số giờ.
    return court->pricePerHour() * b.durationMinutes() / 60;
}

Result Manager::pay(int bookingId) {
    auto it = std::find_if(bookings_.begin(), bookings_.end(),
                           [bookingId](const Booking& b) { return b.id() == bookingId; });
    if (it == bookings_.end()) return {false, "Khong tim thay luot dat.", 0};
    if (it->paid()) return {false, "Luot dat nay da thanh toan roi.", bookingId};
    it->setPaid(true);
    saveBookings();
    return {true, "Thanh toan thanh cong.", bookingId};
}

// ======================= Thống kê =======================

StatsReport Manager::getStats(const std::string& period, const std::string& refDate) const {
    StatsReport report;
    std::string date = refDate.empty() ? utils::today() : refDate;
    if (!utils::isValidDate(date)) date = utils::today();

    std::string normPeriod = (period == "month" || period == "year") ? period : "week";
    report.period = normPeriod;

    int y, m, d;
    utils::parseDateParts(date, y, m, d);

    if (normPeriod == "week") {
        std::string mon, sun;
        utils::getWeekRange(date, mon, sun);
        report.startDate = mon;
        report.endDate = sun;

        int my, mm, md, sy, sm, sd;
        utils::parseDateParts(mon, my, mm, md);
        utils::parseDateParts(sun, sy, sm, sd);
        char lbl[64];
        std::snprintf(lbl, sizeof(lbl), "Tuần (%02d/%02d/%04d - %02d/%02d/%04d)", md, mm, my, sd, sm, sy);
        report.label = lbl;

        // 7 ngày từ Thứ 2 đến Chủ nhật
        for (int i = 0; i < 7; ++i) {
            std::string curDate = utils::addDays(mon, i);
            int dow = utils::dayOfWeek(curDate);
            int cy, cm, cd;
            utils::parseDateParts(curDate, cy, cm, cd);
            char itemLbl[32];
            std::snprintf(itemLbl, sizeof(itemLbl), "%s (%02d/%02d)",
                          utils::dayOfWeekShort(dow).c_str(), cd, cm);
            report.breakdown.push_back({curDate, itemLbl, 0, 0, 0, 0, 0, 0, 0});
        }
    } else if (normPeriod == "month") {
        utils::getMonthRange(date, report.startDate, report.endDate);
        char lbl[32];
        std::snprintf(lbl, sizeof(lbl), "Tháng %02d/%04d", m, y);
        report.label = lbl;

        int totalDays = utils::daysInMonth(y, m);
        for (int day = 1; day <= totalDays; ++day) {
            std::string curDate = utils::formatDate(y, m, day);
            char itemLbl[16];
            std::snprintf(itemLbl, sizeof(itemLbl), "%02d/%02d", day, m);
            report.breakdown.push_back({curDate, itemLbl, 0, 0, 0, 0, 0, 0, 0});
        }
    } else {  // "year"
        utils::getYearRange(date, report.startDate, report.endDate);
        char lbl[32];
        std::snprintf(lbl, sizeof(lbl), "Năm %04d", y);
        report.label = lbl;

        for (int month = 1; month <= 12; ++month) {
            char key[16], itemLbl[32];
            std::snprintf(key, sizeof(key), "%04d-%02d", y, month);
            std::snprintf(itemLbl, sizeof(itemLbl), "Tháng %02d", month);
            report.breakdown.push_back({key, itemLbl, 0, 0, 0, 0, 0, 0, 0});
        }
    }

    // Tổng hợp dữ liệu từ các lượt đặt
    for (const auto& b : bookings_) {
        if (b.date() < report.startDate || b.date() > report.endDate) continue;

        long long amount = calcAmount(b);
        const Court* court = findCourt(b.courtId());
        bool isVip = court && court->isVip();
        int mins = b.durationMinutes();

        report.totalRevenue += amount;
        report.totalBookings += 1;
        report.totalMinutes += mins;

        if (b.paid()) {
            report.paidRevenue += amount;
        } else {
            report.unpaidRevenue += amount;
        }

        if (isVip) {
            report.vipRevenue += amount;
            report.vipBookings += 1;
            report.vipMinutes += mins;
        } else {
            report.standardRevenue += amount;
            report.standardBookings += 1;
            report.standardMinutes += mins;
        }

        // Cập nhật breakdown
        for (auto& item : report.breakdown) {
            bool matches = false;
            if (normPeriod == "year") {
                matches = (b.date().rfind(item.key, 0) == 0);
            } else {
                matches = (b.date() == item.key);
            }

            if (matches) {
                item.totalRevenue += amount;
                item.bookingCount += 1;
                item.durationMinutes += mins;
                if (b.paid()) {
                    item.paidRevenue += amount;
                } else {
                    item.unpaidRevenue += amount;
                }
                if (isVip) {
                    item.vipRevenue += amount;
                } else {
                    item.standardRevenue += amount;
                }
                break;
            }
        }
    }

    return report;
}
