#include <algorithm>
#include <charconv>
#include "deck_manager.h"
#include "data_manager.h"
#include "game.h"
#include "myfilesystem.h"
#include "network.h"

namespace ygo {

DeckManager deckManager;

void DeckManager::LoadLFListSingle(const char* path) {
	auto cur = _lfList.rend();
	FILE* fp = std::fopen(path, "r");
	char linebuf[1024]{};
	wchar_t strBuffer[256]{};
	uint32_t pointHash{};
	auto credit_hash = [](const char* s) -> uint32_t {
		uint32_t h = 2166136261u;
		for(auto p = s; *p; ++p) {
			h ^= static_cast<unsigned char>(*p);
			h *= 16777619u;
		}
		return h;
	};
	auto credit_update_hash = [](uint32_t h, uint32_t a, uint32_t b, uint32_t c) -> uint32_t {
		return h ^ ((a << 18) | (a >> 14)) ^ ((b << 9) | (b >> 23)) ^ ((c << 27) | (c >> 5));
	};
	auto code_update_hash = [](uint32_t hash, uint32_t code, uint32_t count)-> uint32_t {
		return hash ^ ((code << 18) | (code >> 14)) ^ ((code << (27 + count)) | (code >> (5 - count)));
	};
	if(fp) {
		while(std::fgets(linebuf, sizeof linebuf, fp)) {
			if(linebuf[0] == '#')
				continue;
			if(linebuf[0] == '!') {
				auto len = std::strcspn(linebuf, "\r\n");
				linebuf[len] = 0;
				BufferIO::DecodeUTF8(linebuf + 1, strBuffer);
				LFList newlist;
				newlist.listName = strBuffer;
				newlist.hash = 0x7dfcee6a;
				_lfList.push_back(newlist);
				cur = _lfList.rbegin();
				continue;
			}
			if (cur == _lfList.rend())
				continue;
			if (linebuf[0] == 'M' && linebuf[1] == ' ') {
				errno = 0;
				auto type = std::strtoul(linebuf + 2, nullptr, 16);
				if (errno || type > UINT32_MAX)
					continue;
				cur->noMonsterType = static_cast<uint32_t>(type);
				cur->hash = code_update_hash(cur->hash, cur->noMonsterType, 3);
				continue;
			}
			if(linebuf[0] == '$') {
				int limitValue = 0;
				char keybuf[256];
				if (std::sscanf(linebuf, "$%255[^ \t\n] %d", keybuf, &limitValue) != 2)
					continue;
				if (limitValue < 0)
					limitValue = 0;
				cur->pointList.push_back({ keybuf, limitValue });
				pointHash = credit_hash(keybuf);
				cur->hash = credit_update_hash(cur->hash, pointHash, static_cast<uint32_t>(limitValue), 0x43524544u);
				continue;
			}
			char* pos = linebuf;
			char* end = nullptr;
			errno = 0;
			auto result = std::strtoul(pos, &end, 10);
			if (errno || result > UINT32_MAX || end == pos)
				continue;
			uint32_t code = static_cast<uint32_t>(result);
			int creditValue = 0;
			if (std::sscanf(end, " $%*[^ \t\n] %d", &creditValue) == 1) {
				if (cur->pointList.empty())
					continue;
				if (creditValue <= 0)
					continue;
				auto& point = cur->pointList.back();
				point.table[code] = creditValue;
				cur->hash = credit_update_hash(cur->hash, code, pointHash, static_cast<uint32_t>(creditValue));
				continue;
			}
			pos = end;
			end = nullptr;
			errno = 0;
			int count = std::strtol(pos, &end, 10);
			if (errno || end == pos)
				continue;
			if (count < 0 || count > 2)
				continue;
			cur->content[code] = count;
			cur->hash = code_update_hash(cur->hash, code, count);
		}
		std::fclose(fp);
	}
}
void DeckManager::LoadLFList() {
	LoadLFListSingle("expansions/lflist.conf");
	LoadLFListSingle("lflist.conf");
	LFList nolimit;
	nolimit.listName = L"N/A";
	nolimit.hash = 0;
	_lfList.push_back(nolimit);
}
const wchar_t* DeckManager::GetLFListName(unsigned int lfhash) {
	auto lit = std::find_if(_lfList.begin(), _lfList.end(), [lfhash](const ygo::LFList& list) {
		return list.hash == lfhash;
	});
	if(lit != _lfList.end())
		return lit->listName.c_str();
	return dataManager.unknown_string;
}
const LFList* DeckManager::GetLFList(unsigned int lfhash) {
	auto lit = std::find_if(_lfList.begin(), _lfList.end(), [lfhash](const ygo::LFList& list) {
		return list.hash == lfhash;
	});
	if (lit != _lfList.end())
		return &(*lit);
	return nullptr;
}
static unsigned int checkAvail(unsigned int ot, unsigned int avail) {
	if((ot & avail) == avail)
		return 0;
	if((ot & AVAIL_OCG) && (avail != AVAIL_OCG))
		return DECKERROR_OCGONLY;
	if((ot & AVAIL_TCG) && (avail != AVAIL_TCG))
		return DECKERROR_TCGONLY;
	return DECKERROR_NOTAVAIL;
}
uint32_t DeckManager::CheckDeck(const Deck& deck, unsigned int lfhash, size_t rule) {
	std::unordered_map<uint32_t, int> ccount;
	// rule
	if(deck.main.size() < DECK_MIN_SIZE || deck.main.size() > DECK_MAX_SIZE)
		return (DECKERROR_MAINCOUNT << 28) | (unsigned)deck.main.size();
	if(deck.extra.size() > EXTRA_MAX_SIZE)
		return (DECKERROR_EXTRACOUNT << 28) | (unsigned)deck.extra.size();
	if(deck.side.size() > SIDE_MAX_SIZE)
		return (DECKERROR_SIDECOUNT << 28) | (unsigned)deck.side.size();
	auto lflist = GetLFList(lfhash);
	if (!lflist)
		return 0;
	auto& list = lflist->content;
	const unsigned int rule_map[6] = { AVAIL_OCG, AVAIL_TCG, AVAIL_SC, AVAIL_CUSTOM, AVAIL_OCGTCG, 0 };
	unsigned int avail = 0;
	if (rule < sizeof rule_map / sizeof rule_map[0])
		avail = rule_map[rule];
	for (auto& cit : deck.main) {
		auto gameruleDeckError = checkAvail(cit->ot, avail);
		if(gameruleDeckError)
			return (gameruleDeckError << 28) | cit->code;
		if (cit->type & (TYPES_EXTRA_DECK | TYPE_TOKEN))
			return (DECKERROR_MAINCOUNT << 28);
		auto code = cit->get_duel_code();
		ccount[code]++;
		int dc = ccount[code];
		if(dc > 3)
			return (DECKERROR_CARDCOUNT << 28) | cit->code;
		auto it = list.find(code);
		if(it != list.end() && dc > it->second)
			return (DECKERROR_LFLIST << 28) | cit->code;
		if ((cit->type & TYPE_MONSTER) && (cit->type & lflist->noMonsterType))
			return (DECKERROR_LFLIST << 28) | cit->code;
	}
	for (auto& cit : deck.extra) {
		auto gameruleDeckError = checkAvail(cit->ot, avail);
		if(gameruleDeckError)
			return (gameruleDeckError << 28) | cit->code;
		if (!(cit->type & TYPES_EXTRA_DECK) || cit->type & TYPE_TOKEN)
			return (DECKERROR_EXTRACOUNT << 28);
		auto code = cit->get_duel_code();
		ccount[code]++;
		int dc = ccount[code];
		if(dc > 3)
			return (DECKERROR_CARDCOUNT << 28) | cit->code;
		auto it = list.find(code);
		if(it != list.end() && dc > it->second)
			return (DECKERROR_LFLIST << 28) | cit->code;
		if ((cit->type & TYPE_MONSTER) && (cit->type & lflist->noMonsterType))
			return (DECKERROR_LFLIST << 28) | cit->code;
	}
	for (auto& cit : deck.side) {
		auto gameruleDeckError = checkAvail(cit->ot, avail);
		if(gameruleDeckError)
			return (gameruleDeckError << 28) | cit->code;
		if (cit->type & TYPE_TOKEN)
			return (DECKERROR_SIDECOUNT << 28);
		auto code = cit->get_duel_code();
		ccount[code]++;
		int dc = ccount[code];
		if(dc > 3)
			return (DECKERROR_CARDCOUNT << 28) | cit->code;
		auto it = list.find(code);
		if(it != list.end() && dc > it->second)
			return (DECKERROR_LFLIST << 28) | cit->code;
		if ((cit->type & TYPE_MONSTER) && (cit->type & lflist->noMonsterType))
			return (DECKERROR_LFLIST << 28) | cit->code;
	}
	std::vector<int> sum = GetDeckPoint(deck, lflist);
	int result = 0;
	for (size_t i = 0; i < lflist->pointList.size(); ++i) {
		if (sum[i] > lflist->pointList[i].limit) {
			result = sum[i];
			break;
		}
	}
	if (result) {
		uint32_t code = 0;
		if (deck.main.size())
			code = deck.main[0]->code;
		else if (deck.extra.size())
			code = deck.extra[0]->code;
		else if (deck.side.size())
			code = deck.side[0]->code;
		return (DECKERROR_LFLIST << 28) | (code & MAX_CARD_ID);
	}
	return 0;
}
uint32_t DeckManager::LoadDeck(Deck& deck, uint32_t dbuf[], uint32_t mainc, uint32_t sidec, bool is_packlist) {
	deck.clear();
	uint32_t errorcode = 0;
	auto& _datas = dataManager.GetDataTable();
	for(uint32_t i = 0; i < mainc; ++i) {
		auto code = dbuf[i];
		auto it = _datas.find(code);
		if(it == _datas.end()) {
			errorcode = code;
			continue;
		}
		auto& cd = it->second;
		if (cd.type & TYPE_TOKEN) {
			errorcode = code;
			continue;
		}
		if(is_packlist) {
			deck.main.push_back(&cd);
			continue;
		}
		if (cd.type & TYPES_EXTRA_DECK) {
			if (deck.extra.size() < EXTRA_MAX_SIZE)
				deck.extra.push_back(&cd);
		}
		else {
			if (deck.main.size() < DECK_MAX_SIZE)
				deck.main.push_back(&cd);
		}
	}
	for(uint32_t i = 0; i < sidec; ++i) {
		auto code = dbuf[mainc + i];
		auto it = _datas.find(code);
		if(it == _datas.end()) {
			errorcode = code;
			continue;
		}
		auto& cd = it->second;
		if (cd.type & TYPE_TOKEN) {
			errorcode = code;
			continue;
		}
		if(deck.side.size() < SIDE_MAX_SIZE)
			deck.side.push_back(&cd);
	}
	return errorcode;
}
uint32_t DeckManager::LoadDeckFromStream(Deck& deck, std::string_view content, bool is_packlist) {
	size_t ct = 0;
	uint32_t mainc = 0, sidec = 0;
	uint32_t cardlist[PACK_MAX_SIZE]{};
	bool is_side = false;
	while (!content.empty() && ct < PACK_MAX_SIZE) {
		auto pos = content.find('\n');
		std::string_view line = content.substr(0, pos);
		if (pos == std::string_view::npos) {
			content = {};
		}
		else {
			content = content.substr(pos + 1);
		}
		if (!line.empty() && line.back() == '\r') {
			line.remove_suffix(1);
		}
		if (line.empty())
			continue;
		if (line[0] == '#')
			continue;
		if (line[0] == '!') {
			is_side = true;
			continue;
		}
		uint32_t code = 0;
		auto res = std::from_chars(line.data(), line.data() + line.size(), code, 10);
		if (res.ec != std::errc{})
			continue;
		cardlist[ct++] = code;
		if (is_side)
			++sidec;
		else
			++mainc;
	}
	return LoadDeck(deck, cardlist, mainc, sidec, is_packlist);
}
bool DeckManager::LoadSide(Deck& deck, uint32_t dbuf[], uint32_t mainc, uint32_t sidec) {
	std::unordered_map<uint32_t, int> pcount;
	std::unordered_map<uint32_t, int> ncount;
	for(auto card : deck.main)
		pcount[card->code]++;
	for(auto card : deck.extra)
		pcount[card->code]++;
	for(auto card : deck.side)
		pcount[card->code]++;
	Deck ndeck;
	LoadDeck(ndeck, dbuf, mainc, sidec);
	if (ndeck.main.size() != deck.main.size() || ndeck.extra.size() != deck.extra.size() || ndeck.side.size() != deck.side.size())
		return false;
	for(auto card : ndeck.main)
		ncount[card->code]++;
	for(auto card : ndeck.extra)
		ncount[card->code]++;
	for(auto card : ndeck.side)
		ncount[card->code]++;
	for (auto& [code, count] : ncount)
		if (count != pcount[code])
			return false;
	deck = ndeck;
	return true;
}
void DeckManager::GetCategoryPath(wchar_t* ret, int index, const wchar_t* text) {
	switch(index) {
	case DECK_CATEGORY_PACK:
		std::wcsncpy(ret, L"./pack", 7);
		break;
	case DECK_CATEGORY_BOT:
		std::wcsncpy(ret, mainGame->gameConf.bot_deck_path, 256);
		ret[255] = 0;
		break;
	case -1:
	case DECK_CATEGORY_NONE:
	case DECK_CATEGORY_SEPARATOR:
		std::wcsncpy(ret, L"./deck", 7);
		break;
	default:
		std::swprintf(ret, 256, L"./deck/%ls", text);
		break;
	}
}
void DeckManager::GetDeckFile(wchar_t* ret, int category_index, const wchar_t* category_name, const wchar_t* deckname) {
	if (!deckname) {
		ret[0] = 0;
		return;
	}
	if (std::wcschr(deckname, L'/') || std::wcschr(deckname, L'\\')) {
		ret[0] = 0;
		return;
	}
	wchar_t filepath[256]{};
	wchar_t catepath[256]{};
	GetCategoryPath(catepath, category_index, category_name);
	if (myswprintf(filepath, L"%ls/%ls.ydk", catepath, deckname) <= 0) {
		ret[0] = 0;
		return;
	}
	std::wcsncpy(ret, filepath, 256);
	ret[255] = 0;
}
irr::io::IReadFile* DeckManager::OpenDeckReader(const wchar_t* file) {
	char file2[256];
	BufferIO::EncodeUTF8(file, file2);
	auto reader = dataManager.IrrFileSystem->createAndOpenFile(file2);
	return reader;
}
bool DeckManager::LoadCurrentDeck(const wchar_t* file, bool is_packlist) {
	current_deck.clear();
	if (!file[0])
		return false;
	char deckBuffer[MAX_YDK_SIZE]{};
	FILE* fp = mywfopen(file, "rb");
	if (fp) {
		size_t size = std::fread(deckBuffer, 1, sizeof deckBuffer, fp);
		std::fclose(fp);
		if (size >= sizeof deckBuffer)
			return false;
	}
	else if (std::wcsncmp(file, L"./pack", 6) == 0) {
		wchar_t zipfile[256]{};
		if (myswprintf(zipfile, L"%ls", file + 2) <= 0)
			return false;
		auto reader = OpenDeckReader(zipfile);
		if (!reader)
			return false;
		size_t size = reader->read(deckBuffer, sizeof deckBuffer);
		reader->drop();
		if (size >= sizeof deckBuffer)
			return false;
	}
	else
		return false;
	LoadDeckFromStream(current_deck, deckBuffer, is_packlist);
	return true;
}
bool DeckManager::LoadCurrentDeck(int category_index, const wchar_t* category_name, const wchar_t* deckname) {
	wchar_t filepath[256];
	GetDeckFile(filepath, category_index, category_name, deckname);
	bool is_packlist = (category_index == DECK_CATEGORY_PACK);
	if(!LoadCurrentDeck(filepath, is_packlist))
		return false;
	if (mainGame->is_building)
		mainGame->deckBuilder.RefreshPackListScroll();
	return true;
}
bool DeckManager::SaveDeck(const Deck& deck, const wchar_t* file) {
	if(!FileSystem::IsDirExists(L"./deck") && !FileSystem::MakeDir(L"./deck"))
		return false;
	FILE* fp = mywfopen(file, "w");
	if(!fp)
		return false;
	std::fprintf(fp, "#created by ...\n");
	std::fprintf(fp, "#main\n");
	for (size_t i = 0; i < deck.main.size(); ++i)
		std::fprintf(fp, "%u\n", deck.main[i]->code);
	std::fprintf(fp, "#extra\n");
	for (size_t i = 0; i < deck.extra.size(); ++i)
		std::fprintf(fp, "%u\n", deck.extra[i]->code);
	std::fprintf(fp, "!side\n");
	for (size_t i = 0; i < deck.side.size(); ++i)
		std::fprintf(fp, "%u\n", deck.side[i]->code);
	std::fclose(fp);
	return true;
}
bool DeckManager::DeleteDeck(const wchar_t* file) {
	return FileSystem::RemoveFile(file);
}
bool DeckManager::GenerateTestScript(const Deck& deck, const wchar_t* base_name) {
	if (!FileSystem::IsDirExists(L"./single") && !FileSystem::MakeDir(L"./single"))
		return false;
	if (std::wcschr(base_name, L'/') || std::wcschr(base_name, L'\\'))
		return false;
	if (deck.main.empty())
		return false;
	wchar_t path[256]{};
	if (myswprintf(path, L"./single/%ls.lua", base_name) <= 0)
		return false;
	FILE* fp = mywfopen(path, "w");
	if (!fp)
		return false;
	const char AI_NAME[] = "Crescent";
	const char DUEL_FLAG[] = "DUEL_SIMPLE_AI";
	std::fprintf(fp, "Debug.SetAIName('%s')\n", AI_NAME);
	std::fprintf(fp, "Debug.ReloadFieldBegin(%s,%d)\n", DUEL_FLAG, CURRENT_RULE);
	std::fprintf(fp, "Debug.SetPlayerInfo(0,8000,5,1)\n");
	std::fprintf(fp, "Debug.SetPlayerInfo(1,8000,5,1)\n");
	for (auto it = deck.main.rbegin(); it != deck.main.rend(); ++it)
		std::fprintf(fp, "Debug.AddCard(%u,0,0,LOCATION_DECK,0,POS_FACEDOWN_DEFENSE)\n", (*it)->code);
	for (auto it = deck.extra.rbegin(); it != deck.extra.rend(); ++it)
		std::fprintf(fp, "Debug.AddCard(%u,0,0,LOCATION_EXTRA,0,POS_FACEDOWN_DEFENSE)\n", (*it)->code);

	// opponent deck
	constexpr uint32_t DECK_MONSTER = 89631139; // Blue-Eyes White Dragon
	constexpr uint32_t EXTRA_MONSTER[5] = { 43227, 284224, 324483, 1546123, 23995346 };
	std::fprintf(fp, "\n");
	for (int i = 0; i < DECK_MIN_SIZE; ++i)
		std::fprintf(fp, "Debug.AddCard(%u,1,1,LOCATION_DECK,0,POS_FACEDOWN_DEFENSE)\n", DECK_MONSTER + i % 5);
	for (auto id : EXTRA_MONSTER) {
		for (int i = 0; i < 3; ++i)
			std::fprintf(fp, "Debug.AddCard(%u,1,1,LOCATION_EXTRA,0,POS_FACEDOWN_DEFENSE)\n", id);
	}
	std::fprintf(fp, "Debug.ReloadFieldEnd()\n");
	std::fclose(fp);
	return true;
}
bool DeckManager::CreateCategory(const wchar_t* name) {
	if(!FileSystem::IsDirExists(L"./deck") && !FileSystem::MakeDir(L"./deck"))
		return false;
	if(name[0] == 0)
		return false;
	if(std::wcschr(name, L'/') || std::wcschr(name, L'\\'))
		return false;
	wchar_t localname[256];
	if (myswprintf(localname, L"./deck/%ls", name) <= 0)
		return false;
	return FileSystem::MakeDir(localname);
}
bool DeckManager::RenameCategory(const wchar_t* oldname, const wchar_t* newname) {
	if(!FileSystem::IsDirExists(L"./deck") && !FileSystem::MakeDir(L"./deck"))
		return false;
	if(newname[0] == 0)
		return false;
	if (std::wcschr(oldname, L'/') || std::wcschr(oldname, L'\\'))
		return false;
	if (std::wcschr(newname, L'/') || std::wcschr(newname, L'\\'))
		return false;
	wchar_t oldlocalname[256];
	wchar_t newlocalname[256];
	if (myswprintf(oldlocalname, L"./deck/%ls", oldname) <= 0)
		return false;
	if (myswprintf(newlocalname, L"./deck/%ls", newname) <= 0)
		return false;
	return FileSystem::Rename(oldlocalname, newlocalname);
}
bool DeckManager::DeleteCategory(const wchar_t* name) {
	if (std::wcschr(name, L'/') || std::wcschr(name, L'\\'))
		return false;
	wchar_t localname[256];
	if (myswprintf(localname, L"./deck/%ls", name) <= 0)
		return false;
	if(!FileSystem::IsDirExists(localname))
		return false;
	return FileSystem::DeleteDir(localname);
}
bool DeckManager::SaveDeckArray(const DeckArray& deck, const wchar_t* name) {
	if (!FileSystem::IsDirExists(L"./deck") && !FileSystem::MakeDir(L"./deck"))
		return false;
	FILE* fp = mywfopen(name, "w");
	if (!fp)
		return false;
	std::fprintf(fp, "#created by ...\n");
	std::fprintf(fp, "#main\n");
	for (const auto& code : deck.main)
		std::fprintf(fp, "%u\n", code);
	std::fprintf(fp, "#extra\n");
	for (const auto& code : deck.extra)
		std::fprintf(fp, "%u\n", code);
	std::fprintf(fp, "!side\n");
	for (const auto& code : deck.side)
		std::fprintf(fp, "%u\n", code);
	std::fclose(fp);
	return true;
}
std::vector<int> DeckManager::GetDeckPoint(const Deck& deck, const LFList* lflist) {
	std::vector<int> sum;
	if (!lflist || lflist->pointList.empty())
		return sum;
	sum.resize(lflist->pointList.size());
	auto add_card = [&](uint32_t code){
		for (size_t i = 0; i < lflist->pointList.size(); ++i) {
			auto& point = lflist->pointList[i];
			auto it = point.table.find(code);
			if (it == point.table.end())
				continue;
			sum[i] = sum[i] + it->second;
		}
	};
	for (auto& card: deck.main){
		add_card(card->get_original_code());
	}
	for (auto& card: deck.extra){
		add_card(card->get_original_code());
	}
	for (auto& card: deck.side){
		add_card(card->get_original_code());
	}
	return sum;
}
}
