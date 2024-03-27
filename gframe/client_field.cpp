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
		GetCardLocation(pcard, &pcard->curPos, &pcard->curRot, true);
	}
	for(int i = 0; i < extrac; ++i) {
		pcard = new ClientCard;
		extra[player].push_back(pcard);
		pcard->owner = player;
		pcard->controler = player;
		pcard->location = 0x40;
		pcard->sequence = i;
		pcard->position = POS_FACEDOWN_DEFENSE;
		GetCardLocation(pcard, &pcard->curPos, &pcard->curRot, true);
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
void ClientField::GetCardLocation(ClientCard* pcard, irr::core::vector3df* t, irr::core::vector3df* r, bool setTrans) {
	int controler = pcard->controler;
	int sequence = pcard->sequence;
	int location = pcard->location;
	int rule = (mainGame->dInfo.duel_rule >= 4) ? 1 : 0;
	const float overlay_buttom = 0.0f;
	const float material_height = 0.003f;
	const float mzone_buttom = 0.020f;
	switch (location) {
	case LOCATION_DECK: {
		t->X = (matManager.vFieldDeck[controler][0].Pos.X + matManager.vFieldDeck[controler][1].Pos.X) / 2;
		t->Y = (matManager.vFieldDeck[controler][0].Pos.Y + matManager.vFieldDeck[controler][2].Pos.Y) / 2;
		t->Z = 0.01f + 0.01f * sequence;
		if (controler == 0) {
			if(deck_reversed == pcard->is_reversed) {
				r->X = 0.0f;
				r->Y = 3.1415926f;
				r->Z = 0.0f;
			} else {
				r->X = 0.0f;
				r->Y = 0.0f;
				r->Z = 0.0f;
			}
		} else {
			if(deck_reversed == pcard->is_reversed) {
				r->X = 0.0f;
				r->Y = 3.1415926f;
				r->Z = 3.1415926f;
			} else {
				r->X = 0.0f;
				r->Y = 0.0f;
				r->Z = 3.1415926f;
			}
		}
		break;
	}
	case 0:
	case LOCATION_HAND: {
		int count = hand[controler].size();
		if (controler == 0) {
			if (count <= 6)
				t->X = (5.5f - 0.8f * count) / 2 + 1.55f + sequence * 0.8f;
			else
				t->X = 1.9f + sequence * 4.0f / (count - 1);
			if (pcard->is_hovered) {
				t->Y = 3.84f;
				t->Z = 0.656f + 0.001f * sequence;
			} else {
				t->Y = 4.0f;
				t->Z = 0.5f + 0.001f * sequence;
			}
			if(pcard->code) {
				r->X = -0.798056f;
				r->Y = 0.0f;
				r->Z = 0.0f;
			} else {
				r->X = 0.798056f;
				r->Y = 3.1415926f;
				r->Z = 0;
			}
		} else {
			if (count <= 6)
				t->X = 6.25f - (5.5f - 0.8f * count) / 2 - sequence * 0.8f;
			else
				t->X = 5.9f - sequence * 4.0f / (count - 1);
			if (pcard->is_hovered) {
				t->Y = -3.56f;
				t->Z = 0.656f - 0.001f * sequence;
			} else {
				t->Y = -3.4f;
				t->Z = 0.5f - 0.001f * sequence;
			}
			if (pcard->code == 0) {
				r->X = 0.798056f;
				r->Y = 3.1415926f;
				r->Z = 0;
			} else {
				r->X = -0.798056f;
				r->Y = 0;
				r->Z = 0;
			}
		}
		break;
	}
	case LOCATION_MZONE: {
		t->X = (matManager.vFieldMzone[controler][sequence][0].Pos.X + matManager.vFieldMzone[controler][sequence][1].Pos.X) / 2;
		t->Y = (matManager.vFieldMzone[controler][sequence][0].Pos.Y + matManager.vFieldMzone[controler][sequence][2].Pos.Y) / 2;
		t->Z = mzone_buttom;
		if (controler == 0) {
			if (pcard->position & POS_DEFENSE) {
				r->X = 0.0f;
				r->Z = -3.1415926f / 2.0f;
				if (pcard->position & POS_FACEDOWN)
					r->Y = 3.1415926f + 0.001f;
				else r->Y = 0.0f;
			} else {
				r->X = 0.0f;
				r->Z = 0.0f;
				if (pcard->position & POS_FACEDOWN)
					r->Y = 3.1415926f;
				else r->Y = 0.0f;
			}
		} else {
			if (pcard->position & POS_DEFENSE) {
				r->X = 0.0f;
				r->Z = 3.1415926f / 2.0f;
				if (pcard->position & POS_FACEDOWN)
					r->Y = 3.1415926f + 0.001f;
				else r->Y = 0.0f;
			} else {
				r->X = 0.0f;
				r->Z = 3.1415926f;
				if (pcard->position & POS_FACEDOWN)
					r->Y = 3.1415926f;
				else r->Y = 0.0f;
			}
		}
		break;
	}
	case LOCATION_SZONE: {
		t->X = (matManager.vFieldSzone[controler][sequence][rule][0].Pos.X + matManager.vFieldSzone[controler][sequence][rule][1].Pos.X) / 2;
		t->Y = (matManager.vFieldSzone[controler][sequence][rule][0].Pos.Y + matManager.vFieldSzone[controler][sequence][rule][2].Pos.Y) / 2;
		t->Z = 0.01f;
		if (controler == 0) {
			r->X = 0.0f;
			r->Z = 0.0f;
			if (pcard->position & POS_FACEDOWN)
				r->Y = 3.1415926f;
			else r->Y = 0.0f;
		} else {
			r->X = 0.0f;
			r->Z = 3.1415926f;
			if (pcard->position & POS_FACEDOWN)
				r->Y = 3.1415926f;
			else r->Y = 0.0f;
		}
		break;
	}
	case LOCATION_GRAVE: {
		t->X = (matManager.vFieldGrave[controler][rule][0].Pos.X + matManager.vFieldGrave[controler][rule][1].Pos.X) / 2;
		t->Y = (matManager.vFieldGrave[controler][rule][0].Pos.Y + matManager.vFieldGrave[controler][rule][2].Pos.Y) / 2;
		t->Z = 0.01f + 0.01f * sequence;
		if (controler == 0) {
			r->X = 0.0f;
			r->Y = 0.0f;
			r->Z = 0.0f;
		} else {
			r->X = 0.0f;
			r->Y = 0.0f;
			r->Z = 3.1415926f;
		}
		break;
	}
	case LOCATION_REMOVED: {
		t->X = (matManager.vFieldRemove[controler][rule][0].Pos.X + matManager.vFieldRemove[controler][rule][1].Pos.X) / 2;
		t->Y = (matManager.vFieldRemove[controler][rule][0].Pos.Y + matManager.vFieldRemove[controler][rule][2].Pos.Y) / 2;
		t->Z = 0.01f + 0.01f * sequence;
		if (controler == 0) {
			if(pcard->position & POS_FACEUP) {
				r->X = 0.0f;
				r->Y = 0.0f;
				r->Z = 0.0f;
			} else {
				r->X = 0.0f;
				r->Y = 3.1415926f;
				r->Z = 0.0f;
			}
		} else {
			if(pcard->position & POS_FACEUP) {
				r->X = 0.0f;
				r->Y = 0.0f;
				r->Z = 3.1415926f;
			} else {
				r->X = 0.0f;
				r->Y = 3.1415926f;
				r->Z = 3.1415926f;
			}
		}
		break;
	}
	case LOCATION_EXTRA: {
		t->X = (matManager.vFieldExtra[controler][0].Pos.X + matManager.vFieldExtra[controler][1].Pos.X) / 2;
		t->Y = (matManager.vFieldExtra[controler][0].Pos.Y + matManager.vFieldExtra[controler][2].Pos.Y) / 2;
		t->Z = 0.01f + 0.01f * sequence;
		if (controler == 0) {
			r->X = 0.0f;
			if(pcard->position & POS_FACEUP)
				r->Y = 0.0f;
			else r->Y = 3.1415926f;
			r->Z = 0.0f;
		} else {
			r->X = 0.0f;
			if(pcard->position & POS_FACEUP)
				r->Y = 0.0f;
			else r->Y = 3.1415926f;
			r->Z = 3.1415926f;
		}
		break;
	}
	case LOCATION_OVERLAY: {
		if (pcard->overlayTarget->location != LOCATION_MZONE) {
			return;
		}
		int oseq = pcard->overlayTarget->sequence;
		int mseq = (sequence < MAX_LAYER_COUNT) ? sequence : (MAX_LAYER_COUNT - 1);
		if (pcard->overlayTarget->controler == 0) {
			t->X = (matManager.vFieldMzone[0][oseq][0].Pos.X + matManager.vFieldMzone[0][oseq][1].Pos.X) / 2 - 0.12f + 0.06f * mseq;
			t->Y = (matManager.vFieldMzone[0][oseq][0].Pos.Y + matManager.vFieldMzone[0][oseq][2].Pos.Y) / 2 + 0.05f;
			t->Z = overlay_buttom + mseq * material_height;
			r->X = 0.0f;
			r->Y = 0.0f;
			r->Z = 0.0f;
		} else {
			t->X = (matManager.vFieldMzone[1][oseq][0].Pos.X + matManager.vFieldMzone[1][oseq][1].Pos.X) / 2 + 0.12f - 0.06f * mseq;
			t->Y = (matManager.vFieldMzone[1][oseq][0].Pos.Y + matManager.vFieldMzone[1][oseq][2].Pos.Y) / 2 - 0.05f;
			t->Z = overlay_buttom + mseq * material_height;
			r->X = 0.0f;
			r->Y = 0.0f;
			r->Z = 3.1415926f;
		}
		break;
	}
	}
	if(setTrans) {
		pcard->mTransform.setTranslation(*t);
		pcard->mTransform.setRotationRadians(*r);
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
