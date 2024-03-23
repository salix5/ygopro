#include "event_handler.h"
#include "game.h"
#include "deck_manager.h"
#include "client_field.h"
#include "sound_manager.h"
#include "../ocgcore/common.h"

namespace ygo {

bool OnCommonEvent(const irr::SEvent& event) {
	switch (event.EventType) {
	case irr::EET_GUI_EVENT: {
		s32 id = event.GUIEvent.Caller->getID();
		switch (event.GUIEvent.EventType) {
		case irr::gui::EGET_ELEMENT_HOVERED: {
			if (event.GUIEvent.Caller->getType() == EGUIET_EDIT_BOX) {
				mainGame->SetCursor(event.GUIEvent.Caller->isEnabled() ? ECI_IBEAM : ECI_NORMAL);
				return true;
			}
			if (event.GUIEvent.Caller == mainGame->imgCard && mainGame->is_building && !mainGame->is_siding) {
				mainGame->SetCursor(ECI_HAND);
				return true;
			}
			break;
		}
		case irr::gui::EGET_ELEMENT_LEFT: {
			if (event.GUIEvent.Caller->getType() == EGUIET_EDIT_BOX || event.GUIEvent.Caller == mainGame->imgCard) {
				mainGame->SetCursor(ECI_NORMAL);
				return true;
			}
			break;
		}
		case irr::gui::EGET_BUTTON_CLICKED: {
			switch (id) {
			case BUTTON_CLEAR_LOG: {
				soundManager.PlaySoundEffect(SOUND_BUTTON);
				mainGame->lstLog->clear();
				mainGame->logParam.clear();
				return true;
				break;
			}
			case BUTTON_WINDOW_RESIZE_S: {
				mainGame->SetWindowsScale(0.8f);
				return true;
				break;
			}
			case BUTTON_WINDOW_RESIZE_M: {
				mainGame->SetWindowsScale(1.0f);
				return true;
				break;
			}
			case BUTTON_WINDOW_RESIZE_L: {
				mainGame->SetWindowsScale(1.25f);
				return true;
				break;
			}
			case BUTTON_WINDOW_RESIZE_XL: {
				mainGame->SetWindowsScale(1.5f);
				return true;
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_CHECKBOX_CHANGED: {
			switch (id) {
			case CHECKBOX_AUTO_SEARCH: {
				mainGame->gameConf.auto_search_limit = mainGame->chkAutoSearch->isChecked() ? 0 : -1;
				if (mainGame->is_building && !mainGame->is_siding)
					mainGame->deckBuilder.InstantSearch();
				return true;
				break;
			}
			case CHECKBOX_MULTI_KEYWORDS: {
				mainGame->gameConf.search_multiple_keywords = mainGame->chkMultiKeywords->isChecked() ? 1 : 0;
				if (mainGame->is_building && !mainGame->is_siding)
					mainGame->deckBuilder.InstantSearch();
				return true;
				break;
			}
			case CHECKBOX_ENABLE_MUSIC: {
				if (!mainGame->chkEnableMusic->isChecked())
					soundManager.StopBGM();
				return true;
				break;
			}
			case CHECKBOX_DISABLE_CHAT: {
				bool show = mainGame->is_building ? false : !mainGame->chkIgnore1->isChecked();
				mainGame->wChat->setVisible(show);
				if (!show)
					mainGame->ClearChatMsg();
				return true;
				break;
			}
			case CHECKBOX_QUICK_ANIMATION: {
				mainGame->gameConf.quick_animation = mainGame->chkQuickAnimation->isChecked() ? 1 : 0;
				return true;
				break;
			}
			case CHECKBOX_DRAW_SINGLE_CHAIN: {
				mainGame->gameConf.draw_single_chain = mainGame->chkDrawSingleChain->isChecked() ? 1 : 0;
				return true;
				break;
			}
			case CHECKBOX_HIDE_PLAYER_NAME: {
				mainGame->gameConf.hide_player_name = mainGame->chkHidePlayerName->isChecked() ? 1 : 0;
				if (mainGame->gameConf.hide_player_name)
					mainGame->ClearChatMsg();
				return true;
				break;
			}
			case CHECKBOX_PREFER_EXPANSION: {
				mainGame->gameConf.prefer_expansion_script = mainGame->chkPreferExpansionScript->isChecked() ? 1 : 0;
				return true;
				break;
			}
			case CHECKBOX_LFLIST: {
				mainGame->gameConf.use_lflist = mainGame->chkLFlist->isChecked() ? 1 : 0;
				mainGame->cbLFlist->setEnabled(mainGame->gameConf.use_lflist);
				mainGame->cbLFlist->setSelected(mainGame->gameConf.use_lflist ? mainGame->gameConf.default_lflist : mainGame->cbLFlist->getItemCount() - 1);
				mainGame->cbHostLFlist->setSelected(mainGame->gameConf.use_lflist ? mainGame->gameConf.default_lflist : mainGame->cbHostLFlist->getItemCount() - 1);
				mainGame->deckBuilder.filterList = &deckManager._lfList[mainGame->cbLFlist->getSelected()].content;
				return true;
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_COMBO_BOX_CHANGED: {
			switch (id) {
			case COMBOBOX_LFLIST: {
				mainGame->gameConf.default_lflist = mainGame->cbLFlist->getSelected();
				mainGame->cbHostLFlist->setSelected(mainGame->gameConf.default_lflist);
				mainGame->deckBuilder.filterList = &deckManager._lfList[mainGame->gameConf.default_lflist].content;
				return true;
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_LISTBOX_CHANGED: {
			switch (id) {
			case LISTBOX_LOG: {
				int sel = mainGame->lstLog->getSelected();
				if (sel != -1 && (int)mainGame->logParam.size() >= sel && mainGame->logParam[sel]) {
					mainGame->ShowCardInfo(mainGame->logParam[sel]);
				}
				return true;
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_LISTBOX_SELECTED_AGAIN: {
			switch (id) {
			case LISTBOX_LOG: {
				int sel = mainGame->lstLog->getSelected();
				if (sel != -1 && (int)mainGame->logParam.size() >= sel && mainGame->logParam[sel]) {
					mainGame->wInfos->setActiveTab(0);
				}
				return true;
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_SCROLL_BAR_CHANGED: {
			switch (id) {
			case SCROLL_CARDTEXT: {
				if (!mainGame->scrCardText->isVisible()) {
					return true;
					break;
				}
				u32 pos = mainGame->scrCardText->getPos();
				mainGame->SetStaticText(mainGame->stText, mainGame->stText->getRelativePosition().getWidth() - 25, mainGame->guiFont, mainGame->showingtext, pos);
				return true;
				break;
			}
			case SCROLL_VOLUME: {
				mainGame->gameConf.sound_volume = (double)mainGame->scrSoundVolume->getPos() / 100;
				mainGame->gameConf.music_volume = (double)mainGame->scrMusicVolume->getPos() / 100;
				soundManager.SetSoundVolume(mainGame->gameConf.sound_volume);
				soundManager.SetMusicVolume(mainGame->gameConf.music_volume);
				return true;
				break;
			}
			case SCROLL_TAB_HELPER: {
				rect<s32> pos = mainGame->tabHelper->getRelativePosition();
				mainGame->tabHelper->setRelativePosition(recti(0, mainGame->scrTabHelper->getPos() * -1, pos.LowerRightCorner.X, pos.LowerRightCorner.Y));
				return true;
				break;
			}
			case SCROLL_TAB_SYSTEM: {
				rect<s32> pos = mainGame->tabSystem->getRelativePosition();
				mainGame->tabSystem->setRelativePosition(recti(0, mainGame->scrTabSystem->getPos() * -1, pos.LowerRightCorner.X, pos.LowerRightCorner.Y));
				return true;
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_EDITBOX_ENTER: {
			break;
		}
		default:
			break;
		}
		break;
	}
	case irr::EET_KEY_INPUT_EVENT: {
		switch (event.KeyInput.Key) {
		case irr::KEY_KEY_R: {
			if (mainGame->gameConf.control_mode == 0
				&& !event.KeyInput.PressedDown && !mainGame->HasFocus(EGUIET_EDIT_BOX)) {
				mainGame->textFont->setTransparency(true);
				mainGame->guiFont->setTransparency(true);
			}
			return true;
			break;
		}
		case irr::KEY_F9: {
			if (mainGame->gameConf.control_mode == 1
				&& !event.KeyInput.PressedDown && !mainGame->HasFocus(EGUIET_EDIT_BOX)) {
				mainGame->textFont->setTransparency(true);
				mainGame->guiFont->setTransparency(true);
			}
			return true;
			break;
		}
		case irr::KEY_ESCAPE: {
			if (!mainGame->HasFocus(EGUIET_EDIT_BOX))
				mainGame->device->minimizeWindow();
			return true;
			break;
		}
		default:
			break;
		}
		break;
	}
	default:
		break;
	}
	return false;
}
bool ClientField::OnEvent(const irr::SEvent& event) {
	if(ygo::OnCommonEvent(event))
		return false;
	return false;
}
}
