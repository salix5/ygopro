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
