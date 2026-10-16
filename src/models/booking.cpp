#include "models/booking.hpp"

#include <utility>

Booking::Booking(int id, std::string customerName, std::string phone, int courtId,
                 std::string date, int startMin, int endMin, bool paid)
    : Entity(id),
      customerName_(std::move(customerName)),
      phone_(std::move(phone)),
      courtId_(courtId),
      date_(std::move(date)),
      startMin_(startMin),
      endMin_(endMin),
      paid_(paid) {}
