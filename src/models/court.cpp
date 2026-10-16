#include "models/court.hpp"

#include <utility>

Court::Court(int id, std::string name, long long pricePerHour, std::string type)
    : Entity(id),
      name_(std::move(name)),
      pricePerHour_(pricePerHour),
      type_(type == "vip" ? "vip" : "standard") {}
