#pragma once

// Lớp cơ sở cho mọi thực thể (Court, Booking, ...): chỉ giữ ID.
class Entity {
public:
    explicit Entity(int id = 0) : id_(id) {}
    virtual ~Entity() = default;

    int id() const { return id_; }
    void setId(int id) { id_ = id; }

private:
    int id_;
};
