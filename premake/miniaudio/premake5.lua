project "miniaudio"
    kind "StaticLib"
    files { "miniaudio.h", "extras/stb_vorbis.c", "miniaudio_impl.c" }
    defines { "MA_NO_ENCODING", "MA_NO_GENERATION" }
