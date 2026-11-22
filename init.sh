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

rm -rf sqlite3
SQLITE_YEAR=2026
SQLITE_VERSION=3530400
curl -q -f -L -O "https://www.sqlite.org/${SQLITE_YEAR}/sqlite-amalgamation-${SQLITE_VERSION}.zip"
unzip sqlite-amalgamation-${SQLITE_VERSION}.zip
mv sqlite-amalgamation-${SQLITE_VERSION} sqlite3
rm sqlite-amalgamation-${SQLITE_VERSION}.zip

rm -rf lzma
XZ_VERSION=5.8.3
curl -q -f -L -O "https://github.com/tukaani-project/xz/releases/download/v${XZ_VERSION}/xz-${XZ_VERSION}.tar.gz"
tar -xf xz-${XZ_VERSION}.tar.gz
mv xz-${XZ_VERSION} lzma
rm xz-${XZ_VERSION}.tar.gz
