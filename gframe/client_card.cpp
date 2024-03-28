#include "client_card.h"
#include "data_manager.h"

namespace ygo {

bool ClientCard::deck_sort_lv(code_pointer p1, code_pointer p2) {
	if((p1->second.type & 0x7) != (p2->second.type & 0x7))
		return (p1->second.type & 0x7) < (p2->second.type & 0x7);
	if((p1->second.type & 0x7) == 1) {
		int type1 = (p1->second.type & 0x48020c0) ? (p1->second.type & 0x48020c1) : (p1->second.type & 0x31);
		int type2 = (p2->second.type & 0x48020c0) ? (p2->second.type & 0x48020c1) : (p2->second.type & 0x31);
		if(type1 != type2)
			return type1 < type2;
		if(p1->second.level != p2->second.level)
			return p1->second.level > p2->second.level;
		if(p1->second.attack != p2->second.attack)
			return p1->second.attack > p2->second.attack;
		if(p1->second.defense != p2->second.defense)
			return p1->second.defense > p2->second.defense;
		return p1->first < p2->first;
	}
	if((p1->second.type & 0xfffffff8) != (p2->second.type & 0xfffffff8))
		return (p1->second.type & 0xfffffff8) < (p2->second.type & 0xfffffff8);
	return p1->first < p2->first;
}
bool ClientCard::deck_sort_atk(code_pointer p1, code_pointer p2) {
	if((p1->second.type & 0x7) != (p2->second.type & 0x7))
		return (p1->second.type & 0x7) < (p2->second.type & 0x7);
	if((p1->second.type & 0x7) == 1) {
		if(p1->second.attack != p2->second.attack)
			return p1->second.attack > p2->second.attack;
		if(p1->second.defense != p2->second.defense)
			return p1->second.defense > p2->second.defense;
		if(p1->second.level != p2->second.level)
			return p1->second.level > p2->second.level;
		int type1 = (p1->second.type & 0x48020c0) ? (p1->second.type & 0x48020c1) : (p1->second.type & 0x31);
		int type2 = (p2->second.type & 0x48020c0) ? (p2->second.type & 0x48020c1) : (p2->second.type & 0x31);
		if(type1 != type2)
			return type1 < type2;
		return p1->first < p2->first;
	}
	if((p1->second.type & 0xfffffff8) != (p2->second.type & 0xfffffff8))
		return (p1->second.type & 0xfffffff8) < (p2->second.type & 0xfffffff8);
	return p1->first < p2->first;
}
bool ClientCard::deck_sort_def(code_pointer p1, code_pointer p2) {
	if((p1->second.type & 0x7) != (p2->second.type & 0x7))
		return (p1->second.type & 0x7) < (p2->second.type & 0x7);
	if((p1->second.type & 0x7) == 1) {
		if(p1->second.defense != p2->second.defense)
			return p1->second.defense > p2->second.defense;
		if(p1->second.attack != p2->second.attack)
			return p1->second.attack > p2->second.attack;
		if(p1->second.level != p2->second.level)
			return p1->second.level > p2->second.level;
		int type1 = (p1->second.type & 0x48020c0) ? (p1->second.type & 0x48020c1) : (p1->second.type & 0x31);
		int type2 = (p2->second.type & 0x48020c0) ? (p2->second.type & 0x48020c1) : (p2->second.type & 0x31);
		if(type1 != type2)
			return type1 < type2;
		return p1->first < p2->first;
	}
	if((p1->second.type & 0xfffffff8) != (p2->second.type & 0xfffffff8))
		return (p1->second.type & 0xfffffff8) < (p2->second.type & 0xfffffff8);
	return p1->first < p2->first;
}
bool ClientCard::deck_sort_name(code_pointer p1, code_pointer p2) {
	const wchar_t* name1 = dataManager.GetName(p1->first);
	const wchar_t* name2 = dataManager.GetName(p2->first);
	int res = wcscmp(name1, name2);
	if(res != 0)
		return res < 0;
	return p1->first < p2->first;
}
}
