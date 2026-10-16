#pragma once

#include <string>

// Tìm thư mục gốc của dự án (nơi có CMakeLists.txt và data/) bằng cách đi ngược
// từ thư mục hiện tại. Nhờ vậy chạy app từ build/Release/ vẫn tìm thấy data/ và web/.
namespace paths {

std::string projectRoot();  // không tìm thấy -> "."
std::string dataDir();      // <root>/data
std::string webDir();       // <root>/web

}  // namespace paths
