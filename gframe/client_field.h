#ifndef CLIENT_FIELD_H
#define CLIENT_FIELD_H

#include "config.h"
#include "../ocgcore/mtrandom.h"
#include <vector>
#include <set>
#include <map>

namespace ygo {

class ClientField: public irr::IEventReceiver {
public:
	virtual bool OnEvent(const irr::SEvent& event);
	virtual bool OnCommonEvent(const irr::SEvent& event);
};

}

//special cards
#define CARD_MARINE_DOLPHIN	78734254
#define CARD_TWINKLE_MOSS	13857930
#define CARD_QUESTION		38723936

#endif //CLIENT_FIELD_H
