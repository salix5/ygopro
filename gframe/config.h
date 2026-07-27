#ifndef YGOPRO_CONFIG_H
#define YGOPRO_CONFIG_H

#define VERSION_MAJOR 1
#define VERSION_MINOR 7
#define VERSION_PATCH 2

#include <cerrno>
#include <cstdio>
#include <string>
#include "bufferio.h"
#include "../ocgcore/ocgapi.h"

#ifdef _WIN32

#if defined(_MSC_VER) || defined(__MINGW32__)
#define mywcsncasecmp _wcsnicmp
#define mystrncasecmp _strnicmp
#else
#define mywcsncasecmp wcsncasecmp
#define mystrncasecmp strncasecmp
#endif

#else //_WIN32

#define mywcsncasecmp wcsncasecmp
#define mystrncasecmp strncasecmp

#endif // _WIN32

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

#include <irrlicht.h>

constexpr uint16_t PRO_VERSION = 0x1362;

#endif // YGOPRO_CONFIG_H
