#pragma once

#include <string>
#include <vector>

#include "models/booking.hpp"
#include "models/court.hpp"

// Đọc / ghi dữ liệu JSON trong thư mục data/.
class Storage {
public:
    explicit Storage(std::string dataDir);

    std::vector<Court> loadCourts() const;
    std::vector<Booking> loadBookings() const;

    void saveCourts(const std::vector<Court>& courts) const;
    void saveBookings(const std::vector<Booking>& bookings) const;

private:
    std::string courtsPath() const;
    std::string bookingsPath() const;

    std::string dataDir_;
};
