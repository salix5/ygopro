project "miniaudio"
    kind "StaticLib"
    files { "miniaudio.c", "miniaudio.h" }
    defines { "MA_NO_ENCODING", "MA_NO_GENERATION" }

    if USE_SIMD == "none" then
        defines { "MA_NO_SSE2", "MA_NO_AVX2", "MA_NO_NEON" }
    end
