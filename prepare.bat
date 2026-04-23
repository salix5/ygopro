git clone https://github.com/salix5/irrlicht.git
git clone --depth=1 -b 0.11.25 https://github.com/mackron/miniaudio
copy /Y miniaudio\extras\miniaudio_split\miniaudio.* miniaudio

curl -L -O https://www.lua.org/ftp/lua-5.4.8.tar.gz
tar -xf lua-5.4.8.tar.gz
rename lua-5.4.8 lua
del lua-5.4.8.tar.gz

curl -L -O https://downloads.sourceforge.net/freetype/freetype-2.14.2.tar.gz
tar -xf freetype-2.14.2.tar.gz
rename freetype-2.14.2 freetype
del freetype-2.14.2.tar.gz

curl -L -O https://github.com/libevent/libevent/releases/download/release-2.1.12-stable/libevent-2.1.12-stable.tar.gz
tar -xf libevent-2.1.12-stable.tar.gz
rename libevent-2.1.12-stable event
del libevent-2.1.12-stable.tar.gz
copy /Y premake\event\msvc-event-config.h event\include\event2\event-config.h
copy /Y event\WIN32-Code\nmake\evconfig-private.h event\include\evconfig-private.h

curl -L -O https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.1.4.1/libjpeg-turbo-3.1.4.1.tar.gz
tar -xf libjpeg-turbo-3.1.4.1.tar.gz
rename libjpeg-turbo-3.1.4.1 jpeg
del libjpeg-turbo-3.1.4.1.tar.gz
copy /Y jpeg\src\jversion.h.in jpeg\src\jversion.h

curl -L -O https://www.sqlite.org/2026/sqlite-amalgamation-3510300.zip
tar -xf sqlite-amalgamation-3510300.zip
rename sqlite-amalgamation-3510300 sqlite3
del sqlite-amalgamation-3510300.zip
