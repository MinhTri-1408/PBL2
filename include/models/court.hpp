#pragma once

#include <string>

#include "models/base.hpp"

// Sân thể thao: tên sân, loại sân (standard / vip) + giá thuê theo giờ (VND).
class Court : public Entity {
public:
    Court() = default;
    Court(int id, std::string name, long long pricePerHour, std::string type = "standard");

    const std::string& name() const { return name_; }
    long long pricePerHour() const { return pricePerHour_; }
    const std::string& type() const { return type_; }
    bool isVip() const { return type_ == "vip"; }

    void setName(const std::string& name) { name_ = name; }
    void setPricePerHour(long long price) { pricePerHour_ = price; }
    void setType(const std::string& type) { type_ = (type == "vip" ? "vip" : "standard"); }

private:
    std::string name_;
    long long pricePerHour_ = 0;
    std::string type_ = "standard";
};
