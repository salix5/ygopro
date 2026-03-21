#if _WIN32
#include "fillpics.h"
#include "data_manager.h"
#include "game.h"
#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>

typedef long long sqlite3_int64;
typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

typedef HRESULT (WINAPI *URLDownloadToFileA_fn)(void*, const char*, const char*, DWORD, void*);

namespace ygo{
    
bool fillpics::has_pic(sqlite3_int64 id) {
    const std::string pic_path = "pics";
    return std::ifstream(pic_path + "/" + std::to_string(id) + ".jpg").good();
}

bool fillpics::download_pic(sqlite3_int64 id) {
    if(pic_source_url == "") return false;

    const std::string download_url = pic_source_url + std::to_string(id) + ".jpg";
    const std::string savepath = "pics/" + std::to_string(id) + ".jpg";

    HMODULE urlmon = LoadLibraryA("urlmon.dll");
    if (!urlmon) {
        char error_msg[] = "Failed to load urlmon.dll\n";
        mainGame->ErrorLog(error_msg);
        return false;
    }

    URLDownloadToFileA_fn pURLDownloadToFileA =
        (URLDownloadToFileA_fn)GetProcAddress(urlmon, "URLDownloadToFileA");
    if (!pURLDownloadToFileA) {
        char error_msg[] = "Failed to get URLDownloadToFileA\n";
		mainGame->ErrorLog(error_msg);
        FreeLibrary(urlmon);
        return false;
    }

    HRESULT hr = pURLDownloadToFileA(nullptr, download_url.c_str(), savepath.c_str(), 0, nullptr);
    FreeLibrary(urlmon);

    if (FAILED(hr)) {
		char error_msg[128];
		sprintf(error_msg, "Failed id %lld, error code 0x%08X, url:%s\n", id, static_cast<unsigned long>(hr), download_url.c_str());
        mainGame->ErrorLog(error_msg);
        return false;
    }

    return true;
}

bool fillpics::getCardDb(std::string s)
{
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        if (db) {
            sqlite3_close(db);
        }
        return false;
    }
    return true;
}

sqlite3_int64 fillpics::get_card_size()
{
    sqlite3_stmt* stmt_getsize;
    const char* sql_getsize = "SELECT COUNT(*) FROM texts;";
    if (sqlite3_prepare_v2(db, sql_getsize, -1, &stmt_getsize, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return false;
    }
    const int rc = sqlite3_step(stmt_getsize);
    if (rc == SQLITE_ROW) {
        card_size = sqlite3_column_int64(stmt_getsize, 0);
    }
	return card_size;
}

bool fillpics::fetch_and_fill_pic(std::string qry)
{
    if (pic_source_url == "" || pic_source_url == "N") {
		std::wstring wmsg = dataManager.GetSysString(1804);
		mainGame->progressFillPic->setText(wmsg.c_str());
		return false;
    }
    const wchar_t* msg_prefix = dataManager.GetSysString(1803);
    std::wstring wmsg_prefix(msg_prefix);
    sqlite3_stmt* stmt = nullptr;
    const char* sql = qry.c_str();
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return false;
    }
	uint16_t progress = 0;
    while (true) {

        const int rc = sqlite3_step(stmt);
        sqlite3_int64 id = 0;
        progress++;
        if (rc == SQLITE_ROW) {
            id = sqlite3_column_int64(stmt, 0);
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return false;
        }
        std::wstring progress_msg = wmsg_prefix + L" " + std::to_wstring(progress) + L"/" + std::to_wstring(card_size);
		mainGame->progressFillPic->setText(progress_msg.c_str());
        if (!has_pic(id)) {
            download_pic(id);
        }
    }
    sqlite3_finalize(stmt);
    return true;
}

// getCardDb => prepare stmt
fillpics::fillpics(std::string dbPath) {
    db = NULL;
    db_path = dbPath;
	getCardDb(dbPath);
    std::wstring ws(mainGame->gameConf.pic_source_url);
	pic_source_url = std::string(ws.begin(), ws.end());
}

fillpics::~fillpics(){
    if(db != NULL)
        sqlite3_close(db);
    
}

}

#endif
