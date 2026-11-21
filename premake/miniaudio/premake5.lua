project "miniaudio"
    kind "StaticLib"
    files { "miniaudio.c", "miniaudio.h" }
    defines { "MA_NO_ENCODING", "MA_NO_GENERATION" }

    if MINIAUDIO_SUPPORT_VORBIS then
        files { "extras/decoders/libvorbis/*" }
    end
