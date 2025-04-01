#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <string>
#include <thread>
#include <setupapi.h>
#include <devguid.h>
#include <iostream>
#include <sstream>
#include <vector>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "ws2_32.lib")

struct KmboxNetData {
	std::string IP;
	std::string port;
	std::string uuid;
};

class Kmbox {
private:
	void SendKeyboardPacket(uint8_t ctrl, const std::vector<uint8_t>& keys);

	unsigned int StrToHex(const char* pbSrc, int nLen) {
		char h1, h2;
		unsigned char s1, s2;
		unsigned int pbDest[4] = { 0 };
		for (int i = 0; i < nLen; i++) {
			h1 = pbSrc[2 * i];
			h2 = pbSrc[2 * i + 1];
			s1 = toupper(h1) - 0x30;
			if (s1 > 9)
				s1 -= 7;
			s2 = toupper(h2) - 0x30;
			if (s2 > 9)
				s2 -= 7;
			pbDest[i] = s1 * 16 + s2;
		}
		return pbDest[0] << 24 | pbDest[1] << 16 | pbDest[2] << 8 | pbDest[3];
	}

	typedef enum KmboxType {
		None,
		USB,
		NET
	};

	//std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();

	HANDLE serialHandle = nullptr;
	SOCKET socketHandle = INVALID_SOCKET;
	sockaddr_in destination;

	int indexPts;

	KmboxNetData kmboxNet;
	KmboxType type = KmboxType::None;

public:
	std::string FindPort(const std::string& targetDescription);
	void Initialize(LPCSTR port);
	void InitializeNet(const std::string& IP, std::string port, const std::string& uuid);

	void SendCommand(const std::string& command);
	void SendCommandNet(const std::string& command, const std::string& IP, const std::string& port);

	void Move(int x, int y);
	void MoveTo(int x, int y);

	void ClickLeft();
	void ClickRight();

	void Type(const std::string& text);

	void MouseDownLeft();
	void MouseUpLeft();

	void HoldKey(const std::string& key);
	void ReleaseKey(const std::string& key);

}; inline Kmbox kmbox;