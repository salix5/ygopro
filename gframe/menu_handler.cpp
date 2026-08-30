#include "config.h"
#include "menu_handler.h"
#include "data_manager.h"
#include "file_system.h"
#include "netserver.h"
#include "duelclient.h"
#include "deck_manager.h"
#include "replay_mode.h"
#include "single_mode.h"
#include "sound_manager.h"
#include "game.h"

namespace ygo {

MenuHandler::MenuHandler(Game* game) : game_(game) {
}
bool MenuHandler::OnEvent(const irr::SEvent& event) {
	if(game_->dField.OnCommonEvent(event))
		return false;
	switch(event.EventType) {
	case irr::EET_GUI_EVENT: {
		irr::gui::IGUIElement* caller = event.GUIEvent.Caller;
		irr::s32 id = caller->getID();
		if(game_->wQuery->isVisible() && id != BUTTON_YES && id != BUTTON_NO) {
			break;
		}
		if(game_->wReplaySave->isVisible() && id != BUTTON_REPLAY_SAVE && id != BUTTON_REPLAY_CANCEL) {
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
				game_->device->closeDevice();
				break;
			}
			case BUTTON_LAN_MODE: {
				game_->btnCreateHost->setEnabled(true);
				game_->btnJoinHost->setEnabled(true);
				game_->btnJoinCancel->setEnabled(true);
				game_->HideElement(game_->wMainMenu);
				game_->ShowElement(game_->wLanWindow);
				break;
			}
			case BUTTON_JOIN_HOST: {
				game_->bot_mode = false;
				game_->TrimText(game_->ebJoinHost);
				game_->TrimText(game_->ebJoinPort);
				wchar_t hoststr[100];
				wchar_t portstr[6];
				BufferIO::CopyWideString(game_->ebJoinHost->getText(), hoststr);
				BufferIO::CopyWideString(game_->ebJoinPort->getText(), portstr);
				char hostname[100];
				char port[6];
				BufferIO::EncodeUTF8(hoststr, hostname);
				BufferIO::EncodeUTF8(portstr, port);
				unsigned int remote_addr = DuelClient::ResolveHostName(hostname, port);
				if(remote_addr == 0) {
					game_->gMutex.lock();
					soundManager.PlaySoundEffect(SOUND_INFO);
					game_->env->addMessageBox(L"", dataManager.GetSysString(1412));
					game_->gMutex.unlock();
					break;
				}
				unsigned int remote_port = std::wcstol(portstr, nullptr, 10);
				BufferIO::CopyWideString(hoststr, game_->gameConf.lasthost);
				BufferIO::CopyWideString(portstr, game_->gameConf.lastport);
				if(DuelClient::StartClient(remote_addr, remote_port, false)) {
					game_->btnCreateHost->setEnabled(false);
					game_->btnJoinHost->setEnabled(false);
					game_->btnJoinCancel->setEnabled(false);
				}
				break;
			}
			case BUTTON_JOIN_CANCEL: {
				game_->HideElement(game_->wLanWindow);
				game_->ShowElement(game_->wMainMenu);
				if(game_->exit_on_return)
					game_->device->closeDevice();
				break;
			}
			case BUTTON_LAN_REFRESH: {
				DuelClient::BeginRefreshHost();
				break;
			}
			case BUTTON_CREATE_HOST: {
				game_->btnHostConfirm->setEnabled(true);
				game_->btnHostCancel->setEnabled(true);
				game_->HideElement(game_->wLanWindow);
				game_->ShowElement(game_->wCreateHost);
				break;
			}
			case BUTTON_HOST_CONFIRM: {
				game_->bot_mode = false;
				BufferIO::CopyWideString(game_->ebServerName->getText(), game_->gameConf.gamename);
				if(!NetServer::StartServer(game_->gameConf.serverport)) {
					soundManager.PlaySoundEffect(SOUND_INFO);
					game_->env->addMessageBox(L"", dataManager.GetSysString(1402));
					break;
				}
				if(!DuelClient::StartClient(0x7f000001, game_->gameConf.serverport)) {
					NetServer::StopServer();
					soundManager.PlaySoundEffect(SOUND_INFO);
					game_->env->addMessageBox(L"", dataManager.GetSysString(1402));
					break;
				}
				game_->btnHostConfirm->setEnabled(false);
				game_->btnHostCancel->setEnabled(false);
				break;
			}
			case BUTTON_HOST_CANCEL: {
				game_->btnCreateHost->setEnabled(true);
				game_->btnJoinHost->setEnabled(true);
				game_->btnJoinCancel->setEnabled(true);
				game_->HideElement(game_->wCreateHost);
				game_->ShowElement(game_->wLanWindow);
				break;
			}
			case BUTTON_HP_DUELIST: {
				game_->cbCategorySelect->setEnabled(true);
				game_->cbDeckSelect->setEnabled(true);
				DuelClient::SendPacketToServer(CTOS_HS_TODUELIST);
				break;
			}
			case BUTTON_HP_OBSERVER: {
				DuelClient::SendPacketToServer(CTOS_HS_TOOBSERVER);
				break;
			}
			case BUTTON_HP_KICK: {
				int index = 0;
				while(index < 4) {
					if(game_->btnHostPrepKick[index] == caller)
						break;
					++index;
				}
				CTOS_Kick csk;
				csk.pos = index;
				DuelClient::SendPacketToServer(CTOS_HS_KICK, csk);
				break;
			}
			case BUTTON_HP_READY: {
				if(game_->cbCategorySelect->getSelected() == -1 || game_->cbDeckSelect->getSelected() == -1 ||
					!deckManager.LoadCurrentDeck(game_->cbCategorySelect->getSelected(), game_->cbCategorySelect->getText(), game_->cbDeckSelect->getText())) {
					game_->gMutex.lock();
					soundManager.PlaySoundEffect(SOUND_INFO);
					game_->env->addMessageBox(L"", dataManager.GetSysString(1406));
					game_->gMutex.unlock();
					break;
				}
				UpdateDeck();
				DuelClient::SendPacketToServer(CTOS_HS_READY);
				game_->cbCategorySelect->setEnabled(false);
				game_->cbDeckSelect->setEnabled(false);
				break;
			}
			case BUTTON_HP_NOTREADY: {
				DuelClient::SendPacketToServer(CTOS_HS_NOTREADY);
				game_->cbCategorySelect->setEnabled(true);
				game_->cbDeckSelect->setEnabled(true);
				break;
			}
			case BUTTON_HP_START: {
				DuelClient::SendPacketToServer(CTOS_HS_START);
				break;
			}
			case BUTTON_HP_CANCEL: {
				DuelClient::StopClient();
				game_->btnCreateHost->setEnabled(true);
				game_->btnJoinHost->setEnabled(true);
				game_->btnJoinCancel->setEnabled(true);
				game_->btnStartBot->setEnabled(true);
				game_->btnBotCancel->setEnabled(true);
				game_->HideElement(game_->wHostPrepare);
				if(game_->bot_mode)
					game_->ShowElement(game_->wSinglePlay);
				else
					game_->ShowElement(game_->wLanWindow);
				game_->wChat->setVisible(false);
				if(game_->exit_on_return)
					game_->device->closeDevice();
				break;
			}
			case BUTTON_REPLAY_MODE: {
				EnableReplayWindow(true);
				game_->HideElement(game_->wMainMenu);
				game_->ShowElement(game_->wReplay);
				game_->ebRepStartTurn->setText(L"1");
				game_->stReplayInfo->setText(L"");
				game_->RefreshReplay();
				break;
			}
			case BUTTON_SINGLE_MODE: {
				game_->HideElement(game_->wMainMenu);
				game_->ShowElement(game_->wSinglePlay);
				game_->RefreshSingleplay();
				game_->RefreshBot();
				break;
			}
			case BUTTON_LOAD_REPLAY: {
				int start_turn = 1;
				if(game_->open_file) {
					game_->open_file = false;
					if (!ReplayMode::cur_replay.OpenReplay(game_->open_file_name)) {
						if (game_->exit_on_return)
							game_->device->closeDevice();
						break;
					}
				} else {
					auto selected = game_->lstReplayList->getSelected();
					if(selected == -1)
						break;
					wchar_t replay_path[256]{};
					myswprintf(replay_path, L"./replay/%ls", game_->lstReplayList->getListItem(selected));
					if (!ReplayMode::cur_replay.OpenReplay(replay_path))
						break;
					start_turn = std::wcstol(game_->ebRepStartTurn->getText(), nullptr, 10);
				}
				game_->ClearCardInfo();
				game_->wCardImg->setVisible(true);
				game_->wInfos->setVisible(true);
				game_->wReplay->setVisible(true);
				game_->wReplayControl->setVisible(true);
				game_->btnReplayStart->setVisible(false);
				game_->btnReplayPause->setVisible(true);
				game_->btnReplayStep->setVisible(false);
				game_->btnReplayUndo->setVisible(false);
				game_->wPhase->setVisible(true);
				game_->dField.Clear();
				game_->HideElement(game_->wReplay);
				game_->device->setEventReceiver(&game_->dField);
				if(start_turn == 1)
					start_turn = 0;
				ReplayMode::StartReplay(start_turn);
				break;
			}
			case BUTTON_DELETE_REPLAY: {
				int sel = game_->lstReplayList->getSelected();
				if(sel == -1)
					break;
				game_->gMutex.lock();
				EnableReplayWindow(false);
				wchar_t textBuffer[256];
				myswprintf(textBuffer, L"%ls\n%ls", game_->lstReplayList->getListItem(sel), dataManager.GetSysString(1363));
				game_->SetStaticText(game_->stQMessage, 310, game_->guiFont, textBuffer);
				game_->PopupElement(game_->wQuery);
				game_->gMutex.unlock();
				prev_operation = id;
				break;
			}
			case BUTTON_RENAME_REPLAY: {
				int sel = game_->lstReplayList->getSelected();
				if(sel == -1)
					break;
				game_->gMutex.lock();
				EnableReplayWindow(false);
				game_->wReplaySave->setText(dataManager.GetSysString(1364));
				game_->ebRSName->setText(game_->lstReplayList->getListItem(sel));
				game_->PopupElement(game_->wReplaySave);
				game_->gMutex.unlock();
				save_operation = id;
				break;
			}
			case BUTTON_CANCEL_REPLAY: {
				game_->HideElement(game_->wReplay);
				game_->ShowElement(game_->wMainMenu);
				break;
			}
			case BUTTON_EXPORT_DECK: {
				auto selected = game_->lstReplayList->getSelected();
				if(selected == -1)
					break;
				Replay replay;
				wchar_t replay_filename[256]{};
				wchar_t namebuf[4][20]{};
				wchar_t filename[256]{};
				wchar_t replay_path[256]{};
				BufferIO::CopyWideString(game_->lstReplayList->getListItem(selected), replay_filename);
				size_t len = std::wcslen(replay_filename);
				constexpr size_t extension_len = 4;
				if (len < extension_len)
					break;
				if (myswprintf(replay_path, L"./replay/%ls", replay_filename) <= 0)
					break;
				if (!replay.OpenReplay(replay_path))
					break;
				if (replay.pheader.base.flag & REPLAY_SINGLE_MODE)
					break;
				for (size_t i = 0; i < replay.decks.size(); ++i) {
					BufferIO::CopyWideString(replay.players[Replay::GetDeckPlayer(i)].c_str(), namebuf[i]);
					FileUtils::SafeFileName(namebuf[i]);
				}
				replay_filename[len - extension_len] = 0;
				for (size_t i = 0; i < replay.decks.size(); ++i) {
					if (myswprintf(filename, L"./deck/%ls-p%d %ls.ydk", replay_filename, i + 1, namebuf[i]) <= 0)
						continue;
					replay.SaveDeck(i, filename);
				}
				game_->stACMessage->setText(dataManager.GetSysString(1335));
				game_->PopupElement(game_->wACMessage, 20);
				break;
			}
			case BUTTON_BOT_START: {
				int sel = game_->lstBotList->getSelected();
				if(sel == -1)
					break;
				game_->bot_mode = true;
				constexpr unsigned int localhost = 0x7f000001;
				unsigned short bot_server_port = 0;
				unsigned int bot_server_listen = localhost;
				bool bot_server_public = game_->gameConf.bot_room_public;
				if(bot_server_public) {
					bot_server_port = game_->gameConf.serverport;
					bot_server_listen = 0; // INADDR_ANY
				}
				if(!NetServer::StartServer(bot_server_port, bot_server_listen, &bot_server_port, bot_server_public)) {
					soundManager.PlaySoundEffect(SOUND_INFO);
					game_->env->addMessageBox(L"", dataManager.GetSysString(1402));
					break;
				}
				std::vector<std::wstring> processArgs;
				wchar_t arg1[512];
				if(game_->botInfo[sel].select_deckfile) {
					wchar_t botdeck[256];
					DeckManager::GetDeckFile(botdeck, game_->cbBotDeckCategory->getSelected(), game_->cbBotDeckCategory->getText(), game_->cbBotDeck->getText());
					myswprintf(arg1, L"%ls DeckFile='%ls'", game_->botInfo[sel].command, botdeck);
				}
				else
					myswprintf(arg1, L"%ls", game_->botInfo[sel].command);
				processArgs.push_back(arg1);
				int flag = 0;
				flag += (game_->chkBotHand->isChecked() ? 0x1 : 0);
				processArgs.push_back(std::to_wstring(flag));
				processArgs.push_back(std::to_wstring(bot_server_port));
				game_->pending_bot_args = processArgs;
				game_->bot_pending = true;
				if(!DuelClient::StartClient(localhost, bot_server_port)) {
					game_->bot_pending = false;
					game_->pending_bot_args.clear();
					NetServer::StopServer();
					soundManager.PlaySoundEffect(SOUND_INFO);
					game_->env->addMessageBox(L"", dataManager.GetSysString(1402));
					break;
				}
				game_->btnStartBot->setEnabled(false);
				game_->btnBotCancel->setEnabled(false);
				break;
			}
			case BUTTON_LOAD_SINGLEPLAY: {
				if(!game_->open_file && game_->lstSinglePlayList->getSelected() == -1)
					break;
				game_->singleSignal.SetNoWait(false);
				SingleMode::StartPlay();
				break;
			}
			case BUTTON_CANCEL_SINGLEPLAY: {
				game_->HideElement(game_->wSinglePlay);
				game_->ShowElement(game_->wMainMenu);
				break;
			}
			case BUTTON_DECK_EDIT: {
				game_->OpenDeckBuilder(false);
				break;
			}
			case BUTTON_YES: {
				game_->HideElement(game_->wQuery);
				if(prev_operation == BUTTON_DELETE_REPLAY) {
					int sel = game_->lstReplayList->getSelected();
					if(Replay::DeleteReplay(game_->lstReplayList->getListItem(sel))) {
						game_->stReplayInfo->setText(L"");
						game_->lstReplayList->removeItem(sel);
					}
				}
				prev_operation = 0;
				EnableReplayWindow(true);
				break;
			}
			case BUTTON_NO: {
				game_->HideElement(game_->wQuery);
				prev_operation = 0;
				EnableReplayWindow(true);
				break;
			}
			case BUTTON_REPLAY_SAVE: {
				game_->HideElement(game_->wReplaySave);
				if (save_operation == BUTTON_RENAME_REPLAY) {
					wchar_t newname[256]{};
					BufferIO::CopyWideString(game_->ebRSName->getText(), newname);
					if (!IsExtension(newname, L".yrp")) {
						if (myswprintf(newname, L"%ls.yrp", game_->ebRSName->getText()) <= 0) {
							save_operation = 0;
							EnableReplayWindow(true);
							break;
						}
					}
					int sel = game_->lstReplayList->getSelected();
					if(Replay::RenameReplay(game_->lstReplayList->getListItem(sel), newname)) {
						game_->lstReplayList->setItem(sel, newname, -1);
					} else {
						game_->env->addMessageBox(L"", dataManager.GetSysString(1365));
					}
				}
				save_operation = 0;
				EnableReplayWindow(true);
				break;
			}
			case BUTTON_REPLAY_CANCEL: {
				game_->HideElement(game_->wReplaySave);
				save_operation = 0;
				EnableReplayWindow(true);
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_LISTBOX_CHANGED: {
			switch(id) {
			case LISTBOX_LAN_HOST: {
				int sel = game_->lstHostList->getSelected();
				if(sel == -1)
					break;
				int addr = DuelClient::hosts[sel].ipaddr;
				int port = DuelClient::hosts[sel].port;
				wchar_t buf[20];
				myswprintf(buf, L"%d.%d.%d.%d", addr & 0xff, (addr >> 8) & 0xff, (addr >> 16) & 0xff, (addr >> 24) & 0xff);
				game_->ebJoinHost->setText(buf);
				myswprintf(buf, L"%d", port);
				game_->ebJoinPort->setText(buf);
				break;
			}
			case LISTBOX_REPLAY_LIST: {
				int sel = game_->lstReplayList->getSelected();
				if (sel == -1)
					break;
				auto filename = game_->lstReplayList->getListItem(sel);
				wchar_t replay_path[256]{};
				myswprintf(replay_path, L"./replay/%ls", filename);
				if (!temp_replay.OpenReplay(replay_path)) {
					game_->stReplayInfo->setText(L"Error");
					break;
				}
				wchar_t infobuf[256]{};
				std::wstring repinfo;
				time_t curtime;
				const auto& rh = temp_replay.pheader.base;
				if(temp_replay.pheader.base.flag & REPLAY_UNIFORM)
					curtime = rh.start_time;
				else{
					curtime = rh.seed;
					wchar_t version_info[256]{};
					myswprintf(version_info, L"version 0x%X\n", rh.version);
					repinfo.append(version_info);
				}
				std::wcsftime(infobuf, sizeof infobuf / sizeof infobuf[0], L"%Y/%m/%d %H:%M:%S\n", std::localtime(&curtime));
				repinfo.append(infobuf);
				if (rh.flag & REPLAY_SINGLE_MODE) {
					wchar_t path[256]{};
					BufferIO::DecodeUTF8(temp_replay.script_name.c_str(), path);
					repinfo.append(path);
					repinfo.append(L"\n");
				}
				const auto& player_names = temp_replay.players;
				if(rh.flag & REPLAY_TAG)
					myswprintf(infobuf, L"%ls\n%ls\n===VS===\n%ls\n%ls\n", player_names[0].c_str(), player_names[1].c_str(), player_names[2].c_str(), player_names[3].c_str());
				else
					myswprintf(infobuf, L"%ls\n===VS===\n%ls\n", player_names[0].c_str(), player_names[1].c_str());
				repinfo.append(infobuf);
				game_->ebRepStartTurn->setText(L"1");
				game_->SetStaticText(game_->stReplayInfo, 180, game_->guiFont, repinfo.c_str());
				break;
			}
			case LISTBOX_SINGLEPLAY_LIST: {
				int sel = game_->lstSinglePlayList->getSelected();
				if(sel == -1)
					break;
				const wchar_t* name = game_->lstSinglePlayList->getListItem(sel);
				wchar_t fname[256];
				myswprintf(fname, L"./single/%ls", name);
				FILE* fp = FileUtils::mywfopen(fname, "r");
				if(!fp) {
					game_->stSinglePlayInfo->setText(L"");
					break;
				}
				char linebuf[1024];
				wchar_t wlinebuf[1024];
				std::wstring message = L"";
				bool in_message = false;
				while(std::fgets(linebuf, 1024, fp)) {
					if(!std::strncmp(linebuf, "--[[message", 11)) {
						size_t len = std::strlen(linebuf);
						char* msgend = std::strrchr(linebuf, ']');
						if(len <= 13) {
							in_message = true;
							continue;
						} else if(len > 15 && msgend) {
							*(msgend - 1) = '\0';
							BufferIO::DecodeUTF8(linebuf + 12, wlinebuf);
							message.append(wlinebuf);
							break;
						}
					}
					if(!std::strncmp(linebuf, "]]", 2)) {
						in_message = false;
						break;
					}
					if(in_message) {
						BufferIO::DecodeUTF8(linebuf, wlinebuf);
						message.append(wlinebuf);
					}
				}
				std::fclose(fp);
				game_->SetStaticText(game_->stSinglePlayInfo, 200, game_->guiFont, message.c_str());
				break;
			}
			case LISTBOX_BOT_LIST: {
				int sel = game_->lstBotList->getSelected();
				if(sel == -1)
					break;
				game_->SetStaticText(game_->stBotInfo, 200, game_->guiFont, game_->botInfo[sel].desc);
				game_->cbBotDeckCategory->setVisible(game_->botInfo[sel].select_deckfile);
				game_->cbBotDeck->setVisible(game_->botInfo[sel].select_deckfile);
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
				game_->env->setFocus(game_->wHostPrepare);
				if(static_cast<irr::gui::IGUICheckBox*>(caller)->isChecked()) {
					if(game_->cbCategorySelect->getSelected() == -1 || game_->cbDeckSelect->getSelected() == -1 ||
						!deckManager.LoadCurrentDeck(game_->cbCategorySelect->getSelected(), game_->cbCategorySelect->getText(), game_->cbDeckSelect->getText())) {
						game_->gMutex.lock();
						static_cast<irr::gui::IGUICheckBox*>(caller)->setChecked(false);
						soundManager.PlaySoundEffect(SOUND_INFO);
						game_->env->addMessageBox(L"", dataManager.GetSysString(1406));
						game_->gMutex.unlock();
						break;
					}
					UpdateDeck();
					DuelClient::SendPacketToServer(CTOS_HS_READY);
					game_->cbCategorySelect->setEnabled(false);
					game_->cbDeckSelect->setEnabled(false);
				} else {
					DuelClient::SendPacketToServer(CTOS_HS_NOTREADY);
					game_->cbCategorySelect->setEnabled(true);
					game_->cbDeckSelect->setEnabled(true);
				}
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_COMBO_BOX_CHANGED: {
			switch(id) {
			case COMBOBOX_BOT_RULE: {
				game_->RefreshBot();
				break;
			}
			case COMBOBOX_HP_CATEGORY: {
				int catesel = game_->cbCategorySelect->getSelected();
				if(catesel == 3) {
					catesel = 2;
					game_->cbCategorySelect->setSelected(2);
				}
				if(catesel >= 0) {
					game_->RefreshDeck(game_->cbCategorySelect, game_->cbDeckSelect);
					game_->cbDeckSelect->setSelected(0);
				}
				break;
			}
			case COMBOBOX_BOT_DECKCATEGORY: {
				int catesel = game_->cbBotDeckCategory->getSelected();
				if(catesel == 3) {
					catesel = 2;
					game_->cbBotDeckCategory->setSelected(2);
				}
				if(catesel >= 0) {
					game_->RefreshDeck(game_->cbBotDeckCategory, game_->cbBotDeck);
					game_->cbBotDeck->setSelected(0);
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

void MenuHandler::EnableReplayWindow(bool enabled) {
	game_->wReplay->setEnabled(enabled);
	game_->lstReplayList->setEnabled(enabled);
	game_->ebRepStartTurn->setEnabled(enabled);
}

void MenuHandler::UpdateDeck() {
	BufferIO::CopyWideString(game_->cbCategorySelect->getText(), game_->gameConf.lastcategory);
	BufferIO::CopyWideString(game_->cbDeckSelect->getText(), game_->gameConf.lastdeck);
	DuelClient::SendUpdateDeck(deckManager.current_deck);
}

}
