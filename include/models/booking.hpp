#pragma once

#include <string>

#include "models/base.hpp"

// Lượt đặt sân. Giờ lưu dưới dạng số phút kể từ 00:00 (vd 18:30 -> 1110).
class Booking : public Entity {
public:
    Booking() = default;
    Booking(int id, std::string customerName, std::string phone, int courtId,
            std::string date, int startMin, int endMin, bool paid = false);

    const std::string& customerName() const { return customerName_; }
    const std::string& phone() const { return phone_; }
    int courtId() const { return courtId_; }
    const std::string& date() const { return date_; }  // YYYY-MM-DD
    int startMin() const { return startMin_; }
    int endMin() const { return endMin_; }
    bool paid() const { return paid_; }

    void setPaid(bool paid) { paid_ = paid; }

    int durationMinutes() const { return endMin_ - startMin_; }

    // Hai khung giờ [s, e) có chồng lên nhau không?
    bool overlaps(int s, int e) const { return startMin_ < e && s < endMin_; }

private:
    std::string customerName_;
    std::string phone_;
    int courtId_ = 0;
    std::string date_;
    int startMin_ = 0;
    int endMin_ = 0;
    bool paid_ = false;
};
