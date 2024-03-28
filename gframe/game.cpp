#include "config.h"
#include "game.h"
#include "image_manager.h"
#include "data_manager.h"
#include "deck_manager.h"
#include "sound_manager.h"
#include "materials.h"

const unsigned short PRO_VERSION = 0x1360;

constexpr char config_filename[] = "setting.conf";

namespace ygo {

Game* mainGame;

void DuelInfo::Clear() {
	isStarted = false;
	isFinished = false;
	isReplay = false;
	isReplaySkiping = false;
	isFirst = false;
	isTag = false;
	isSingleMode = false;
	is_shuffling = false;
	tag_player[0] = false;
	tag_player[1] = false;
	isReplaySwapped = false;
	lp[0] = 0;
	lp[1] = 0;
	start_lp = 0;
	duel_rule = 0;
	turn = 0;
	curMsg = 0;
	hostname[0] = 0;
	clientname[0] = 0;
	hostname_tag[0] = 0;
	clientname_tag[0] = 0;
	strLP[0][0] = 0;
	strLP[1][0] = 0;
	vic_string = 0;
	player_type = 0;
	time_player = 0;
	time_limit = 0;
	time_left[0] = 0;
	time_left[1] = 0;
}

bool Game::Initialize() {
	LoadConfig();
	irr::SIrrlichtCreationParameters params = irr::SIrrlichtCreationParameters();
	params.AntiAlias = gameConf.antialias;
	if(gameConf.use_d3d)
		params.DriverType = irr::video::EDT_DIRECT3D9;
	else
		params.DriverType = irr::video::EDT_OPENGL;
	params.WindowSize = irr::core::dimension2d<u32>(gameConf.window_width, gameConf.window_height);
	device = irr::createDeviceEx(params);
	if(!device) {
		ErrorLog("Failed to create Irrlicht Engine device!");
		return false;
	}
#ifndef _DEBUG
	device->getLogger()->setLogLevel(irr::ELOG_LEVEL::ELL_ERROR);
#endif
	xScale = 1;
	yScale = 1;
	linePatternD3D = 0;
	linePatternGL = 0x0f0f;
	waitFrame = 0;
	signalFrame = 0;
	showcard = 0;
	is_attacking = false;
	lpframe = 0;
	lpcstring = 0;
	always_chain = false;
	ignore_chain = false;
	chain_when_avail = false;
	is_building = false;
	menuHandler.prev_operation = 0;
	menuHandler.prev_sel = -1;
	deckManager.LoadLFList();
	driver = device->getVideoDriver();
	driver->setTextureCreationFlag(irr::video::ETCF_CREATE_MIP_MAPS, false);
	driver->setTextureCreationFlag(irr::video::ETCF_OPTIMIZED_FOR_QUALITY, true);
	imageManager.SetDevice(device);
	if(!imageManager.Initial()) {
		ErrorLog("Failed to load textures!");
		return false;
	}
	dataManager.FileSystem = device->getFileSystem();
	if(!dataManager.LoadDatabase("cards.cdb")) {
		ErrorLog("Failed to load card database (cards.cdb)!");
		return false;
	}
	if(!dataManager.LoadStrings("strings.conf")) {
		ErrorLog("Failed to load strings!");
		return false;
	}
	LoadExpansions();
	env = device->getGUIEnvironment();
	numFont = irr::gui::CGUITTFont::createTTFont(env, gameConf.numfont, 16);
	if(!numFont) {
		const wchar_t* numFontPaths[] = {
			L"C:/Windows/Fonts/arialbd.ttf",
			L"/usr/share/fonts/truetype/DroidSansFallbackFull.ttf",
			L"/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc",
			L"/usr/share/fonts/google-noto-cjk/NotoSansCJK-Bold.ttc",
			L"/usr/share/fonts/noto-cjk/NotoSansCJK-Bold.ttc",
			L"/System/Library/Fonts/SFNSTextCondensed-Bold.otf",
			L"/System/Library/Fonts/SFNS.ttf",
			L"./fonts/numFont.ttf",
			L"./fonts/numFont.ttc",
			L"./fonts/numFont.otf"
		};
		for(const wchar_t* path : numFontPaths) {
			myswprintf(gameConf.numfont, path);
			numFont = irr::gui::CGUITTFont::createTTFont(env, gameConf.numfont, 16);
			if(numFont)
				break;
		}
	}
	textFont = irr::gui::CGUITTFont::createTTFont(env, gameConf.textfont, gameConf.textfontsize);
	if(!textFont) {
		const wchar_t* textFontPaths[] = {
			L"C:/Windows/Fonts/msyh.ttc",
			L"C:/Windows/Fonts/msyh.ttf",
			L"C:/Windows/Fonts/simsun.ttc",
			L"C:/Windows/Fonts/YuGothM.ttc",
			L"C:/Windows/Fonts/meiryo.ttc",
			L"C:/Windows/Fonts/msgothic.ttc",
			L"/usr/share/fonts/truetype/DroidSansFallbackFull.ttf",
			L"/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
			L"/usr/share/fonts/google-noto-cjk/NotoSansCJK-Regular.ttc",
			L"/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
			L"/System/Library/Fonts/PingFang.ttc",
			L"./fonts/textFont.ttf",
			L"./fonts/textFont.ttc",
			L"./fonts/textFont.otf"
		};
		for(const wchar_t* path : textFontPaths) {
			myswprintf(gameConf.textfont, path);
			textFont = irr::gui::CGUITTFont::createTTFont(env, gameConf.textfont, gameConf.textfontsize);
			if(textFont)
				break;
		}
	}
	if(!numFont || !textFont) {
		wchar_t fpath[1024];
		fpath[0] = 0;
		FileSystem::TraversalDir(L"./fonts", [&fpath](const wchar_t* name, bool isdir) {
			if(!isdir && wcsrchr(name, '.') && (!mywcsncasecmp(wcsrchr(name, '.'), L".ttf", 4) || !mywcsncasecmp(wcsrchr(name, '.'), L".ttc", 4) || !mywcsncasecmp(wcsrchr(name, '.'), L".otf", 4))) {
				myswprintf(fpath, L"./fonts/%ls", name);
			}
		});
		if(fpath[0] == 0) {
			ErrorLog("Failed to load font(s)!");
			return false;
		}
		if(!numFont) {
			myswprintf(gameConf.numfont, fpath);
			numFont = irr::gui::CGUITTFont::createTTFont(env, gameConf.numfont, 16);
		}
		if(!textFont) {
			myswprintf(gameConf.textfont, fpath);
			textFont = irr::gui::CGUITTFont::createTTFont(env, gameConf.textfont, gameConf.textfontsize);
		}
	}
	adFont = irr::gui::CGUITTFont::createTTFont(env, gameConf.numfont, 12);
	lpcFont = irr::gui::CGUITTFont::createTTFont(env, gameConf.numfont, 48);
	guiFont = irr::gui::CGUITTFont::createTTFont(env, gameConf.textfont, gameConf.textfontsize);
	smgr = device->getSceneManager();
	device->setWindowCaption(L"YGOPro");
	device->setResizable(true);
	if(gameConf.window_maximized)
		device->maximizeWindow();
#ifdef _WIN32
	irr::video::SExposedVideoData exposedData = driver->getExposedVideoData();
	if(gameConf.use_d3d)
		hWnd = reinterpret_cast<HWND>(exposedData.D3D9.HWnd);
	else
		hWnd = reinterpret_cast<HWND>(exposedData.OpenGLWin32.HWnd);
#endif
	SetWindowsIcon();
	//main menu
	wchar_t strbuf[256];
	myswprintf(strbuf, L"YGOPro Version:%X.0%X.%X", PRO_VERSION >> 12, (PRO_VERSION >> 4) & 0xff, PRO_VERSION & 0xf);
	wMainMenu = env->addWindow(rect<s32>(370, 200, 650, 415), false, strbuf);
	wMainMenu->getCloseButton()->setVisible(false);
//	btnLanMode = env->addButton(rect<s32>(10, 30, 270, 60), wMainMenu, BUTTON_LAN_MODE, dataManager.GetSysString(1200));
//	btnSingleMode = env->addButton(rect<s32>(10, 65, 270, 95), wMainMenu, BUTTON_SINGLE_MODE, dataManager.GetSysString(1201));
//	btnReplayMode = env->addButton(rect<s32>(10, 100, 270, 130), wMainMenu, BUTTON_REPLAY_MODE, dataManager.GetSysString(1202));
//	btnTestMode = env->addButton(rect<s32>(10, 135, 270, 165), wMainMenu, BUTTON_TEST_MODE, dataManager.GetSysString(1203));
	btnDeckEdit = env->addButton(rect<s32>(10, 135, 270, 165), wMainMenu, BUTTON_DECK_EDIT, dataManager.GetSysString(1204));
	btnModeExit = env->addButton(rect<s32>(10, 170, 270, 200), wMainMenu, BUTTON_MODE_EXIT, dataManager.GetSysString(1210));
	//lan mode
	wLanWindow = env->addWindow(rect<s32>(220, 100, 800, 520), false, dataManager.GetSysString(1200));
	wLanWindow->getCloseButton()->setVisible(false);
	wLanWindow->setVisible(false);
	env->addStaticText(dataManager.GetSysString(1220), rect<s32>(10, 30, 220, 50), false, false, wLanWindow);
	//create host
	wCreateHost = env->addWindow(rect<s32>(320, 100, 700, 520), false, dataManager.GetSysString(1224));
	wCreateHost->getCloseButton()->setVisible(false);
	wCreateHost->setVisible(false);
	env->addStaticText(dataManager.GetSysString(1226), rect<s32>(20, 30, 220, 50), false, false, wCreateHost);
	//host(single)
	wHostPrepare = env->addWindow(rect<s32>(270, 120, 750, 440), false, dataManager.GetSysString(1250));
	wHostPrepare->getCloseButton()->setVisible(false);
	wHostPrepare->setVisible(false);
	env->addStaticText(dataManager.GetSysString(1254), rect<s32>(10, 210, 110, 230), false, false, wHostPrepare);
	cbCategorySelect = env->addComboBox(rect<s32>(10, 230, 138, 255), wHostPrepare, COMBOBOX_HP_CATEGORY);
	cbCategorySelect->setMaxSelectionRows(10);
	cbDeckSelect = env->addComboBox(rect<s32>(142, 230, 340, 255), wHostPrepare);
	cbDeckSelect->setMaxSelectionRows(10);
	//img
	wCardImg = env->addStaticText(L"", rect<s32>(1, 1, 1 + CARD_IMG_WIDTH + 20, 1 + CARD_IMG_HEIGHT + 18), true, false, 0, -1, true);
	wCardImg->setBackgroundColor(0xc0c0c0c0);
	wCardImg->setVisible(false);
	imgCard = env->addImage(rect<s32>(10, 9, 10 + CARD_IMG_WIDTH, 9 + CARD_IMG_HEIGHT), wCardImg);
	imgCard->setImage(imageManager.tCover[0]);
	showingcode = 0;
	imgCard->setScaleImage(true);
	imgCard->setUseAlphaChannel(true);
	//phase
	wPhase = env->addStaticText(L"", rect<s32>(480, 310, 855, 330));
	wPhase->setVisible(false);
	//tab
	wInfos = env->addTabControl(rect<s32>(1, 275, 301, 639), 0, true);
	wInfos->setTabExtraWidth(16);
	wInfos->setVisible(false);
	//info
	irr::gui::IGUITab* tabInfo = wInfos->addTab(dataManager.GetSysString(1270));
	stName = env->addStaticText(L"", rect<s32>(10, 10, 287, 32), true, false, tabInfo, -1, false);
	stName->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);
	stInfo = env->addStaticText(L"", rect<s32>(15, 37, 296, 60), false, true, tabInfo, -1, false);
	stInfo->setOverrideColor(SColor(255, 0, 0, 255));
	stDataInfo = env->addStaticText(L"", rect<s32>(15, 60, 296, 83), false, true, tabInfo, -1, false);
	stDataInfo->setOverrideColor(SColor(255, 0, 0, 255));
	stSetName = env->addStaticText(L"", rect<s32>(15, 83, 296, 106), false, true, tabInfo, -1, false);
	stSetName->setOverrideColor(SColor(255, 0, 0, 255));
	stText = env->addStaticText(L"", rect<s32>(15, 106, 287, 324), false, true, tabInfo, -1, false);
	scrCardText = env->addScrollBar(false, rect<s32>(267, 106, 287, 324), tabInfo, SCROLL_CARDTEXT);
	scrCardText->setLargeStep(1);
	scrCardText->setSmallStep(1);
	scrCardText->setVisible(false);
	//log
	irr::gui::IGUITab* tabLog = wInfos->addTab(dataManager.GetSysString(1271));
	lstLog = env->addListBox(rect<s32>(10, 10, 290, 290), tabLog, LISTBOX_LOG, false);
	lstLog->setItemHeight(18);
	btnClearLog = env->addButton(rect<s32>(160, 300, 260, 325), tabLog, BUTTON_CLEAR_LOG, dataManager.GetSysString(1272));
	int posX = 0;
	int posY = 0;
	//system
	irr::gui::IGUITab* _tabSystem = wInfos->addTab(dataManager.GetSysString(1273));
	_tabSystem->setRelativePosition(recti(16, 49, 299, 362));
	tabSystem = env->addWindow(recti(0, 0, 250, 300), false, L"", _tabSystem);
	tabSystem->setDrawTitlebar(false);
	tabSystem->getCloseButton()->setVisible(false);
	tabSystem->setDrawBackground(false);
	tabSystem->setDraggable(false);
	scrTabSystem = env->addScrollBar(false, rect<s32>(252, 0, 272, 300), _tabSystem, SCROLL_TAB_SYSTEM);
	scrTabSystem->setLargeStep(1);
	scrTabSystem->setSmallStep(1);
	scrTabSystem->setVisible(false);
	posY = 0;
	chkIgnoreDeckChanges = env->addCheckBox(false, rect<s32>(posX, posY, posX + 260, posY + 25), tabSystem, -1, dataManager.GetSysString(1357));
	chkIgnoreDeckChanges->setChecked(gameConf.chkIgnoreDeckChanges != 0);
	posY += 30;
	chkAutoSearch = env->addCheckBox(false, rect<s32>(posX, posY, posX + 260, posY + 25), tabSystem, CHECKBOX_AUTO_SEARCH, dataManager.GetSysString(1358));
	chkAutoSearch->setChecked(gameConf.auto_search_limit >= 0);
	posY += 30;
	chkMultiKeywords = env->addCheckBox(false, rect<s32>(posX, posY, posX + 260, posY + 25), tabSystem, CHECKBOX_MULTI_KEYWORDS, dataManager.GetSysString(1378));
	chkMultiKeywords->setChecked(gameConf.search_multiple_keywords > 0);
	posY += 30;
	chkPreferExpansionScript = env->addCheckBox(false, rect<s32>(posX, posY, posX + 260, posY + 25), tabSystem, CHECKBOX_PREFER_EXPANSION, dataManager.GetSysString(1379));
	chkPreferExpansionScript->setChecked(gameConf.prefer_expansion_script != 0);
	posY += 30;
	env->addStaticText(dataManager.GetSysString(1282), rect<s32>(posX + 23, posY + 3, posX + 110, posY + 28), false, false, tabSystem);
	btnWinResizeS = env->addButton(rect<s32>(posX + 115, posY, posX + 145, posY + 25), tabSystem, BUTTON_WINDOW_RESIZE_S, dataManager.GetSysString(1283));
	btnWinResizeM = env->addButton(rect<s32>(posX + 150, posY, posX + 180, posY + 25), tabSystem, BUTTON_WINDOW_RESIZE_M, dataManager.GetSysString(1284));
	btnWinResizeL = env->addButton(rect<s32>(posX + 185, posY, posX + 215, posY + 25), tabSystem, BUTTON_WINDOW_RESIZE_L, dataManager.GetSysString(1285));
	btnWinResizeXL = env->addButton(rect<s32>(posX + 220, posY, posX + 250, posY + 25), tabSystem, BUTTON_WINDOW_RESIZE_XL, dataManager.GetSysString(1286));
	posY += 30;
	chkLFlist = env->addCheckBox(false, rect<s32>(posX, posY, posX + 110, posY + 25), tabSystem, CHECKBOX_LFLIST, dataManager.GetSysString(1288));
	chkLFlist->setChecked(gameConf.use_lflist);
	cbLFlist = env->addComboBox(rect<s32>(posX + 115, posY, posX + 250, posY + 25), tabSystem, COMBOBOX_LFLIST);
	cbLFlist->setMaxSelectionRows(6);
	for(unsigned int i = 0; i < deckManager._lfList.size(); ++i)
		cbLFlist->addItem(deckManager._lfList[i].listName.c_str());
	cbLFlist->setEnabled(gameConf.use_lflist);
	cbLFlist->setSelected(gameConf.use_lflist ? gameConf.default_lflist : cbLFlist->getItemCount() - 1);
	posY += 30;
	chkEnableSound = env->addCheckBox(gameConf.enable_sound, rect<s32>(posX, posY, posX + 120, posY + 25), tabSystem, -1, dataManager.GetSysString(1279));
	chkEnableSound->setChecked(gameConf.enable_sound);
	scrSoundVolume = env->addScrollBar(true, rect<s32>(posX + 116, posY + 4, posX + 250, posY + 21), tabSystem, SCROLL_VOLUME);
	scrSoundVolume->setMax(100);
	scrSoundVolume->setMin(0);
	scrSoundVolume->setPos(gameConf.sound_volume * 100);
	scrSoundVolume->setLargeStep(1);
	scrSoundVolume->setSmallStep(1);
	posY += 30;
	chkEnableMusic = env->addCheckBox(gameConf.enable_music, rect<s32>(posX, posY, posX + 120, posY + 25), tabSystem, CHECKBOX_ENABLE_MUSIC, dataManager.GetSysString(1280));
	chkEnableMusic->setChecked(gameConf.enable_music);
	scrMusicVolume = env->addScrollBar(true, rect<s32>(posX + 116, posY + 4, posX + 250, posY + 21), tabSystem, SCROLL_VOLUME);
	scrMusicVolume->setMax(100);
	scrMusicVolume->setMin(0);
	scrMusicVolume->setPos(gameConf.music_volume * 100);
	scrMusicVolume->setLargeStep(1);
	scrMusicVolume->setSmallStep(1);
	posY += 30;
	chkMusicMode = env->addCheckBox(false, rect<s32>(posX, posY, posX + 260, posY + 25), tabSystem, -1, dataManager.GetSysString(1281));
	chkMusicMode->setChecked(gameConf.music_mode != 0);
	elmTabSystemLast = chkMusicMode;
	//message (310)
	wMessage = env->addWindow(rect<s32>(490, 200, 840, 340), false, dataManager.GetSysString(1216));
	wMessage->getCloseButton()->setVisible(false);
	wMessage->setVisible(false);
	stMessage =  env->addStaticText(L"", rect<s32>(20, 20, 350, 100), false, true, wMessage, -1, false);
	stMessage->setTextAlignment(irr::gui::EGUIA_UPPERLEFT, irr::gui::EGUIA_CENTER);
	btnMsgOK = env->addButton(rect<s32>(130, 105, 220, 130), wMessage, BUTTON_MSG_OK, dataManager.GetSysString(1211));
	//auto fade message (310)
	wACMessage = env->addWindow(rect<s32>(490, 240, 840, 300), false, L"");
	wACMessage->getCloseButton()->setVisible(false);
	wACMessage->setVisible(false);
	wACMessage->setDrawBackground(false);
	stACMessage = env->addStaticText(L"", rect<s32>(0, 0, 350, 60), true, true, wACMessage, -1, true);
	stACMessage->setBackgroundColor(0xc0c0c0ff);
	stACMessage->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);
	//yes/no (310)
	wQuery = env->addWindow(rect<s32>(490, 200, 840, 340), false, dataManager.GetSysString(560));
	wQuery->getCloseButton()->setVisible(false);
	wQuery->setVisible(false);
	stQMessage =  env->addStaticText(L"", rect<s32>(20, 20, 350, 100), false, true, wQuery, -1, false);
	stQMessage->setTextAlignment(irr::gui::EGUIA_UPPERLEFT, irr::gui::EGUIA_CENTER);
	btnYes = env->addButton(rect<s32>(100, 105, 150, 130), wQuery, BUTTON_YES, dataManager.GetSysString(1213));
	btnNo = env->addButton(rect<s32>(200, 105, 250, 130), wQuery, BUTTON_NO, dataManager.GetSysString(1214));
	//surrender yes/no (310)
	wSurrender = env->addWindow(rect<s32>(490, 200, 840, 340), false, dataManager.GetSysString(560));
	wSurrender->getCloseButton()->setVisible(false);
	wSurrender->setVisible(false);
	//options (310)
	wOptions = env->addWindow(rect<s32>(490, 200, 840, 340), false, L"");
	wOptions->getCloseButton()->setVisible(false);
	wOptions->setVisible(false);
	stOptions = env->addStaticText(L"", rect<s32>(20, 20, 350, 100), false, true, wOptions, -1, false);
	stOptions->setTextAlignment(irr::gui::EGUIA_UPPERLEFT, irr::gui::EGUIA_CENTER);
	btnOptionOK = env->addButton(rect<s32>(130, 105, 220, 130), wOptions, BUTTON_OPTION_OK, dataManager.GetSysString(1211));
	btnOptionp = env->addButton(rect<s32>(20, 105, 60, 130), wOptions, BUTTON_OPTION_PREV, L"<<<");
	btnOptionn = env->addButton(rect<s32>(290, 105, 330, 130), wOptions, BUTTON_OPTION_NEXT, L">>>");
	for(int i = 0; i < 5; ++i) {
		btnOption[i] = env->addButton(rect<s32>(10, 30 + 40 * i, 340, 60 + 40 * i), wOptions, BUTTON_OPTION_0 + i, L"");
	}
	scrOption = env->addScrollBar(false, rect<s32>(350, 30, 365, 220), wOptions, SCROLL_OPTION_SELECT);
	scrOption->setLargeStep(1);
	scrOption->setSmallStep(1);
	scrOption->setMin(0);
	//announce number
	wANNumber = env->addWindow(rect<s32>(550, 180, 780, 430), false, L"");
	wANNumber->getCloseButton()->setVisible(false);
	wANNumber->setVisible(false);
	//announce card
	wANCard = env->addWindow(rect<s32>(510, 120, 820, 420), false, L"");
	wANCard->getCloseButton()->setVisible(false);
	wANCard->setVisible(false);
	//announce attribute
	wANAttribute = env->addWindow(rect<s32>(500, 200, 830, 285), false, dataManager.GetSysString(562));
	wANAttribute->getCloseButton()->setVisible(false);
	wANAttribute->setVisible(false);
	//announce race
	wANRace = env->addWindow(rect<s32>(480, 200, 850, 410), false, dataManager.GetSysString(563));
	wANRace->getCloseButton()->setVisible(false);
	wANRace->setVisible(false);
	//selection hint
	stHintMsg = env->addStaticText(L"", rect<s32>(500, 60, 820, 90), true, false, 0, -1, false);
	stHintMsg->setBackgroundColor(0xc0ffffff);
	stHintMsg->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);
	stHintMsg->setVisible(false);
	//cmd menu
	wCmdMenu = env->addWindow(rect<s32>(10, 10, 110, 179), false, L"");
	wCmdMenu->setDrawTitlebar(false);
	wCmdMenu->setVisible(false);
	wCmdMenu->getCloseButton()->setVisible(false);
	//deck edit
	wDeckEdit = env->addStaticText(L"", rect<s32>(309, 5, 605, 130), true, false, 0, -1, true);
	wDeckEdit->setVisible(false);
	btnManageDeck = env->addButton(rect<s32>(225, 5, 290, 30), wDeckEdit, BUTTON_MANAGE_DECK, dataManager.GetSysString(1328));
	//deck manage
	wDeckManage = env->addWindow(rect<s32>(310, 135, 800, 465), false, dataManager.GetSysString(1460), 0, WINDOW_DECK_MANAGE);
	wDeckManage->setVisible(false);
	lstCategories = env->addListBox(rect<s32>(10, 30, 140, 320), wDeckManage, LISTBOX_CATEGORIES, true);
	lstDecks = env->addListBox(rect<s32>(150, 30, 340, 320), wDeckManage, LISTBOX_DECKS, true);
	posY = 30;
	btnNewCategory = env->addButton(rect<s32>(350, posY, 480, posY + 25), wDeckManage, BUTTON_NEW_CATEGORY, dataManager.GetSysString(1461));
	posY += 35;
	btnRenameCategory = env->addButton(rect<s32>(350, posY, 480, posY + 25), wDeckManage, BUTTON_RENAME_CATEGORY, dataManager.GetSysString(1462));
	posY += 35;
	btnDeleteCategory = env->addButton(rect<s32>(350, posY, 480, posY + 25), wDeckManage, BUTTON_DELETE_CATEGORY, dataManager.GetSysString(1463));
	posY += 35;
	btnNewDeck = env->addButton(rect<s32>(350, posY, 480, posY + 25), wDeckManage, BUTTON_NEW_DECK, dataManager.GetSysString(1464));
	posY += 35;
	btnRenameDeck = env->addButton(rect<s32>(350, posY, 480, posY + 25), wDeckManage, BUTTON_RENAME_DECK, dataManager.GetSysString(1465));
	posY += 35;
	btnDMDeleteDeck = env->addButton(rect<s32>(350, posY, 480, posY + 25), wDeckManage, BUTTON_DELETE_DECK_DM, dataManager.GetSysString(1466));
	posY += 35;
	btnMoveDeck = env->addButton(rect<s32>(350, posY, 480, posY + 25), wDeckManage, BUTTON_MOVE_DECK, dataManager.GetSysString(1467));
	posY += 35;
	btnCopyDeck = env->addButton(rect<s32>(350, posY, 480, posY + 25), wDeckManage, BUTTON_COPY_DECK, dataManager.GetSysString(1468));
	//deck manage query
	wDMQuery = env->addWindow(rect<s32>(400, 200, 710, 320), false, dataManager.GetSysString(1460));
	wDMQuery->getCloseButton()->setVisible(false);
	wDMQuery->setVisible(false);
	stDMMessage = env->addStaticText(L"", rect<s32>(20, 25, 290, 45), false, false, wDMQuery);
	stDMMessage2 = env->addStaticText(L"", rect<s32>(20, 50, 290, 70), false, false, wDMQuery, -1, true);
	stDMMessage2->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);
	ebDMName = env->addEditBox(L"", rect<s32>(20, 50, 290, 70), true, wDMQuery, -1);
	ebDMName->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);
	cbDMCategory = env->addComboBox(rect<s32>(20, 50, 290, 70), wDMQuery, -1);
	stDMMessage2->setVisible(false);
	ebDMName->setVisible(false);
	cbDMCategory->setVisible(false);
	cbDMCategory->setMaxSelectionRows(10);
	btnDMOK = env->addButton(rect<s32>(70, 80, 140, 105), wDMQuery, BUTTON_DM_OK, dataManager.GetSysString(1211));
	btnDMCancel = env->addButton(rect<s32>(170, 80, 240, 105), wDMQuery, BUTTON_DM_CANCEL, dataManager.GetSysString(1212));
	scrPackCards = env->addScrollBar(false, recti(775, 161, 795, 629), 0, SCROLL_FILTER);
	scrPackCards->setLargeStep(1);
	scrPackCards->setSmallStep(1);
	scrPackCards->setVisible(false);

	stDBCategory = env->addStaticText(dataManager.GetSysString(1300), rect<s32>(10, 9, 100, 29), false, false, wDeckEdit);
	cbDBCategory = env->addComboBox(rect<s32>(80, 5, 220, 30), wDeckEdit, COMBOBOX_DBCATEGORY);
	cbDBCategory->setMaxSelectionRows(15);
	stDeck = env->addStaticText(dataManager.GetSysString(1301), rect<s32>(10, 39, 100, 59), false, false, wDeckEdit);
	cbDBDecks = env->addComboBox(rect<s32>(80, 35, 220, 60), wDeckEdit, COMBOBOX_DBDECKS);
	cbDBDecks->setMaxSelectionRows(15);
	btnSaveDeck = env->addButton(rect<s32>(225, 35, 290, 60), wDeckEdit, BUTTON_SAVE_DECK, dataManager.GetSysString(1302));
	ebDeckname = env->addEditBox(L"", rect<s32>(80, 65, 220, 90), true, wDeckEdit, -1);
	ebDeckname->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);
	btnSaveDeckAs = env->addButton(rect<s32>(225, 65, 290, 90), wDeckEdit, BUTTON_SAVE_DECK_AS, dataManager.GetSysString(1303));
	btnDeleteDeck = env->addButton(rect<s32>(225, 95, 290, 120), wDeckEdit, BUTTON_DELETE_DECK, dataManager.GetSysString(1308));
	btnShuffleDeck = env->addButton(rect<s32>(5, 99, 55, 120), wDeckEdit, BUTTON_SHUFFLE_DECK, dataManager.GetSysString(1307));
	btnSortDeck = env->addButton(rect<s32>(60, 99, 110, 120), wDeckEdit, BUTTON_SORT_DECK, dataManager.GetSysString(1305));
	btnClearDeck = env->addButton(rect<s32>(115, 99, 165, 120), wDeckEdit, BUTTON_CLEAR_DECK, dataManager.GetSysString(1304));
	btnSideOK = env->addButton(rect<s32>(510, 40, 820, 80), 0, BUTTON_SIDE_OK, dataManager.GetSysString(1334));
	btnSideOK->setVisible(false);
	btnSideShuffle = env->addButton(rect<s32>(310, 100, 370, 130), 0, BUTTON_SHUFFLE_DECK, dataManager.GetSysString(1307));
	btnSideShuffle->setVisible(false);
	btnSideSort = env->addButton(rect<s32>(375, 100, 435, 130), 0, BUTTON_SORT_DECK, dataManager.GetSysString(1305));
	btnSideSort->setVisible(false);
	btnSideReload = env->addButton(rect<s32>(440, 100, 500, 130), 0, BUTTON_SIDE_RELOAD, dataManager.GetSysString(1309));
	btnSideReload->setVisible(false);
	//
	scrFilter = env->addScrollBar(false, recti(999, 161, 1019, 629), 0, SCROLL_FILTER);
	scrFilter->setLargeStep(10);
	scrFilter->setSmallStep(1);
	scrFilter->setVisible(false);
	//sort type
	wSort = env->addStaticText(L"", rect<s32>(930, 132, 1020, 156), true, false, 0, -1, true);
	cbSortType = env->addComboBox(rect<s32>(10, 2, 85, 22), wSort, COMBOBOX_SORTTYPE);
	cbSortType->setMaxSelectionRows(10);
	for(int i = 1370; i <= 1373; i++)
		cbSortType->addItem(dataManager.GetSysString(i));
	wSort->setVisible(false);
	//filters
	wFilter = env->addStaticText(L"", rect<s32>(610, 5, 1020, 130), true, false, 0, -1, true);
	wFilter->setVisible(false);
	stCategory = env->addStaticText(dataManager.GetSysString(1311), rect<s32>(10, 2 + 25 / 6, 70, 22 + 25 / 6), false, false, wFilter);
	cbCardType = env->addComboBox(rect<s32>(60, 25 / 6, 120, 20 + 25 / 6), wFilter, COMBOBOX_MAINTYPE);
	cbCardType->addItem(dataManager.GetSysString(1310));
	cbCardType->addItem(dataManager.GetSysString(1312));
	cbCardType->addItem(dataManager.GetSysString(1313));
	cbCardType->addItem(dataManager.GetSysString(1314));
	cbCardType2 = env->addComboBox(rect<s32>(125, 25 / 6, 195, 20 + 25 / 6), wFilter, COMBOBOX_SECONDTYPE);
	cbCardType2->setMaxSelectionRows(10);
	cbCardType2->addItem(dataManager.GetSysString(1310), 0);
	stLimit = env->addStaticText(dataManager.GetSysString(1315), rect<s32>(205, 2 + 25 / 6, 280, 22 + 25 / 6), false, false, wFilter);
	cbLimit = env->addComboBox(rect<s32>(260, 25 / 6, 390, 20 + 25 / 6), wFilter, COMBOBOX_LIMIT);
	cbLimit->setMaxSelectionRows(10);
	cbLimit->addItem(dataManager.GetSysString(1310));
	cbLimit->addItem(dataManager.GetSysString(1316));
	cbLimit->addItem(dataManager.GetSysString(1317));
	cbLimit->addItem(dataManager.GetSysString(1318));
	cbLimit->addItem(dataManager.GetSysString(1481));
	cbLimit->addItem(dataManager.GetSysString(1482));
	cbLimit->addItem(dataManager.GetSysString(1483));
	cbLimit->addItem(dataManager.GetSysString(1484));
	cbLimit->addItem(dataManager.GetSysString(1485));
	stAttribute = env->addStaticText(dataManager.GetSysString(1319), rect<s32>(10, 22 + 50 / 6, 70, 42 + 50 / 6), false, false, wFilter);
	cbAttribute = env->addComboBox(rect<s32>(60, 20 + 50 / 6, 195, 40 + 50 / 6), wFilter, COMBOBOX_ATTRIBUTE);
	cbAttribute->setMaxSelectionRows(10);
	cbAttribute->addItem(dataManager.GetSysString(1310), 0);
	for(int filter = 0x1; filter != 0x80; filter <<= 1)
		cbAttribute->addItem(dataManager.FormatAttribute(filter), filter);
	stRace = env->addStaticText(dataManager.GetSysString(1321), rect<s32>(10, 42 + 75 / 6, 70, 62 + 75 / 6), false, false, wFilter);
	cbRace = env->addComboBox(rect<s32>(60, 40 + 75 / 6, 195, 60 + 75 / 6), wFilter, COMBOBOX_RACE);
	cbRace->setMaxSelectionRows(10);
	cbRace->addItem(dataManager.GetSysString(1310), 0);
	for(int filter = 0x1; filter < (1 << RACES_COUNT); filter <<= 1)
		cbRace->addItem(dataManager.FormatRace(filter), filter);
	stAttack = env->addStaticText(dataManager.GetSysString(1322), rect<s32>(205, 22 + 50 / 6, 280, 42 + 50 / 6), false, false, wFilter);
	ebAttack = env->addEditBox(L"", rect<s32>(260, 20 + 50 / 6, 340, 40 + 50 / 6), true, wFilter, EDITBOX_INPUTS);
	ebAttack->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);
	stDefense = env->addStaticText(dataManager.GetSysString(1323), rect<s32>(205, 42 + 75 / 6, 280, 62 + 75 / 6), false, false, wFilter);
	ebDefense = env->addEditBox(L"", rect<s32>(260, 40 + 75 / 6, 340, 60 + 75 / 6), true, wFilter, EDITBOX_INPUTS);
	ebDefense->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);
	stStar = env->addStaticText(dataManager.GetSysString(1324), rect<s32>(10, 62 + 100 / 6, 80, 82 + 100 / 6), false, false, wFilter);
	ebStar = env->addEditBox(L"", rect<s32>(60, 60 + 100 / 6, 100, 80 + 100 / 6), true, wFilter, EDITBOX_INPUTS);
	ebStar->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);
	stScale = env->addStaticText(dataManager.GetSysString(1336), rect<s32>(101, 62 + 100 / 6, 150, 82 + 100 / 6), false, false, wFilter);
	ebScale = env->addEditBox(L"", rect<s32>(150, 60 + 100 / 6, 195, 80 + 100 / 6), true, wFilter, EDITBOX_INPUTS);
	ebScale->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);
	stSearch = env->addStaticText(dataManager.GetSysString(1325), rect<s32>(205, 62 + 100 / 6, 280, 82 + 100 / 6), false, false, wFilter);
	ebCardName = env->addEditBox(L"", rect<s32>(260, 60 + 100 / 6, 390, 80 + 100 / 6), true, wFilter, EDITBOX_KEYWORD);
	ebCardName->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);
	btnEffectFilter = env->addButton(rect<s32>(345, 20 + 50 / 6, 390, 60 + 75 / 6), wFilter, BUTTON_EFFECT_FILTER, dataManager.GetSysString(1326));
	btnStartFilter = env->addButton(rect<s32>(205, 80 + 125 / 6, 390, 100 + 125 / 6), wFilter, BUTTON_START_FILTER, dataManager.GetSysString(1327));
	if(gameConf.separate_clear_button) {
		btnStartFilter->setRelativePosition(rect<s32>(260, 80 + 125 / 6, 390, 100 + 125 / 6));
		btnClearFilter = env->addButton(rect<s32>(205, 80 + 125 / 6, 255, 100 + 125 / 6), wFilter, BUTTON_CLEAR_FILTER, dataManager.GetSysString(1304));
	}
	wCategories = env->addWindow(rect<s32>(600, 60, 1000, 305), false, L"");
	wCategories->getCloseButton()->setVisible(false);
	wCategories->setDrawTitlebar(false);
	wCategories->setDraggable(false);
	wCategories->setVisible(false);
	btnCategoryOK = env->addButton(rect<s32>(150, 210, 250, 235), wCategories, BUTTON_CATEGORY_OK, dataManager.GetSysString(1211));
	int catewidth = 0;
	for(int i = 0; i < 32; ++i) {
		irr::core::dimension2d<unsigned int> dtxt = guiFont->getDimension(dataManager.GetSysString(1100 + i));
		if((int)dtxt.Width + 40 > catewidth)
			catewidth = dtxt.Width + 40;
	}
	for(int i = 0; i < 32; ++i)
		chkCategory[i] = env->addCheckBox(false, recti(10 + (i % 4) * catewidth, 5 + (i / 4) * 25, 10 + (i % 4 + 1) * catewidth, 5 + (i / 4 + 1) * 25), wCategories, -1, dataManager.GetSysString(1100 + i));
	int wcatewidth = catewidth * 4 + 16;
	wCategories->setRelativePosition(rect<s32>(1000 - wcatewidth, 60, 1000, 305));
	btnCategoryOK->setRelativePosition(recti(wcatewidth / 2 - 50, 210, wcatewidth / 2 + 50, 235));
	btnMarksFilter = env->addButton(rect<s32>(60, 80 + 125 / 6, 195, 100 + 125 / 6), wFilter, BUTTON_MARKS_FILTER, dataManager.GetSysString(1374));
	wLinkMarks = env->addWindow(rect<s32>(700, 30, 820, 150), false, L"");
	wLinkMarks->getCloseButton()->setVisible(false);
	wLinkMarks->setDrawTitlebar(false);
	wLinkMarks->setDraggable(false);
	wLinkMarks->setVisible(false);
	btnMarksOK = env->addButton(recti(45, 45, 75, 75), wLinkMarks, BUTTON_MARKERS_OK, L"OK");
	btnMark[0] = env->addButton(recti(10, 10, 40, 40), wLinkMarks, -1, L"\u2196");
	btnMark[1] = env->addButton(recti(45, 10, 75, 40), wLinkMarks, -1, L"\u2191");
	btnMark[2] = env->addButton(recti(80, 10, 110, 40), wLinkMarks, -1, L"\u2197");
	btnMark[3] = env->addButton(recti(10, 45, 40, 75), wLinkMarks, -1, L"\u2190");
	btnMark[4] = env->addButton(recti(80, 45, 110, 75), wLinkMarks, -1, L"\u2192");
	btnMark[5] = env->addButton(recti(10, 80, 40, 110), wLinkMarks, -1, L"\u2199");
	btnMark[6] = env->addButton(recti(45, 80, 75, 110), wLinkMarks, -1, L"\u2193");
	btnMark[7] = env->addButton(recti(80, 80, 110, 110), wLinkMarks, -1, L"\u2198");
	for(int i=0;i<8;i++)
		btnMark[i]->setIsPushButton(true);
	//replay window
	wReplay = env->addWindow(rect<s32>(220, 100, 800, 520), false, dataManager.GetSysString(1202));
	wReplay->getCloseButton()->setVisible(false);
	wReplay->setVisible(false);
	//single play window
	wSinglePlay = env->addWindow(rect<s32>(220, 100, 800, 520), false, dataManager.GetSysString(1201));
	wSinglePlay->getCloseButton()->setVisible(false);
	wSinglePlay->setVisible(false);
	//replay save
	wReplaySave = env->addWindow(rect<s32>(510, 200, 820, 320), false, dataManager.GetSysString(1340));
	wReplaySave->getCloseButton()->setVisible(false);
	wReplaySave->setVisible(false);
	//chat
	wChat = env->addWindow(rect<s32>(305, 615, 1020, 640), false, L"");
	wChat->getCloseButton()->setVisible(false);
	wChat->setDraggable(false);
	wChat->setDrawTitlebar(false);
	wChat->setVisible(false);
	ebChatInput = env->addEditBox(L"", rect<s32>(3, 2, 710, 22), true, wChat, EDITBOX_CHAT);
	//big picture
	wBigCard = env->addWindow(rect<s32>(0, 0, 0, 0), false, L"");
	wBigCard->getCloseButton()->setVisible(false);
	wBigCard->setDrawTitlebar(false);
	wBigCard->setDrawBackground(false);
	wBigCard->setVisible(false);
	imgBigCard = env->addImage(rect<s32>(0, 0, 0, 0), wBigCard);
	imgBigCard->setScaleImage(false);
	imgBigCard->setUseAlphaChannel(true);
	btnBigCardOriginalSize = env->addButton(rect<s32>(205, 100, 295, 135), 0, BUTTON_BIG_CARD_ORIG_SIZE, dataManager.GetSysString(1443));
	btnBigCardZoomIn = env->addButton(rect<s32>(205, 140, 295, 175), 0, BUTTON_BIG_CARD_ZOOM_IN, dataManager.GetSysString(1441));
	btnBigCardZoomOut = env->addButton(rect<s32>(205, 180, 295, 215), 0, BUTTON_BIG_CARD_ZOOM_OUT, dataManager.GetSysString(1442));
	btnBigCardClose = env->addButton(rect<s32>(205, 230, 295, 265), 0, BUTTON_BIG_CARD_CLOSE, dataManager.GetSysString(1440));
	btnBigCardOriginalSize->setVisible(false);
	btnBigCardZoomIn->setVisible(false);
	btnBigCardZoomOut->setVisible(false);
	btnBigCardClose->setVisible(false);
	//leave/surrender/exit
	btnLeaveGame = env->addButton(rect<s32>(205, 5, 295, 80), 0, BUTTON_LEAVE_GAME, L"");
	btnLeaveGame->setVisible(false);
	//tip
	stTip = env->addStaticText(L"", rect<s32>(0, 0, 150, 150), false, true, 0, -1, true);
	stTip->setBackgroundColor(0xc0ffffff);
	stTip->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);
	stTip->setVisible(false);
	device->setEventReceiver(&menuHandler);
	if(!soundManager.Init()) {
		chkEnableSound->setChecked(false);
		chkEnableSound->setEnabled(false);
		chkEnableSound->setVisible(false);
		chkEnableMusic->setChecked(false);
		chkEnableMusic->setEnabled(false);
		chkEnableMusic->setVisible(false);
		scrSoundVolume->setVisible(false);
		scrMusicVolume->setVisible(false);
		chkMusicMode->setEnabled(false);
		chkMusicMode->setVisible(false);
	}
	env->getSkin()->setFont(guiFont);
	env->setFocus(wMainMenu);
	for (u32 i = 0; i < EGDC_COUNT; ++i) {
		SColor col = env->getSkin()->getColor((EGUI_DEFAULT_COLOR)i);
		col.setAlpha(224);
		env->getSkin()->setColor((EGUI_DEFAULT_COLOR)i, col);
	}
	dimension2du size = driver->getScreenSize();
	if(window_size != size) {
		window_size = size;
		xScale = window_size.Width / 1024.0;
		yScale = window_size.Height / 640.0;
		OnResize();
	}
	hideChat = false;
	hideChatTimer = 0;
	return true;
}
void Game::MainLoop() {
	wchar_t cap[256];
	camera = smgr->addCameraSceneNode(0);
	irr::core::matrix4 mProjection;
	BuildProjectionMatrix(mProjection, -0.90f, 0.45f, -0.42f, 0.42f, 1.0f, 100.0f);
	camera->setProjectionMatrix(mProjection);

	mProjection.buildCameraLookAtMatrixLH(vector3df(4.2f, 8.0f, 7.8f), vector3df(4.2f, 0, 0), vector3df(0, 0, 1));
	camera->setViewMatrixAffector(mProjection);
	smgr->setAmbientLight(SColorf(1.0f, 1.0f, 1.0f));
	float atkframe = 0.1f;
	irr::ITimer* timer = device->getTimer();
	timer->setTime(0);
	int fps = 0;
	int cur_time = 0;
	while(device->run()) {
		dimension2du size = driver->getScreenSize();
		if(window_size != size) {
			window_size = size;
			xScale = window_size.Width / 1024.0;
			yScale = window_size.Height / 640.0;
			OnResize();
		}
		linePatternD3D = (linePatternD3D + 1) % 30;
		linePatternGL = (linePatternGL << 1) | (linePatternGL >> 15);
		atkframe += 0.1f;
		atkdy = (float)sin(atkframe);
		driver->beginScene(true, true, SColor(0, 0, 0, 0));
		gMutex.lock();
		if(dInfo.isStarted) {
			if(dInfo.isFinished && showcardcode == 1)
				soundManager.PlayBGM(BGM_WIN);
			else if(dInfo.isFinished && (showcardcode == 2 || showcardcode == 3))
				soundManager.PlayBGM(BGM_LOSE);
			else if(dInfo.lp[0] > 0 && dInfo.lp[0] <= dInfo.lp[1] / 2)
				soundManager.PlayBGM(BGM_DISADVANTAGE);
			else if(dInfo.lp[0] > 0 && dInfo.lp[0] >= dInfo.lp[1] * 2)
				soundManager.PlayBGM(BGM_ADVANTAGE);
			else
				soundManager.PlayBGM(BGM_DUEL);
			DrawBackImage(imageManager.tBackGround);
			smgr->drawAll();
			driver->setMaterial(irr::video::IdentityMaterial);
			driver->clearZBuffer();
		} else if(is_building) {
			soundManager.PlayBGM(BGM_DECK);
			DrawBackImage(imageManager.tBackGround_deck);
			DrawDeckBd();
		} else {
			soundManager.PlayBGM(BGM_MENU);
			DrawBackImage(imageManager.tBackGround_menu);
		}
		DrawGUI();
		DrawSpec();
		gMutex.unlock();
		if(signalFrame > 0) {
			signalFrame--;
			if(!signalFrame)
				frameSignal.Set();
		}
		if(waitFrame >= 0) {
			waitFrame++;
			if(waitFrame % 90 == 0) {
				stHintMsg->setText(dataManager.GetSysString(1390));
			} else if(waitFrame % 90 == 30) {
				stHintMsg->setText(dataManager.GetSysString(1391));
			} else if(waitFrame % 90 == 60) {
				stHintMsg->setText(dataManager.GetSysString(1392));
			}
		}
		driver->endScene();
		if(closeSignal.Wait(1))
			CloseDuelWindow();
		fps++;
		cur_time = timer->getTime();
		if(cur_time < fps * 17 - 20)
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		if(cur_time >= 1000) {
			myswprintf(cap, L"YGOPro FPS: %d", fps);
			device->setWindowCaption(cap);
			fps = 0;
			cur_time -= 1000;
			timer->setTime(0);
			if(dInfo.time_player == 0 || dInfo.time_player == 1)
				if(dInfo.time_left[dInfo.time_player])
					dInfo.time_left[dInfo.time_player]--;
		}
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	SaveConfig();
	device->drop();
}
void Game::BuildProjectionMatrix(irr::core::matrix4& mProjection, f32 left, f32 right, f32 bottom, f32 top, f32 znear, f32 zfar) {
	for(int i = 0; i < 16; ++i)
		mProjection[i] = 0;
	mProjection[0] = 2.0f * znear / (right - left);
	mProjection[5] = 2.0f * znear / (top - bottom);
	mProjection[8] = (left + right) / (left - right);
	mProjection[9] = (top + bottom) / (bottom - top);
	mProjection[10] = zfar / (zfar - znear);
	mProjection[11] = 1.0f;
	mProjection[14] = znear * zfar / (znear - zfar);
}
void Game::InitStaticText(irr::gui::IGUIStaticText* pControl, u32 cWidth, u32 cHeight, irr::gui::CGUITTFont* font, const wchar_t* text) {
	std::wstring format_text;
	format_text = SetStaticText(pControl, cWidth, font, text);
	if(font->getDimension(format_text.c_str()).Height <= cHeight) {
		scrCardText->setVisible(false);
		if(env->hasFocus(scrCardText))
			env->removeFocus(scrCardText);
		return;
	}
	format_text = SetStaticText(pControl, cWidth-25, font, text);
	u32 fontheight = font->getDimension(L"A").Height + font->getKerningHeight();
	u32 step = (font->getDimension(format_text.c_str()).Height - cHeight) / fontheight + 1;
	scrCardText->setVisible(true);
	scrCardText->setMin(0);
	scrCardText->setMax(step);
	scrCardText->setPos(0);
}
std::wstring Game::SetStaticText(irr::gui::IGUIStaticText* pControl, u32 cWidth, irr::gui::CGUITTFont* font, const wchar_t* text, u32 pos) {
	int pbuffer = 0;
	u32 _width = 0, _height = 0;
	wchar_t prev = 0;
	wchar_t strBuffer[4096];
	std::wstring ret;

	for(size_t i = 0; text[i] != 0 && i < wcslen(text); ++i) {
		wchar_t c = text[i];
		u32 w = font->getCharDimension(c).Width + font->getKerningWidth(c, prev);
		prev = c;
		if(text[i] == L'\r') {
			continue;
		} else if(text[i] == L'\n') {
			strBuffer[pbuffer++] = L'\n';
			_width = 0;
			_height++;
			prev = 0;
			if(_height == pos)
				pbuffer = 0;
			continue;
		} else if(_width > 0 && _width + w > cWidth) {
			strBuffer[pbuffer++] = L'\n';
			_width = 0;
			_height++;
			prev = 0;
			if(_height == pos)
				pbuffer = 0;
		}
		_width += w;
		strBuffer[pbuffer++] = c;
	}
	strBuffer[pbuffer] = 0;
	pControl->setText(strBuffer);
	ret.assign(strBuffer);
	return ret;
}
void Game::LoadExpansions() {
}
void Game::RefreshCategoryDeck(irr::gui::IGUIComboBox* cbCategory, irr::gui::IGUIComboBox* cbDeck, bool selectlastused) {
	cbCategory->clear();
	cbCategory->addItem(dataManager.GetSysString(1450));
	cbCategory->addItem(dataManager.GetSysString(1451));
	cbCategory->addItem(dataManager.GetSysString(1452));
	cbCategory->addItem(dataManager.GetSysString(1453));
	FileSystem::TraversalDir(L"./deck", [cbCategory](const wchar_t* name, bool isdir) {
		if(isdir) {
			cbCategory->addItem(name);
		}
	});
	cbCategory->setSelected(2);
	if(selectlastused) {
		for(size_t i = 0; i < cbCategory->getItemCount(); ++i) {
			if(!wcscmp(cbCategory->getItem(i), gameConf.lastcategory)) {
				cbCategory->setSelected(i);
				break;
			}
		}
	}
	RefreshDeck(cbCategory, cbDeck);
	if(selectlastused) {
		for(size_t i = 0; i < cbDeck->getItemCount(); ++i) {
			if(!wcscmp(cbDeck->getItem(i), gameConf.lastdeck)) {
				cbDeck->setSelected(i);
				break;
			}
		}
	}
}
void Game::RefreshDeck(irr::gui::IGUIComboBox* cbCategory, irr::gui::IGUIComboBox* cbDeck) {
	if(cbCategory != cbDBCategory && cbCategory->getSelected() == 0) {
		// can't use pack list in duel
		cbDeck->clear();
		return;
	}
	wchar_t catepath[256];
	deckManager.GetCategoryPath(catepath, cbCategory->getSelected(), cbCategory->getText());
	cbDeck->clear();
	RefreshDeck(catepath, [cbDeck](const wchar_t* item) { cbDeck->addItem(item); });
}
void Game::RefreshDeck(const wchar_t* deckpath, const std::function<void(const wchar_t*)>& additem) {
	if(!mywcsncasecmp(deckpath, L"./pack", 6)) {
		for(auto& pack : deckBuilder.expansionPacks) {
			additem(pack.substr(5, pack.size() - 9).c_str());
		}
	}
	FileSystem::TraversalDir(deckpath, [additem](const wchar_t* name, bool isdir) {
		if(!isdir && wcsrchr(name, '.') && !mywcsncasecmp(wcsrchr(name, '.'), L".ydk", 4)) {
			size_t len = wcslen(name);
			wchar_t deckname[256];
			wcsncpy(deckname, name, len - 4);
			deckname[len - 4] = 0;
			additem(deckname);
		}
	});
}
void Game::LoadConfig() {
	FILE* fp = fopen(config_filename, "r");
	if(!fp)
		return;
	char linebuf[256];
	char strbuf[32];
	char valbuf[256];
	wchar_t wstr[256];
	while(fgets(linebuf, 256, fp)) {
		sscanf(linebuf, "%s = %s", strbuf, valbuf);
		if(!strcmp(strbuf, "antialias")) {
			gameConf.antialias = atoi(valbuf);
		} else if(!strcmp(strbuf, "use_d3d")) {
			gameConf.use_d3d = atoi(valbuf) > 0;
		} else if(!strcmp(strbuf, "use_image_scale")) {
			gameConf.use_image_scale = atoi(valbuf) > 0;
		} else if(!strcmp(strbuf, "errorlog")) {
			enable_log = atoi(valbuf);
		} else if(!strcmp(strbuf, "textfont")) {
			BufferIO::DecodeUTF8(valbuf, wstr);
			int textfontsize = gameConf.textfontsize;
			sscanf(linebuf, "%s = %s %d", strbuf, valbuf, &textfontsize);
			gameConf.textfontsize = textfontsize;
			BufferIO::CopyWStr(wstr, gameConf.textfont, 256);
		} else if(!strcmp(strbuf, "numfont")) {
			BufferIO::DecodeUTF8(valbuf, wstr);
			BufferIO::CopyWStr(wstr, gameConf.numfont, 256);
		} else if(!strcmp(strbuf, "use_lflist")) {
			gameConf.use_lflist = atoi(valbuf);
		} else if(!strcmp(strbuf, "default_lflist")) {
			gameConf.default_lflist = atoi(valbuf);
		} else if(!strcmp(strbuf, "default_rule")) {
			gameConf.default_rule = atoi(valbuf);
			if(gameConf.default_rule <= 0)
				gameConf.default_rule = DEFAULT_DUEL_RULE;
		} else if(!strcmp(strbuf, "hide_setname")) {
			gameConf.hide_setname = atoi(valbuf);
		} else if(!strcmp(strbuf, "hide_hint_button")) {
			gameConf.hide_hint_button = atoi(valbuf);
		} else if(!strcmp(strbuf, "control_mode")) {
			gameConf.control_mode = atoi(valbuf);
		} else if(!strcmp(strbuf, "draw_field_spell")) {
			gameConf.draw_field_spell = atoi(valbuf);
		} else if(!strcmp(strbuf, "separate_clear_button")) {
			gameConf.separate_clear_button = atoi(valbuf);
		} else if(!strcmp(strbuf, "auto_search_limit")) {
			gameConf.auto_search_limit = atoi(valbuf);
		} else if(!strcmp(strbuf, "search_multiple_keywords")) {
			gameConf.search_multiple_keywords = atoi(valbuf);
		} else if(!strcmp(strbuf, "ignore_deck_changes")) {
			gameConf.chkIgnoreDeckChanges = atoi(valbuf);
		} else if(!strcmp(strbuf, "default_ot")) {
			gameConf.defaultOT = atoi(valbuf);
		} else if(!strcmp(strbuf, "prefer_expansion_script")) {
			gameConf.prefer_expansion_script = atoi(valbuf);
		} else if(!strcmp(strbuf, "window_maximized")) {
			gameConf.window_maximized = atoi(valbuf) > 0;
		} else if(!strcmp(strbuf, "window_width")) {
			gameConf.window_width = atoi(valbuf);
		} else if(!strcmp(strbuf, "window_height")) {
			gameConf.window_height = atoi(valbuf);
		} else if(!strcmp(strbuf, "resize_popup_menu")) {
			gameConf.resize_popup_menu = atoi(valbuf) > 0;
#ifdef YGOPRO_USE_IRRKLANG
		} else if(!strcmp(strbuf, "enable_sound")) {
			gameConf.enable_sound = atoi(valbuf) > 0;
		} else if(!strcmp(strbuf, "sound_volume")) {
			gameConf.sound_volume = atof(valbuf) / 100;
		} else if(!strcmp(strbuf, "enable_music")) {
			gameConf.enable_music = atoi(valbuf) > 0;
		} else if(!strcmp(strbuf, "music_volume")) {
			gameConf.music_volume = atof(valbuf) / 100;
		} else if(!strcmp(strbuf, "music_mode")) {
			gameConf.music_mode = atoi(valbuf);
#endif
		} else {
			// options allowing multiple words
			sscanf(linebuf, "%s = %240[^\n]", strbuf, valbuf);
			if (!strcmp(strbuf, "nickname")) {
				BufferIO::DecodeUTF8(valbuf, wstr);
				BufferIO::CopyWStr(wstr, gameConf.nickname, 20);
			} else if (!strcmp(strbuf, "lastcategory")) {
				BufferIO::DecodeUTF8(valbuf, wstr);
				BufferIO::CopyWStr(wstr, gameConf.lastcategory, 64);
			} else if (!strcmp(strbuf, "lastdeck")) {
				BufferIO::DecodeUTF8(valbuf, wstr);
				BufferIO::CopyWStr(wstr, gameConf.lastdeck, 64);
			} else if (!strcmp(strbuf, "bot_deck_path")) {
				BufferIO::DecodeUTF8(valbuf, wstr);
				BufferIO::CopyWStr(wstr, gameConf.bot_deck_path, 64);
			}
		}
	}
	fclose(fp);
}
void Game::SaveConfig() {
	FILE* fp = fopen(config_filename, "w");
	fprintf(fp, "#config file\n#nickname & gamename should be less than 20 characters\n");
	char linebuf[256];
	fprintf(fp, "use_d3d = %d\n", gameConf.use_d3d ? 1 : 0);
	fprintf(fp, "use_image_scale = %d\n", gameConf.use_image_scale ? 1 : 0);
	fprintf(fp, "antialias = %d\n", gameConf.antialias);
	fprintf(fp, "errorlog = %d\n", enable_log);
	BufferIO::EncodeUTF8(gameConf.nickname, linebuf);
	fprintf(fp, "nickname = %s\n", linebuf);
	BufferIO::EncodeUTF8(gameConf.lastcategory, linebuf);
	fprintf(fp, "lastcategory = %s\n", linebuf);
	BufferIO::EncodeUTF8(gameConf.lastdeck, linebuf);
	fprintf(fp, "lastdeck = %s\n", linebuf);
	BufferIO::EncodeUTF8(gameConf.textfont, linebuf);
	fprintf(fp, "textfont = %s %d\n", linebuf, gameConf.textfontsize);
	BufferIO::EncodeUTF8(gameConf.numfont, linebuf);
	fprintf(fp, "numfont = %s\n", linebuf);
	//settings
	fprintf(fp, "use_lflist = %d\n", gameConf.use_lflist);
	fprintf(fp, "default_lflist = %d\n", gameConf.default_lflist);
	fprintf(fp, "default_rule = %d\n", gameConf.default_rule == DEFAULT_DUEL_RULE ? 0 : gameConf.default_rule);
	fprintf(fp, "hide_setname = %d\n", gameConf.hide_setname);
	fprintf(fp, "hide_hint_button = %d\n", gameConf.hide_hint_button);
	fprintf(fp, "#control_mode = 0: Key A/S/D/R Chain Buttons. control_mode = 1: MouseLeft/MouseRight/NULL/F9 Without Chain Buttons\n");
	fprintf(fp, "control_mode = %d\n", gameConf.control_mode);
	fprintf(fp, "draw_field_spell = %d\n", gameConf.draw_field_spell);
	fprintf(fp, "separate_clear_button = %d\n", gameConf.separate_clear_button);
	fprintf(fp, "#auto_search_limit >= 0: Start search automatically when the user enters N chars\n");
	fprintf(fp, "auto_search_limit = %d\n", gameConf.auto_search_limit);
	fprintf(fp, "#search_multiple_keywords = 0: Disable. 1: Search mutiple keywords with separator \" \". 2: with separator \"+\"\n");
	fprintf(fp, "search_multiple_keywords = %d\n", gameConf.search_multiple_keywords);
	fprintf(fp, "ignore_deck_changes = %d\n", (chkIgnoreDeckChanges->isChecked() ? 1 : 0));
	fprintf(fp, "default_ot = %d\n", gameConf.defaultOT);
	BufferIO::EncodeUTF8(gameConf.bot_deck_path, linebuf);
	fprintf(fp, "bot_deck_path = %s\n", linebuf);
	fprintf(fp, "prefer_expansion_script = %d\n", gameConf.prefer_expansion_script);
	fprintf(fp, "window_maximized = %d\n", (gameConf.window_maximized ? 1 : 0));
	fprintf(fp, "window_width = %d\n", gameConf.window_width);
	fprintf(fp, "window_height = %d\n", gameConf.window_height);
	fprintf(fp, "resize_popup_menu = %d\n", gameConf.resize_popup_menu ? 1 : 0);
#ifdef YGOPRO_USE_IRRKLANG
	fprintf(fp, "enable_sound = %d\n", (chkEnableSound->isChecked() ? 1 : 0));
	fprintf(fp, "enable_music = %d\n", (chkEnableMusic->isChecked() ? 1 : 0));
	fprintf(fp, "#Volume of sound and music, between 0 and 100\n");
	int vol = gameConf.sound_volume * 100;
	if(vol < 0) vol = 0; else if(vol > 100) vol = 100;
	fprintf(fp, "sound_volume = %d\n", vol);
	vol = gameConf.music_volume * 100;
	if(vol < 0) vol = 0; else if(vol > 100) vol = 100;
	fprintf(fp, "music_volume = %d\n", vol);
	fprintf(fp, "music_mode = %d\n", (chkMusicMode->isChecked() ? 1 : 0));
#endif
	fclose(fp);
}
void Game::ShowCardInfo(int code, bool resize) {
	if(showingcode == code && !resize)
		return;
	wchar_t formatBuffer[256];
	auto cit = dataManager.GetCodePointer(code);
	bool is_valid = (cit != dataManager.datas_end);
	imgCard->setImage(imageManager.GetTexture(code, true));
	if (is_valid) {
		auto& cd = cit->second;
		if (cd.is_alternative())
			myswprintf(formatBuffer, L"%ls[%08d]", dataManager.GetName(cd.alias), cd.alias);
		else
			myswprintf(formatBuffer, L"%ls[%08d]", dataManager.GetName(code), code);
	}
	else {
		myswprintf(formatBuffer, L"%ls[%08d]", dataManager.GetName(code), code);
	}
	stName->setText(formatBuffer);
	int offset = 0;
	if (is_valid && !gameConf.hide_setname) {
		auto& cd = cit->second;
		auto target = cit;
		if (cd.alias && dataManager.GetCodePointer(cd.alias) != dataManager.datas_end) {
			target = dataManager.GetCodePointer(cd.alias);
		}
		if (target->second.setcode[0]) {
			offset = 23;// *yScale;
			myswprintf(formatBuffer, L"%ls%ls", dataManager.GetSysString(1329), dataManager.FormatSetName(target->second.setcode));
			stSetName->setText(formatBuffer);
		}
		else
			stSetName->setText(L"");
	}
	else {
		stSetName->setText(L"");
	}
	if(is_valid && cit->second.type & TYPE_MONSTER) {
		auto& cd = cit->second;
		myswprintf(formatBuffer, L"[%ls] %ls/%ls", dataManager.FormatType(cd.type), dataManager.FormatRace(cd.race), dataManager.FormatAttribute(cd.attribute));
		stInfo->setText(formatBuffer);
		int offset_info = 0;
		irr::core::dimension2d<unsigned int> dtxt = guiFont->getDimension(formatBuffer);
		if(dtxt.Width > (300 * xScale - 13) - 15)
			offset_info = 15;
		if(!(cd.type & TYPE_LINK)) {
			const wchar_t* form = L"\u2605";
			if(cd.type & TYPE_XYZ) form = L"\u2606";
			myswprintf(formatBuffer, L"[%ls%d] ", form, cd.level);
			wchar_t adBuffer[16];
			if(cd.attack < 0 && cd.defense < 0)
				myswprintf(adBuffer, L"?/?");
			else if(cd.attack < 0)
				myswprintf(adBuffer, L"?/%d", cd.defense);
			else if(cd.defense < 0)
				myswprintf(adBuffer, L"%d/?", cd.attack);
			else
				myswprintf(adBuffer, L"%d/%d", cd.attack, cd.defense);
			wcscat(formatBuffer, adBuffer);
		} else {
			myswprintf(formatBuffer, L"[LINK-%d] ", cd.level);
			wchar_t adBuffer[16];
			if(cd.attack < 0)
				myswprintf(adBuffer, L"?/-   ");
			else
				myswprintf(adBuffer, L"%d/-   ", cd.attack);
			wcscat(formatBuffer, adBuffer);
			wcscat(formatBuffer, dataManager.FormatLinkMarker(cd.link_marker));
		}
		if(cd.type & TYPE_PENDULUM) {
			wchar_t scaleBuffer[16];
			myswprintf(scaleBuffer, L"   %d/%d", cd.lscale, cd.rscale);
			wcscat(formatBuffer, scaleBuffer);
		}
		stDataInfo->setText(formatBuffer);
		int offset_arrows = offset_info;
		dtxt = guiFont->getDimension(formatBuffer);
		if(dtxt.Width > (300 * xScale - 13) - 15)
			offset_arrows += 15;
		stInfo->setRelativePosition(rect<s32>(15, 37, 300 * xScale - 13, (60 + offset_info)));
		stDataInfo->setRelativePosition(rect<s32>(15, (60 + offset_info), 300 * xScale - 13, (83 + offset_arrows)));
		stSetName->setRelativePosition(rect<s32>(15, (83 + offset_arrows), 296 * xScale, (83 + offset_arrows) + offset));
		stText->setRelativePosition(rect<s32>(15, (83 + offset_arrows) + offset, 287 * xScale, 324 * yScale));
		scrCardText->setRelativePosition(rect<s32>(287 * xScale - 20, (83 + offset_arrows) + offset, 287 * xScale, 324 * yScale));
	}
	else {
		if (is_valid)
			myswprintf(formatBuffer, L"[%ls]", dataManager.FormatType(cit->second.type));
		else
			myswprintf(formatBuffer, L"[%ls]", dataManager.FormatType(0));
		stInfo->setText(formatBuffer);
		stDataInfo->setText(L"");
		stSetName->setRelativePosition(rect<s32>(15, 60, 296 * xScale, 60 + offset));
		stText->setRelativePosition(rect<s32>(15, 60 + offset, 287 * xScale, 324 * yScale));
		scrCardText->setRelativePosition(rect<s32>(287 * xScale - 20, 60 + offset, 287 * xScale, 324 * yScale));
	}
	showingcode = code;
	showingtext = dataManager.GetText(code);
	const auto& tsize = stText->getRelativePosition();
	InitStaticText(stText, tsize.getWidth(), tsize.getHeight(), guiFont, showingtext);
}
void Game::ClearCardInfo(int player) {
	imgCard->setImage(imageManager.tCover[player]);
	showingcode = 0;
	stName->setText(L"");
	stInfo->setText(L"");
	stDataInfo->setText(L"");
	stSetName->setText(L"");
	stText->setText(L"");
	scrCardText->setVisible(false);
}
void Game::AddLog(const wchar_t* msg, int param) {
	logParam.push_back(param);
	lstLog->addItem(msg);
	if(!env->hasFocus(lstLog)) {
		lstLog->setSelected(-1);
	}
}
void Game::AddChatMsg(const wchar_t* msg, int player) {
	for(int i = 7; i > 0; --i) {
		chatMsg[i] = chatMsg[i - 1];
		chatTiming[i] = chatTiming[i - 1];
		chatType[i] = chatType[i - 1];
	}
	chatMsg[0].clear();
	chatTiming[0] = 1200;
	chatType[0] = player;
	if(player < 4)
		player = 10;
	switch(player) {
	case 0: //from host
	case 1: //from client
	case 2: //host tag
	case 3: //client tag
	case 7: //local name
	case 8: //system custom message, no prefix.
		soundManager.PlaySoundEffect(SOUND_CHAT);
		chatMsg[0].append(L"[System]: ");
		break;
	case 9: //error message
		chatMsg[0].append(L"[Script Error]: ");
		break;
	case 10: //hidden name
		chatMsg[0].append(L"[********]: ");
		break;
	default: //from watcher or unknown
		if(player < 11 || player > 19)
			chatMsg[0].append(L"[---]: ");
	}
	chatMsg[0].append(msg);
}
void Game::ClearChatMsg() {
	for(int i = 7; i >= 0; --i) {
		chatTiming[i] = 0;
	}
}
void Game::AddDebugMsg(const char* msg) {
	if (enable_log & 0x1) {
		wchar_t wbuf[1024];
		BufferIO::DecodeUTF8(msg, wbuf);
		AddChatMsg(wbuf, 9);
	}
	if (enable_log & 0x2) {
		char msgbuf[1040];
		sprintf(msgbuf, "[Script Error]: %s", msg);
		ErrorLog(msgbuf);
	}
}
void Game::ErrorLog(const char* msg) {
	FILE* fp = fopen("error.log", "at");
	if(!fp)
		return;
	time_t nowtime = time(NULL);
	tm* localedtime = localtime(&nowtime);
	char timebuf[40];
	strftime(timebuf, 40, "%Y-%m-%d %H:%M:%S", localedtime);
	fprintf(fp, "[%s]%s\n", timebuf, msg);
	fclose(fp);
}
void Game::ClearTextures() {
	matManager.mCard.setTexture(0, 0);
	ClearCardInfo(0);
	imageManager.ClearTexture();
}
void Game::CloseGameButtons() {
	wSurrender->setVisible(false);
}
void Game::CloseGameWindow() {
	CloseGameButtons();
	for(auto wit = fadingList.begin(); wit != fadingList.end(); ++wit) {
		if(wit->isFadein)
			wit->autoFadeoutFrame = 1;
	}
	wACMessage->setVisible(false);
	wANAttribute->setVisible(false);
	wANCard->setVisible(false);
	wANNumber->setVisible(false);
	wANRace->setVisible(false);;
	wCmdMenu->setVisible(false);
	wMessage->setVisible(false);
	wOptions->setVisible(false);
	wPhase->setVisible(false);
	wQuery->setVisible(false);
	wReplaySave->setVisible(false);
	stHintMsg->setVisible(false);
	stTip->setVisible(false);
}
void Game::CloseDuelWindow() {
	CloseGameWindow();
	wCardImg->setVisible(false);
	wInfos->setVisible(false);
	wChat->setVisible(false);
	btnSideOK->setVisible(false);
	btnSideShuffle->setVisible(false);
	btnSideSort->setVisible(false);
	btnSideReload->setVisible(false);
	btnLeaveGame->setVisible(false);
	lstLog->clear();
	logParam.clear();
	ClearTextures();
	closeDoneSignal.Set();
}
int Game::LocalPlayer(int player) {
	return dInfo.isFirst ? player : 1 - player;
}
const wchar_t* Game::LocalName(int local_player) {
	return local_player == 0 ? dInfo.hostname : dInfo.clientname;
}
void Game::OnResize() {
#ifdef _WIN32
	WINDOWPLACEMENT plc;
	plc.length = sizeof(WINDOWPLACEMENT);
	if(GetWindowPlacement(hWnd, &plc))
		gameConf.window_maximized = (plc.showCmd == SW_SHOWMAXIMIZED);
#endif // _WIN32
	if(!gameConf.window_maximized) {
		gameConf.window_width = window_size.Width;
		gameConf.window_height = window_size.Height;
	}

	irr::gui::CGUITTFont* old_numFont = numFont;
	irr::gui::CGUITTFont* old_adFont = adFont;
	irr::gui::CGUITTFont* old_lpcFont = lpcFont;
	irr::gui::CGUITTFont* old_textFont = textFont;
	numFont = irr::gui::CGUITTFont::createTTFont(env, gameConf.numfont, (yScale > 0.5 ? 16 * yScale : 8));
	adFont = irr::gui::CGUITTFont::createTTFont(env, gameConf.numfont, (yScale > 0.75 ? 12 * yScale : 9));
	lpcFont = irr::gui::CGUITTFont::createTTFont(env, gameConf.numfont, 48 * yScale);
	textFont = irr::gui::CGUITTFont::createTTFont(env, gameConf.textfont, (yScale > 0.642 ? gameConf.textfontsize * yScale : 9));
	old_numFont->drop();
	old_adFont->drop();
	old_lpcFont->drop();
	old_textFont->drop();

	imageManager.ClearTexture();
	imageManager.ResizeTexture();

	wMainMenu->setRelativePosition(ResizeWin(370, 200, 650, 415));
	wDeckEdit->setRelativePosition(Resize(309, 5, 605, 130));
	cbDBDecks->setRelativePosition(Resize(80, 35, 220, 60));
	btnClearDeck->setRelativePosition(Resize(115, 99, 165, 120));
	btnSortDeck->setRelativePosition(Resize(60, 99, 110, 120));
	btnShuffleDeck->setRelativePosition(Resize(5, 99, 55, 120));
	btnSaveDeck->setRelativePosition(Resize(225, 35, 290, 60));
	btnSaveDeckAs->setRelativePosition(Resize(225, 65, 290, 90));
	ebDeckname->setRelativePosition(Resize(80, 65, 220, 90));
	cbDBCategory->setRelativePosition(Resize(80, 5, 220, 30));
	btnManageDeck->setRelativePosition(Resize(225, 5, 290, 30));
	wDeckManage->setRelativePosition(ResizeWin(310, 135, 800, 465));
	scrPackCards->setRelativePosition(Resize(775, 161, 795, 629));

	wSort->setRelativePosition(Resize(930, 132, 1020, 156));
	cbSortType->setRelativePosition(Resize(10, 2, 85, 22));
	wFilter->setRelativePosition(Resize(610, 5, 1020, 130));
	scrFilter->setRelativePosition(Resize(999, 161, 1019, 629));
	cbCardType->setRelativePosition(Resize(60, 25 / 6, 120, 20 + 25 / 6));
	cbCardType2->setRelativePosition(Resize(125, 25 / 6, 195, 20 + 25 / 6));
	cbRace->setRelativePosition(Resize(60, 40 + 75 / 6, 195, 60 + 75 / 6));
	cbAttribute->setRelativePosition(Resize(60, 20 + 50 / 6, 195, 40 + 50 / 6));
	cbLimit->setRelativePosition(Resize(260, 25 / 6, 390, 20 + 25 / 6));
	ebStar->setRelativePosition(Resize(60, 60 + 100 / 6, 95, 80 + 100 / 6));
	ebScale->setRelativePosition(Resize(155, 60 + 100 / 6, 195, 80 + 100 / 6));
	ebAttack->setRelativePosition(Resize(260, 20 + 50 / 6, 340, 40 + 50 / 6));
	ebDefense->setRelativePosition(Resize(260, 40 + 75 / 6, 340, 60 + 75 / 6));
	ebCardName->setRelativePosition(Resize(260, 60 + 100 / 6, 390, 80 + 100 / 6));
	btnEffectFilter->setRelativePosition(Resize(345, 20 + 50 / 6, 390, 60 + 75 / 6));
	btnStartFilter->setRelativePosition(Resize(260, 80 + 125 / 6, 390, 100 + 125 / 6));
	if(btnClearFilter)
		btnClearFilter->setRelativePosition(Resize(205, 80 + 125 / 6, 255, 100 + 125 / 6));
	btnMarksFilter->setRelativePosition(Resize(60, 80 + 125 / 6, 195, 100 + 125 / 6));

	recti btncatepos = btnEffectFilter->getAbsolutePosition();
	wCategories->setRelativePosition(recti(
		btncatepos.LowerRightCorner.X - wCategories->getRelativePosition().getWidth(),
		btncatepos.LowerRightCorner.Y - btncatepos.getHeight() / 2,
		btncatepos.LowerRightCorner.X,
		btncatepos.LowerRightCorner.Y - btncatepos.getHeight() / 2 + 245));

	wLinkMarks->setRelativePosition(ResizeWin(700, 30, 820, 150));
	stDBCategory->setRelativePosition(Resize(10, 9, 100, 29));
	stDeck->setRelativePosition(Resize(10, 39, 100, 59));
	stCategory->setRelativePosition(Resize(10, 2 + 25 / 6, 70, 22 + 25 / 6));
	stLimit->setRelativePosition(Resize(205, 2 + 25 / 6, 280, 22 + 25 / 6));
	stAttribute->setRelativePosition(Resize(10, 22 + 50 / 6, 70, 42 + 50 / 6));
	stRace->setRelativePosition(Resize(10, 42 + 75 / 6, 70, 62 + 75 / 6));
	stAttack->setRelativePosition(Resize(205, 22 + 50 / 6, 280, 42 + 50 / 6));
	stDefense->setRelativePosition(Resize(205, 42 + 75 / 6, 280, 62 + 75 / 6));
	stStar->setRelativePosition(Resize(10, 62 + 100 / 6, 70, 82 + 100 / 6));
	stSearch->setRelativePosition(Resize(205, 62 + 100 / 6, 280, 82 + 100 / 6));
	stScale->setRelativePosition(Resize(105, 62 + 100 / 6, 165, 82 + 100 / 6));
	btnSideOK->setRelativePosition(Resize(510, 40, 820, 80));
	btnSideShuffle->setRelativePosition(Resize(310, 100, 370, 130));
	btnSideSort->setRelativePosition(Resize(375, 100, 435, 130));
	btnSideReload->setRelativePosition(Resize(440, 100, 500, 130));
	btnDeleteDeck->setRelativePosition(Resize(225, 95, 290, 120));

	wLanWindow->setRelativePosition(ResizeWin(220, 100, 800, 520));
	wCreateHost->setRelativePosition(ResizeWin(320, 100, 700, 520));
	wHostPrepare->setRelativePosition(ResizeWin(270, 120, 750, 440));
	wReplay->setRelativePosition(ResizeWin(220, 100, 800, 520));
	wSinglePlay->setRelativePosition(ResizeWin(220, 100, 800, 520));

	wMessage->setRelativePosition(ResizeWin(490, 200, 840, 340));
	wACMessage->setRelativePosition(ResizeWin(490, 240, 840, 300));
	wQuery->setRelativePosition(ResizeWin(490, 200, 840, 340));
	wSurrender->setRelativePosition(ResizeWin(490, 200, 840, 340));
	wOptions->setRelativePosition(ResizeWin(490, 200, 840, 340));
	wANNumber->setRelativePosition(ResizeWin(550, 180, 780, 430));
	wANCard->setRelativePosition(ResizeWin(510, 120, 820, 420));
	wANAttribute->setRelativePosition(ResizeWin(500, 200, 830, 285));
	wANRace->setRelativePosition(ResizeWin(480, 200, 850, 410));
	wReplaySave->setRelativePosition(ResizeWin(510, 200, 820, 320));
	wDMQuery->setRelativePosition(ResizeWin(400, 200, 710, 320));

	stHintMsg->setRelativePosition(ResizeWin(660 - 160 * xScale, 60, 660 + 160 * xScale, 90));

	//sound / music volume bar
	scrSoundVolume->setRelativePosition(recti(scrSoundVolume->getRelativePosition().UpperLeftCorner.X, scrSoundVolume->getRelativePosition().UpperLeftCorner.Y, 20 + (300 * xScale) - 70, scrSoundVolume->getRelativePosition().LowerRightCorner.Y));
	scrMusicVolume->setRelativePosition(recti(scrMusicVolume->getRelativePosition().UpperLeftCorner.X, scrMusicVolume->getRelativePosition().UpperLeftCorner.Y, 20 + (300 * xScale) - 70, scrMusicVolume->getRelativePosition().LowerRightCorner.Y));

	recti tabSystemPos = recti(0, 0, 300 * xScale - 50, 365 * yScale - 65);
	tabSystem->setRelativePosition(tabSystemPos);
	scrTabSystem->setRelativePosition(recti(tabSystemPos.LowerRightCorner.X + 2, 0, tabSystemPos.LowerRightCorner.X + 22, tabSystemPos.LowerRightCorner.Y));
	s32 tabSystemLastY = elmTabSystemLast->getRelativePosition().LowerRightCorner.Y;
	if(tabSystemLastY > tabSystemPos.LowerRightCorner.Y) {
		scrTabSystem->setMax(tabSystemLastY - tabSystemPos.LowerRightCorner.Y + 5);
		scrTabSystem->setPos(0);
		scrTabSystem->setVisible(true);
	} else
		scrTabSystem->setVisible(false);

	if(gameConf.resize_popup_menu) {
		int width = 100 * xScale;
		int height = (yScale >= 0.666) ? 21 * yScale : 14;
		wCmdMenu->setRelativePosition(recti(1, 1, width + 1, 1));
	}

	wCardImg->setRelativePosition(ResizeCardImgWin(1, 1, 20, 18));
	imgCard->setRelativePosition(ResizeCardImgWin(10, 9, 0, 0));
	wInfos->setRelativePosition(Resize(1, 275, 301, 639));
	stName->setRelativePosition(recti(10, 10, 300 * xScale - 13, 10 + 22));
	lstLog->setRelativePosition(Resize(10, 10, 290, 290));
	if(showingcode)
		ShowCardInfo(showingcode, true);
	else
		ClearCardInfo();
	btnClearLog->setRelativePosition(Resize(160, 300, 260, 325));

	wPhase->setRelativePosition(Resize(480, 310, 855, 330));

	wChat->setRelativePosition(recti(wInfos->getRelativePosition().LowerRightCorner.X + 6, window_size.Height - 25, window_size.Width, window_size.Height));
	ebChatInput->setRelativePosition(recti(3, 2, window_size.Width - wChat->getRelativePosition().UpperLeftCorner.X - 6, 22));

	btnLeaveGame->setRelativePosition(Resize(205, 5, 295, 80));
	btnBigCardOriginalSize->setRelativePosition(Resize(205, 100, 295, 135));
	btnBigCardZoomIn->setRelativePosition(Resize(205, 140, 295, 175));
	btnBigCardZoomOut->setRelativePosition(Resize(205, 180, 295, 215));
	btnBigCardClose->setRelativePosition(Resize(205, 230, 295, 265));
}
recti Game::Resize(s32 x, s32 y, s32 x2, s32 y2) {
	x = x * xScale;
	y = y * yScale;
	x2 = x2 * xScale;
	y2 = y2 * yScale;
	return recti(x, y, x2, y2);
}
recti Game::Resize(s32 x, s32 y, s32 x2, s32 y2, s32 dx, s32 dy, s32 dx2, s32 dy2) {
	x = x * xScale + dx;
	y = y * yScale + dy;
	x2 = x2 * xScale + dx2;
	y2 = y2 * yScale + dy2;
	return recti(x, y, x2, y2);
}
position2di Game::Resize(s32 x, s32 y) {
	x = x * xScale;
	y = y * yScale;
	return position2di(x, y);
}
position2di Game::ResizeReverse(s32 x, s32 y) {
	x = x / xScale;
	y = y / yScale;
	return position2di(x, y);
}
recti Game::ResizeWin(s32 x, s32 y, s32 x2, s32 y2) {
	s32 w = x2 - x;
	s32 h = y2 - y;
	x = (x + w / 2) * xScale - w / 2;
	y = (y + h / 2) * yScale - h / 2;
	x2 = w + x;
	y2 = h + y;
	return recti(x, y, x2, y2);
}
recti Game::ResizePhaseHint(s32 x, s32 y, s32 x2, s32 y2, s32 width) {
	x = x * xScale - width / 2;
	y = y * yScale;
	x2 = x2 * xScale;
	y2 = y2 * yScale;
	return recti(x, y, x2, y2);
}
recti Game::ResizeCardImgWin(s32 x, s32 y, s32 mx, s32 my) {
	float mul = xScale;
	if(xScale > yScale)
		mul = yScale;
	s32 w = CARD_IMG_WIDTH * mul + mx * xScale;
	s32 h = CARD_IMG_HEIGHT * mul + my * yScale;
	x = x * xScale;
	y = y * yScale;
	return recti(x, y, x + w, y + h);
}
recti Game::ResizeCardHint(s32 x, s32 y, s32 x2, s32 y2) {
	return ResizeCardMid(x, y, x2, y2, (x + x2) * 0.5, (y + y2) * 0.5);
}
position2di Game::ResizeCardHint(s32 x, s32 y) {
	return ResizeCardMid(x, y, x + CARD_IMG_WIDTH * 0.5, y + CARD_IMG_HEIGHT * 0.5);
}
recti Game::ResizeCardMid(s32 x, s32 y, s32 x2, s32 y2, s32 midx, s32 midy) {
	float mul = xScale;
	if(xScale > yScale)
		mul = yScale;
	s32 cx = midx * xScale;
	s32 cy = midy * yScale;
	x = cx + (x - midx) * mul;
	y = cy + (y - midy) * mul;
	x2 = cx + (x2 - midx) * mul;
	y2 = cy + (y2 - midy) * mul;
	return recti(x, y, x2, y2);
}
position2di Game::ResizeCardMid(s32 x, s32 y, s32 midx, s32 midy) {
	float mul = xScale;
	if(xScale > yScale)
		mul = yScale;
	s32 cx = midx * xScale;
	s32 cy = midy * yScale;
	x = cx + (x - midx) * mul;
	y = cy + (y - midy) * mul;
	return position2di(x, y);
}
recti Game::ResizeFit(s32 x, s32 y, s32 x2, s32 y2) {
	float mul = xScale;
	if(xScale > yScale)
		mul = yScale;
	x = x * mul;
	y = y * mul;
	x2 = x2 * mul;
	y2 = y2 * mul;
	return recti(x, y, x2, y2);
}
void Game::SetWindowsIcon() {
#ifdef _WIN32
	HINSTANCE hInstance = (HINSTANCE)GetModuleHandleW(NULL);
	HICON hSmallIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	HICON hBigIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
	SendMessageW(hWnd, WM_SETICON, ICON_SMALL, (long)hSmallIcon);
	SendMessageW(hWnd, WM_SETICON, ICON_BIG, (long)hBigIcon);
#endif
}
void Game::SetWindowsScale(float scale) {
#ifdef _WIN32
	WINDOWPLACEMENT plc;
	plc.length = sizeof(WINDOWPLACEMENT);
	if(GetWindowPlacement(hWnd, &plc) && (plc.showCmd == SW_SHOWMAXIMIZED))
		ShowWindow(hWnd, SW_RESTORE);
	RECT rcWindow, rcClient;
	GetWindowRect(hWnd, &rcWindow);
	GetClientRect(hWnd, &rcClient);
	MoveWindow(hWnd, rcWindow.left, rcWindow.top,
		(rcWindow.right - rcWindow.left) - rcClient.right + 1024 * scale,
		(rcWindow.bottom - rcWindow.top) - rcClient.bottom + 640 * scale,
		true);
#endif
}
void Game::FlashWindow() {
#ifdef _WIN32
	FLASHWINFO fi;
	fi.cbSize = sizeof(FLASHWINFO);
	fi.hwnd = hWnd;
	fi.dwFlags = FLASHW_TRAY | FLASHW_TIMERNOFG;
	fi.uCount = 0;
	fi.dwTimeout = 0;
	FlashWindowEx(&fi);
#endif
}
void Game::SetCursor(ECURSOR_ICON icon) {
	ICursorControl* cursor = device->getCursorControl();
	if(cursor->getActiveIcon() != icon) {
		cursor->setActiveIcon(icon);
	}
}

}
