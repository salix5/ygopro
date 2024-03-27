#include <stack>
#include "client_field.h"
#include "client_card.h"
#include "data_manager.h"
#include "image_manager.h"
#include "game.h"
#include "materials.h"
#include "../ocgcore/common.h"

namespace ygo {

ClientField::ClientField() {
	for(int p = 0; p < 2; ++p) {
		mzone[p].resize(7, 0);
		szone[p].resize(8, 0);
	}
	rnd.reset((uint_fast32_t)std::random_device()());
}
ClientField::~ClientField() {
	for (int i = 0; i < 2; ++i) {
		for (auto& card : deck[i]) {
			delete card;
		}
		deck[i].clear();
		for (auto& card : hand[i]) {
			delete card;
		}
		hand[i].clear();
		for (auto& card : mzone[i]) {
			if (card)
				delete card;
			card = nullptr;
		}
		for (auto& card : szone[i]) {
			if (card)
				delete card;
			card = nullptr;
		}
		for (auto& card : grave[i]) {
			delete card;
		}
		grave[i].clear();
		for (auto& card : remove[i]) {
			delete card;
		}
		remove[i].clear();

		for (auto& card : extra[i]) {
			delete card;
		}
		extra[i].clear();
	}
	for (auto& card : overlay_cards) {
		delete card;
	}
	overlay_cards.clear();
}
void ClientField::Clear() {
	for(int i = 0; i < 2; ++i) {
		for(auto cit = deck[i].begin(); cit != deck[i].end(); ++cit)
			delete *cit;
		deck[i].clear();
		for(auto cit = hand[i].begin(); cit != hand[i].end(); ++cit)
			delete *cit;
		hand[i].clear();
		for(auto cit = mzone[i].begin(); cit != mzone[i].end(); ++cit) {
			if(*cit)
				delete *cit;
			*cit = 0;
		}
		for(auto cit = szone[i].begin(); cit != szone[i].end(); ++cit) {
			if(*cit)
				delete *cit;
			*cit = 0;
		}
		for(auto cit = grave[i].begin(); cit != grave[i].end(); ++cit)
			delete *cit;
		grave[i].clear();
		for(auto cit = remove[i].begin(); cit != remove[i].end(); ++cit)
			delete *cit;
		remove[i].clear();
		for(auto cit = extra[i].begin(); cit != extra[i].end(); ++cit)
			delete *cit;
		extra[i].clear();
	}
	for(auto sit = overlay_cards.begin(); sit != overlay_cards.end(); ++sit)
		delete *sit;
	overlay_cards.clear();
	extra_p_count[0] = 0;
	extra_p_count[1] = 0;
	player_desc_hints[0].clear();
	player_desc_hints[1].clear();
	chains.clear();
	activatable_cards.clear();
	summonable_cards.clear();
	spsummonable_cards.clear();
	msetable_cards.clear();
	ssetable_cards.clear();
	reposable_cards.clear();
	attackable_cards.clear();
	disabled_field = 0;
	panel = 0;
	hovered_card = 0;
	clicked_card = 0;
	highlighting_card = 0;
	menu_card = 0;
	hovered_controler = 0;
	hovered_location = 0;
	hovered_sequence = 0;
	deck_act = false;
	grave_act = false;
	remove_act = false;
	extra_act = false;
	pzone_act[0] = false;
	pzone_act[1] = false;
	conti_act = false;
	deck_reversed = false;
	cant_check_grave = false;
}
void ClientField::Initial(int player, int deckc, int extrac) {
	ClientCard* pcard;
	for(int i = 0; i < deckc; ++i) {
		pcard = new ClientCard;
		deck[player].push_back(pcard);
		pcard->owner = player;
		pcard->controler = player;
		pcard->location = 0x1;
		pcard->sequence = i;
		pcard->position = POS_FACEDOWN_DEFENSE;
	}
	for(int i = 0; i < extrac; ++i) {
		pcard = new ClientCard;
		extra[player].push_back(pcard);
		pcard->owner = player;
		pcard->controler = player;
		pcard->location = 0x40;
		pcard->sequence = i;
		pcard->position = POS_FACEDOWN_DEFENSE;
	}
}
ClientCard* ClientField::GetCard(int controler, int location, int sequence, int sub_seq) {
	std::vector<ClientCard*>* lst = 0;
	bool is_xyz = (location & LOCATION_OVERLAY) != 0;
	location &= 0x7f;
	switch(location) {
	case LOCATION_DECK:
		lst = &deck[controler];
		break;
	case LOCATION_HAND:
		lst = &hand[controler];
		break;
	case LOCATION_MZONE:
		lst = &mzone[controler];
		break;
	case LOCATION_SZONE:
		lst = &szone[controler];
		break;
	case LOCATION_GRAVE:
		lst = &grave[controler];
		break;
	case LOCATION_REMOVED:
		lst = &remove[controler];
		break;
	case LOCATION_EXTRA:
		lst = &extra[controler];
		break;
	}
	if(!lst)
		return 0;
	if(is_xyz) {
		if(sequence >= (int)lst->size())
			return 0;
		ClientCard* scard = (*lst)[sequence];
		if(scard && (int)scard->overlayed.size() > sub_seq)
			return scard->overlayed[sub_seq];
		else
			return 0;
	} else {
		if(sequence >= (int)lst->size())
			return 0;
		return (*lst)[sequence];
	}
}
template <class T>
static bool is_declarable(T const& cd, const std::vector<int>& opcode) {
	std::stack<int> stack;
	for(auto it = opcode.begin(); it != opcode.end(); ++it) {
		switch(*it) {
		case OPCODE_ADD: {
			if (stack.size() >= 2) {
				int rhs = stack.top();
				stack.pop();
				int lhs = stack.top();
				stack.pop();
				stack.push(lhs + rhs);
			}
			break;
		}
		case OPCODE_SUB: {
			if (stack.size() >= 2) {
				int rhs = stack.top();
				stack.pop();
				int lhs = stack.top();
				stack.pop();
				stack.push(lhs - rhs);
			}
			break;
		}
		case OPCODE_MUL: {
			if (stack.size() >= 2) {
				int rhs = stack.top();
				stack.pop();
				int lhs = stack.top();
				stack.pop();
				stack.push(lhs * rhs);
			}
			break;
		}
		case OPCODE_DIV: {
			if (stack.size() >= 2) {
				int rhs = stack.top();
				stack.pop();
				int lhs = stack.top();
				stack.pop();
				stack.push(lhs / rhs);
			}
			break;
		}
		case OPCODE_AND: {
			if (stack.size() >= 2) {
				int rhs = stack.top();
				stack.pop();
				int lhs = stack.top();
				stack.pop();
				stack.push(lhs && rhs);
			}
			break;
		}
		case OPCODE_OR: {
			if (stack.size() >= 2) {
				int rhs = stack.top();
				stack.pop();
				int lhs = stack.top();
				stack.pop();
				stack.push(lhs || rhs);
			}
			break;
		}
		case OPCODE_NEG: {
			if (stack.size() >= 1) {
				int val = stack.top();
				stack.pop();
				stack.push(-val);
			}
			break;
		}
		case OPCODE_NOT: {
			if (stack.size() >= 1) {
				int val = stack.top();
				stack.pop();
				stack.push(!val);
			}
			break;
		}
		case OPCODE_ISCODE: {
			if (stack.size() >= 1) {
				int code = stack.top();
				stack.pop();
				stack.push(cd.code == code);
			}
			break;
		}
		case OPCODE_ISSETCARD: {
			if (stack.size() >= 1) {
				int set_code = stack.top();
				stack.pop();
				bool res = cd.is_setcode(set_code);
				stack.push(res);
			}
			break;
		}
		case OPCODE_ISTYPE: {
			if (stack.size() >= 1) {
				int val = stack.top();
				stack.pop();
				stack.push(cd.type & val);
			}
			break;
		}
		case OPCODE_ISRACE: {
			if (stack.size() >= 1) {
				int race = stack.top();
				stack.pop();
				stack.push(cd.race & race);
			}
			break;
		}
		case OPCODE_ISATTRIBUTE: {
			if (stack.size() >= 1) {
				int attribute = stack.top();
				stack.pop();
				stack.push(cd.attribute & attribute);
			}
			break;
		}
		default: {
			stack.push(*it);
			break;
		}
		}
	}
	if(stack.size() != 1 || stack.top() == 0)
		return false;
	return cd.code == CARD_MARINE_DOLPHIN || cd.code == CARD_TWINKLE_MOSS
		|| (!cd.alias && (cd.type & (TYPE_MONSTER + TYPE_TOKEN)) != (TYPE_MONSTER + TYPE_TOKEN));
}
}
