project "sqlite"
    kind "StaticLib"

    files { "sqlite3.c", "sqlite3.h" }
    defines {
        "SQLITE_DQS=0",
        "SQLITE_THREADSAFE=0",
        "SQLITE_DEFAULT_MEMSTATUS=0",
        "SQLITE_LIKE_DOESNT_MATCH_BLOBS",
        "SQLITE_MAX_EXPR_DEPTH=0",
        "SQLITE_OMIT_DECLTYPE",
        "SQLITE_OMIT_DEPRECATED",
        "SQLITE_OMIT_PROGRESS_CALLBACK",
        "SQLITE_OMIT_SHARED_CACHE",
        "SQLITE_OMIT_LOAD_EXTENSION",
        "SQLITE_TRUSTED_SCHEMA=0",
        "SQLITE_STRICT_SUBTYPE=1",
    }
