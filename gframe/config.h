#ifndef YGOPRO_CONFIG_H
#define YGOPRO_CONFIG_H

#define VERSION_MAJOR 1
#define VERSION_MINOR 8
#define VERSION_PATCH 0

#include <cstdio>
#include "bufferio.h"
#include "../ocgcore/common.h"

#ifdef _WIN32
#define mywcsncasecmp _wcsnicmp
#define mystrncasecmp _strnicmp
#else
#define mywcsncasecmp wcsncasecmp
#define mystrncasecmp strncasecmp
#endif

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

constexpr uint16_t PRO_VERSION = 0x1362;

#endif // YGOPRO_CONFIG_H
