#include "config.h"
#include "game.h"
#include "client_card.h"
#include "client_field.h"
#include "math.h"
#include "network.h"
#include "duelclient.h"
#include "data_manager.h"
#include "image_manager.h"
#include "sound_manager.h"
#include "materials.h"
#include "../ocgcore/common.h"

namespace ygo {

bool ClientField::OnEvent(const irr::SEvent& event) {
	if(mainGame->OnCommonEvent(event))
		return false;
	return false;
}
void ClientField::UpdateChainButtons() {
	if(mainGame->btnChainAlways->isVisible()) {
		mainGame->btnChainIgnore->setPressed(mainGame->ignore_chain);
		mainGame->btnChainAlways->setPressed(mainGame->always_chain);
		mainGame->btnChainWhenAvail->setPressed(mainGame->chain_when_avail);
	}
}
void ClientField::ShowCancelOrFinishButton(int buttonOp) {
	if (!mainGame->gameConf.hide_hint_button && !mainGame->dInfo.isReplay) {
		switch (buttonOp) {
		case 1:
			mainGame->btnCancelOrFinish->setText(dataManager.GetSysString(1295));
			mainGame->btnCancelOrFinish->setVisible(true);
			break;
		case 2:
			mainGame->btnCancelOrFinish->setText(dataManager.GetSysString(1296));
			mainGame->btnCancelOrFinish->setVisible(true);
			break;
		case 0:
		default:
			mainGame->btnCancelOrFinish->setVisible(false);
			break;
		}
	} else {
		mainGame->btnCancelOrFinish->setVisible(false);
	}
}
void ClientField::SetShowMark(ClientCard* pcard, bool enable) {
	if(pcard->equipTarget)
		pcard->equipTarget->is_showequip = enable;
	for(auto cit = pcard->equipped.begin(); cit != pcard->equipped.end(); ++cit)
		(*cit)->is_showequip = enable;
	for(auto cit = pcard->cardTarget.begin(); cit != pcard->cardTarget.end(); ++cit)
		(*cit)->is_showtarget = enable;
	for(auto cit = pcard->ownerTarget.begin(); cit != pcard->ownerTarget.end(); ++cit)
		(*cit)->is_showtarget = enable;
	for(auto chit = chains.begin(); chit != chains.end(); ++chit) {
		if(pcard == chit->chain_card) {
			for(auto tgit = chit->target.begin(); tgit != chit->target.end(); ++tgit)
				(*tgit)->is_showchaintarget = enable;
		}
		if(chit->target.find(pcard) != chit->target.end())
			chit->chain_card->is_showchaintarget = enable;
	}
}
void ClientField::ShowCardInfoInList(ClientCard* pcard, irr::gui::IGUIElement* element, irr::gui::IGUIElement* parent) {
	std::wstring str(L"");
	wchar_t formatBuffer[2048];
	if(pcard->code) {
		str.append(dataManager.GetName(pcard->code));
	}
	if(pcard->overlayTarget) {
		myswprintf(formatBuffer, dataManager.GetSysString(225), dataManager.GetName(pcard->overlayTarget->code), pcard->overlayTarget->sequence + 1);
		str.append(L"\n").append(formatBuffer);
	}
	if((pcard->status & STATUS_PROC_COMPLETE)
		&& (pcard->type & (TYPE_RITUAL | TYPE_FUSION | TYPE_SYNCHRO | TYPE_XYZ | TYPE_LINK | TYPE_SPSUMMON)))
		str.append(L"\n").append(dataManager.GetSysString(224));
	for(auto iter = pcard->desc_hints.begin(); iter != pcard->desc_hints.end(); ++iter) {
		myswprintf(formatBuffer, L"\n*%ls", dataManager.GetDesc(iter->first));
		str.append(formatBuffer);
	}
	for(size_t i = 0; i < chains.size(); ++i) {
		auto chit = chains[i];
		if(pcard == chit.chain_card) {
			myswprintf(formatBuffer, dataManager.GetSysString(216), i + 1);
			str.append(L"\n").append(formatBuffer);
		}
		if(chit.target.find(pcard) != chit.target.end()) {
			myswprintf(formatBuffer, dataManager.GetSysString(217), i + 1, dataManager.GetName(chit.chain_card->code));
			str.append(L"\n").append(formatBuffer);
		}
	}
	if(str.length() > 0) {
		parent->addChild(mainGame->stCardListTip);
		irr::core::rect<s32> ePos = element->getRelativePosition();
		s32 x = (ePos.UpperLeftCorner.X + ePos.LowerRightCorner.X) / 2;
		s32 y = ePos.LowerRightCorner.Y;
		mainGame->SetStaticText(mainGame->stCardListTip, 320, mainGame->guiFont, str.c_str());
		irr::core::dimension2d<unsigned int> dTip = mainGame->guiFont->getDimension(mainGame->stCardListTip->getText()) + irr::core::dimension2d<unsigned int>(10, 10);
		s32 w = dTip.Width / 2;
		if(x - w < 10)
			x = w + 10;
		if(x + w > 670)
			x = 670 - w;
		mainGame->stCardListTip->setRelativePosition(recti(x - w, y - 10, x + w, y - 10 + dTip.Height));
		mainGame->stCardListTip->setVisible(true);
	}
}
void ClientField::SetResponseSelectedCards() const {
	unsigned char respbuf[SIZE_RETURN_VALUE];
	respbuf[0] = selected_cards.size();
	for (size_t i = 0; i < selected_cards.size(); ++i)
		respbuf[i + 1] = selected_cards[i]->select_seq;
	DuelClient::SetResponseB(respbuf, selected_cards.size() + 1);
}
void ClientField::SetResponseSelectedOption() const {
	if(mainGame->dInfo.curMsg == MSG_SELECT_OPTION) {
		DuelClient::SetResponseI(selected_option);
	} else {
		int index = select_options_index[selected_option];
		if(mainGame->dInfo.curMsg == MSG_SELECT_IDLECMD) {
			DuelClient::SetResponseI((index << 16) + 5);
		} else if(mainGame->dInfo.curMsg == MSG_SELECT_BATTLECMD) {
			DuelClient::SetResponseI(index << 16);
		} else {
			DuelClient::SetResponseI(index);
		}
	}
	mainGame->HideElement(mainGame->wOptions, true);
}
void ClientField::CancelOrFinish() {
	switch(mainGame->dInfo.curMsg) {
	case MSG_WAITING: {
		if(mainGame->wCardSelect->isVisible()) {
			mainGame->HideElement(mainGame->wCardSelect);
			ShowCancelOrFinishButton(0);
		}
		break;
	}
	case MSG_SELECT_BATTLECMD: {
		if(mainGame->wCardSelect->isVisible()) {
			mainGame->HideElement(mainGame->wCardSelect);
			ShowCancelOrFinishButton(0);
		}
		if(mainGame->wOptions->isVisible()) {
			mainGame->HideElement(mainGame->wOptions);
			ShowCancelOrFinishButton(0);
		}
		break;
	}
	case MSG_SELECT_IDLECMD: {
		if(mainGame->wCardSelect->isVisible()) {
			mainGame->HideElement(mainGame->wCardSelect);
			ShowCancelOrFinishButton(0);
		}
		if(mainGame->wOptions->isVisible()) {
			mainGame->HideElement(mainGame->wOptions);
			ShowCancelOrFinishButton(0);
		}
		break;
	}
	case MSG_SELECT_YESNO:
	case MSG_SELECT_EFFECTYN: {
		if(highlighting_card)
			highlighting_card->is_highlighting = false;
		highlighting_card = 0;
		DuelClient::SetResponseI(0);
		mainGame->HideElement(mainGame->wQuery, true);
		break;
	}
	case MSG_SELECT_CARD: {
		if(selected_cards.size() == 0) {
			if(select_cancelable) {
				DuelClient::SetResponseI(-1);
				ShowCancelOrFinishButton(0);
				if(mainGame->wCardSelect->isVisible())
					mainGame->HideElement(mainGame->wCardSelect, true);
				else
					DuelClient::SendResponse();
			}
		}
		if(mainGame->wQuery->isVisible()) {
			SetResponseSelectedCards();
			ShowCancelOrFinishButton(0);
			mainGame->HideElement(mainGame->wQuery, true);
			break;
		}
		if(select_ready) {
			SetResponseSelectedCards();
			ShowCancelOrFinishButton(0);
			if(mainGame->wCardSelect->isVisible())
				mainGame->HideElement(mainGame->wCardSelect, true);
			else
				DuelClient::SendResponse();
		}
		break;
	}
	case MSG_SELECT_UNSELECT_CARD: {
        if (select_cancelable) {
            DuelClient::SetResponseI(-1);
            ShowCancelOrFinishButton(0);
            if (mainGame->wCardSelect->isVisible())
                mainGame->HideElement(mainGame->wCardSelect, true);
            else
                DuelClient::SendResponse();
        }
        break;
    }
	case MSG_SELECT_TRIBUTE: {
		if(selected_cards.size() == 0) {
			if(select_cancelable) {
				DuelClient::SetResponseI(-1);
				ShowCancelOrFinishButton(0);
				if(mainGame->wCardSelect->isVisible())
					mainGame->HideElement(mainGame->wCardSelect, true);
				else
					DuelClient::SendResponse();
			}
			break;
		}
		if(mainGame->wQuery->isVisible()) {
			SetResponseSelectedCards();
			ShowCancelOrFinishButton(0);
			mainGame->HideElement(mainGame->wQuery, true);
			break;
		}
		if(select_ready) {
			SetResponseSelectedCards();
			ShowCancelOrFinishButton(0);
			DuelClient::SendResponse();
		}
		break;
	}
	case MSG_SELECT_SUM: {
		if(mainGame->wQuery->isVisible()) {
			SetResponseSelectedCards();
			ShowCancelOrFinishButton(0);
			mainGame->HideElement(mainGame->wQuery, true);
			break;
		}
		break;
	}
	case MSG_SELECT_CHAIN: {
		if(chain_forced)
			break;
		if(mainGame->wCardSelect->isVisible()) {
			mainGame->HideElement(mainGame->wCardSelect);
			break;
		}
		if(mainGame->wQuery->isVisible()) {
			DuelClient::SetResponseI(-1);
			ShowCancelOrFinishButton(0);
			mainGame->HideElement(mainGame->wQuery, true);
		} else {
			mainGame->PopupElement(mainGame->wQuery);
			ShowCancelOrFinishButton(0);
		}
		if(mainGame->wOptions->isVisible()) {
			DuelClient::SetResponseI(-1);
			ShowCancelOrFinishButton(0);
			mainGame->HideElement(mainGame->wOptions);
		}
		break;
	}
	case MSG_SORT_CARD: {
		if(mainGame->wCardSelect->isVisible()) {
			DuelClient::SetResponseI(-1);
			mainGame->HideElement(mainGame->wCardSelect, true);
			sort_list.clear();
		}
		break;
	}
	case MSG_SELECT_PLACE: {
		if(select_cancelable) {
			unsigned char respbuf[3];
			respbuf[0] = mainGame->LocalPlayer(0);
			respbuf[1] = 0;
			respbuf[2] = 0;
			mainGame->dField.selectable_field = 0;
			DuelClient::SetResponseB(respbuf, 3);
			DuelClient::SendResponse();
			ShowCancelOrFinishButton(0);
		}
		break;
	}
	}
}
}
