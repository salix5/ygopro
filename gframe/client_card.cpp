#include "client_card.h"
#include "client_field.h"
#include "data_manager.h"
#include "game.h"

namespace ygo {

ClientCard::~ClientCard() {
	ClearTarget();
	if (equipTarget) {
		equipTarget->is_showequip = false;
		equipTarget->equipped.erase(this);
		equipTarget = nullptr;
	}
	for (auto& card : equipped) {
		card->is_showequip = false;
		card->equipTarget = nullptr;
	}
	equipped.clear();
	if (overlayTarget) {
		for (auto it = overlayTarget->overlayed.begin(); it != overlayTarget->overlayed.end(); ) {
			if (*it == this) {
				it = overlayTarget->overlayed.erase(it);
			}
			else
				++it;
		}
		overlayTarget = nullptr;
	}
	for (auto& card : overlayed) {
		card->overlayTarget = nullptr;
	}
	overlayed.clear();
}
void ClientCard::SetCode(int code) {
	if((location == LOCATION_HAND) && (this->code != (unsigned int)code)) {
		this->code = code;
	} else
		this->code = code;
}
void ClientCard::ClearTarget() {
	for(auto cit = cardTarget.begin(); cit != cardTarget.end(); ++cit) {
		(*cit)->is_showtarget = false;
		(*cit)->ownerTarget.erase(this);
	}
	for(auto cit = ownerTarget.begin(); cit != ownerTarget.end(); ++cit) {
		(*cit)->is_showtarget = false;
		(*cit)->cardTarget.erase(this);
	}
	cardTarget.clear();
	ownerTarget.clear();
}
void ClientCard::ClearData() {
	alias = 0;
	type = 0;
	level = 0;
	rank = 0;
	race = 0;
	attribute = 0;
	attack = 0;
	defense = 0;
	base_attack = 0;
	base_defense = 0;
	lscale = 0;
	rscale = 0;
	link = 0;
	link_marker = 0;
	status = 0;
	
	atkstring[0] = 0;
	defstring[0] = 0;
	lvstring[0] = 0;
	linkstring[0] = 0;
	rscstring[0] = 0;
	lscstring[0] = 0;
	counters.clear();
}
bool ClientCard::client_card_sort(ClientCard* c1, ClientCard* c2) {
	if(c1->is_selected != c2->is_selected)
		return c1->is_selected < c2->is_selected;
	int32 cp1 = c1->overlayTarget ? c1->overlayTarget->controler : c1->controler;
	int32 cp2 = c2->overlayTarget ? c2->overlayTarget->controler : c2->controler;
	if(cp1 != cp2)
		return cp1 < cp2;
	if(c1->location != c2->location)
		return c1->location < c2->location;
	if (c1->location & LOCATION_OVERLAY) {
		if (c1->overlayTarget != c2->overlayTarget)
			return c1->overlayTarget->sequence < c2->overlayTarget->sequence;
		else
			return c1->sequence < c2->sequence;
	}
	else {
		if(c1->location & (LOCATION_DECK | LOCATION_GRAVE | LOCATION_REMOVED | LOCATION_EXTRA)) {
			auto it1 = std::find_if(mainGame->dField.chains.rbegin(), mainGame->dField.chains.rend(), [c1](const ChainInfo& ch) {
				return c1 == ch.chain_card || ch.target.find(c1) != ch.target.end();
				});
			auto it2 = std::find_if(mainGame->dField.chains.rbegin(), mainGame->dField.chains.rend(), [c2](const ChainInfo& ch) {
				return c2 == ch.chain_card || ch.target.find(c2) != ch.target.end();
				});
			if(it1 != mainGame->dField.chains.rend() || it2 != mainGame->dField.chains.rend()) {
				return it1 < it2;
			}
			return c1->sequence > c2->sequence;
		}
		else
			return c1->sequence < c2->sequence;
	}
}
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
