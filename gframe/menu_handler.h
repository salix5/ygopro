#ifndef MENU_HANDLER_H
#define MENU_HANDLER_H

#include <IEventReceiver.h>
#include "replay.h"

namespace ygo {

class Game;

class MenuHandler: public irr::IEventReceiver {
public:
	MenuHandler(Game* game);
	bool OnEvent(const irr::SEvent& event) override;
	void EnableReplayWindow(bool enabled);

	irr::s32 prev_operation{ 0 };
	irr::s32 save_operation{ 0 };
	Replay temp_replay;

private:
	void UpdateDeck();

	Game* game_{ nullptr };
};

}

#endif //MENU_HANDLER_H
