#!/bin/bash
git submodule update --init ocgcore
git -C ocgcore checkout master

git clone https://github.com/salix5/irrlicht.git
git clone --depth=1 -b 0.11.25 https://github.com/mackron/miniaudio
cp miniaudio/extras/miniaudio_split/miniaudio.* miniaudio

rm -rf lua
curl -q -f -L -O https://www.lua.org/ftp/lua-5.4.8.tar.gz
tar -xf lua-5.4.8.tar.gz
mv lua-5.4.8 lua
rm lua-5.4.8.tar.gz

rm -rf freetype
curl -q -f -L -O https://downloads.sourceforge.net/freetype/freetype-2.14.2.tar.gz
tar -xf freetype-2.14.2.tar.gz
mv freetype-2.14.2 freetype
rm freetype-2.14.2.tar.gz

rm -rf event
curl -q -f -L -O https://github.com/libevent/libevent/releases/download/release-2.1.12-stable/libevent-2.1.12-stable.tar.gz
tar -xf libevent-2.1.12-stable.tar.gz
mv libevent-2.1.12-stable event
rm libevent-2.1.12-stable.tar.gz
cp premake/event/msvc-event-config.h event/include/event2/event-config.h
cp event/WIN32-Code/nmake/evconfig-private.h event/include/evconfig-private.h

rm -rf jpeg
curl -q -f -L -O https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.1.4.1/libjpeg-turbo-3.1.4.1.tar.gz
tar -xf libjpeg-turbo-3.1.4.1.tar.gz
mv libjpeg-turbo-3.1.4.1 jpeg
rm libjpeg-turbo-3.1.4.1.tar.gz
cp jpeg/src/jversion.h.in jpeg/src/jversion.h

rm -rf sqlite3
curl -q -f -L -O https://www.sqlite.org/2026/sqlite-amalgamation-3510300.zip
unzip sqlite-amalgamation-3510300.zip
mv sqlite-amalgamation-3510300 sqlite3
rm sqlite-amalgamation-3510300.zip

rm -rf png
curl -q -f -L -O https://downloads.sourceforge.net/libpng/libpng-1.6.58.tar.gz
tar -xf libpng-1.6.58.tar.gz
mv libpng-1.6.58 png
cp png/scripts/pnglibconf.h.prebuilt png/pnglibconf.h
rm libpng-1.6.58.tar.gz

rm -rf zlib
curl -q -f -L -O https://github.com/madler/zlib/releases/download/v1.3.2/zlib-1.3.2.tar.gz
tar -xf zlib-1.3.2.tar.gz
mv zlib-1.3.2 zlib
rm zlib-1.3.2.tar.gz

rm -rf lzma
curl -q -f -L -O https://github.com/tukaani-project/xz/releases/download/v5.8.3/xz-5.8.3.tar.gz
tar -xf xz-5.8.3.tar.gz
mv xz-5.8.3 lzma
rm xz-5.8.3.tar.gz
