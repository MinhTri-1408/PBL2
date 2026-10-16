#pragma once

#include <string>
#include <vector>

#include "core/storage.hpp"
#include "models/booking.hpp"
#include "models/court.hpp"

// Kết quả của một thao tác nghiệp vụ.
struct Result {
    bool ok = false;
    std::string message;
    int id = 0;  // id của đối tượng vừa tạo (nếu có)
};

// Khoảng giờ trống trong ngày.
struct TimeSlot {
    int startMin;
    int endMin;
};

// Chi tiết thống kê theo từng mốc (từng ngày trong tuần/tháng, hoặc từng tháng trong năm).
struct StatBreakdownItem {
    std::string key;            // "YYYY-MM-DD" hoặc "YYYY-MM"
    std::string label;          // "T2 (12/10)" hoặc "01/10" hoặc "Tháng 01"
    long long totalRevenue = 0; // tổng doanh thu
    long long paidRevenue = 0;  // doanh thu đã thanh toán
    long long unpaidRevenue = 0;// doanh thu chưa thanh toán
    int bookingCount = 0;       // số lượt đặt
    int durationMinutes = 0;    // tổng phút
    long long vipRevenue = 0;   // doanh thu sân VIP
    long long standardRevenue = 0; // doanh thu sân thường
};

// Báo cáo thống kê tổng hợp.
struct StatsReport {
    std::string period;         // "week", "month", "year"
    std::string label;          // vd: "Tuần (12/10/2026 - 18/10/2026)"
    std::string startDate;      // "YYYY-MM-DD"
    std::string endDate;        // "YYYY-MM-DD"
    long long totalRevenue = 0;
    long long paidRevenue = 0;
    long long unpaidRevenue = 0;
    int totalBookings = 0;
    int totalMinutes = 0;

    long long vipRevenue = 0;
    int vipBookings = 0;
    int vipMinutes = 0;

    long long standardRevenue = 0;
    int standardBookings = 0;
    int standardMinutes = 0;

    std::vector<StatBreakdownItem> breakdown;
};

// Logic nghiệp vụ: quản lý sân, đặt/hủy sân, thanh toán, thống kê.
// Mỗi thay đổi đều được lưu xuống file ngay.
class Manager {
public:
    explicit Manager(Storage& storage);

    // ---- Quản lý sân ----
    const std::vector<Court>& courts() const { return courts_; }
    const Court* findCourt(int id) const;
    Result addCourt(const std::string& name, long long pricePerHour, const std::string& type = "standard");
    Result updateCourt(int id, const std::string& name, long long pricePerHour, const std::string& type = "");
    Result removeCourt(int id);

    // ---- Đặt sân ----
    Result book(const std::string& customerName, const std::string& phone, int courtId,
                const std::string& date, int startMin, int endMin);
    Result cancel(int bookingId);

    const Booking* findBooking(int id) const;
    // Lịch đặt của 1 sân trong 1 ngày, sắp xếp theo giờ bắt đầu.
    std::vector<Booking> bookingsOfCourt(int courtId, const std::string& date) const;
    // Các lượt đặt chưa thanh toán của 1 số điện thoại.
    std::vector<Booking> bookingsByPhone(const std::string& phone) const;
    // Các khoảng giờ còn trống của sân trong ngày (trong giờ mở cửa).
    std::vector<TimeSlot> freeSlots(int courtId, const std::string& date) const;
    bool isFree(int courtId, const std::string& date, int startMin, int endMin) const;

    // ---- Thanh toán ----
    // tiền = (giờ kết thúc - giờ bắt đầu) * tiền sân theo giờ
    long long calcAmount(const Booking& b) const;
    Result pay(int bookingId);

    // ---- Thống kê ----
    StatsReport getStats(const std::string& period, const std::string& refDate = "") const;

private:
    int nextCourtId() const;
    int nextBookingId() const;
    void saveCourts() const { storage_.saveCourts(courts_); }
    void saveBookings() const { storage_.saveBookings(bookings_); }

    Storage& storage_;
    std::vector<Court> courts_;
    std::vector<Booking> bookings_;
};
