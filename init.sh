#!/bin/bash
git submodule update --init ocgcore
git -C ocgcore checkout master

git clone https://github.com/salix5/irrlicht.git
git clone --depth=1 -b 0.11.25 https://github.com/mackron/miniaudio
cp miniaudio/extras/miniaudio_split/miniaudio.* miniaudio

rm -rf lua
LUA_VERSION=5.4.8
curl -q -f -L -O "https://www.lua.org/ftp/lua-${LUA_VERSION}.tar.gz"
tar -xf lua-${LUA_VERSION}.tar.gz
mv lua-${LUA_VERSION} lua
rm lua-${LUA_VERSION}.tar.gz

rm -rf freetype
FREETYPE_VERSION=2.14.3
curl -q -f -L -O "https://downloads.sourceforge.net/freetype/freetype-${FREETYPE_VERSION}.tar.gz"
tar -xf freetype-${FREETYPE_VERSION}.tar.gz
mv freetype-${FREETYPE_VERSION} freetype
rm freetype-${FREETYPE_VERSION}.tar.gz

rm -rf event
LIBEVENT_VERSION=2.1.13-stable
curl -q -f -L -O "https://github.com/libevent/libevent/releases/download/release-${LIBEVENT_VERSION}/libevent-${LIBEVENT_VERSION}.tar.gz"
tar -xf libevent-${LIBEVENT_VERSION}.tar.gz
mv libevent-${LIBEVENT_VERSION} event
rm libevent-${LIBEVENT_VERSION}.tar.gz
cp premake/event/msvc-event-config.h event/include/event2/event-config.h
cp event/WIN32-Code/nmake/evconfig-private.h event/include/evconfig-private.h

rm -rf jpeg
LIBJPEG_VERSION=3.2.0
curl -q -f -L -O "https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/${LIBJPEG_VERSION}/libjpeg-turbo-${LIBJPEG_VERSION}.tar.gz"
tar -xf libjpeg-turbo-${LIBJPEG_VERSION}.tar.gz
mv libjpeg-turbo-${LIBJPEG_VERSION} jpeg
rm libjpeg-turbo-${LIBJPEG_VERSION}.tar.gz
cp jpeg/src/jversion.h.in jpeg/src/jversion.h

rm -rf sqlite3
SQLITE_YEAR=2026
SQLITE_VERSION=3530300
curl -q -f -L -O "https://www.sqlite.org/${SQLITE_YEAR}/sqlite-amalgamation-${SQLITE_VERSION}.zip"
unzip sqlite-amalgamation-${SQLITE_VERSION}.zip
mv sqlite-amalgamation-${SQLITE_VERSION} sqlite3
rm sqlite-amalgamation-${SQLITE_VERSION}.zip

rm -rf png
LIBPNG_VERSION=1.6.58
curl -q -f -L -O "https://downloads.sourceforge.net/libpng/libpng-${LIBPNG_VERSION}.tar.gz"
tar -xf libpng-${LIBPNG_VERSION}.tar.gz
mv libpng-${LIBPNG_VERSION} png
cp png/scripts/pnglibconf.h.prebuilt png/pnglibconf.h
rm libpng-${LIBPNG_VERSION}.tar.gz

rm -rf zlib
ZLIB_VERSION=1.3.2
curl -q -f -L -O "https://github.com/madler/zlib/releases/download/v${ZLIB_VERSION}/zlib-${ZLIB_VERSION}.tar.gz"
tar -xf zlib-${ZLIB_VERSION}.tar.gz
mv zlib-${ZLIB_VERSION} zlib
rm zlib-${ZLIB_VERSION}.tar.gz

rm -rf lzma
XZ_VERSION=5.8.3
curl -q -f -L -O "https://github.com/tukaani-project/xz/releases/download/v${XZ_VERSION}/xz-${XZ_VERSION}.tar.gz"
tar -xf xz-${XZ_VERSION}.tar.gz
mv xz-${XZ_VERSION} lzma
rm xz-${XZ_VERSION}.tar.gz
