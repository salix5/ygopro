#include "config.h"
#include "menu_handler.h"
#include "duelclient.h"
#include "deck_manager.h"
#include "image_manager.h"
#include "sound_manager.h"
#include "game.h"

namespace ygo {

void UpdateDeck() {
	BufferIO::CopyWStr(mainGame->cbCategorySelect->getItem(mainGame->cbCategorySelect->getSelected()),
		mainGame->gameConf.lastcategory, 64);
	BufferIO::CopyWStr(mainGame->cbDeckSelect->getItem(mainGame->cbDeckSelect->getSelected()),
		mainGame->gameConf.lastdeck, 64);
	unsigned char deckbuf[1024];
	auto pdeck = deckbuf;
	BufferIO::WriteInt32(pdeck, deckManager.current_deck.main.size() + deckManager.current_deck.extra.size());
	BufferIO::WriteInt32(pdeck, deckManager.current_deck.side.size());
	for(size_t i = 0; i < deckManager.current_deck.main.size(); ++i)
		BufferIO::WriteInt32(pdeck, deckManager.current_deck.main[i]->first);
	for(size_t i = 0; i < deckManager.current_deck.extra.size(); ++i)
		BufferIO::WriteInt32(pdeck, deckManager.current_deck.extra[i]->first);
	for(size_t i = 0; i < deckManager.current_deck.side.size(); ++i)
		BufferIO::WriteInt32(pdeck, deckManager.current_deck.side[i]->first);
	DuelClient::SendBufferToServer(CTOS_UPDATE_DECK, deckbuf, pdeck - deckbuf);
}
bool MenuHandler::OnEvent(const irr::SEvent& event) {
	if(mainGame->dField.OnCommonEvent(event))
		return false;
	switch(event.EventType) {
	case irr::EET_GUI_EVENT: {
		irr::gui::IGUIElement* caller = event.GUIEvent.Caller;
		s32 id = caller->getID();
		if(mainGame->wQuery->isVisible() && id != BUTTON_YES && id != BUTTON_NO) {
			mainGame->wQuery->getParent()->bringToFront(mainGame->wQuery);
			break;
		}
		if(mainGame->wReplaySave->isVisible() && id != BUTTON_REPLAY_SAVE && id != BUTTON_REPLAY_CANCEL) {
			mainGame->wReplaySave->getParent()->bringToFront(mainGame->wReplaySave);
			break;
		}
		switch(event.GUIEvent.EventType) {
		case irr::gui::EGET_BUTTON_CLICKED: {
			if(id < 110)
				soundManager.PlaySoundEffect(SOUND_MENU);
			else
				soundManager.PlaySoundEffect(SOUND_BUTTON);
			switch(id) {
			case BUTTON_MODE_EXIT: {
				mainGame->device->closeDevice();
				break;
			}
			case BUTTON_JOIN_HOST: {
				break;
			}
			case BUTTON_JOIN_CANCEL: {
				mainGame->HideElement(mainGame->wLanWindow);
				mainGame->ShowElement(mainGame->wMainMenu);
				if(exit_on_return)
					mainGame->device->closeDevice();
				break;
			}
			case BUTTON_LAN_REFRESH: {
				break;
			}
			case BUTTON_CREATE_HOST: {
				mainGame->btnHostConfirm->setEnabled(true);
				mainGame->btnHostCancel->setEnabled(true);
				mainGame->HideElement(mainGame->wLanWindow);
				mainGame->ShowElement(mainGame->wCreateHost);
				break;
			}
			case BUTTON_HOST_CONFIRM: {
				break;
			}
			case BUTTON_HOST_CANCEL: {
				mainGame->btnCreateHost->setEnabled(true);
				mainGame->btnJoinHost->setEnabled(true);
				mainGame->btnJoinCancel->setEnabled(true);
				mainGame->HideElement(mainGame->wCreateHost);
				mainGame->ShowElement(mainGame->wLanWindow);
				break;
			}
			case BUTTON_HP_DUELIST: {
				mainGame->cbCategorySelect->setEnabled(true);
				mainGame->cbDeckSelect->setEnabled(true);
				DuelClient::SendPacketToServer(CTOS_HS_TODUELIST);
				break;
			}
			case BUTTON_HP_OBSERVER: {
				DuelClient::SendPacketToServer(CTOS_HS_TOOBSERVER);
				break;
			}
			case BUTTON_HP_KICK: {
				break;
			}
			case BUTTON_HP_READY: {
				if(mainGame->cbCategorySelect->getSelected() == -1 || mainGame->cbDeckSelect->getSelected() == -1 ||
					!deckManager.LoadDeck(mainGame->cbCategorySelect, mainGame->cbDeckSelect)) {
					mainGame->gMutex.lock();
					soundManager.PlaySoundEffect(SOUND_INFO);
					mainGame->env->addMessageBox(L"", dataManager.GetSysString(1406));
					mainGame->gMutex.unlock();
					break;
				}
				UpdateDeck();
				DuelClient::SendPacketToServer(CTOS_HS_READY);
				mainGame->cbCategorySelect->setEnabled(false);
				mainGame->cbDeckSelect->setEnabled(false);
				break;
			}
			case BUTTON_HP_NOTREADY: {
				DuelClient::SendPacketToServer(CTOS_HS_NOTREADY);
				mainGame->cbCategorySelect->setEnabled(true);
				mainGame->cbDeckSelect->setEnabled(true);
				break;
			}
			case BUTTON_HP_START: {
				DuelClient::SendPacketToServer(CTOS_HS_START);
				break;
			}
			case BUTTON_HP_CANCEL: {
				DuelClient::StopClient();
				mainGame->btnCreateHost->setEnabled(true);
				mainGame->btnJoinHost->setEnabled(true);
				mainGame->btnJoinCancel->setEnabled(true);
				mainGame->btnStartBot->setEnabled(true);
				mainGame->btnBotCancel->setEnabled(true);
				mainGame->HideElement(mainGame->wHostPrepare);
				if(bot_mode)
					mainGame->ShowElement(mainGame->wSinglePlay);
				else
					mainGame->ShowElement(mainGame->wLanWindow);
				mainGame->wChat->setVisible(false);
				if(exit_on_return)
					mainGame->device->closeDevice();
				break;
			}
			case BUTTON_DECK_EDIT: {
				mainGame->RefreshCategoryDeck(mainGame->cbDBCategory, mainGame->cbDBDecks);
				if(open_file && deckManager.LoadDeck(open_file_name)) {
#ifdef WIN32
					wchar_t *dash = wcsrchr(open_file_name, L'\\');
#else
					wchar_t *dash = wcsrchr(open_file_name, L'/');
#endif
					wchar_t *dot = wcsrchr(open_file_name, L'.');
					if(dash && dot && !mywcsncasecmp(dot, L".ydk", 4)) { // full path
						wchar_t deck_name[256];
						wcsncpy(deck_name, dash + 1, dot - dash - 1);
						deck_name[dot - dash - 1] = L'\0';
						mainGame->ebDeckname->setText(deck_name);
						mainGame->cbDBCategory->setSelected(-1);
						mainGame->cbDBDecks->setSelected(-1);
						mainGame->btnManageDeck->setEnabled(false);
						mainGame->cbDBCategory->setEnabled(false);
						mainGame->cbDBDecks->setEnabled(false);
					} else if(dash) { // has category
						wchar_t deck_name[256];
						wcsncpy(deck_name, dash + 1, 256);
						for(size_t i = 0; i < mainGame->cbDBDecks->getItemCount(); ++i) {
							if(!wcscmp(mainGame->cbDBDecks->getItem(i), deck_name)) {
								wcscpy(mainGame->gameConf.lastdeck, deck_name);
								mainGame->cbDBDecks->setSelected(i);
								break;
							}
						}
					} else { // only deck name
						for(size_t i = 0; i < mainGame->cbDBDecks->getItemCount(); ++i) {
							if(!wcscmp(mainGame->cbDBDecks->getItem(i), open_file_name)) {
								wcscpy(mainGame->gameConf.lastdeck, open_file_name);
								mainGame->cbDBDecks->setSelected(i);
								break;
							}
						}
					}
					open_file = false;
				} else if(mainGame->cbDBCategory->getSelected() != -1 && mainGame->cbDBDecks->getSelected() != -1) {
					deckManager.LoadDeck(mainGame->cbDBCategory, mainGame->cbDBDecks);
					mainGame->ebDeckname->setText(L"");
				}
				mainGame->HideElement(mainGame->wMainMenu);
				mainGame->deckBuilder.Initialize();
				break;
			}
			case BUTTON_YES: {
				mainGame->HideElement(mainGame->wQuery);
				prev_operation = 0;
				prev_sel = -1;
				break;
			}
			case BUTTON_NO: {
				mainGame->HideElement(mainGame->wQuery);
				prev_operation = 0;
				prev_sel = -1;
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_LISTBOX_CHANGED: {
			switch(id) {
			case LISTBOX_LAN_HOST: {
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_CHECKBOX_CHANGED: {
			switch(id) {
			case CHECKBOX_HP_READY: {
				if(!caller->isEnabled())
					break;
				mainGame->env->setFocus(mainGame->wHostPrepare);
				if(static_cast<irr::gui::IGUICheckBox*>(caller)->isChecked()) {
					if(mainGame->cbCategorySelect->getSelected() == -1 || mainGame->cbDeckSelect->getSelected() == -1 ||
						!deckManager.LoadDeck(mainGame->cbCategorySelect, mainGame->cbDeckSelect)) {
						mainGame->gMutex.lock();
						static_cast<irr::gui::IGUICheckBox*>(caller)->setChecked(false);
						soundManager.PlaySoundEffect(SOUND_INFO);
						mainGame->env->addMessageBox(L"", dataManager.GetSysString(1406));
						mainGame->gMutex.unlock();
						break;
					}
					UpdateDeck();
					DuelClient::SendPacketToServer(CTOS_HS_READY);
					mainGame->cbCategorySelect->setEnabled(false);
					mainGame->cbDeckSelect->setEnabled(false);
				} else {
					DuelClient::SendPacketToServer(CTOS_HS_NOTREADY);
					mainGame->cbCategorySelect->setEnabled(true);
					mainGame->cbDeckSelect->setEnabled(true);
				}
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_COMBO_BOX_CHANGED: {
			switch(id) {
			case COMBOBOX_HP_CATEGORY: {
				int catesel = mainGame->cbCategorySelect->getSelected();
				if(catesel == 3) {
					catesel = 2;
					mainGame->cbCategorySelect->setSelected(2);
				}
				if(catesel >= 0) {
					mainGame->RefreshDeck(mainGame->cbCategorySelect, mainGame->cbDeckSelect);
					mainGame->cbDeckSelect->setSelected(0);
				}
				break;
			}
			}
			break;
		}
		default: break;
		}
		break;
	}
	default: break;
	}
	return false;
}

}
