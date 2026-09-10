#include "core/app.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        miqumusic::App app(argc, argv);
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "[miqumusic] Fatal exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "[miqumusic] Unknown fatal exception\n";
        return 1;
    }
}
