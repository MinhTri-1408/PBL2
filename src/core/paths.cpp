#include "core/paths.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace paths {

std::string projectRoot() {
    std::error_code ec;
    fs::path p = fs::current_path(ec);
    if (ec) return ".";
    for (int i = 0; i < 6; ++i) {
        if (fs::exists(p / "CMakeLists.txt", ec) && fs::exists(p / "data", ec)) {
            return p.string();
        }
        if (!p.has_parent_path() || p.parent_path() == p) break;
        p = p.parent_path();
    }
    return ".";
}

std::string dataDir() { return (fs::path(projectRoot()) / "data").string(); }

std::string webDir() { return (fs::path(projectRoot()) / "web").string(); }

}  // namespace paths
