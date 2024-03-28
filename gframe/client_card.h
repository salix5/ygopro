#ifndef CLIENT_CARD_H
#define CLIENT_CARD_H

#include "config.h"
#include "../ocgcore/card_data.h"
#include <unordered_map>

namespace ygo {

using CardData = card_data;
struct CardDataC : card_data {
	unsigned int ot{};
	unsigned int category{};
};
struct CardString {
	std::wstring name;
	std::wstring text;
	std::wstring desc[16];
};
typedef std::unordered_map<unsigned int, CardDataC>::const_iterator code_pointer;
typedef std::unordered_map<unsigned int, CardString>::const_iterator string_pointer;

class ClientCard {
public:
	static bool deck_sort_lv(code_pointer l1, code_pointer l2);
	static bool deck_sort_atk(code_pointer l1, code_pointer l2);
	static bool deck_sort_def(code_pointer l1, code_pointer l2);
	static bool deck_sort_name(code_pointer l1, code_pointer l2);
};

}

#endif //CLIENT_CARD_H
