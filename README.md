
# 🖱️ KMBox SDK (C++) - NET Mode

This SDK allows you to send mouse and keyboard commands to a [KMBox Pro](https://www.kmbox.cn/) device using the UDP-based NET mode.

## ✨ Features

- ✅ Send raw keyboard input (HID level)
- ✅ Type full strings (simulate typing)
- ✅ Move the mouse (absolute or relative)
- ✅ Simulate mouse clicks (left/right/middle)
- ✅ Minimal and lightweight (C++ only)
- ✅ Fully customizable for your automation or testing needs

---

## 🚀 Getting Started

### 🛠 Prerequisites

- Windows with Winsock support
- C++ compiler (MSVC / MinGW / Clang)
- KMBox Pro device running in NET mode
- Your device’s IP address and UUID

### 🔧 Setup

Clone or copy the following structure into your project:

📁 your-project
┣ 📄 kmbox.h
┣ 📄 kmbox.cpp
┗ 📄 main.cpp

### 🧪 Example Usage

```cpp
#include "kmbox.h"

int main() {
    // Connect to KMBox NET mode
    kmbox.InitializeNet("192.168.2.188", "8808", "62547019");

    // Type text and press Enter
    kmbox.Type("hello world");
    kmbox.HoldKey("enter");
    kmbox.ReleaseKey("enter");

    // Move the mouse and click
    kmbox.Move(100, 50);
    kmbox.ClickLeft();

    return 0;
}
```

## 🎮 Available Commands

### Keyboard

* `kmbox.Type("text")` – types a full string
* `kmbox.HoldKey("a")` – holds a key down
* `kmbox.ReleaseKey("a")` – releases a held key

### Mouse

* `kmbox.Move(x, y)` – move mouse relatively
* `kmbox.MoveTo(x, y)` – move mouse to absolute position
* `kmbox.ClickLeft()`, `ClickRight()` – simulate mouse clicks
* `kmbox.MouseDownLeft()`, `MouseUpLeft()` – mouse button press and release

---

## 🧠 Key Mapping

Uses standard HID usage IDs:

* `"a"` → `0x04`
* `"z"` → `0x1D`
* `"0"` → `0x27`
* `"enter"` → `0x28`
* `"space"` → `0x2C`
* `"ctrl"`, `"shift"`, `"alt"`, `"gui"` → Modifier keys (`0xE0` - `0xE7`)

---

## 📦 License

MIT License

---

## 🧩 About

This SDK is designed to interact with KMBox Pro in NET mode for automation, scripting, testing, and input emulation.

It’s ideal for stealthy tasks, remote automation, or advanced control scenarios.

> 💡 Want to contribute? Fork the repo, submit PRs, or open issues — all improvements are welcome!
>
