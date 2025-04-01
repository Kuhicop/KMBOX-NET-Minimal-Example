#include "kmbox.h"

int main() {
    std::string ip = "192.168.2.188";
    std::string port = "8808";
    std::string uuid = "62547019";

    // Initialize KMBox in NET mode
    kmbox.InitializeNet(ip, port, uuid);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 1. Move mouse slightly
    kmbox.Move(50, 50);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 2. Perform left click
    kmbox.ClickLeft();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 3. Type lowercase message
    kmbox.Type("hello world");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    kmbox.HoldKey("enter");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    kmbox.ReleaseKey("enter");

    // 4. Type uppercase message using SHIFT
    kmbox.HoldKey("shift");
    kmbox.Type("demo");
    kmbox.ReleaseKey("shift");
    kmbox.HoldKey("enter");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    kmbox.ReleaseKey("enter");

    // 5. Perform right click
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    kmbox.ClickRight();

    return 0;
}