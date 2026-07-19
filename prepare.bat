git submodule update ocgcore
git -C ocgcore checkout master

git clone https://github.com/salix5/irrlicht.git
git clone --depth=1 -b 0.11.25 https://github.com/mackron/miniaudio
copy /Y miniaudio\extras\miniaudio_split\miniaudio.* miniaudio

rmdir /S /Q lua
curl -L -O https://www.lua.org/ftp/lua-5.4.8.tar.gz
tar -xf lua-5.4.8.tar.gz
rename lua-5.4.8 lua
del lua-5.4.8.tar.gz

rmdir /S /Q freetype
curl -L -O https://downloads.sourceforge.net/freetype/freetype-2.14.2.tar.gz
tar -xf freetype-2.14.2.tar.gz
rename freetype-2.14.2 freetype
del freetype-2.14.2.tar.gz

rmdir /S /Q event
curl -L -O https://github.com/libevent/libevent/releases/download/release-2.1.12-stable/libevent-2.1.12-stable.tar.gz
tar -xf libevent-2.1.12-stable.tar.gz
rename libevent-2.1.12-stable event
del libevent-2.1.12-stable.tar.gz
copy /Y premake\event\msvc-event-config.h event\include\event2\event-config.h
copy /Y event\WIN32-Code\nmake\evconfig-private.h event\include\evconfig-private.h

rmdir /S /Q jpeg
curl -L -O https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.1.4.1/libjpeg-turbo-3.1.4.1.tar.gz
tar -xf libjpeg-turbo-3.1.4.1.tar.gz
rename libjpeg-turbo-3.1.4.1 jpeg
del libjpeg-turbo-3.1.4.1.tar.gz
copy /Y jpeg\src\jversion.h.in jpeg\src\jversion.h

rmdir /S /Q sqlite3
curl -L -O https://www.sqlite.org/2026/sqlite-amalgamation-3510300.zip
tar -xf sqlite-amalgamation-3510300.zip
rename sqlite-amalgamation-3510300 sqlite3
del sqlite-amalgamation-3510300.zip

rmdir /S /Q png
curl -L -O https://downloads.sourceforge.net/libpng/libpng-1.6.58.tar.gz
tar -xf libpng-1.6.58.tar.gz
rename libpng-1.6.58 png
copy /Y png\scripts\pnglibconf.h.prebuilt png\pnglibconf.h
del libpng-1.6.58.tar.gz

rmdir /S /Q zlib
curl -L -O https://github.com/madler/zlib/releases/download/v1.3.2/zlib-1.3.2.tar.gz
tar -xf zlib-1.3.2.tar.gz
rename zlib-1.3.2 zlib
del zlib-1.3.2.tar.gz

rmdir /S /Q lzma
curl -L -O https://github.com/tukaani-project/xz/releases/download/v5.8.3/xz-5.8.3.tar.gz
tar -xf xz-5.8.3.tar.gz
rename xz-5.8.3 lzma
del xz-5.8.3.tar.gz
