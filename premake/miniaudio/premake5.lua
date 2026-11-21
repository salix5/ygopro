project "miniaudio"
    kind "StaticLib"
    files { "miniaudio.c", "miniaudio.h" }
    defines { "MA_NO_ENCODING", "MA_NO_GENERATION" }
