#include <algorithm>
#include <array>
#include "config.h"
#include "deck_con.h"
#include "data_manager.h"
#include "deck_manager.h"
#include "myfilesystem.h"
#include "image_manager.h"
#include "sound_manager.h"
#include "game.h"
#include "duelclient.h"

namespace ygo {

static int parse_filter(const wchar_t* pstr, unsigned int* type) {
	if(*pstr == L'=') {
		*type = 1;
		return BufferIO::GetVal(pstr + 1);
	} else if(*pstr >= L'0' && *pstr <= L'9') {
		*type = 1;
		return BufferIO::GetVal(pstr);
	} else if(*pstr == L'>') {
		if(*(pstr + 1) == L'=') {
			*type = 2;
			return BufferIO::GetVal(pstr + 2);
		} else {
			*type = 3;
			return BufferIO::GetVal(pstr + 1);
		}
	} else if(*pstr == L'<') {
		if(*(pstr + 1) == L'=') {
			*type = 4;
			return BufferIO::GetVal(pstr + 2);
		} else {
			*type = 5;
			return BufferIO::GetVal(pstr + 1);
		}
	} else if(*pstr == L'?') {
		*type = 6;
		return 0;
	}
	*type = 0;
	return 0;
}


static inline bool havePopupWindow() {
	irr::gui::IGUIWindow* window_list[] = {
		mainGame->wQuery,
		mainGame->wCategories,
		mainGame->wLinkMarks,
		mainGame->wDeckManage,
		mainGame->wDMQuery,
		mainGame->wBigCard
	};
	for (const auto& window : window_list) {
		if (window->isVisible())
			return true;
	}
	return false;
}

static inline void get_deck_file(wchar_t* ret, const wchar_t* deck_name) {
	DeckManager::GetDeckFile(ret, mainGame->cbDBCategory->getSelected(), mainGame->cbDBCategory->getText(), deck_name);
}

static inline void get_list_file(wchar_t* ret, const wchar_t* deck_name) {
	DeckManager::GetDeckFile(ret, mainGame->lstCategories->getSelected(), mainGame->lstCategories->getListItem(mainGame->lstCategories->getSelected()), deck_name);
}

static inline void load_current_deck(irr::gui::IGUIComboBox* cbCategory, irr::gui::IGUIComboBox* cbDeck) {
	deckManager.LoadCurrentDeck(cbCategory->getSelected(), cbCategory->getText(), cbDeck->getText());
}

DeckBuilder::DeckBuilder() {
	std::random_device rd;
	std::array<uint32_t, 8> seed{};
	for (auto& x : seed)
		x = rd();
	std::seed_seq seq(seed.begin(), seed.end());
	rnd.seed(seq);
}
void DeckBuilder::Initialize() {
	mainGame->is_building = true;
	mainGame->is_siding = false;
	mainGame->ClearCardInfo();
	mainGame->wInfos->setVisible(true);
	mainGame->wCardImg->setVisible(true);
	mainGame->wDeckEdit->setVisible(true);
	mainGame->wFilter->setVisible(true);
	mainGame->wSort->setVisible(true);
	mainGame->btnLeaveGame->setVisible(true);
	mainGame->btnLeaveGame->setText(dataManager.GetSysString(1306));
	mainGame->btnSideOK->setVisible(false);
	mainGame->btnSideShuffle->setVisible(false);
	mainGame->btnSideSort->setVisible(false);
	mainGame->btnSideReload->setVisible(false);
	mainGame->btnGenerateScript->setVisible(true);
	if (mainGame->gameConf.use_lflist) {
		if (mainGame->gameConf.default_lflist >= 0 && mainGame->gameConf.default_lflist < (int)deckManager._lfList.size()) {
			filterList = &deckManager._lfList[mainGame->gameConf.default_lflist];
		}
		else {
			mainGame->gameConf.default_lflist = 0;
			filterList = &deckManager._lfList.front();
		}
	}
	else {
		filterList = &deckManager._lfList.back();
	}
	ClearSearch();
	mouse_pos.set(0, 0);
	hovered_code = 0;
	hovered_pos = 0;
	hovered_seq = -1;
	is_lastcard = 0;
	is_draging = false;
	is_starting_dragging = false;
	RefreshReadonly(mainGame->cbDBCategory->getSelected());
	RefreshPackListScroll();
	prev_operation = 0;
	is_modified = false;
	mainGame->device->setEventReceiver(this);
}
void DeckBuilder::Terminate() {
	mainGame->is_building = false;
	mainGame->ClearCardInfo();
	mainGame->wDeckEdit->setVisible(false);
	mainGame->wCategories->setVisible(false);
	mainGame->wFilter->setVisible(false);
	mainGame->wSort->setVisible(false);
	mainGame->wCardImg->setVisible(false);
	mainGame->wInfos->setVisible(false);
	mainGame->btnLeaveGame->setVisible(false);
	mainGame->wBigCard->setVisible(false);
	mainGame->btnBigCardOriginalSize->setVisible(false);
	mainGame->btnBigCardZoomIn->setVisible(false);
	mainGame->btnBigCardZoomOut->setVisible(false);
	mainGame->btnBigCardClose->setVisible(false);
	mainGame->btnGenerateScript->setVisible(false);
	EnableEditWindow(true);
	EnableManageWindow(true);
	mainGame->ResizeChatInputWindow();
	mainGame->PopupElement(mainGame->wMainMenu);
	mainGame->device->setEventReceiver(&mainGame->menuHandler);
	mainGame->wACMessage->setVisible(false);
	mainGame->ClearTextures();
	mainGame->showingcode = 0;
	mainGame->scrFilter->setVisible(false);
	mainGame->scrPackCards->setVisible(false);
	mainGame->scrPackCards->setPos(0);
	if (const wchar_t* name = mainGame->cbDBCategory->getText())
		BufferIO::CopyWideString(name, mainGame->gameConf.lastcategory);
	if (const wchar_t* name = mainGame->cbDBDecks->getText())
		BufferIO::CopyWideString(name, mainGame->gameConf.lastdeck);
	if(mainGame->exit_on_return)
		mainGame->device->closeDevice();
}
bool DeckBuilder::OnEvent(const irr::SEvent& event) {
	if(mainGame->dField.OnCommonEvent(event))
		return false;
	auto& _datas = dataManager.GetDataTable();
	if (event.EventType == irr::EET_GUI_EVENT) {
		if (event.GUIEvent.EventType == irr::gui::EGET_ELEMENT_FOCUS_LOST) {
			auto element = event.GUIEvent.Element;
			auto element_id = element ? element->getID() : 0;
			if (mainGame->wDMQuery->isVisible()) {
				return element_id != WINDOW_DM_QUERY && !mainGame->wDMQuery->isMyChild(element);
			}
			if (mainGame->wDeckManage->isVisible()) {
				return element_id != WINDOW_DECK_MANAGE && element_id != WINDOW_DM_QUERY && !mainGame->wDeckManage->isMyChild(element);
			}
			if (mainGame->wQuery->isVisible()) {
				return element_id != WINDOW_QUERY && !mainGame->wQuery->isMyChild(element);
			}
			if (mainGame->wCategories->isVisible()) {
				return element_id != WINDOW_CATEGORY && !mainGame->wCategories->isMyChild(element);
			}
			if (mainGame->wLinkMarks->isVisible()) {
				return element_id != WINDOW_LINK_MARKER && !mainGame->wLinkMarks->isMyChild(element);
			}
			if( mainGame->wBigCard->isVisible()) {
				return (element_id < WINDOW_BIG_CARD || element_id > BUTTON_BIG_CARD_ORIG_SIZE) && !mainGame->wBigCard->isMyChild(element);
			}
			return false;
		}
		else if (event.GUIEvent.EventType == irr::gui::EGET_ELEMENT_CLOSED) {
			if (event.GUIEvent.Caller->getID() == WINDOW_DECK_MANAGE) {
				mainGame->HideElement(mainGame->wDeckManage);
				EnableEditWindow(true);
				return true;
			}
			return false;
		}
		auto id = event.GUIEvent.Caller->getID();
		switch (event.GUIEvent.EventType) {
		case irr::gui::EGET_BUTTON_CLICKED: {
			ButtonHandler(event);
			break;
		}
		case irr::gui::EGET_SCROLL_BAR_CHANGED: {
			switch (id) {
			case SCROLL_FILTER: {
				GetHoveredCard();
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_EDITBOX_ENTER: {
			switch (id) {
			case EDITBOX_ATK:
			case EDITBOX_DEF:
			case EDITBOX_LEVEL:
			case EDITBOX_SCALE:
			case EDITBOX_KEYWORD: {
				StartFilter();
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_EDITBOX_CHANGED: {
			switch (id) {
			case EDITBOX_KEYWORD: {
				InstantSearch();
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_COMBO_BOX_CHANGED: {
			ComboBoxHandler(event);
			break;
		}
		case irr::gui::EGET_LISTBOX_CHANGED: {
			switch (id) {
			case LISTBOX_CATEGORIES: {
				int catesel = mainGame->lstCategories->getSelected();
				mainGame->cbDBCategory->setSelected(catesel);
				ChangeCategory();
				break;
			}
			case LISTBOX_DECKS: {
				int catesel = mainGame->lstCategories->getSelected();
				int decksel = mainGame->lstDecks->getSelected();
				mainGame->cbDBDecks->setSelected(decksel);
				if (decksel == -1)
					break;
				wchar_t filepath[256]{};
				deckManager.LoadCurrentDeck(catesel, mainGame->lstCategories->getListItem(catesel), mainGame->lstDecks->getListItem(decksel));
				RefreshPackListScroll();
				break;
			}
			}
			break;
		}
		default:
			break;
		}
		return false;
	}
	else if (event.EventType == irr::EET_MOUSE_INPUT_EVENT) {
		irr::core::vector2di current_pos(event.MouseInput.X, event.MouseInput.Y);
		irr::gui::IGUIElement* root = mainGame->env->getRootGUIElement();
		switch (event.MouseInput.Event) {
		case irr::EMIE_LMOUSE_PRESSED_DOWN: {
			if (readonly)
				break;
			if (havePopupWindow())
				break;
			if (root->getElementFromPoint(current_pos) != root)
				break;
			if (hovered_pos == 0 || hovered_seq == -1)
				break;
			click_pos = hovered_pos;
			dragx = event.MouseInput.X;
			dragy = event.MouseInput.Y;
			auto dit = _datas.find(hovered_code);
			if (dit == _datas.end()) {
				draging_pointer = nullptr;
				is_starting_dragging = false;
				is_draging = false;
				break;
			}
			draging_pointer = &dit->second;
			if (hovered_pos == 4) {
				if (!check_limit(draging_pointer))
					break;
			}
			is_starting_dragging = true;
			break;
		}
		case irr::EMIE_LMOUSE_LEFT_UP: {
			is_starting_dragging = false;
			if (!is_draging && !mainGame->is_siding && !havePopupWindow() && root->getElementFromPoint(current_pos) == mainGame->imgCard) {
				ShowBigCard(mainGame->showingcode, 1);
				break;
			}
			if (!is_draging)
				break;
			soundManager.PlaySoundEffect(SOUND_CARD_DROP);
			bool pushed = false;
			if (hovered_pos == 1)
				pushed = push_main(draging_pointer, hovered_seq);
			else if (hovered_pos == 2)
				pushed = push_extra(draging_pointer, hovered_seq + is_lastcard);
			else if (hovered_pos == 3)
				pushed = push_side(draging_pointer, hovered_seq + is_lastcard);
			else if (hovered_pos == 4 && !mainGame->is_siding)
				pushed = true;
			if (!pushed) {
				if (click_pos == 1)
					push_main(draging_pointer);
				else if (click_pos == 2)
					push_extra(draging_pointer);
				else if (click_pos == 3)
					push_side(draging_pointer);
			}
			is_draging = false;
			break;
		}
		case irr::EMIE_LMOUSE_DOUBLE_CLICK: {
			if (!is_draging && !mainGame->is_siding && hovered_code && !havePopupWindow() && root->getElementFromPoint(current_pos) == root) {
				ShowBigCard(hovered_code, 1);
				break;
			}
			break;
		}
		case irr::EMIE_RMOUSE_LEFT_UP: {
			if (mainGame->wBigCard->isVisible()) {
				CloseBigCard();
				break;
			}
			if (mainGame->is_siding) {
				if (is_draging)
					break;
				if (hovered_pos == 0 || hovered_seq == -1)
					break;
				auto pointer = _datas.find(hovered_code);
				if (pointer == _datas.end())
					break;
				auto cd = &pointer->second;
				soundManager.PlaySoundEffect(SOUND_CARD_DROP);
				if (hovered_pos == 1) {
					if (push_side(cd))
						pop_main(hovered_seq);
				}
				else if (hovered_pos == 2) {
					if (push_side(cd))
						pop_extra(hovered_seq);
				}
				else {
					if (push_extra(cd) || push_main(cd))
						pop_side(hovered_seq);
				}
				break;
			}
			if (havePopupWindow())
				break;
			if (!is_draging) {
				if (hovered_pos == 0 || hovered_seq == -1)
					break;
				if (readonly)
					break;
				soundManager.PlaySoundEffect(SOUND_CARD_DROP);
				if (hovered_pos == 1) {
					pop_main(hovered_seq);
				}
				else if (hovered_pos == 2) {
					pop_extra(hovered_seq);
				}
				else if (hovered_pos == 3) {
					pop_side(hovered_seq);
				}
				else {
					auto pointer = _datas.find(hovered_code);
					if (pointer == _datas.end())
						break;
					auto cd = &pointer->second;
					if (!check_limit(cd))
						break;
					if (!push_extra(cd) && !push_main(cd))
						push_side(cd);
				}
			}
			else {
				soundManager.PlaySoundEffect(SOUND_CARD_PICK);
				if (click_pos == 1) {
					push_side(draging_pointer);
				}
				else if (click_pos == 2) {
					push_side(draging_pointer);
				}
				else if (click_pos == 3) {
					if (!push_extra(draging_pointer))
						push_main(draging_pointer);
				}
				else {
					push_side(draging_pointer);
				}
			}
			is_draging = false;
			break;
		}
		case irr::EMIE_MMOUSE_LEFT_UP: {
			if (mainGame->is_siding)
				break;
			if (havePopupWindow())
				break;
			if (hovered_pos == 0 || hovered_seq == -1)
				break;
			if (readonly)
				break;
			if (is_draging)
				break;
			auto pointer = _datas.find(hovered_code);
			if (pointer == _datas.end())
				break;
			auto cd = &pointer->second;
			if (!check_limit(cd))
				break;
			soundManager.PlaySoundEffect(SOUND_CARD_PICK);
			if (hovered_pos == 1) {
				if (!push_main(cd))
					push_side(cd);
			}
			else if (hovered_pos == 2) {
				if (!push_extra(cd))
					push_side(cd);
			}
			else if (hovered_pos == 3) {
				if (!push_side(cd) && !push_extra(cd))
					push_main(cd);
			}
			else {
				if (!push_extra(cd) && !push_main(cd))
					push_side(cd);
			}
			break;
		}
		case irr::EMIE_MOUSE_MOVED: {
			if (is_starting_dragging) {
				is_draging = true;
				soundManager.PlaySoundEffect(SOUND_CARD_PICK);
				if (hovered_pos == 1)
					pop_main(hovered_seq);
				else if (hovered_pos == 2)
					pop_extra(hovered_seq);
				else if (hovered_pos == 3)
					pop_side(hovered_seq);
				is_starting_dragging = false;
			}
			mouse_pos = current_pos;
			GetHoveredCard();
			break;
		}
		case irr::EMIE_MOUSE_WHEEL: {
			auto element = root->getElementFromPoint(current_pos);
			if (element == mainGame->imgBigCard) {
				ZoomBigCard(0.1f * event.MouseInput.Wheel, event.MouseInput.X, event.MouseInput.Y);
				break;
			}
			if (element != root)
				break;
			if (!mainGame->scrFilter->isVisible())
				break;
			if (mainGame->env->hasFocus(mainGame->scrFilter))
				break;
			if (event.MouseInput.Wheel < 0) {
				if (mainGame->scrFilter->getPos() < mainGame->scrFilter->getMax())
					mainGame->scrFilter->setPos(mainGame->scrFilter->getPos() + 1);
			}
			else {
				if (mainGame->scrFilter->getPos() > 0)
					mainGame->scrFilter->setPos(mainGame->scrFilter->getPos() - 1);
			}
			GetHoveredCard();
			break;
		}
		default:
			break;
		}
		return false;
	}
	return false;
}
void DeckBuilder::ButtonHandler(const irr::SEvent& event) {
	soundManager.PlaySoundEffect(SOUND_BUTTON);
	auto id = event.GUIEvent.Caller->getID();
	switch (id) {
	case BUTTON_CLEAR_DECK: {
		mainGame->gMutex.lock();
		EnableEditWindow(false);
		mainGame->SetStaticText(mainGame->stQMessage, 310, mainGame->guiFont, dataManager.GetSysString(1339));
		mainGame->PopupElement(mainGame->wQuery);
		mainGame->gMutex.unlock();
		prev_operation = id;
		break;
	}
	case BUTTON_SORT_DECK: {
		std::sort(deckManager.current_deck.main.begin(), deckManager.current_deck.main.end(), DataManager::deck_sort_lv);
		std::sort(deckManager.current_deck.extra.begin(), deckManager.current_deck.extra.end(), DataManager::deck_sort_lv);
		std::sort(deckManager.current_deck.side.begin(), deckManager.current_deck.side.end(), DataManager::deck_sort_lv);
		if (!readonly && deckManager.current_deck.size())
			is_modified = true;
		break;
	}
	case BUTTON_SHUFFLE_DECK: {
		std::shuffle(deckManager.current_deck.main.begin(), deckManager.current_deck.main.end(), rnd);
		if (!readonly && deckManager.current_deck.main.size())
			is_modified = true;
		break;
	}
	case BUTTON_SAVE_DECK: {
		int sel = mainGame->cbDBDecks->getSelected();
		if (sel == -1)
			break;
		wchar_t filepath[256]{};
		get_deck_file(filepath, mainGame->cbDBDecks->getText());
		if (!DeckManager::SaveDeck(deckManager.current_deck, filepath))
			break;
		mainGame->stACMessage->setText(dataManager.GetSysString(1335));
		mainGame->PopupElement(mainGame->wACMessage, 40);
		is_modified = false;
		break;
	}
	case BUTTON_SAVE_DECK_AS: {
		int catesel = mainGame->cbDBCategory->getSelected();
		const wchar_t* dname = mainGame->ebDeckname->getText();
		if (!dname[0])
			break;
		wchar_t filepath[256]{};
		get_deck_file(filepath, dname);
		if (!filepath[0])
			break;
		bool is_exist = FileSystem::IsFileExists(filepath);
		if (!DeckManager::SaveDeck(deckManager.current_deck, filepath))
			break;
		int sel = -1;
		if (is_exist) {
			for (int i = 0; i < (int)mainGame->cbDBDecks->getItemCount(); ++i) {
				if (mywcsncasecmp(dname, mainGame->cbDBDecks->getItem(i), 256) == 0) {
					sel = i;
					break;
				}
			}
		}
		if (sel >= 0)
			mainGame->cbDBDecks->setSelected(sel);
		else
			mainGame->cbDBDecks->setSelected(mainGame->cbDBDecks->addItem(dname));
		mainGame->stACMessage->setText(dataManager.GetSysString(1335));
		mainGame->PopupElement(mainGame->wACMessage, 40);
		is_modified = false;
		if (catesel == -1) {
			catesel = DECK_CATEGORY_NONE;
			mainGame->cbDBCategory->setSelected(catesel);
			mainGame->btnManageDeck->setEnabled(true);
			mainGame->cbDBCategory->setEnabled(true);
			mainGame->cbDBDecks->setEnabled(true);
		}
		RefreshReadonly(catesel);
		break;
	}
	case BUTTON_DELETE_DECK: {
		int sel = mainGame->cbDBDecks->getSelected();
		if (sel == -1)
			break;
		mainGame->gMutex.lock();
		EnableEditWindow(false);
		wchar_t textBuffer[256];
		myswprintf(textBuffer, L"%ls\n%ls", mainGame->cbDBDecks->getItem(sel), dataManager.GetSysString(1337));
		mainGame->SetStaticText(mainGame->stQMessage, 310, mainGame->guiFont, textBuffer);
		mainGame->PopupElement(mainGame->wQuery);
		mainGame->gMutex.unlock();
		prev_operation = id;
		break;
	}
	case BUTTON_LEAVE_GAME: {
		if (is_modified && !readonly && !mainGame->chkIgnoreDeckChanges->isChecked()) {
			mainGame->gMutex.lock();
			EnableEditWindow(false);
			mainGame->SetStaticText(mainGame->stQMessage, 310, mainGame->guiFont, dataManager.GetSysString(1356));
			mainGame->PopupElement(mainGame->wQuery);
			mainGame->gMutex.unlock();
			prev_operation = id;
			break;
		}
		Terminate();
		break;
	}
	case BUTTON_EFFECT_FILTER: {
		mainGame->PopupElement(mainGame->wCategories);
		break;
	}
	case BUTTON_START_FILTER: {
		StartFilter();
		if (!mainGame->gameConf.separate_clear_button)
			ClearFilter();
		break;
	}
	case BUTTON_CLEAR_FILTER: {
		ClearSearch();
		break;
	}
	case BUTTON_CATEGORY_OK: {
		filter_effect = 0;
		long long filter = 0x1;
		for (int i = 0; i < 32; ++i, filter <<= 1)
			if (mainGame->chkCategory[i]->isChecked())
				filter_effect |= filter;
		mainGame->btnEffectFilter->setPressed(filter_effect > 0);
		mainGame->HideElement(mainGame->wCategories);
		InstantSearch();
		break;
	}
	case BUTTON_MANAGE_DECK: {
		if (is_modified && !readonly && !mainGame->chkIgnoreDeckChanges->isChecked()) {
			mainGame->gMutex.lock();
			EnableEditWindow(false);
			mainGame->SetStaticText(mainGame->stQMessage, 310, mainGame->guiFont, dataManager.GetSysString(1356));
			mainGame->PopupElement(mainGame->wQuery);
			mainGame->gMutex.unlock();
			prev_operation = id;
			break;
		}
		ShowDeckManage();
		break;
	}
	case BUTTON_NEW_CATEGORY: {
		mainGame->gMutex.lock();
		EnableManageWindow(false);
		mainGame->stDMMessage->setText(dataManager.GetSysString(1469));
		mainGame->ebDMName->setVisible(true);
		mainGame->ebDMName->setText(L"");
		mainGame->PopupElement(mainGame->wDMQuery);
		mainGame->gMutex.unlock();
		dmquery_operation = id;
		break;
	}
	case BUTTON_RENAME_CATEGORY: {
		if (mainGame->lstCategories->getSelected() < DECK_CATEGORY_CUSTOM)
			break;
		mainGame->gMutex.lock();
		EnableManageWindow(false);
		mainGame->stDMMessage->setText(dataManager.GetSysString(1469));
		mainGame->ebDMName->setVisible(true);
		mainGame->ebDMName->setText(mainGame->lstCategories->getListItem(mainGame->lstCategories->getSelected()));
		mainGame->PopupElement(mainGame->wDMQuery);
		mainGame->gMutex.unlock();
		dmquery_operation = id;
		break;
	}
	case BUTTON_DELETE_CATEGORY: {
		if (mainGame->lstCategories->getSelected() < DECK_CATEGORY_CUSTOM)
			break;
		mainGame->gMutex.lock();
		EnableManageWindow(false);
		mainGame->stDMMessage->setText(dataManager.GetSysString(1470));
		mainGame->stDMMessage2->setVisible(true);
		mainGame->stDMMessage2->setText(mainGame->lstCategories->getListItem(mainGame->lstCategories->getSelected()));
		mainGame->PopupElement(mainGame->wDMQuery);
		mainGame->gMutex.unlock();
		dmquery_operation = id;
		break;
	}
	case BUTTON_NEW_DECK: {
		mainGame->gMutex.lock();
		EnableManageWindow(false);
		mainGame->stDMMessage->setText(dataManager.GetSysString(1471));
		mainGame->ebDMName->setVisible(true);
		mainGame->ebDMName->setText(L"");
		mainGame->PopupElement(mainGame->wDMQuery);
		mainGame->gMutex.unlock();
		dmquery_operation = id;
		break;
	}
	case BUTTON_RENAME_DECK: {
		mainGame->gMutex.lock();
		EnableManageWindow(false);
		mainGame->stDMMessage->setText(dataManager.GetSysString(1471));
		mainGame->ebDMName->setVisible(true);
		mainGame->ebDMName->setText(mainGame->lstDecks->getListItem(mainGame->lstDecks->getSelected()));
		mainGame->PopupElement(mainGame->wDMQuery);
		mainGame->gMutex.unlock();
		dmquery_operation = id;
		break;
	}
	case BUTTON_DELETE_DECK_DM: {
		mainGame->gMutex.lock();
		EnableManageWindow(false);
		mainGame->stDMMessage->setText(dataManager.GetSysString(1337));
		mainGame->stDMMessage2->setVisible(true);
		mainGame->stDMMessage2->setText(mainGame->lstDecks->getListItem(mainGame->lstDecks->getSelected()));
		mainGame->PopupElement(mainGame->wDMQuery);
		mainGame->gMutex.unlock();
		dmquery_operation = id;
		break;
	}
	case BUTTON_MOVE_DECK: {
		mainGame->gMutex.lock();
		EnableManageWindow(false);
		mainGame->stDMMessage->setText(dataManager.GetSysString(1472));
		mainGame->cbDMCategory->setVisible(true);
		mainGame->cbDMCategory->clear();
		int catesel = mainGame->lstCategories->getSelected();
		if (catesel != DECK_CATEGORY_NONE)
			mainGame->cbDMCategory->addItem(dataManager.GetSysString(1452), DECK_CATEGORY_NONE);
		for (int i = DECK_CATEGORY_CUSTOM; i < (int)mainGame->lstCategories->getItemCount(); i++) {
			if (i != catesel)
				mainGame->cbDMCategory->addItem(mainGame->lstCategories->getListItem(i), i);
		}
		mainGame->PopupElement(mainGame->wDMQuery);
		mainGame->gMutex.unlock();
		dmquery_operation = id;
		break;
	}
	case BUTTON_COPY_DECK: {
		mainGame->gMutex.lock();
		EnableManageWindow(false);
		mainGame->stDMMessage->setText(dataManager.GetSysString(1473));
		mainGame->cbDMCategory->setVisible(true);
		mainGame->cbDMCategory->clear();
		int catesel = mainGame->lstCategories->getSelected();
		if (catesel != DECK_CATEGORY_NONE)
			mainGame->cbDMCategory->addItem(dataManager.GetSysString(1452), DECK_CATEGORY_NONE);
		for (int i = DECK_CATEGORY_CUSTOM; i < (int)mainGame->lstCategories->getItemCount(); i++) {
			if (i != catesel)
				mainGame->cbDMCategory->addItem(mainGame->lstCategories->getListItem(i), i);
		}
		mainGame->PopupElement(mainGame->wDMQuery);
		mainGame->gMutex.unlock();
		dmquery_operation = id;
		break;
	}
	case BUTTON_IMPORT_DECK_CODE: {
		time_t nowtime = std::time(nullptr);
		wchar_t timetext[40];
		std::wcsftime(timetext, sizeof timetext / sizeof timetext[0], L"%Y-%m-%d %H-%M-%S", std::localtime(&nowtime));
		mainGame->gMutex.lock();
		EnableManageWindow(false);
		mainGame->stDMMessage->setText(dataManager.GetSysString(1471));
		mainGame->ebDMName->setVisible(true);
		mainGame->ebDMName->setText(timetext);
		mainGame->PopupElement(mainGame->wDMQuery);
		mainGame->gMutex.unlock();
		dmquery_operation = id;
		break;
	}
	case BUTTON_EXPORT_DECK_CODE: {
		// TODO: export deck code
		break;
	}
	case BUTTON_DM_OK: {
		switch (dmquery_operation) {
		case BUTTON_NEW_CATEGORY: {
			int catesel = 0;
			const wchar_t* catename = mainGame->ebDMName->getText();
			if (DeckManager::CreateCategory(catename)) {
				mainGame->cbDBCategory->addItem(catename);
				catesel = mainGame->lstCategories->addItem(catename);
			}
			else {
				for (int i = DECK_CATEGORY_CUSTOM; i < (int)mainGame->lstCategories->getItemCount(); i++) {
					if (mywcsncasecmp(mainGame->lstCategories->getListItem(i), catename, 256) == 0) {
						catesel = i;
						mainGame->stACMessage->setText(dataManager.GetSysString(1474));
						mainGame->PopupElement(mainGame->wACMessage, 40);
						break;
					}
				}
			}
			if (catesel > 0) {
				mainGame->lstCategories->setSelected(catesel);
				mainGame->cbDBCategory->setSelected(catesel);
				ChangeCategory();
			}
			break;
		}
		case BUTTON_RENAME_CATEGORY: {
			int catesel = mainGame->lstCategories->getSelected();
			if (catesel < DECK_CATEGORY_CUSTOM)
				break;
			const wchar_t* oldcatename = mainGame->lstCategories->getListItem(catesel);
			const wchar_t* newcatename = mainGame->ebDMName->getText();
			if (DeckManager::RenameCategory(oldcatename, newcatename)) {
				mainGame->cbDBCategory->removeItem(catesel);
				mainGame->cbDBCategory->addItem(newcatename);
				mainGame->lstCategories->removeItem(catesel);
				catesel = mainGame->lstCategories->addItem(newcatename);
			}
			else {
				catesel = 0;
				for (int i = DECK_CATEGORY_CUSTOM; i < (int)mainGame->lstCategories->getItemCount(); i++) {
					if (mywcsncasecmp(mainGame->lstCategories->getListItem(i), newcatename, 256) == 0) {
						catesel = i;
						mainGame->stACMessage->setText(dataManager.GetSysString(1474));
						mainGame->PopupElement(mainGame->wACMessage, 40);
						break;
					}
				}
			}
			if (catesel > 0) {
				mainGame->lstCategories->setSelected(catesel);
				mainGame->cbDBCategory->setSelected(catesel);
				ChangeCategory();
			}
			break;
		}
		case BUTTON_DELETE_CATEGORY: {
			int catesel = mainGame->lstCategories->getSelected();
			if (catesel < DECK_CATEGORY_CUSTOM)
				break;
			const wchar_t* catename = mainGame->lstCategories->getListItem(catesel);
			if (DeckManager::DeleteCategory(catename)) {
				mainGame->cbDBCategory->removeItem(catesel);
				mainGame->lstCategories->removeItem(catesel);
				mainGame->lstCategories->setSelected(DECK_CATEGORY_NONE);
				mainGame->cbDBCategory->setSelected(DECK_CATEGORY_NONE);
				ChangeCategory();
			}
			else {
				mainGame->stACMessage->setText(dataManager.GetSysString(1476));
				mainGame->PopupElement(mainGame->wACMessage, 40);
			}
			break;
		}
		case BUTTON_NEW_DECK:
		case BUTTON_IMPORT_DECK_CODE: {
			int category_index = mainGame->lstCategories->getSelected();
			if (category_index < DECK_CATEGORY_NONE)
				break;
			const wchar_t* deckname = mainGame->ebDMName->getText();
			wchar_t filepath[256]{};
			get_list_file(filepath, deckname);
			if (!filepath[0])
				break;
			if (FileSystem::IsFileExists(filepath)) {
				ChangeCategory(deckname);
				mainGame->stACMessage->setText(dataManager.GetSysString(1475));
				mainGame->PopupElement(mainGame->wACMessage, 40);
				break;
			}
			Deck new_deck;
			if (dmquery_operation == BUTTON_IMPORT_DECK_CODE) {
				if (const char* txt = mainGame->env->getOSOperator()->getTextFromClipboard()) {
					DeckManager::LoadDeckFromStream(new_deck, txt);
				}
			}
			if (!DeckManager::SaveDeck(new_deck, filepath))
				break;
			ChangeCategory(deckname);
			break;
		}
		case BUTTON_RENAME_DECK: {
			int catesel = mainGame->lstCategories->getSelected();
			if (catesel < DECK_CATEGORY_NONE)
				break;
			int decksel = mainGame->lstDecks->getSelected();
			if (decksel == -1)
				break;
			const wchar_t* catename = mainGame->lstCategories->getListItem(catesel);
			wchar_t oldfilepath[256]{};
			get_list_file(oldfilepath, mainGame->lstDecks->getListItem(decksel));
			if (!oldfilepath[0])
				break;
			const wchar_t* newdeckname = mainGame->ebDMName->getText();
			wchar_t newfilepath[256]{};
			get_list_file(newfilepath, newdeckname);
			if (!newfilepath[0])
				break;
			if (FileSystem::IsFileExists(newfilepath)) {
				ChangeCategory(newdeckname);
				mainGame->stACMessage->setText(dataManager.GetSysString(1475));
				mainGame->PopupElement(mainGame->wACMessage, 40);
				break;
			}
			if (!FileSystem::Rename(oldfilepath, newfilepath))
				break;
			ChangeCategory(newdeckname);
			break;
		}
		case BUTTON_DELETE_DECK_DM: {
			int catesel = mainGame->lstCategories->getSelected();
			if (catesel < DECK_CATEGORY_NONE)
				break;
			int decksel = mainGame->lstDecks->getSelected();
			if (decksel == -1)
				break;
			wchar_t filepath[256];
			get_list_file(filepath, mainGame->lstDecks->getListItem(decksel));
			if (!DeckManager::DeleteDeck(filepath)) {
				mainGame->stACMessage->setText(dataManager.GetSysString(1476));
				mainGame->PopupElement(mainGame->wACMessage, 40);
				break;
			}
			ChangeCategory();
			break;
		}
		case BUTTON_MOVE_DECK: {
			int oldcatesel = mainGame->lstCategories->getSelected();
			if (oldcatesel < DECK_CATEGORY_NONE)
				break;
			int decksel = mainGame->lstDecks->getSelected();
			if (decksel == -1)
				break;
			int selected = mainGame->cbDMCategory->getSelected();
			if (selected == -1)
				break;
			int new_category_index = mainGame->cbDMCategory->getItemData(selected);
			const wchar_t* newcatename = mainGame->cbDMCategory->getText();
			const wchar_t* deckname = mainGame->lstDecks->getListItem(decksel);
			wchar_t oldfilepath[256];
			get_list_file(oldfilepath, deckname);
			if (!oldfilepath[0])
				break;
			wchar_t newfilepath[256];
			DeckManager::GetDeckFile(newfilepath, new_category_index, newcatename, deckname);
			if (!newfilepath[0])
				break;
			if (FileSystem::IsFileExists(newfilepath)) {
				mainGame->lstCategories->setSelected(new_category_index);
				mainGame->cbDBCategory->setSelected(new_category_index);
				ChangeCategory(deckname);
				mainGame->stACMessage->setText(dataManager.GetSysString(1475));
				mainGame->PopupElement(mainGame->wACMessage, 40);
				break;
			}
			if (!FileSystem::Rename(oldfilepath, newfilepath))
				break;
			mainGame->lstCategories->setSelected(new_category_index);
			mainGame->cbDBCategory->setSelected(new_category_index);
			ChangeCategory(deckname);
			break;
		}
		case BUTTON_COPY_DECK: {
			int oldcatesel = mainGame->lstCategories->getSelected();
			if (oldcatesel == -1)
				break;
			int decksel = mainGame->lstDecks->getSelected();
			if (decksel == -1)
				break;
			int selected = mainGame->cbDMCategory->getSelected();
			if (selected == -1)
				break;
			int new_category_index = mainGame->cbDMCategory->getItemData(selected);
			const wchar_t* newcatename = mainGame->cbDMCategory->getText();
			const wchar_t* deckname = mainGame->lstDecks->getListItem(decksel);
			wchar_t oldfilepath[256];
			get_list_file(oldfilepath, deckname);
			if (!oldfilepath[0])
				break;
			wchar_t newfilepath[256];
			DeckManager::GetDeckFile(newfilepath, new_category_index, newcatename, deckname);
			if (!newfilepath[0])
				break;
			if (FileSystem::IsFileExists(newfilepath)) {
				mainGame->lstCategories->setSelected(new_category_index);
				mainGame->cbDBCategory->setSelected(new_category_index);
				ChangeCategory(deckname);
				mainGame->stACMessage->setText(dataManager.GetSysString(1475));
				mainGame->PopupElement(mainGame->wACMessage, 40);
				break;
			}
			if (!DeckManager::SaveDeck(deckManager.current_deck, newfilepath))
				break;
			mainGame->lstCategories->setSelected(new_category_index);
			mainGame->cbDBCategory->setSelected(new_category_index);
			ChangeCategory(deckname);
			break;
		}
		default:
			break;
		}
		mainGame->HideElement(mainGame->wDMQuery);
		mainGame->stDMMessage2->setVisible(false);
		mainGame->ebDMName->setVisible(false);
		mainGame->cbDMCategory->setVisible(false);
		dmquery_operation = 0;
		EnableManageWindow(true);
		break;
	}
	case BUTTON_DM_CANCEL: {
		mainGame->HideElement(mainGame->wDMQuery);
		mainGame->stDMMessage2->setVisible(false);
		mainGame->ebDMName->setVisible(false);
		mainGame->cbDMCategory->setVisible(false);
		dmquery_operation = 0;
		EnableManageWindow(true);
		break;
	}
	case BUTTON_SIDE_OK: {
		if (deckManager.current_deck.main.size() != pre_mainc
			|| deckManager.current_deck.extra.size() != pre_extrac
			|| deckManager.current_deck.side.size() != pre_sidec) {
			soundManager.PlaySoundEffect(SOUND_INFO);
			mainGame->env->addMessageBox(L"", dataManager.GetSysString(1410));
			break;
		}
		mainGame->ClearCardInfo();
		DuelClient::SendUpdateDeck(deckManager.current_deck);
		break;
	}
	case BUTTON_SIDE_RELOAD: {
		load_current_deck(mainGame->cbCategorySelect, mainGame->cbDeckSelect);
		break;
	}
	case BUTTON_BIG_CARD_ORIG_SIZE: {
		ShowBigCard(bigcard_code, 1);
		break;
	}
	case BUTTON_BIG_CARD_ZOOM_IN: {
		ZoomBigCard(0.2f);
		break;
	}
	case BUTTON_BIG_CARD_ZOOM_OUT: {
		ZoomBigCard(-0.2f);
		break;
	}
	case BUTTON_BIG_CARD_CLOSE: {
		CloseBigCard();
		break;
	}
	case BUTTON_MSG_OK: {
		mainGame->HideElement(mainGame->wMessage);
		mainGame->actionSignal.Set();
		break;
	}
	case BUTTON_YES: {
		mainGame->HideElement(mainGame->wQuery);
		if (!mainGame->is_building || mainGame->is_siding)
			break;
		if (prev_operation == BUTTON_CLEAR_DECK) {
			deckManager.current_deck.clear();
			EnableEditWindow(true);
		}
		else if (prev_operation == BUTTON_DELETE_DECK) {
			wchar_t filepath[256];
			get_deck_file(filepath, mainGame->cbDBDecks->getText());
			if (DeckManager::DeleteDeck(filepath)) {
				ChangeCategory();
				mainGame->stACMessage->setText(dataManager.GetSysString(1338));
				mainGame->PopupElement(mainGame->wACMessage, 40);
			}
			EnableEditWindow(true);
		}
		else if (prev_operation == BUTTON_LEAVE_GAME) {
			Terminate();
		}
		else if (prev_operation == BUTTON_MANAGE_DECK) {
			ShowDeckManage();
		}
		prev_operation = 0;
		break;
	}
	case BUTTON_NO: {
		mainGame->HideElement(mainGame->wQuery);
		prev_operation = 0;
		EnableEditWindow(true);
		break;
	}
	case BUTTON_MARKS_FILTER: {
		mainGame->PopupElement(mainGame->wLinkMarks);
		break;
	}
	case BUTTON_MARKERS_OK: {
		filter_marks = 0;
		if (mainGame->btnMark[0]->isPressed())
			filter_marks |= 0100;
		if (mainGame->btnMark[1]->isPressed())
			filter_marks |= 0200;
		if (mainGame->btnMark[2]->isPressed())
			filter_marks |= 0400;
		if (mainGame->btnMark[3]->isPressed())
			filter_marks |= 0010;
		if (mainGame->btnMark[4]->isPressed())
			filter_marks |= 0040;
		if (mainGame->btnMark[5]->isPressed())
			filter_marks |= 0001;
		if (mainGame->btnMark[6]->isPressed())
			filter_marks |= 0002;
		if (mainGame->btnMark[7]->isPressed())
			filter_marks |= 0004;
		mainGame->HideElement(mainGame->wLinkMarks);
		mainGame->btnMarksFilter->setPressed(filter_marks > 0);
		InstantSearch();
		break;
	}
	case BUTTON_GENERATE_SCRIPT: {
		if (mainGame->cbDBDecks->getSelected() == -1)
			break;
		wchar_t base_name[256]{};
		if (myswprintf(base_name, L"_deck_%ls", mainGame->cbDBDecks->getText()) <= 0)
			break;
		if (DeckManager::GenerateTestScript(deckManager.current_deck, base_name)) {
			mainGame->stACMessage->setText(dataManager.GetSysString(1801));
			mainGame->PopupElement(mainGame->wACMessage, 40);
		}
		break;
	}
	}
}
void DeckBuilder::ComboBoxHandler(const irr::SEvent& event) {
	auto id = event.GUIEvent.Caller->getID();
	switch (id) {
	case COMBOBOX_DBCATEGORY: {
		int catesel = mainGame->cbDBCategory->getSelected();
		ChangeCategory();
		break;
	}
	case COMBOBOX_DBDECKS: {
		int decksel = mainGame->cbDBDecks->getSelected();
		if (decksel == -1)
			break;
		load_current_deck(mainGame->cbDBCategory, mainGame->cbDBDecks);
		is_modified = false;
		break;
	}
	case COMBOBOX_MAINTYPE: {
		mainGame->cbCardType2->setSelected(0);
		mainGame->cbAttribute->setSelected(0);
		mainGame->cbRace->setSelected(0);
		mainGame->ebAttack->setText(L"");
		mainGame->ebDefense->setText(L"");
		mainGame->ebStar->setText(L"");
		mainGame->ebScale->setText(L"");
		switch (mainGame->cbCardType->getSelected()) {
		case 0: {
			mainGame->cbCardType2->setEnabled(false);
			mainGame->cbCardType2->setSelected(0);
			mainGame->cbRace->setEnabled(false);
			mainGame->cbAttribute->setEnabled(false);
			mainGame->ebAttack->setEnabled(false);
			mainGame->ebDefense->setEnabled(false);
			mainGame->ebStar->setEnabled(false);
			mainGame->ebScale->setEnabled(false);
			break;
		}
		case 1: {
			wchar_t normaltuner[32];
			wchar_t normalpen[32];
			wchar_t syntuner[32];
			mainGame->cbCardType2->setEnabled(true);
			mainGame->cbRace->setEnabled(true);
			mainGame->cbAttribute->setEnabled(true);
			mainGame->ebAttack->setEnabled(true);
			mainGame->ebDefense->setEnabled(true);
			mainGame->ebStar->setEnabled(true);
			mainGame->ebScale->setEnabled(true);
			mainGame->cbCardType2->clear();
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1080), 0);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1054), TYPE_MONSTER + TYPE_NORMAL);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1055), TYPE_MONSTER + TYPE_EFFECT);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1056), TYPE_MONSTER + TYPE_FUSION);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1057), TYPE_MONSTER + TYPE_RITUAL);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1063), TYPE_MONSTER + TYPE_SYNCHRO);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1073), TYPE_MONSTER + TYPE_XYZ);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1074), TYPE_MONSTER + TYPE_PENDULUM);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1076), TYPE_MONSTER + TYPE_LINK);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1075), TYPE_MONSTER + TYPE_SPSUMMON);
			myswprintf(normaltuner, L"%ls|%ls", dataManager.GetSysString(1054), dataManager.GetSysString(1062));
			mainGame->cbCardType2->addItem(normaltuner, TYPE_MONSTER + TYPE_NORMAL + TYPE_TUNER);
			myswprintf(normalpen, L"%ls|%ls", dataManager.GetSysString(1054), dataManager.GetSysString(1074));
			mainGame->cbCardType2->addItem(normalpen, TYPE_MONSTER + TYPE_NORMAL + TYPE_PENDULUM);
			myswprintf(syntuner, L"%ls|%ls", dataManager.GetSysString(1063), dataManager.GetSysString(1062));
			mainGame->cbCardType2->addItem(syntuner, TYPE_MONSTER + TYPE_SYNCHRO + TYPE_TUNER);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1062), TYPE_MONSTER + TYPE_TUNER);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1061), TYPE_MONSTER + TYPE_DUAL);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1060), TYPE_MONSTER + TYPE_UNION);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1059), TYPE_MONSTER + TYPE_SPIRIT);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1071), TYPE_MONSTER + TYPE_FLIP);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1072), TYPE_MONSTER + TYPE_TOON);
			break;
		}
		case 2: {
			mainGame->cbCardType2->setEnabled(true);
			mainGame->cbRace->setEnabled(false);
			mainGame->cbAttribute->setEnabled(false);
			mainGame->ebAttack->setEnabled(false);
			mainGame->ebDefense->setEnabled(false);
			mainGame->ebStar->setEnabled(false);
			mainGame->ebScale->setEnabled(false);
			mainGame->cbCardType2->clear();
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1080), 0);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1054), TYPE_SPELL);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1066), TYPE_SPELL + TYPE_QUICKPLAY);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1067), TYPE_SPELL + TYPE_CONTINUOUS);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1057), TYPE_SPELL + TYPE_RITUAL);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1068), TYPE_SPELL + TYPE_EQUIP);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1069), TYPE_SPELL + TYPE_FIELD);
			break;
		}
		case 3: {
			mainGame->cbCardType2->setEnabled(true);
			mainGame->cbRace->setEnabled(false);
			mainGame->cbAttribute->setEnabled(false);
			mainGame->ebAttack->setEnabled(false);
			mainGame->ebDefense->setEnabled(false);
			mainGame->ebStar->setEnabled(false);
			mainGame->ebScale->setEnabled(false);
			mainGame->cbCardType2->clear();
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1080), 0);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1054), TYPE_TRAP);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1067), TYPE_TRAP + TYPE_CONTINUOUS);
			mainGame->cbCardType2->addItem(dataManager.GetSysString(1070), TYPE_TRAP + TYPE_COUNTER);
			break;
		}
		}
		mainGame->env->setFocus(0);
		InstantSearch();
		break;
	}
	case COMBOBOX_SORTTYPE: {
		SortList();
		mainGame->env->setFocus(0);
		break;
	}
	case COMBOBOX_SECONDTYPE: {
		if (mainGame->cbCardType->getSelected() == 1) {
			if (mainGame->cbCardType2->getSelected() == 8) {
				mainGame->ebDefense->setEnabled(false);
				mainGame->ebDefense->setText(L"");
			}
			else {
				mainGame->ebDefense->setEnabled(true);
			}
		}
		mainGame->env->setFocus(0);
		InstantSearch();
		break;
	}
	case COMBOBOX_ATTRIBUTE:
	case COMBOBOX_RACE:
	case COMBOBOX_LIMIT:
		mainGame->env->setFocus(0);
		InstantSearch();
		break;
	}
}
void DeckBuilder::GetHoveredCard() {
	int pre_code = hovered_code;
	hovered_pos = 0;
	hovered_code = 0;
	irr::gui::IGUIElement* root = mainGame->env->getRootGUIElement();
	if(root->getElementFromPoint(mouse_pos) != root)
		return;
	irr::core::vector2di pos = mainGame->ResizeReverse(mouse_pos.X, mouse_pos.Y);
	int x = pos.X;
	int y = pos.Y;
	is_lastcard = 0;
	if(x >= 314 && x <= 794) {
		if(showing_pack) {
			if((x <= 772 || !mainGame->scrPackCards->isVisible()) && y >= 164 && y <= 624) {
				int mainsize = deckManager.current_deck.main.size();
				int lx = 10;
				int dy = 68;
				if(mainsize > 10 * 7)
					lx = 11;
				if(mainsize > 11 * 7)
					lx = 12;
				if(mainsize > 60)
					dy = 66;
				int px;
				int py = (y - 164) / dy;
				hovered_pos = 1;
				if(x >= 750)
					px = lx - 1;
				else
					px = (x - 314) * (lx - 1) / (mainGame->scrPackCards->isVisible() ? 414.0f : 436.0f);
				hovered_seq = py * lx + px + mainGame->scrPackCards->getPos() * lx;
				if(hovered_seq >= mainsize) {
					hovered_seq = -1;
					hovered_code = 0;
				} else {
					hovered_code = deckManager.current_deck.main[hovered_seq]->code;
				}
			}
		} else if(y >= 164 && y <= 435) {
			int lx = 10, px, py = (y - 164) / 68;
			hovered_pos = 1;
			if(deckManager.current_deck.main.size() > DECK_MIN_SIZE)
				lx = (deckManager.current_deck.main.size() - 41) / 4 + 11;
			if(x >= 750)
				px = lx - 1;
			else
				px = (x - 314) * (lx - 1) / 436;
			hovered_seq = py * lx + px;
			if(hovered_seq >= (int)deckManager.current_deck.main.size()) {
				hovered_seq = -1;
				hovered_code = 0;
			} else {
				hovered_code = deckManager.current_deck.main[hovered_seq]->code;
			}
		} else if(y >= 466 && y <= 530) {
			int lx = deckManager.current_deck.extra.size();
			hovered_pos = 2;
			if(lx < 10)
				lx = 10;
			if(x >= 750)
				hovered_seq = lx - 1;
			else
				hovered_seq = (x - 314) * (lx - 1) / 436;
			if(hovered_seq >= (int)deckManager.current_deck.extra.size()) {
				hovered_seq = -1;
				hovered_code = 0;
			} else {
				hovered_code = deckManager.current_deck.extra[hovered_seq]->code;
				if(x >= 772)
					is_lastcard = 1;
			}
		} else if (y >= 564 && y <= 628) {
			int lx = deckManager.current_deck.side.size();
			hovered_pos = 3;
			if(lx < 10)
				lx = 10;
			if(x >= 750)
				hovered_seq = lx - 1;
			else
				hovered_seq = (x - 314) * (lx - 1) / 436;
			if(hovered_seq >= (int)deckManager.current_deck.side.size()) {
				hovered_seq = -1;
				hovered_code = 0;
			} else {
				hovered_code = deckManager.current_deck.side[hovered_seq]->code;
				if(x >= 772)
					is_lastcard = 1;
			}
		}
	} else if(x >= 810 && x <= 995 && y >= 165 && y <= 626) {
		hovered_pos = 4;
		hovered_seq = (y - 165) / 66;
		int current_pos = mainGame->scrFilter->getPos() + hovered_seq;
		if(current_pos >= (int)results.size()) {
			hovered_seq = -1;
			hovered_code = 0;
		} else {
			hovered_code = results[current_pos]->code;
		}
	}
	if(is_draging) {
		dragx = mouse_pos.X;
		dragy = mouse_pos.Y;
	}
	if(!is_draging && pre_code != hovered_code) {
		if(hovered_code)
			mainGame->ShowCardInfo(hovered_code);
	}
}
void DeckBuilder::StartFilter() {
	filter_type = mainGame->cbCardType->getSelected();
	filter_type2 = mainGame->cbCardType2->getItemData(mainGame->cbCardType2->getSelected());
	filter_lm = mainGame->cbLimit->getSelected();
	if(filter_type == 1) {
		filter_attrib = mainGame->cbAttribute->getItemData(mainGame->cbAttribute->getSelected());
		filter_race = mainGame->cbRace->getItemData(mainGame->cbRace->getSelected());
		filter_atk = parse_filter(mainGame->ebAttack->getText(), &filter_atktype);
		filter_def = parse_filter(mainGame->ebDefense->getText(), &filter_deftype);
		filter_lv = parse_filter(mainGame->ebStar->getText(), &filter_lvtype);
		filter_scl = parse_filter(mainGame->ebScale->getText(), &filter_scltype);
	}
	FilterCards();
}
void DeckBuilder::FilterCards() {
	results.clear();
	struct element_t {
		std::wstring keyword;
		std::vector<uint32_t> setcodes;
		enum class type_t {
			all,
			name,
			setcode
		} type{ type_t::all };
		bool exclude{ false };
	};
	const wchar_t* pstr = mainGame->ebCardName->getText();
	int trycode = BufferIO::GetVal(pstr);
	std::wstring str{ pstr };
	std::vector<element_t> query_elements;
	if(mainGame->gameConf.search_multiple_keywords) {
		const wchar_t separator = mainGame->gameConf.search_multiple_keywords == 1 ? L' ' : L'+';
		const wchar_t minussign = L'-';
		const wchar_t quotation = L'\"';
		size_t element_start = 0;
		for(;;) {
			element_start = str.find_first_not_of(separator, element_start);
			if(element_start == std::wstring::npos)
				break;
			element_t element;
			if(str[element_start] == minussign) {
				element.exclude = true;
				element_start++;
			}
			if(element_start >= str.size())
				break;
			if(str[element_start] == L'$') {
				element.type = element_t::type_t::name;
				element_start++;
			} else if(str[element_start] == L'@') {
				element.type = element_t::type_t::setcode;
				element_start++;
			}
			if(element_start >= str.size())
				break;
			wchar_t delimiter = separator;
			if(str[element_start] == quotation) {
				delimiter = quotation;
				element_start++;
			}
			size_t element_end = str.find_first_of(delimiter, element_start);
			if(element_end != std::wstring::npos) {
				size_t length = element_end - element_start;
				element.keyword = str.substr(element_start, length);
			} else
				element.keyword = str.substr(element_start);
			element.setcodes = dataManager.GetSetCodes(element.keyword);
			query_elements.push_back(element);
			if(element_end == std::wstring::npos)
				break;
			element_start = element_end + 1;
		}
	} else {
		element_t element;
		size_t element_start = 0;
		if(str[element_start] == L'$') {
			element.type = element_t::type_t::name;
			element_start++;
		} else if(str[element_start] == L'@') {
			element.type = element_t::type_t::setcode;
			element_start++;
		}
		if(element_start < str.size()) {
			element.keyword = str.substr(element_start);
			element.setcodes = dataManager.GetSetCodes(element.keyword);
			query_elements.push_back(element);
		}
	}
	auto& _datas = dataManager.GetDataTable();
	auto& _strings = dataManager.GetStringTable();
	for (auto& [code, data] : _datas) {
		auto strpointer = _strings.find(code);
		if (strpointer == _strings.end())
			continue;
		const CardString& strings = strpointer->second;
		if(data.type & TYPE_TOKEN)
			continue;
		switch(filter_type) {
		case 1: {
			if(!(data.type & TYPE_MONSTER) || (data.type & filter_type2) != filter_type2)
				continue;
			if(filter_race && data.race != filter_race)
				continue;
			if(filter_attrib && data.attribute != filter_attrib)
				continue;
			if(filter_atktype) {
				if((filter_atktype == 1 && data.attack != filter_atk) || (filter_atktype == 2 && data.attack < filter_atk)
				        || (filter_atktype == 3 && data.attack <= filter_atk) || (filter_atktype == 4 && (data.attack > filter_atk || data.attack < 0))
				        || (filter_atktype == 5 && (data.attack >= filter_atk || data.attack < 0)) || (filter_atktype == 6 && data.attack != -2))
					continue;
			}
			if(filter_deftype) {
				if((filter_deftype == 1 && data.defense != filter_def) || (filter_deftype == 2 && data.defense < filter_def)
				        || (filter_deftype == 3 && data.defense <= filter_def) || (filter_deftype == 4 && (data.defense > filter_def || data.defense < 0))
				        || (filter_deftype == 5 && (data.defense >= filter_def || data.defense < 0)) || (filter_deftype == 6 && data.defense != -2)
				        || (data.type & TYPE_LINK))
					continue;
			}
			if(filter_lvtype) {
				if((filter_lvtype == 1 && data.level != filter_lv) || (filter_lvtype == 2 && data.level < filter_lv)
				        || (filter_lvtype == 3 && data.level <= filter_lv) || (filter_lvtype == 4 && data.level > filter_lv)
				        || (filter_lvtype == 5 && data.level >= filter_lv) || filter_lvtype == 6)
					continue;
			}
			if(filter_scltype) {
				if((filter_scltype == 1 && data.lscale != filter_scl) || (filter_scltype == 2 && data.lscale < filter_scl)
				        || (filter_scltype == 3 && data.lscale <= filter_scl) || (filter_scltype == 4 && (data.lscale > filter_scl))
				        || (filter_scltype == 5 && (data.lscale >= filter_scl)) || filter_scltype == 6
				        || !(data.type & TYPE_PENDULUM))
					continue;
			}
			break;
		}
		case 2: {
			if(!(data.type & TYPE_SPELL))
				continue;
			if(filter_type2 && data.type != filter_type2)
				continue;
			break;
		}
		case 3: {
			if(!(data.type & TYPE_TRAP))
				continue;
			if(filter_type2 && data.type != filter_type2)
				continue;
			break;
		}
		}
		if(filter_effect && !(data.category & filter_effect))
			continue;
		if(filter_marks && (data.link_marker & filter_marks) != filter_marks)
			continue;
		if(filter_lm) {
			if(filter_lm <= 3 && (!filterList->content.count(code) || filterList->content.at(code) != filter_lm - 1))
				continue;
			if(filter_lm == 4 && !(data.ot & AVAIL_OCG))
				continue;
			if(filter_lm == 5 && !(data.ot & AVAIL_TCG))
				continue;
			if(filter_lm == 6 && !(data.ot & AVAIL_SC))
				continue;
			if(filter_lm == 7 && !(data.ot & AVAIL_CUSTOM))
				continue;
			if(filter_lm == 8 && ((data.ot & AVAIL_OCGTCG) != AVAIL_OCGTCG))
				continue;
		}
		bool is_target = true;
		for (auto elements_iterator = query_elements.begin(); elements_iterator != query_elements.end(); ++elements_iterator) {
			bool match = false;
			if (elements_iterator->type == element_t::type_t::name) {
				match = DataManager::CardNameContains(strings.name.c_str(), elements_iterator->keyword.c_str());
			} else if (elements_iterator->type == element_t::type_t::setcode) {
				match = data.is_setcodes(elements_iterator->setcodes);
			} else if (trycode && data.get_original_code() == trycode) {
				match = true;
			} else {
				match = DataManager::CardNameContains(strings.name.c_str(), elements_iterator->keyword.c_str())
					|| strings.text.find(elements_iterator->keyword) != std::wstring::npos
					|| data.is_setcodes(elements_iterator->setcodes);
			}
			if(elements_iterator->exclude)
				match = !match;
			if(!match) {
				is_target = false;
				break;
			}
		}
		if(is_target)
			results.push_back(&data);
		else
			continue;
	}
	myswprintf(result_string, L"%zu", results.size());
	if(results.size() > 7) {
		mainGame->scrFilter->setVisible(true);
		mainGame->scrFilter->setMax(results.size() - 7);
		mainGame->scrFilter->setPos(0);
	} else {
		mainGame->scrFilter->setVisible(false);
		mainGame->scrFilter->setPos(0);
	}
	SortList();
}
void DeckBuilder::InstantSearch() {
	if(mainGame->gameConf.auto_search_limit >= 0 && ((int)std::wcslen(mainGame->ebCardName->getText()) >= mainGame->gameConf.auto_search_limit))
		StartFilter();
}
void DeckBuilder::ClearSearch() {
	mainGame->cbCardType->setSelected(0);
	mainGame->cbCardType2->setSelected(0);
	mainGame->cbCardType2->setEnabled(false);
	mainGame->cbRace->setEnabled(false);
	mainGame->cbAttribute->setEnabled(false);
	mainGame->ebAttack->setEnabled(false);
	mainGame->ebDefense->setEnabled(false);
	mainGame->ebStar->setEnabled(false);
	mainGame->ebScale->setEnabled(false);
	mainGame->ebCardName->setText(L"");
	mainGame->scrFilter->setVisible(false);
	mainGame->scrFilter->setPos(0);
	ClearFilter();
	results.clear();
	myswprintf(result_string, L"%d", 0);
}
void DeckBuilder::ClearFilter() {
	mainGame->cbAttribute->setSelected(0);
	mainGame->cbRace->setSelected(0);
	mainGame->cbLimit->setSelected(0);
	mainGame->ebAttack->setText(L"");
	mainGame->ebDefense->setText(L"");
	mainGame->ebStar->setText(L"");
	mainGame->ebScale->setText(L"");
	filter_effect = 0;
	for(int i = 0; i < 32; ++i)
		mainGame->chkCategory[i]->setChecked(false);
	filter_marks = 0;
	for(int i = 0; i < 8; i++)
		mainGame->btnMark[i]->setPressed(false);
	mainGame->btnEffectFilter->setPressed(false);
	mainGame->btnMarksFilter->setPressed(false);
}
void DeckBuilder::SortList() {
	auto left = results.begin();
	const wchar_t* pstr = mainGame->ebCardName->getText();
	for(auto it = results.begin(); it != results.end(); ++it) {
		if(std::wcscmp(pstr, dataManager.GetName((*it)->code)) == 0) {
			std::iter_swap(left, it);
			++left;
		}
	}
	switch(mainGame->cbSortType->getSelected()) {
	case 0:
		std::sort(left, results.end(), DataManager::deck_sort_lv);
		break;
	case 1:
		std::sort(left, results.end(), DataManager::deck_sort_atk);
		break;
	case 2:
		std::sort(left, results.end(), DataManager::deck_sort_def);
		break;
	case 3:
		std::sort(left, results.end(), DataManager::deck_sort_name);
		break;
	}
}

void DeckBuilder::RefreshDeckList() {
	mainGame->lstDecks->clear();
	for (int i = 0; i < (int)mainGame->cbDBDecks->getItemCount(); ++i) {
		mainGame->lstDecks->addItem(mainGame->cbDBDecks->getItem(i));
	}
}
void DeckBuilder::RefreshReadonly(int catesel) {
	bool hasDeck = mainGame->cbDBDecks->getItemCount() != 0;
	readonly = catesel < DECK_CATEGORY_NONE;
	showing_pack = catesel == DECK_CATEGORY_PACK;
	mainGame->btnSaveDeck->setEnabled(!readonly);
	mainGame->btnSaveDeckAs->setEnabled(!readonly);
	mainGame->btnClearDeck->setEnabled(!readonly);
	mainGame->btnShuffleDeck->setEnabled(!showing_pack);
	mainGame->btnSortDeck->setEnabled(!showing_pack);
	mainGame->btnDeleteDeck->setEnabled(hasDeck && !readonly);
	mainGame->btnRenameCategory->setEnabled(catesel > DECK_CATEGORY_SEPARATOR);
	mainGame->btnDeleteCategory->setEnabled(catesel > DECK_CATEGORY_SEPARATOR);
	mainGame->btnNewDeck->setEnabled(!readonly);
	mainGame->btnRenameDeck->setEnabled(hasDeck && !readonly);
	mainGame->btnDMDeleteDeck->setEnabled(hasDeck && !readonly);
	mainGame->btnMoveDeck->setEnabled(hasDeck && !readonly);
	mainGame->btnCopyDeck->setEnabled(hasDeck);
}
void DeckBuilder::RefreshPackListScroll() {
	if(showing_pack) {
		mainGame->scrPackCards->setPos(0);
		int mainsize = deckManager.current_deck.main.size();
		if(mainsize <= 7 * 12) {
			mainGame->scrPackCards->setVisible(false);
		} else {
			mainGame->scrPackCards->setVisible(true);
			mainGame->scrPackCards->setMax((int)ceil(((float)mainsize - 7 * 12) / 12.0f));
		}
	} else {
		mainGame->scrPackCards->setVisible(false);
		mainGame->scrPackCards->setPos(0);
	}
}
void DeckBuilder::ChangeCategory(const wchar_t* deck_name) {
	mainGame->RefreshDeck(mainGame->cbDBCategory, mainGame->cbDBDecks);
	RefreshDeckList();
	RefreshReadonly(mainGame->cbDBCategory->getSelected());
	is_modified = false;
	int sel = 0;
	if(deck_name) {
		for (int i = 0; i < (int)mainGame->cbDBDecks->getItemCount(); ++i) {
			if (mywcsncasecmp(mainGame->cbDBDecks->getItem(i), deck_name, 256) == 0) {
				sel = i;
				break;
			}
		}
	}
	mainGame->cbDBDecks->setSelected(sel);
	mainGame->lstDecks->setSelected(sel);
	deckManager.LoadCurrentDeck(mainGame->cbDBCategory->getSelected(), mainGame->cbDBCategory->getText(), mainGame->cbDBDecks->getText());
}
void DeckBuilder::ShowDeckManage() {
	wchar_t category_name[256]{};
	wchar_t deck_name[256]{};
	const wchar_t* current_deck = nullptr;
	BufferIO::CopyWideString(mainGame->cbDBCategory->getText(), category_name);
	if (mainGame->cbDBDecks->getSelected() >= 0) {
		BufferIO::CopyWideString(mainGame->cbDBDecks->getText(), deck_name);
		current_deck = deck_name;
	}
	mainGame->RefreshCategoryDeck(mainGame->cbDBCategory, mainGame->cbDBDecks, false);
	irr::gui::IGUIListBox* lstCategories = mainGame->lstCategories;
	lstCategories->clear();
	for (int i = 0; i < (int)mainGame->cbDBCategory->getItemCount(); ++i) {
		lstCategories->addItem(mainGame->cbDBCategory->getItem(i));	
	}
	lstCategories->setSelected(DECK_CATEGORY_NONE);
	for (int i = 0; i < (int)mainGame->cbDBCategory->getItemCount(); ++i) {
		if (std::wcscmp(mainGame->cbDBCategory->getItem(i), category_name) == 0) {
			mainGame->cbDBCategory->setSelected(i);
			lstCategories->setSelected(i);
		}
	}
	ChangeCategory(current_deck);
	EnableEditWindow(false);
	mainGame->PopupElement(mainGame->wDeckManage);
}

void DeckBuilder::ShowBigCard(int code, float zoom) {
	bigcard_code = code;
	bigcard_zoom = zoom;
	auto img = imageManager.GetBigPicture(code, zoom);
	mainGame->imgBigCard->setImage(img);
	auto size = img->getSize();
	irr::s32 left = mainGame->window_size.Width / 2 - size.Width / 2;
	irr::s32 top = mainGame->window_size.Height / 2 - size.Height / 2;
	mainGame->imgBigCard->setRelativePosition(irr::core::recti(0, 0, size.Width, size.Height));
	mainGame->wBigCard->setRelativePosition(irr::core::recti(left, top, left + size.Width, top + size.Height));
	mainGame->gMutex.lock();
	mainGame->btnBigCardOriginalSize->setVisible(true);
	mainGame->btnBigCardZoomIn->setVisible(true);
	mainGame->btnBigCardZoomOut->setVisible(true);
	mainGame->btnBigCardClose->setVisible(true);
	mainGame->ShowElement(mainGame->wBigCard);
	mainGame->env->getRootGUIElement()->bringToFront(mainGame->wBigCard);
	mainGame->gMutex.unlock();
}
void DeckBuilder::ZoomBigCard(float delta, irr::s32 centerx, irr::s32 centery) {
	if (bigcard_zoom + delta > 4.0f || bigcard_zoom + delta < 0.2f)
		return;
	bigcard_zoom += delta;
	auto img = imageManager.GetBigPicture(bigcard_code, bigcard_zoom);
	mainGame->imgBigCard->setImage(img);
	auto& size = img->getSize();
	auto pos = mainGame->wBigCard->getRelativePosition();
	if(centerx == -1) {
		centerx = pos.UpperLeftCorner.X + pos.getWidth() / 2;
		centery = pos.UpperLeftCorner.Y + pos.getHeight() * 0.444f;
	}
	float posx = (float)(centerx - pos.UpperLeftCorner.X) / pos.getWidth();
	float posy = (float)(centery - pos.UpperLeftCorner.Y) / pos.getHeight();
	irr::s32 left = centerx - size.Width * posx;
	irr::s32 top = centery - size.Height * posy;
	mainGame->imgBigCard->setRelativePosition(irr::core::recti(0, 0, size.Width, size.Height));
	mainGame->wBigCard->setRelativePosition(irr::core::recti(left, top, left + size.Width, top + size.Height));
}
void DeckBuilder::CloseBigCard() {
	mainGame->HideElement(mainGame->wBigCard);
	mainGame->btnBigCardOriginalSize->setVisible(false);
	mainGame->btnBigCardZoomIn->setVisible(false);
	mainGame->btnBigCardZoomOut->setVisible(false);
	mainGame->btnBigCardClose->setVisible(false);
}
void DeckBuilder::EnableEditWindow(bool enabled) {
	mainGame->cbDBCategory->setEnabled(enabled);
	mainGame->cbDBDecks->setEnabled(enabled);
	mainGame->ebDeckname->setEnabled(enabled);
}
void DeckBuilder::EnableManageWindow(bool enabled) {
	mainGame->wDeckManage->setEnabled(enabled);
	mainGame->lstCategories->setEnabled(enabled);
	mainGame->lstDecks->setEnabled(enabled);
}

bool DeckBuilder::push_main(const CardDataC* pointer, int seq) {
	if(pointer->type & (TYPE_FUSION | TYPE_SYNCHRO | TYPE_XYZ | TYPE_LINK))
		return false;
	auto& container = deckManager.current_deck.main;
	int maxc = mainGame->is_siding ? DECK_MAX_SIZE + 5 : DECK_MAX_SIZE;
	if((int)container.size() >= maxc)
		return false;
	if(seq >= 0 && seq < (int)container.size())
		container.insert(container.begin() + seq, pointer);
	else
		container.push_back(pointer);
	is_modified = true;
	GetHoveredCard();
	return true;
}
bool DeckBuilder::push_extra(const CardDataC* pointer, int seq) {
	if(!(pointer->type & (TYPE_FUSION | TYPE_SYNCHRO | TYPE_XYZ | TYPE_LINK)))
		return false;
	auto& container = deckManager.current_deck.extra;
	int maxc = mainGame->is_siding ? EXTRA_MAX_SIZE + 5 : EXTRA_MAX_SIZE;
	if((int)container.size() >= maxc)
		return false;
	if(seq >= 0 && seq < (int)container.size())
		container.insert(container.begin() + seq, pointer);
	else
		container.push_back(pointer);
	is_modified = true;
	GetHoveredCard();
	return true;
}
bool DeckBuilder::push_side(const CardDataC* pointer, int seq) {
	auto& container = deckManager.current_deck.side;
	int maxc = mainGame->is_siding ? SIDE_MAX_SIZE + 5 : SIDE_MAX_SIZE;
	if((int)container.size() >= maxc)
		return false;
	if(seq >= 0 && seq < (int)container.size())
		container.insert(container.begin() + seq, pointer);
	else
		container.push_back(pointer);
	is_modified = true;
	GetHoveredCard();
	return true;
}
void DeckBuilder::pop_main(int seq) {
	auto& container = deckManager.current_deck.main;
	container.erase(container.begin() + seq);
	is_modified = true;
	GetHoveredCard();
}
void DeckBuilder::pop_extra(int seq) {
	auto& container = deckManager.current_deck.extra;
	container.erase(container.begin() + seq);
	is_modified = true;
	GetHoveredCard();
}
void DeckBuilder::pop_side(int seq) {
	auto& container = deckManager.current_deck.side;
	container.erase(container.begin() + seq);
	is_modified = true;
	GetHoveredCard();
}
bool DeckBuilder::check_limit(const CardDataC* pointer) {
	auto limitcode = pointer->get_duel_code();
	int limit = 3;
	auto flit = filterList->content.find(limitcode);
	if(flit != filterList->content.end())
		limit = flit->second;
	for (auto& card : deckManager.current_deck.main) {
		if (card->get_duel_code() == limitcode)
			limit--;
	}
	for (auto& card : deckManager.current_deck.extra) {
		if (card->get_duel_code() == limitcode)
			limit--;
	}
	for (auto& card : deckManager.current_deck.side) {
		if (card->get_duel_code() == limitcode)
			limit--;
	}
	if (limit <= 0)
		return false;
	
	uint32_t original_code = pointer->get_original_code();
	bool has_point = false;
	for (auto& point : filterList->pointList) {
		if (point.table.find(original_code) != point.table.end()) {
			has_point = true;
			break;
		}
	}
	if (!has_point)
		return true;
	std::vector<int> sum = DeckManager::GetDeckPoint(deckManager.current_deck, filterList);
	for (size_t i = 0; i < filterList->pointList.size(); ++i) {
		auto& point = filterList->pointList[i];
		auto it = point.table.find(original_code);
		if (it == point.table.end())
			continue;
		sum[i] = sum[i] + it->second;
		if (sum[i] > point.limit)
			return false;
	}
	return true;
}
}
