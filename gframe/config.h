#ifndef YGOPRO_CONFIG_H
#define YGOPRO_CONFIG_H

#define VERSION_MAJOR 1
#define VERSION_MINOR 7
#define VERSION_PATCH 2

#define IRR_COMPILE_WITH_DX9_DEV_PACK

#include <cerrno>

#ifdef _WIN32

#define NOMINMAX 1
#include <WinSock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
#define mywcsncasecmp _wcsnicmp
#define mystrncasecmp _strnicmp
#else
#define mywcsncasecmp wcsncasecmp
#define mystrncasecmp strncasecmp
#endif

#define socklen_t int

#else //_WIN32

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define SD_BOTH 2
#define SOCKET int
#define closesocket close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define SOCKADDR_IN sockaddr_in
#define SOCKADDR sockaddr
#define SOCKET_ERRNO() (errno)

#define mywcsncasecmp wcsncasecmp
#define mystrncasecmp strncasecmp
#endif

#include <cstdio>
#include "bufferio.h"
#include "../ocgcore/ocgapi.h"

template<size_t N, typename... TR>
inline int myswprintf(wchar_t(&buf)[N], const wchar_t* fmt, TR... args) {
	if constexpr (sizeof...(args) == 0)
		return std::swprintf(buf, N, L"%ls", fmt);
	else
		return std::swprintf(buf, N, fmt, args...);
}
template<size_t N, typename... TR>
inline int mysnprintf(char(&buf)[N], const char* fmt, TR... args) {
	if constexpr (sizeof...(args) == 0)
		return std::snprintf(buf, N, "%s", fmt);
	else
		return std::snprintf(buf, N, fmt, args...);
}

inline FILE* mywfopen(const wchar_t* filename, const char* mode) {
	char fname[1024]{};
	std::mbstate_t state{};
	std::wcsrtombs(fname, &filename, sizeof fname, &state);
	if (filename != nullptr)
		return nullptr;
	return std::fopen(fname, mode);
}

#include <irrlicht.h>

constexpr uint16_t PRO_VERSION = 0x1362;

#endif
