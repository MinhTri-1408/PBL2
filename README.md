# Quản lý đặt sân (C++17)

Quản lý sân thể thao: danh sách sân, đặt/hủy sân và thanh toán. Dữ liệu lưu bằng file JSON trong `data/`.
Có hai cách dùng, cùng một bộ xử lý phía sau:

| Chương trình | Giao diện | Chạy |
|---|---|---|
| `app_web` | Web (mở bằng trình duyệt) | `build\Release\app_web.exe` |
| `app` | Console (gõ menu) | `build\Release\app.exe` |

## Chức năng

**Quản lý sân**
- Phân loại sân: **Sân thường** và **Sân VIP ⭐** (giá thuê riêng, hiển thị nổi bật)
- Xem danh sách sân theo ngày: trống cả ngày / đã có lượt đặt / kín lịch
- Thêm, sửa, xóa sân (không xóa được sân còn lượt đặt chưa thanh toán)

**Đặt sân**
- Đặt sân: tên, số điện thoại, ngày, chọn sân (thường hoặc VIP) và khung giờ. Chặn đặt trùng giờ
- Hủy đặt sân (tìm theo số điện thoại)
- Xem trạng thái 1 sân cụ thể trong ngày: giờ đã đặt và giờ còn trống

**Thanh toán**
- `Tiền = (giờ kết thúc - giờ bắt đầu) × giá sân theo giờ`
- Hóa đơn, đánh dấu đã thanh toán (lượt đã thanh toán không thể hủy)

**Thống kê & Báo cáo doanh thu**
- Thống kê theo **tuần**, **tháng**, **năm**
- Báo cáo 4 chỉ số KPI: Tổng doanh thu (Đã thu / Chờ thu), Lượt đặt sân, Thời lượng chơi, Cơ cấu doanh thu Sân VIP vs Sân thường
- Biểu đồ phân bố doanh thu trực quan + Bảng dữ liệu chi tiết theo từng mốc thời gian

Giờ mở cửa: 05:00 - 23:00 (đổi trong `include/core/utils.hpp`).

## Build và chạy

Cần trình biên dịch C++17 (Visual Studio, g++, clang) và CMake ≥ 3.16. Không cần cài thêm thư viện nào.

```bash
cmake -S . -B build
cmake --build build --config Release
```

Chạy bản web, trình duyệt sẽ tự mở trên Windows. Nếu không, mở http://127.0.0.1:8080
```bash
./build/app_web                 # Windows: .\build\Release\app_web.exe
./build/app_web 8081            # đổi cổng nếu 8080 đang bận
```
Chạy bản console: `./build/app` (Windows: `.\build\Release\app.exe`).

Chương trình tự tìm thư mục dự án (nơi có `CMakeLists.txt` và `data/`), nên chạy từ đâu trong dự án cũng được.

Không dùng CMake (Windows, Developer PowerShell for VS, đứng ở thư mục dự án):
```powershell
cl /std:c++17 /EHsc /utf-8 /DNOMINMAX /Iinclude /Ithird_party src\server.cpp src\web\http_server.cpp src\core\*.cpp src\models\*.cpp /Fe:app_web.exe
.\app_web.exe
```

## Cấu trúc thư mục

```
quan-ly-san/
├── data/                  # courts.json, bookings.json
├── include/
│   ├── models/            # base.hpp, court.hpp, booking.hpp
│   ├── core/              # storage.hpp, manager.hpp, utils.hpp, paths.hpp
│   └── web/               # http_server.hpp
├── src/
│   ├── models/            # court.cpp, booking.cpp
│   ├── core/              # storage.cpp (đọc/ghi JSON), manager.cpp (nghiệp vụ), utils.cpp, paths.cpp
│   ├── web/               # http_server.cpp (máy chủ HTTP nhỏ, chỉ nghe ở 127.0.0.1)
│   ├── main.cpp           # Giao diện console (có menu thống kê)
│   └── server.cpp         # API JSON cho giao diện web
├── web/index.html         # Giao diện web (lịch sân, đặt sân, hóa đơn, popup thống kê)
├── third_party/nlohmann/  # json.hpp (đã kèm sẵn)
├── CMakeLists.txt
└── README.md
```

| Lớp | Vai trò |
|---|---|
| `Entity` (base) | Lớp cha, giữ `id` |
| `Court` | Sân: tên, loại sân (`type`: standard / vip), giá/giờ |
| `Booking` | Lượt đặt: khách, SĐT, sân, ngày, giờ bắt đầu/kết thúc, đã thanh toán |
| `Storage` | Đọc/ghi JSON |
| `Manager` | Toàn bộ logic: CRUD sân, đặt/hủy, kiểm tra trùng giờ, tính tiền, thống kê tuần/tháng/năm |

## API (bản web)

| Phương thức | Đường dẫn | Việc |
|---|---|---|
| GET | `/api/board?date=YYYY-MM-DD` | Tất cả sân + lịch đặt trong ngày |
| GET | `/api/courts/:id/schedule?date=` | Trạng thái 1 sân trong ngày |
| POST / PUT / DELETE | `/api/courts`, `/api/courts/:id` | Thêm, sửa, xóa sân (kèm `type`: "standard" / "vip") |
| POST | `/api/bookings` | Đặt sân |
| GET | `/api/bookings?phone=` | Lượt đặt chưa thanh toán của 1 khách |
| DELETE | `/api/bookings/:id` | Hủy đặt |
| POST | `/api/bookings/:id/pay` | Thanh toán |
| GET | `/api/stats?period=week\|month\|year&date=` | Thống kê doanh thu, lượt đặt, tỷ lệ sân theo tuần/tháng/năm |

## Phân công thành viên

| Thành viên | Công việc |
|---|---|
| ... | ... |
