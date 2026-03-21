#ifndef FILLPICS_H
#define FILLPICS_H

#if _WIN32

#include <iostream>
#include <sqlite3.h>

namespace ygo{
class fillpics
{
    private:
        sqlite3* db;
        std::string pic_source_url;
        std::string db_path;
        bool getCardDb(std::string path);
		sqlite3_int64 card_size;
    public:
        bool download_pic(sqlite3_int64 id);
        sqlite3_int64 get_card_size();
        bool fetch_and_fill_pic(std::string qry = "SELECT id FROM texts;");
        bool has_pic(sqlite3_int64 id);
        fillpics(std::string dbPath);
        ~fillpics();
};
    

}

#endif
#endif