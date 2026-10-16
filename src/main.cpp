// Chuong trinh quan ly dat san - giao dien console.
// Chay: ./app [thu_muc_data]   (mac dinh: data)

#include <iomanip>
#include <iostream>
#include <string>

#include "core/manager.hpp"
#include "core/paths.hpp"
#include "core/storage.hpp"
#include "core/utils.hpp"

// ======================= Nhap lieu =======================

static std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cout << "\nKet thuc.\n";
        std::exit(0);
    }
    return utils::trim(line);
}

// Nhap so nguyen; tra ve false neu bo trong hoac khong phai so.
static bool readLong(const std::string& prompt, long long& out) {
    std::string s = readLine(prompt);
    if (s.empty()) return false;
    try {
        size_t pos = 0;
        out = std::stoll(s, &pos);
        return pos == s.size();
    } catch (const std::exception&) {
        return false;
    }
}

static bool readInt(const std::string& prompt, int& out) {
    long long v;
    if (!readLong(prompt, v)) return false;
    out = static_cast<int>(v);
    return true;
}

static bool readTime(const std::string& prompt, int& minutes) {
    if (utils::parseTime(readLine(prompt), minutes)) return true;
    std::cout << "Gio khong hop le (dinh dang HH:MM, vd 18:30).\n";
    return false;
}

// Nhap ngay; de trong = hom nay.
static bool readDate(const std::string& prompt, std::string& date) {
    date = readLine(prompt + " (YYYY-MM-DD, Enter = hom nay): ");
    if (date.empty()) date = utils::today();
    if (!utils::isValidDate(date)) {
        std::cout << "Ngay khong hop le.\n";
        return false;
    }
    return true;
}

static void printResult(const Result& r) {
    std::cout << (r.ok ? "[OK] " : "[LOI] ") << r.message << "\n";
}

static void pause() { readLine("\nNhan Enter de tiep tuc..."); }

// ======================= Hien thi =======================

static void printCourtTable(const Manager& m, const std::string& date) {
    std::cout << "\nDanh sach san - ngay " << date << "\n";
    std::cout << std::left << std::setw(5) << "ID" << std::setw(20) << "Ten san"
              << std::setw(12) << "Loai" << std::setw(18) << "Gia/gio" << "Trang thai\n";
    std::cout << std::string(74, '-') << "\n";
    for (const auto& c : m.courts()) {
        auto booked = m.bookingsOfCourt(c.id(), date);
        std::string status;
        if (booked.empty()) {
            status = "Con trong ca ngay";
        } else if (m.freeSlots(c.id(), date).empty()) {
            status = "Da kin lich";
        } else {
            status = "Da co " + std::to_string(booked.size()) + " luot dat";
        }
        std::string typeStr = c.isVip() ? "VIP *" : "Thuong";
        std::cout << std::left << std::setw(5) << c.id() << std::setw(20) << c.name()
                  << std::setw(12) << typeStr << std::setw(18) << utils::formatMoney(c.pricePerHour())
                  << status << "\n";
    }
    if (m.courts().empty()) std::cout << "(Chua co san nao)\n";
}

static void printBookingLine(const Manager& m, const Booking& b) {
    const Court* c = m.findCourt(b.courtId());
    std::cout << "  #" << b.id() << " | " << b.date() << " | " << utils::formatTime(b.startMin())
              << "-" << utils::formatTime(b.endMin()) << " | " << (c ? c->name() : "(san da xoa)")
              << " | " << b.customerName() << " - " << b.phone() << " | "
              << (b.paid() ? "Da thanh toan" : "Chua thanh toan") << "\n";
}

// ======================= Cac chuc nang =======================

static void viewCourts(Manager& m) {
    std::string date;
    if (!readDate("Xem trang thai san ngay", date)) return;
    printCourtTable(m, date);
}

static void addCourt(Manager& m) {
    std::string typeInput = readLine("Loai san (1. Thuong, 2. VIP, Enter = Thuong): ");
    std::string type = (typeInput == "2" || typeInput == "vip" || typeInput == "VIP") ? "vip" : "standard";

    int maxNum = 0;
    int count = 0;
    for (const auto& c : m.courts()) {
        if (c.type() != type) continue;
        count++;
        const std::string& n = c.name();
        size_t idx = n.find_last_not_of("0123456789");
        if (idx != std::string::npos && idx + 1 < n.size()) {
            try {
                int num = std::stoi(n.substr(idx + 1));
                if (num > maxNum) maxNum = num;
            } catch (...) {}
        }
    }
    int nextNum = maxNum > 0 ? maxNum + 1 : count + 1;
    std::string defaultName = (type == "vip" ? "San VIP " : "San ") + std::to_string(nextNum);

    std::string name = readLine("Ten san moi (Enter = \"" + defaultName + "\"): ");
    if (name.empty()) name = defaultName;

    long long defaultPrice = (type == "vip" ? 250000 : 150000);
    std::string priceStr = readLine("Gia thue moi gio (VND, Enter = " + utils::formatMoney(defaultPrice) + "): ");
    long long price = defaultPrice;
    if (!priceStr.empty()) {
        try {
            size_t pos = 0;
            price = std::stoll(priceStr, &pos);
            if (pos != priceStr.size()) throw std::invalid_argument("x");
        } catch (...) {
            std::cout << "Gia khong hop le.\n";
            return;
        }
    }

    printResult(m.addCourt(name, price, type));
}

static void editCourt(Manager& m) {
    int id;
    if (!readInt("ID san can sua: ", id)) { std::cout << "ID khong hop le.\n"; return; }
    const Court* c = m.findCourt(id);
    if (!c) { std::cout << "Khong tim thay san.\n"; return; }

    std::string name = readLine("Ten moi (Enter = giu \"" + c->name() + "\"): ");
    if (name.empty()) name = c->name();
    long long price = c->pricePerHour();
    std::string priceStr = readLine("Gia moi/gio (Enter = giu " + std::to_string(price) + "): ");
    if (!priceStr.empty()) {
        try {
            size_t pos = 0;
            long long v = std::stoll(priceStr, &pos);
            if (pos != priceStr.size()) throw std::invalid_argument("x");
            price = v;
        } catch (const std::exception&) {
            std::cout << "Gia khong hop le.\n";
            return;
        }
    }
    std::string curTypeStr = c->isVip() ? "VIP" : "Thuong";
    std::string typeInput = readLine("Loai san (1. Thuong, 2. VIP, Enter = giu " + curTypeStr + "): ");
    std::string type = "";
    if (typeInput == "1" || typeInput == "thuong") type = "standard";
    else if (typeInput == "2" || typeInput == "vip" || typeInput == "VIP") type = "vip";

    printResult(m.updateCourt(id, name, price, type));
}

static void removeCourt(Manager& m) {
    int id;
    if (!readInt("ID san can xoa: ", id)) { std::cout << "ID khong hop le.\n"; return; }
    const Court* c = m.findCourt(id);
    if (!c) { std::cout << "Khong tim thay san.\n"; return; }
    if (readLine("Xoa san \"" + c->name() + "\"? (y/n): ") != "y") return;
    printResult(m.removeCourt(id));
}

static void bookCourt(Manager& m) {
    std::string name = readLine("Ten khach: ");
    std::string phone = readLine("So dien thoai: ");
    std::string date;
    if (!readDate("Ngay dat", date)) return;

    printCourtTable(m, date);
    int courtId;
    if (!readInt("\nChon ID san: ", courtId)) { std::cout << "ID khong hop le.\n"; return; }
    if (!m.findCourt(courtId)) { std::cout << "Khong tim thay san.\n"; return; }

    std::cout << "Khung gio con trong: ";
    for (const auto& s : m.freeSlots(courtId, date)) {
        std::cout << "[" << utils::formatTime(s.startMin) << "-" << utils::formatTime(s.endMin) << "] ";
    }
    std::cout << "\n";

    int start, end;
    if (!readTime("Gio bat dau (HH:MM): ", start)) return;
    if (!readTime("Gio ket thuc (HH:MM): ", end)) return;

    Result r = m.book(name, phone, courtId, date, start, end);
    printResult(r);
    if (r.ok) {
        const Booking* b = m.findBooking(r.id);
        std::cout << "Ma dat san: #" << r.id << " | Tam tinh: "
                  << utils::formatMoney(m.calcAmount(*b)) << "\n";
    }
}

static void cancelBooking(Manager& m) {
    std::string phone = readLine("So dien thoai khach: ");
    auto list = m.bookingsByPhone(phone);
    if (list.empty()) {
        std::cout << "Khong co luot dat nao (chua thanh toan) cho so nay.\n";
        return;
    }
    std::cout << "Cac luot dat:\n";
    for (const auto& b : list) printBookingLine(m, b);

    int id;
    if (!readInt("Nhap ma luot dat can huy: ", id)) { std::cout << "Ma khong hop le.\n"; return; }
    const Booking* b = m.findBooking(id);
    if (!b || b->phone() != phone) {
        std::cout << "Ma luot dat khong thuoc so dien thoai nay.\n";
        return;
    }
    if (readLine("Xac nhan huy luot dat #" + std::to_string(id) + "? (y/n): ") != "y") return;
    printResult(m.cancel(id));
}

static void viewCourtSchedule(Manager& m) {
    int courtId;
    if (!readInt("ID san: ", courtId)) { std::cout << "ID khong hop le.\n"; return; }
    const Court* c = m.findCourt(courtId);
    if (!c) { std::cout << "Khong tim thay san.\n"; return; }
    std::string date;
    if (!readDate("Ngay can xem", date)) return;

    std::cout << "\n" << c->name() << " - ngay " << date << " (mo cua "
              << utils::formatTime(utils::OPEN_MIN) << "-" << utils::formatTime(utils::CLOSE_MIN)
              << ")\n";
    auto booked = m.bookingsOfCourt(courtId, date);
    std::cout << "Da dat:\n";
    if (booked.empty()) std::cout << "  (chua co)\n";
    for (const auto& b : booked) printBookingLine(m, b);

    std::cout << "Con trong:\n";
    auto slots = m.freeSlots(courtId, date);
    if (slots.empty()) std::cout << "  (da kin lich)\n";
    for (const auto& s : slots) {
        std::cout << "  " << utils::formatTime(s.startMin) << " - " << utils::formatTime(s.endMin)
                  << "\n";
    }
}

static void payBooking(Manager& m) {
    std::string phone = readLine("So dien thoai khach: ");
    auto list = m.bookingsByPhone(phone);
    if (list.empty()) {
        std::cout << "Khong co luot dat nao can thanh toan cho so nay.\n";
        return;
    }
    std::cout << "Cac luot dat chua thanh toan:\n";
    for (const auto& b : list) {
        printBookingLine(m, b);
        std::cout << "     Thanh tien: " << utils::formatMoney(m.calcAmount(b)) << "\n";
    }

    int id;
    if (!readInt("Nhap ma luot dat can thanh toan: ", id)) { std::cout << "Ma khong hop le.\n"; return; }
    const Booking* b = m.findBooking(id);
    if (!b || b->phone() != phone) {
        std::cout << "Ma luot dat khong thuoc so dien thoai nay.\n";
        return;
    }
    const Court* c = m.findCourt(b->courtId());
    std::cout << "\n===== HOA DON =====\n"
              << "Khach hang : " << b->customerName() << " (" << b->phone() << ")\n"
              << "San        : " << (c ? c->name() : "?") << " - " << (c ? utils::formatMoney(c->pricePerHour()) : "") << "/gio\n"
              << "Ngay       : " << b->date() << "\n"
              << "Khung gio  : " << utils::formatTime(b->startMin()) << " - " << utils::formatTime(b->endMin())
              << " (" << std::fixed << std::setprecision(2) << b->durationMinutes() / 60.0 << " gio)\n"
              << "TONG TIEN  : " << utils::formatMoney(m.calcAmount(*b)) << "\n"
              << "===================\n";
    if (readLine("Xac nhan thanh toan? (y/n): ") != "y") return;
    printResult(m.pay(id));
}

// ======================= Menu =======================

static void courtMenu(Manager& m) {
    while (true) {
        std::cout << "\n--- QUAN LY SAN ---\n"
                     "1. Xem danh sach san\n"
                     "2. Them san\n"
                     "3. Sua san\n"
                     "4. Xoa san\n"
                     "0. Quay lai\n";
        std::string c = readLine("Chon: ");
        if (c == "0") return;
        if (c == "1") viewCourts(m);
        else if (c == "2") addCourt(m);
        else if (c == "3") editCourt(m);
        else if (c == "4") removeCourt(m);
        else std::cout << "Lua chon khong hop le.\n";
        pause();
    }
}

static void bookingMenu(Manager& m) {
    while (true) {
        std::cout << "\n--- DAT SAN ---\n"
                     "1. Dat san\n"
                     "2. Huy dat san\n"
                     "3. Xem trang thai 1 san trong ngay\n"
                     "0. Quay lai\n";
        std::string c = readLine("Chon: ");
        if (c == "0") return;
        if (c == "1") bookCourt(m);
        else if (c == "2") cancelBooking(m);
        else if (c == "3") viewCourtSchedule(m);
        else std::cout << "Lua chon khong hop le.\n";
        pause();
    }
}

// ======================= Thong ke =======================

static void printStatsReport(const StatsReport& r) {
    std::cout << "\n======================================================\n";
    std::cout << "          BAO CAO THONG KE - " << r.label << "\n";
    std::cout << "  Thoi gian: " << r.startDate << " den " << r.endDate << "\n";
    std::cout << "======================================================\n";
    std::cout << "  Tong luot dat     : " << r.totalBookings << " luot\n";
    std::cout << "  Tong thoi luong   : " << std::fixed << std::setprecision(1) << r.totalMinutes / 60.0 << " gio\n";
    std::cout << "  Tong doanh thu    : " << utils::formatMoney(r.totalRevenue) << "\n";
    std::cout << "    - Da thanh toan : " << utils::formatMoney(r.paidRevenue) << "\n";
    std::cout << "    - Cho thanh toan: " << utils::formatMoney(r.unpaidRevenue) << "\n";
    std::cout << "------------------------------------------------------\n";
    std::cout << "  Phan loai theo san:\n";
    std::cout << "    * San Thuong: " << r.standardBookings << " luot ("
              << std::fixed << std::setprecision(1) << r.standardMinutes / 60.0 << " gio) - "
              << utils::formatMoney(r.standardRevenue) << "\n";
    std::cout << "    * San VIP   : " << r.vipBookings << " luot ("
              << std::fixed << std::setprecision(1) << r.vipMinutes / 60.0 << " gio) - "
              << utils::formatMoney(r.vipRevenue) << "\n";
    std::cout << "------------------------------------------------------\n";
    std::cout << "  Chi tiet:\n";
    std::cout << std::left << std::setw(18) << "  Moc" << std::setw(10) << "Luot dat"
              << std::setw(12) << "Gio choi" << std::setw(18) << "Doanh thu" << "VIP\n";
    for (const auto& item : r.breakdown) {
        std::cout << std::left << "  " << std::setw(16) << item.label
                  << std::setw(10) << item.bookingCount
                  << std::setw(12) << (std::to_string(item.durationMinutes / 60) + "h" + std::to_string(item.durationMinutes % 60) + "m")
                  << std::setw(18) << utils::formatMoney(item.totalRevenue)
                  << utils::formatMoney(item.vipRevenue) << "\n";
    }
    std::cout << "======================================================\n";
}

static void statsMenu(Manager& m) {
    while (true) {
        std::cout << "\n--- THONG KE DOANH THU ---\n"
                     "1. Thong ke theo tuan\n"
                     "2. Thong ke theo thang\n"
                     "3. Thong ke theo nam\n"
                     "0. Quay lai\n";
        std::string c = readLine("Chon: ");
        if (c == "0") return;
        std::string date;
        if (c == "1") {
            if (!readDate("Nhap 1 ngay trong tuan can xem", date)) continue;
            printStatsReport(m.getStats("week", date));
        } else if (c == "2") {
            if (!readDate("Nhap 1 ngay trong thang can xem", date)) continue;
            printStatsReport(m.getStats("month", date));
        } else if (c == "3") {
            if (!readDate("Nhap 1 ngay trong nam can xem", date)) continue;
            printStatsReport(m.getStats("year", date));
        } else {
            std::cout << "Lua chon khong hop le.\n";
        }
        pause();
    }
}

int main(int argc, char* argv[]) {
    std::string dataDir = argc > 1 ? argv[1] : paths::dataDir();
    Storage storage(dataDir);
    Manager manager(storage);

    while (true) {
        std::cout << "\n========== QUAN LY DAT SAN ==========\n"
                     "1. Quan ly san\n"
                     "2. Dat san / Huy san\n"
                     "3. Thanh toan\n"
                     "4. Thong ke doanh thu (tuan/thang/nam)\n"
                     "0. Thoat\n";
        std::string c = readLine("Chon: ");
        if (c == "0") break;
        if (c == "1") courtMenu(manager);
        else if (c == "2") bookingMenu(manager);
        else if (c == "3") { payBooking(manager); pause(); }
        else if (c == "4") statsMenu(manager);
        else std::cout << "Lua chon khong hop le.\n";
    }
    std::cout << "Tam biet!\n";
    return 0;
}
