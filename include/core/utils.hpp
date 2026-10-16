#pragma once

#include <string>

namespace utils {

constexpr int OPEN_MIN = 5 * 60;    // sân mở cửa 05:00
constexpr int CLOSE_MIN = 23 * 60;  // sân đóng cửa 23:00

// "HH:MM" -> số phút. Trả về false nếu sai định dạng.
bool parseTime(const std::string& s, int& minutes);

// số phút -> "HH:MM"
std::string formatTime(int minutes);

// Kiểm tra "YYYY-MM-DD" hợp lệ (có xét năm nhuận).
bool isValidDate(const std::string& s);

// Ngày hôm nay dạng "YYYY-MM-DD".
std::string today();

// Số điện thoại: 9-11 chữ số.
bool isValidPhone(const std::string& s);

// 1500000 -> "1,500,000 VND"
std::string formatMoney(long long amount);

// Bỏ khoảng trắng đầu/cuối.
std::string trim(const std::string& s);

// Tách ngày thành y, m, d. Trả về false nếu sai định dạng.
bool parseDateParts(const std::string& s, int& y, int& m, int& d);

// Tạo chuỗi "YYYY-MM-DD" từ y, m, d.
std::string formatDate(int y, int m, int d);

// Số ngày trong tháng (có tính năm nhuận).
int daysInMonth(int y, int m);

// Thứ trong tuần (0=Chủ nhật, 1=Thứ 2, ..., 6=Thứ 7).
int dayOfWeek(const std::string& date);
std::string dayOfWeekShort(int dow); // "CN", "T2", "T3"...
std::string dayOfWeekName(int dow);  // "Chủ nhật", "Thứ hai"...

// Khoảng thời gian tuần (Thứ 2 đến Chủ nhật) chứa ngày refDate.
bool getWeekRange(const std::string& refDate, std::string& startMonday, std::string& endSunday);

// Khoảng thời gian tháng (ngày 1 đến ngày cuối tháng) chứa ngày refDate.
bool getMonthRange(const std::string& refDate, std::string& startDay, std::string& endDay);

// Khoảng thời gian năm (01-01 đến 12-31) chứa ngày refDate.
bool getYearRange(const std::string& refDate, std::string& startDay, std::string& endDay);

// Cộng n ngày vào ngày YYYY-MM-DD.
std::string addDays(const std::string& date, int days);

}  // namespace utils
