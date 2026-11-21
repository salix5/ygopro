project "YGOPro"
    kind "WindowedApp"
    cppdialect "C++17"
    rtti "Off"
    if USE_OPENMP then
        openmp "On"
    end

    dofile("../irrlicht/defines.lua")
    files { "*.cpp", "*.h" }

    includedirs { "../ocgcore" }
    links { "ocgcore" }

    includedirs { IRRLICHT_INCLUDE_DIR }
    links { "irrlicht" }

    includedirs { SQLITE3_INCLUDE_DIR }
    links { "sqlite3" }

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

    if USE_AUDIO then
        defines { "YGOPRO_USE_AUDIO" }
        if AUDIO_LIB == "miniaudio" then
            defines { "YGOPRO_USE_MINIAUDIO" }
            includedirs { _MAIN_SCRIPT_DIR .. "/miniaudio" }
            links { "miniaudio" }
            if MINIAUDIO_SUPPORT_OPUS_VORBIS then
                defines { "YGOPRO_MINIAUDIO_SUPPORT_OPUS_VORBIS" }
                includedirs { _MAIN_SCRIPT_DIR .. "/miniaudio/extras/decoders/libvorbis" }
                links {
                    "vorbisfile",
                    "vorbis",
                    "ogg"
                }
            end
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

    filter { "system:windows", "action:gmake" }
        links { "opengl32", "imm32" }
        linkoptions { "-static" }

    filter "system:macosx"
        links { "OpenGL.framework", "Cocoa.framework", "IOKit.framework", "Carbon.framework" }
        defines { "GL_SILENCE_DEPRECATION" }

    filter "system:linux"
        links { "GL", "X11", "dl", "pthread" }
        fatalwarnings { "All" }

    filter { "system:windows", "action:vs*", "platforms:Win32", "configurations:Release" }
        libdirs { _MAIN_SCRIPT_DIR .. "/vcpkg_installed/x86-windows-static/lib" }
    filter { "system:windows", "action:vs*", "platforms:Win32", "configurations:Debug" }
        libdirs { _MAIN_SCRIPT_DIR .. "/vcpkg_installed/x86-windows-static/debug/lib" }

    filter { "system:windows", "action:vs*", "platforms:x64", "configurations:Release" }
        libdirs { _MAIN_SCRIPT_DIR .. "/vcpkg_installed/x64-windows-static/lib" }
    filter { "system:windows", "action:vs*", "platforms:x64", "configurations:Debug" }
        libdirs { _MAIN_SCRIPT_DIR .. "/vcpkg_installed/x64-windows-static/debug/lib" }

    filter { "system:windows", "action:vs*", "platforms:ARM64", "configurations:Release" }
        libdirs { _MAIN_SCRIPT_DIR .. "/vcpkg_installed/arm64-windows-static/lib" }
    filter { "system:windows", "action:vs*", "platforms:ARM64", "configurations:Debug" }
        libdirs { _MAIN_SCRIPT_DIR .. "/vcpkg_installed/arm64-windows-static/debug/lib" }

    filter { "system:windows", "action:gmake" }
        libdirs { _MAIN_SCRIPT_DIR .. "/vcpkg_installed/x64-mingw-static/lib" }

    filter { "system:windows", "action:vs*", "configurations:Release" }
        links { "jpeg", "libpng16", "libzs" }

    filter { "system:windows", "action:vs*", "configurations:Debug" }
        links { "jpeg", "libpng16d", "libzsd" }
    
    filter { "action:gmake" }
        links { "jpeg", "png", "z" }
