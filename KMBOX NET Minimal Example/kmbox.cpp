#include "kmbox.h"

int clamp(int i) {
	if (i > 127)
		i = 127;
	if (i < -128)
		i = -128;

	return i;
}

int getKeyCode(const std::string& key) {
	if (key.length() == 1) {
		char c = tolower(key[0]);
		if (c >= 'a' && c <= 'z') return c - 'a' + 0x04;  // HID A=0x04 ... Z=0x1D
		if (c >= '1' && c <= '9') return c - '1' + 0x1E;  // HID 1=0x1E ... 9=0x26
		if (c == '0') return 0x27;
		if (c == ' ') return 0x2C;
	}

	// Special keys
	if (key == "enter") return 0x28;
	if (key == "esc") return 0x29;
	if (key == "backspace") return 0x2A;
	if (key == "tab") return 0x2B;
	if (key == "ctrl") return 0xE0;
	if (key == "shift") return 0xE1;
	if (key == "alt") return 0xE2;
	if (key == "gui") return 0xE3;

	return 0; // Unknown key
}


std::string Kmbox::FindPort(const std::string& targetDescription) {
	HDEVINFO hDevInfo = SetupDiGetClassDevsA(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE) return "";

	SP_DEVINFO_DATA deviceInfoData;
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &deviceInfoData); ++i) {
		char buf[512];
		DWORD nSize = 0;

		if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &deviceInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)buf, sizeof(buf), &nSize) && nSize > 0) {
			buf[nSize] = '\0';
			std::string deviceDescription = buf;

			size_t comPos = deviceDescription.find("COM");
			size_t endPos = deviceDescription.find(")", comPos);

			if (comPos != std::string::npos && endPos != std::string::npos && deviceDescription.find(targetDescription) != std::string::npos) {
				SetupDiDestroyDeviceInfoList(hDevInfo);
				return deviceDescription.substr(comPos, endPos - comPos);
			}
		}
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);
	return "";
}

void Kmbox::Initialize(LPCSTR port) {
	serialHandle = CreateFileA(port, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (serialHandle == INVALID_HANDLE_VALUE) {
		std::cout << "Failed to open serial port!" << std::endl;
		return;
	}

	if (!SetupComm(serialHandle, 8192, 8192)) {
		std::cout << "Failed to setup serial port!" << std::endl;
		return;
	}

	COMMTIMEOUTS timeouts = { 0 };
	if (!GetCommTimeouts(serialHandle, &timeouts)) {
		std::cout << "Failed to get serial timeouts!" << std::endl;
		return;
	}

	timeouts.ReadIntervalTimeout = 0xFFFFFFFF;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 2000;

	if (!SetCommTimeouts(serialHandle, &timeouts))
	{
		std::cout << "Failed to set serial parameters!" << std::endl;
		CloseHandle(serialHandle);
		return;
	}

	PurgeComm(serialHandle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	DCB dcbSerialParams = { 0 };
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	if (!GetCommState(serialHandle, &dcbSerialParams)) {
		std::cout << "Failed to get serial state!" << std::endl;
		CloseHandle(serialHandle);
		return;
	}

	int baud = 115200;
	dcbSerialParams.BaudRate = baud;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;

	if (!SetCommState(serialHandle, &dcbSerialParams)) {
		std::cout << "Failed to set serial parameters!" << std::endl;
		CloseHandle(serialHandle);
		return;
	}

	std::cout << "Connected to KMBOX on port " << std::string(port).c_str() << std::endl;
	type = KmboxType::USB;
}

void Kmbox::InitializeNet(const std::string& IP, std::string port, const std::string& uuid) {
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		std::cout << "WSAStartup failed -> " << iResult << std::endl;
		return;
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		std::cout << "Could not find a usable version of Winsock.dll" << std::endl;
		WSACleanup();
		return;
	}

	srand((unsigned)time(NULL));
	socketHandle = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketHandle == INVALID_SOCKET) {
		std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return;
	}

	destination.sin_addr.S_un.S_addr = inet_addr(IP.c_str());
	destination.sin_family = AF_INET;
	destination.sin_port = htons(atoi(port.c_str()));

	std::cout << "Connected to KMBOX Net on IP " << IP.c_str() << ":" << port.c_str() << std::endl;
	type = KmboxType::NET;
	kmboxNet = { IP, port, uuid };

	return;
}

void Kmbox::SendCommand(const std::string& command) {
	DWORD bytesWritten;
	if (!WriteFile(serialHandle, command.c_str(), command.length(), &bytesWritten, NULL))
		std::cout << "Failed to send command to KMBOX!" << std::endl;
}

void Kmbox::SendCommandNet(const std::string& command, const std::string& IP, const std::string& port) {
	if (!sendto(socketHandle, command.c_str(), command.length(), 0, reinterpret_cast<sockaddr*>(&destination), sizeof(destination)))
		std::cout << "Failed to send command to KMBOX Net!" << std::endl;
}

void Kmbox::Move(int x, int y) {
	if (type == KmboxType::None) {
		std::cout << "KMBOX not connected?" << std::endl;
		return;
	}

	if (type == KmboxType::USB) {
		std::string command = "km.move(" + std::to_string(x) + "," + std::to_string(y) + ")\r\n";
		SendCommand(command);
		return;
	}

	if (type == KmboxType::NET) {
		std::stringstream command;
		auto addInt = [&command](unsigned int value) {
			command.write(reinterpret_cast<const char*>(&value), sizeof(unsigned int));
			};

		addInt(StrToHex(kmboxNet.uuid.c_str(), 4));
		addInt(rand());
		addInt(indexPts++);
		addInt(0xAEDE7345);
		addInt(0);
		addInt(x);
		addInt(y);
		addInt(0);
		for (int i = 0; i < 10; ++i) {
			addInt(0);
		}

		SendCommandNet(command.str(), kmboxNet.IP, kmboxNet.port);
		return;
	}
}

void Kmbox::MoveTo(int x, int y) {
	POINT currentPos;
	if (!GetCursorPos(&currentPos)) {
		std::cout << "No se pudo obtener la posición actual del cursor." << std::endl;
		return;
	}

	int dx = x - currentPos.x;
	int dy = y - currentPos.y;

	Move(dx, dy);
}

void Kmbox::ClickLeft() {
	if (type != KmboxType::NET) return;

	auto sendMousePacket = [&](int buttonFlags) {
		std::stringstream packet;
		auto addInt = [&](unsigned int value) {
			packet.write(reinterpret_cast<const char*>(&value), sizeof(unsigned int));
			};

		addInt(StrToHex(kmboxNet.uuid.c_str(), 4));
		addInt(rand());
		addInt(indexPts++);
		addInt(0xAEDE7345); // same as Move
		addInt(buttonFlags); // This is where we set the mouse button flags (e.g., left click)
		addInt(0); // x
		addInt(0); // y
		addInt(0); // wheel

		for (int i = 0; i < 10; ++i)
			addInt(0);

		SendCommandNet(packet.str(), kmboxNet.IP, kmboxNet.port);
		};

	sendMousePacket(0x01); // Left button down
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	sendMousePacket(0x00); // Button up
}

void Kmbox::ClickRight() {
	if (type != KmboxType::NET) return;

	auto sendMousePacket = [&](int buttonFlags) {
		std::stringstream packet;
		auto addInt = [&](unsigned int value) {
			packet.write(reinterpret_cast<const char*>(&value), sizeof(unsigned int));
			};

		addInt(StrToHex(kmboxNet.uuid.c_str(), 4));
		addInt(rand());
		addInt(indexPts++);
		addInt(0xAEDE7345); // same mouse command
		addInt(buttonFlags); // 0x02 for right click
		addInt(0); // x
		addInt(0); // y
		addInt(0); // wheel

		for (int i = 0; i < 10; ++i)
			addInt(0);

		SendCommandNet(packet.str(), kmboxNet.IP, kmboxNet.port);
		};

	sendMousePacket(0x02); // Right down
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	sendMousePacket(0x00); // Button up
}

void Kmbox::MouseDownLeft() {
	if (type != KmboxType::NET) return;

	std::stringstream packet;
	auto addInt = [&](unsigned int value) {
		packet.write(reinterpret_cast<const char*>(&value), sizeof(unsigned int));
		};

	addInt(StrToHex(kmboxNet.uuid.c_str(), 4));
	addInt(rand());
	addInt(indexPts++);
	addInt(0xAEDE7345); // Mouse command
	addInt(0x01);       // Button down (left)
	addInt(0);          // x
	addInt(0);          // y
	addInt(0);          // wheel
	for (int i = 0; i < 10; ++i)
		addInt(0);

	SendCommandNet(packet.str(), kmboxNet.IP, kmboxNet.port);
}

void Kmbox::MouseUpLeft() {
	if (type != KmboxType::NET) return;

	std::stringstream packet;
	auto addInt = [&](unsigned int value) {
		packet.write(reinterpret_cast<const char*>(&value), sizeof(unsigned int));
		};

	addInt(StrToHex(kmboxNet.uuid.c_str(), 4));
	addInt(rand());
	addInt(indexPts++);
	addInt(0xAEDE7345); // Mouse command
	addInt(0x00);       // No button pressed
	addInt(0);
	addInt(0);
	addInt(0);
	for (int i = 0; i < 10; ++i)
		addInt(0);

	SendCommandNet(packet.str(), kmboxNet.IP, kmboxNet.port);
}

void Kmbox::SendKeyboardPacket(uint8_t ctrl, const std::vector<uint8_t>& keys) {
	if (type != KmboxType::NET) return;

	char packet[28] = { 0 };
	uint32_t mac = StrToHex(kmboxNet.uuid.c_str(), 4);
	uint32_t randVal = rand();
	uint32_t cmd_keyboard_all = 0x123c2c2f;

	memcpy(packet + 0, &mac, 4);
	memcpy(packet + 4, &randVal, 4);
	memcpy(packet + 8, &indexPts, 4);
	memcpy(packet + 12, &cmd_keyboard_all, 4);
	indexPts++;

	packet[16] = ctrl;
	packet[17] = 0x00;

	for (int i = 0; i < 10; ++i) {
		packet[18 + i] = (i < keys.size()) ? keys[i] : 0x00;
	}

	sendto(socketHandle, packet, sizeof(packet), 0, (sockaddr*)&destination, sizeof(destination));
}

void Kmbox::HoldKey(const std::string& key) {
	int keycode = getKeyCode(key);
	if (keycode == 0) return;

	uint8_t ctrl = 0;
	std::vector<uint8_t> keys;

	if (keycode >= 0xE0 && keycode <= 0xE7) {
		ctrl = 1 << (keycode - 0xE0);
	}
	else {
		keys.push_back(static_cast<uint8_t>(keycode));
	}

	SendKeyboardPacket(ctrl, keys);
}

void Kmbox::ReleaseKey(const std::string&) {
	SendKeyboardPacket(0, {});
}

void Kmbox::Type(const std::string& text) {
	for (char c : text) {
		std::string letter(1, c);
		HoldKey(letter);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		ReleaseKey(letter);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}