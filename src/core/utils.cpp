#include "core/utils.hpp"

#include <cctype>
#include <cstdio>
#include <ctime>

namespace utils {

bool parseTime(const std::string& s, int& minutes) {
    if (s.size() != 5 || s[2] != ':') return false;
    for (int i : {0, 1, 3, 4}) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    int h = (s[0] - '0') * 10 + (s[1] - '0');
    int m = (s[3] - '0') * 10 + (s[4] - '0');
    if (h > 23 || m > 59) return false;
    minutes = h * 60 + m;
    return true;
}

std::string formatTime(int minutes) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", minutes / 60, minutes % 60);
    return buf;
}

bool isValidDate(const std::string& s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (int i : {0, 1, 2, 3, 5, 6, 8, 9}) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    int y = std::stoi(s.substr(0, 4));
    int m = std::stoi(s.substr(5, 2));
    int d = std::stoi(s.substr(8, 2));
    if (y < 2000 || m < 1 || m > 12 || d < 1) return false;
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    int maxDay = days[m - 1] + ((m == 2 && leap) ? 1 : 0);
    return d <= maxDay;
}

std::string today() {
    std::time_t t = std::time(nullptr);
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tmv);
    return buf;
}

bool isValidPhone(const std::string& s) {
    if (s.size() < 9 || s.size() > 11) return false;
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

std::string formatMoney(long long amount) {
    std::string digits = std::to_string(amount < 0 ? -amount : amount);
    std::string out;
    int count = 0;
    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
        out.insert(out.begin(), digits[i]);
        if (++count % 3 == 0 && i > 0) out.insert(out.begin(), ',');
    }
    if (amount < 0) out.insert(out.begin(), '-');
    return out + " VND";
}

std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

bool parseDateParts(const std::string& s, int& y, int& m, int& d) {
    if (!isValidDate(s)) return false;
    y = std::stoi(s.substr(0, 4));
    m = std::stoi(s.substr(5, 2));
    d = std::stoi(s.substr(8, 2));
    return true;
}

std::string formatDate(int y, int m, int d) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", y, m, d);
    return buf;
}

int daysInMonth(int y, int m) {
    if (m < 1 || m > 12) return 30;
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    return days[m - 1] + ((m == 2 && leap) ? 1 : 0);
}

int dayOfWeek(const std::string& date) {
    int y, m, d;
    if (!parseDateParts(date, y, m, d)) return 0;
    std::tm tmv{};
    tmv.tm_year = y - 1900;
    tmv.tm_mon = m - 1;
    tmv.tm_mday = d;
    tmv.tm_hour = 12;
    tmv.tm_isdst = -1;
    std::mktime(&tmv);
    return tmv.tm_wday;
}

std::string dayOfWeekShort(int dow) {
    static const char* names[] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
    if (dow >= 0 && dow <= 6) return names[dow];
    return "";
}

std::string dayOfWeekName(int dow) {
    static const char* names[] = {
        "Chủ nhật", "Thứ hai", "Thứ ba", "Thứ tư", "Thứ năm", "Thứ sáu", "Thứ bảy"};
    if (dow >= 0 && dow <= 6) return names[dow];
    return "";
}

std::string addDays(const std::string& date, int days) {
    int y, m, d;
    if (!parseDateParts(date, y, m, d)) return date;
    std::tm tmv{};
    tmv.tm_year = y - 1900;
    tmv.tm_mon = m - 1;
    tmv.tm_mday = d + days;
    tmv.tm_hour = 12;
    tmv.tm_isdst = -1;
    std::mktime(&tmv);
    return formatDate(tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday);
}

bool getWeekRange(const std::string& refDate, std::string& startMonday, std::string& endSunday) {
    int y, m, d;
    if (!parseDateParts(refDate, y, m, d)) return false;
    int dow = dayOfWeek(refDate);
    int offset = (dow == 0 ? 6 : dow - 1);
    startMonday = addDays(refDate, -offset);
    endSunday = addDays(startMonday, 6);
    return true;
}

bool getMonthRange(const std::string& refDate, std::string& startDay, std::string& endDay) {
    int y, m, d;
    if (!parseDateParts(refDate, y, m, d)) return false;
    startDay = formatDate(y, m, 1);
    endDay = formatDate(y, m, daysInMonth(y, m));
    return true;
}

bool getYearRange(const std::string& refDate, std::string& startDay, std::string& endDay) {
    int y, m, d;
    if (!parseDateParts(refDate, y, m, d)) return false;
    startDay = formatDate(y, 1, 1);
    endDay = formatDate(y, 12, 31);
    return true;
}

}  // namespace utils
