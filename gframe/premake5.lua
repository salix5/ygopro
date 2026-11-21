project "YGOPro"
    kind "WindowedApp"
    rtti "Off"
    if USE_OPENMP then
        openmp "On"
    end

    dofile("../irrlicht/defines.lua")
    files { "*.cpp", "*.h" }

    includedirs { "../ocgcore" }
    links { "ocgcore" }


    if BUILD_FREETYPE then
        -- Add custom include directory for FreeType before the default include directory
        includedirs { FREETYPE_CUSTOM_INCLUDE_DIR }
    end

    for _, dep in ipairs(DEPENDENCIES_METADATA) do
        local upper = string.upper(dep.name)
        includedirs { _G[upper .. "_INCLUDE_DIR"] }
        if _G["BUILD_" .. upper] then
            -- When building from source, the dependencies will be linked with their project names, which can't be changed via options.
            links { dep.name }
        else
            links { _G[upper .. "_LIB_NAME"] }
            libdirs { _G[upper .. "_LIB_DIR"] }
        end
    end

    if not BUILD_EVENT and not os.istarget("windows") then
        links { EVENT_PTHREADS_LIB_NAME }
    end

    if USE_SIMD == "none" then
        defines { "STBIR_NO_SIMD" }
    end

    if USE_AUDIO then
        defines { "YGOPRO_USE_AUDIO" }
        if AUDIO_LIB == "miniaudio" then
            defines { "YGOPRO_USE_MINIAUDIO" }
            includedirs { MINIAUDIO_INCLUDE_DIR }
            links { "miniaudio" }
        end
    end

    filter "system:windows"
        files "ygopro.rc"
        links { "ws2_32", "iphlpapi", "winmm" }
        defines { "NOMINMAX=1", "WIN32_LEAN_AND_MEAN" }
        if USE_DXSDK then
            defines { "IRR_COMPILE_WITH_DX9_DEV_PACK" }
        else
            defines { "NO_IRR_COMPILE_WITH_DIRECT3D_9_" }
        end

    filter "not action:vs*"
        cppdialect "C++14"

    filter { "system:windows", "action:gmake" }
        links { "opengl32", "imm32" }
        linkoptions { "-static" }

    filter "system:macosx"
        links { "OpenGL.framework", "Cocoa.framework", "IOKit.framework", "Carbon.framework" }
        defines { "GL_SILENCE_DEPRECATION" }

    filter "system:linux"
        links { "GL", "X11", "dl", "pthread" }
