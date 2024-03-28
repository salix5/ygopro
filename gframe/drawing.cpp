#include "game.h"
#include "materials.h"
#include "image_manager.h"
#include "deck_manager.h"
#include "sound_manager.h"
#include "../ocgcore/common.h"

namespace ygo {

void Game::DrawShadowText(CGUITTFont * font, const core::stringw & text, const core::rect<s32>& position, const core::rect<s32>& padding,
						  video::SColor color, video::SColor shadowcolor, bool hcenter, bool vcenter, const core::rect<s32>* clip) {
	core::rect<s32> shadowposition = recti(position.UpperLeftCorner.X - padding.UpperLeftCorner.X, position.UpperLeftCorner.Y - padding.UpperLeftCorner.Y, 
										   position.LowerRightCorner.X - padding.LowerRightCorner.X, position.LowerRightCorner.Y - padding.LowerRightCorner.Y);
	font->draw(text, shadowposition, shadowcolor, hcenter, vcenter, clip);
	font->draw(text, position, color, hcenter, vcenter, clip);
}
void Game::DrawGUI() {
	for(auto fit = fadingList.begin(); fit != fadingList.end();) {
		auto fthis = fit++;
		FadingUnit& fu = *fthis;
		if(fu.fadingFrame) {
			fu.guiFading->setVisible(true);
			if(fu.isFadein) {
				if(fu.fadingFrame > 5) {
					fu.fadingUL.X -= fu.fadingDiff.X;
					fu.fadingLR.X += fu.fadingDiff.X;
					fu.fadingFrame--;
					fu.guiFading->setRelativePosition(irr::core::recti(fu.fadingUL, fu.fadingLR));
				} else {
					fu.fadingUL.Y -= fu.fadingDiff.Y;
					fu.fadingLR.Y += fu.fadingDiff.Y;
					fu.fadingFrame--;
					if(!fu.fadingFrame) {
						fu.guiFading->setRelativePosition(fu.fadingSize);
						env->setFocus(fu.guiFading);
					} else
						fu.guiFading->setRelativePosition(irr::core::recti(fu.fadingUL, fu.fadingLR));
				}
			} else {
				if(fu.fadingFrame > 5) {
					fu.fadingUL.Y += fu.fadingDiff.Y;
					fu.fadingLR.Y -= fu.fadingDiff.Y;
					fu.fadingFrame--;
					fu.guiFading->setRelativePosition(irr::core::recti(fu.fadingUL, fu.fadingLR));
				} else {
					fu.fadingUL.X += fu.fadingDiff.X;
					fu.fadingLR.X -= fu.fadingDiff.X;
					fu.fadingFrame--;
					if(!fu.fadingFrame) {
						fu.guiFading->setVisible(false);
						fu.guiFading->setRelativePosition(fu.fadingSize);
					} else
						fu.guiFading->setRelativePosition(irr::core::recti(fu.fadingUL, fu.fadingLR));
				}
				if(fu.signalAction && !fu.fadingFrame) {
					fu.signalAction = false;
				}
			}
		} else if(fu.autoFadeoutFrame) {
			fu.autoFadeoutFrame--;
			if(!fu.autoFadeoutFrame)
				HideElement(fu.guiFading);
		} else
			fadingList.erase(fthis);
	}
	env->drawAll();
}
void Game::DrawSpec() {
	s32 midx = 574 + (CARD_IMG_WIDTH * 0.5);
	s32 midy = 150 + (CARD_IMG_HEIGHT * 0.5);
	if(showcard) {
		switch(showcard) {
		case 1: {
			driver->draw2DImage(imageManager.GetTexture(showcardcode, true), ResizeCardHint(574, 150));
			driver->draw2DImage(imageManager.tMask, ResizeCardMid(574, 150, 574 + (showcarddif > CARD_IMG_WIDTH ? CARD_IMG_WIDTH : showcarddif), 150 + CARD_IMG_HEIGHT, midx, midy),
								recti(CARD_IMG_HEIGHT - showcarddif, 0, CARD_IMG_HEIGHT - (showcarddif > CARD_IMG_WIDTH ? showcarddif - CARD_IMG_WIDTH : 0), CARD_IMG_HEIGHT), 0, 0, true);
			showcarddif += 15;
			if(showcarddif >= CARD_IMG_HEIGHT) {
				showcard = 2;
				showcarddif = 0;
			}
			break;
		}
		case 2: {
			driver->draw2DImage(imageManager.GetTexture(showcardcode, true), ResizeCardHint(574, 150));
			driver->draw2DImage(imageManager.tMask, ResizeCardMid(574 + showcarddif, 150, 574 + CARD_IMG_WIDTH, 150 + CARD_IMG_HEIGHT, midx, midy),
								recti(0, 0, CARD_IMG_WIDTH - showcarddif, CARD_IMG_HEIGHT), 0, 0, true);
			showcarddif += 15;
			if(showcarddif >= CARD_IMG_WIDTH) {
				showcard = 0;
			}
			break;
		}
		case 3: {
			driver->draw2DImage(imageManager.GetTexture(showcardcode, true), ResizeCardHint(574, 150));
			driver->draw2DImage(imageManager.tNegated, ResizeCardMid(536 + showcarddif, 141 + showcarddif, 792 - showcarddif, 397 - showcarddif, midx, midy), recti(0, 0, 128, 128), 0, 0, true);
			if(showcarddif < 64)
				showcarddif += 4;
			break;
		}
		case 4: {
			matManager.c2d[0] = (showcarddif << 24) | 0xffffff;
			matManager.c2d[1] = (showcarddif << 24) | 0xffffff;
			matManager.c2d[2] = (showcarddif << 24) | 0xffffff;
			matManager.c2d[3] = (showcarddif << 24) | 0xffffff;
			driver->draw2DImage(imageManager.GetTexture(showcardcode, true), ResizeCardHint(574, 150, 574 + CARD_IMG_WIDTH, 150 + CARD_IMG_HEIGHT),
								ResizeFit(0, 0, CARD_IMG_WIDTH, CARD_IMG_HEIGHT), 0, matManager.c2d, true);
			if(showcarddif < 255)
				showcarddif += 17;
			break;
		}
		case 5: {
			matManager.c2d[0] = (showcarddif << 25) | 0xffffff;
			matManager.c2d[1] = (showcarddif << 25) | 0xffffff;
			matManager.c2d[2] = (showcarddif << 25) | 0xffffff;
			matManager.c2d[3] = (showcarddif << 25) | 0xffffff;
			driver->draw2DImage(imageManager.GetTexture(showcardcode, true), ResizeCardMid(662 - showcarddif * 0.69685f, 277 - showcarddif, 662 + showcarddif * 0.69685f, 277 + showcarddif, midx, midy),
								ResizeFit(0, 0, CARD_IMG_WIDTH, CARD_IMG_HEIGHT), 0, matManager.c2d, true);
			if(showcarddif < 127)
				showcarddif += 9;
			break;
		}
		case 6: {
			driver->draw2DImage(imageManager.GetTexture(showcardcode, true), ResizeCardHint(574, 150));
			driver->draw2DImage(imageManager.tNumber, ResizeCardMid(536 + showcarddif, 141 + showcarddif, 792 - showcarddif, 397 - showcarddif, midx, midy),
			                    recti((showcardp % 5) * 64, (showcardp / 5) * 64, (showcardp % 5 + 1) * 64, (showcardp / 5 + 1) * 64), 0, 0, true);
			if(showcarddif < 64)
				showcarddif += 4;
			break;
		}
		case 7: {
			float mul = xScale;
			if(xScale > yScale)
				mul = yScale;
			core::position2d<s32> corner[4];
			float y = sin(showcarddif * 3.1415926f / 180.0f) * CARD_IMG_HEIGHT * mul;
			s32 winx = midx * xScale + (574 - midx) * mul;
			s32 winx2 = midx * xScale + (751 - midx) * mul;
			s32 winy = midy * yScale + (404 - midy) * mul;
			corner[0] = core::position2d<s32>(winx - (CARD_IMG_HEIGHT * mul - y) * 0.3f, winy - y);
			corner[1] = core::position2d<s32>(winx2 + (CARD_IMG_HEIGHT * mul - y) * 0.3f, winy - y);
			corner[2] = core::position2d<s32>(winx, winy);
			corner[3] = core::position2d<s32>(winx2, winy);
			irr::gui::Draw2DImageQuad(driver, imageManager.GetTexture(showcardcode, true), ResizeFit(0, 0, CARD_IMG_WIDTH, CARD_IMG_HEIGHT), corner);
			showcardp++;
			showcarddif += 9;
			if(showcarddif >= 90)
				showcarddif = 90;
			if(showcardp == 60) {
				showcardp = 0;
				showcarddif = 0;
			}
			break;
		}
		case 100: {
			if(showcardp < 60) {
				driver->draw2DImage(imageManager.tHand[(showcardcode >> 16) & 0x3], position2di((615 + 44.5) * xScale - 44.5, (showcarddif + 64) * yScale - 64));
				driver->draw2DImage(imageManager.tHand[showcardcode & 0x3], position2di((615 + 44.5) * xScale - 44.5, (540 - showcarddif + 64) * yScale - 64));
				float dy = -0.333333f * showcardp + 10;
				showcardp++;
				if(showcardp < 30)
					showcarddif += (int)dy;
			} else
				showcard = 0;
			break;
		}
		case 101: {
			const wchar_t* lstr = L"";
			switch(showcardcode) {
			case 1:
				lstr = L"You Win!";
				break;
			case 2:
				lstr = L"You Lose!";
				break;
			case 3:
				lstr = L"Draw Game";
				break;
			case 4:
				lstr = L"Draw Phase";
				break;
			case 5:
				lstr = L"Standby Phase";
				break;
			case 6:
				lstr = L"Main Phase 1";
				break;
			case 7:
				lstr = L"Battle Phase";
				break;
			case 8:
				lstr = L"Main Phase 2";
				break;
			case 9:
				lstr = L"End Phase";
				break;
			case 10:
				lstr = L"Next Players Turn";
				break;
			case 11:
				lstr = L"Duel Start";
				break;
			case 12:
				lstr = L"Duel1 Start";
				break;
			case 13:
				lstr = L"Duel2 Start";
				break;
			case 14:
				lstr = L"Duel3 Start";
				break;
			}
			auto pos = lpcFont->getDimension(lstr);
			if(showcardp < 10) {
				int alpha = (showcardp * 25) << 24;
				DrawShadowText(lpcFont, lstr, ResizePhaseHint(660 - (9 - showcardp) * 40, 290, 960, 370, pos.Width), Resize(-1, -1, 0, 0), alpha | 0xffffff, alpha);
			} else if(showcardp < showcarddif) {
				DrawShadowText(lpcFont, lstr, ResizePhaseHint(660, 290, 960, 370, pos.Width), Resize(-1, -1, 0, 0), 0xffffffff);
				if(dInfo.vic_string && (showcardcode == 1 || showcardcode == 2)) {
					int w = guiFont->getDimension(dInfo.vic_string).Width;
					if(w < 200)
						w = 200;
					driver->draw2DRectangle(0xa0000000, ResizeWin(640 - w / 2, 320, 690 + w / 2, 340));
					DrawShadowText(guiFont, dInfo.vic_string, ResizeWin(640 - w / 2, 320, 690 + w / 2, 340), Resize(-2, -1, 0, 0), 0xffffffff, 0xff000000, true, true, 0);
				}
			} else if(showcardp < showcarddif + 10) {
				int alpha = ((showcarddif + 10 - showcardp) * 25) << 24;
				DrawShadowText(lpcFont, lstr, ResizePhaseHint(660 + (showcardp - showcarddif) * 40, 290, 960, 370, pos.Width), Resize(-1, -1, 0, 0), alpha | 0xffffff, alpha);
			}
			showcardp++;
			break;
		}
		}
	}
	if(is_attacking) {
		irr::core::matrix4 matk;
		matk.setTranslation(atk_t);
		matk.setRotationRadians(atk_r);
		driver->setTransform(irr::video::ETS_WORLD, matk);
		driver->setMaterial(matManager.mATK);
		driver->drawVertexPrimitiveList(&matManager.vArrow[attack_sv], 12, matManager.iArrow, 10, EVT_STANDARD, EPT_TRIANGLE_STRIP);
		attack_sv += 4;
		if (attack_sv > 28)
			attack_sv = 0;
	}
	bool showChat = true;
	if(hideChat) {
	    showChat = false;
	    hideChatTimer = 10;
	} else if(hideChatTimer > 0) {
	    showChat = false;
	    hideChatTimer--;
	}
	for(int i = 0; i < 8; ++i) {
		static unsigned int chatColor[] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xff8080ff, 0xffff4040, 0xffff4040,
		                                   0xffff4040, 0xff40ff40, 0xff4040ff, 0xff40ffff, 0xffff40ff, 0xffffff40, 0xffffffff, 0xff808080, 0xff404040};
		if(chatTiming[i]) {
			chatTiming[i]--;
			if(mainGame->dInfo.isStarted && i >= 5)
				continue;
			if(!showChat && i > 2)
				continue;
			int w = guiFont->getDimension(chatMsg[i].c_str()).Width;

			recti rectloc(mainGame->wChat->getRelativePosition().UpperLeftCorner.X, mainGame->window_size.Height - 45, mainGame->wChat->getRelativePosition().UpperLeftCorner.X + 2 + w, mainGame->window_size.Height - 25);
			rectloc -= position2di(0, i * 20);
			recti msgloc(mainGame->wChat->getRelativePosition().UpperLeftCorner.X, mainGame->window_size.Height - 45, mainGame->wChat->getRelativePosition().UpperLeftCorner.X - 4, mainGame->window_size.Height - 25);
			msgloc -= position2di(0, i * 20);
			recti shadowloc = msgloc + position2di(1, 1);

			driver->draw2DRectangle(rectloc, 0xa0000000, 0xa0000000, 0xa0000000, 0xa0000000);
			guiFont->draw(chatMsg[i].c_str(), msgloc, 0xff000000, false, false);
			guiFont->draw(chatMsg[i].c_str(), shadowloc, chatColor[chatType[i]], false, false);
		}
	}
}
void Game::DrawBackImage(irr::video::ITexture* texture) {
	if(!texture)
		return;
	driver->draw2DImage(texture, Resize(0, 0, 1024, 640), recti(0, 0, texture->getOriginalSize().Width, texture->getOriginalSize().Height));
}
void Game::ShowElement(irr::gui::IGUIElement * win, int autoframe) {
	FadingUnit fu;
	fu.fadingSize = win->getRelativePosition();
	for(auto fit = fadingList.begin(); fit != fadingList.end(); ++fit)
		if(win == fit->guiFading && win != wOptions && win != wANNumber) // the size of wOptions is always setted by ClientField::ShowSelectOption before showing it
			fu.fadingSize = fit->fadingSize;
	irr::core::position2di center = fu.fadingSize.getCenter();
	fu.fadingDiff.X = fu.fadingSize.getWidth() / 10;
	fu.fadingDiff.Y = (fu.fadingSize.getHeight() - 4) / 10;
	fu.fadingUL = center;
	fu.fadingLR = center;
	fu.fadingUL.Y -= 2;
	fu.fadingLR.Y += 2;
	fu.guiFading = win;
	fu.isFadein = true;
	fu.fadingFrame = 10;
	fu.autoFadeoutFrame = autoframe;
	fu.signalAction = 0;
	win->setRelativePosition(irr::core::recti(center.X, center.Y, 0, 0));
	win->setVisible(true);
	fadingList.push_back(fu);
}
void Game::HideElement(irr::gui::IGUIElement * win, bool set_action) {
	if(!win->isVisible() && !set_action)
		return;
	FadingUnit fu;
	fu.fadingSize = win->getRelativePosition();
	for(auto fit = fadingList.begin(); fit != fadingList.end(); ++fit)
		if(win == fit->guiFading)
			fu.fadingSize = fit->fadingSize;
	fu.fadingDiff.X = fu.fadingSize.getWidth() / 10;
	fu.fadingDiff.Y = (fu.fadingSize.getHeight() - 4) / 10;
	fu.fadingUL = fu.fadingSize.UpperLeftCorner;
	fu.fadingLR = fu.fadingSize.LowerRightCorner;
	fu.guiFading = win;
	fu.isFadein = false;
	fu.fadingFrame = 10;
	fu.autoFadeoutFrame = 0;
	fu.signalAction = set_action;
	fadingList.push_back(fu);
}
void Game::PopupElement(irr::gui::IGUIElement * element, int hideframe) {
	soundManager.PlayDialogSound(element);
	element->getParent()->bringToFront(element);
	env->setFocus(element);
	if(!hideframe)
		ShowElement(element);
	else ShowElement(element, hideframe);
}
void Game::DrawThumb(code_pointer cp, position2di pos, const std::unordered_map<int,int>* lflist, bool drag) {
	int code = cp->first;
	int lcode = cp->second.alias;
	if(lcode == 0)
		lcode = code;
	irr::video::ITexture* img = imageManager.GetTextureThumb(code);
	if(img == NULL)
		return; //NULL->getSize() will cause a crash
	dimension2d<u32> size = img->getOriginalSize();
	recti dragloc = mainGame->Resize(pos.X, pos.Y, pos.X + CARD_THUMB_WIDTH, pos.Y + CARD_THUMB_HEIGHT);
	recti limitloc = mainGame->Resize(pos.X, pos.Y, pos.X + 20, pos.Y + 20);
	recti otloc = Resize(pos.X + 7, pos.Y + 50, pos.X + 37, pos.Y + 65);
	if(drag) {
		dragloc = recti(pos.X, pos.Y, pos.X + CARD_THUMB_WIDTH * mainGame->xScale, pos.Y + CARD_THUMB_HEIGHT * mainGame->yScale);
		limitloc = recti(pos.X, pos.Y, pos.X + 20 * mainGame->xScale, pos.Y + 20 * mainGame->yScale);
		otloc = recti(pos.X + 7, pos.Y + 50 * mainGame->yScale, pos.X + 37 * mainGame->xScale, pos.Y + 65 * mainGame->yScale);
	}
	driver->draw2DImage(img, dragloc, rect<s32>(0, 0, size.Width, size.Height));
	if(lflist->count(lcode)) {
		switch((*lflist).at(lcode)) {
		case 0:
			driver->draw2DImage(imageManager.tLim, limitloc, recti(0, 0, 64, 64), 0, 0, true);
			break;
		case 1:
			driver->draw2DImage(imageManager.tLim, limitloc, recti(64, 0, 128, 64), 0, 0, true);
			break;
		case 2:
			driver->draw2DImage(imageManager.tLim, limitloc, recti(0, 64, 64, 128), 0, 0, true);
			break;
		}
	}
	bool showAvail = false;
	bool showNotAvail = false;
	int filter_lm = cbLimit->getSelected();
	bool avail = !((filter_lm == 4 && !(cp->second.ot & AVAIL_OCG)
				|| (filter_lm == 5 && !(cp->second.ot & AVAIL_TCG))
				|| (filter_lm == 6 && !(cp->second.ot & AVAIL_SC))
				|| (filter_lm == 7 && !(cp->second.ot & AVAIL_CUSTOM))
				|| (filter_lm == 8 && (cp->second.ot & AVAIL_OCGTCG) != AVAIL_OCGTCG)));
	if(filter_lm >= 4) {
		showAvail = avail;
		showNotAvail = !avail;
	} else if(!(cp->second.ot & gameConf.defaultOT)) {
		showNotAvail = true;
	}
	if(showAvail) {
		if((cp->second.ot & AVAIL_OCG) && !(cp->second.ot & AVAIL_TCG))
			driver->draw2DImage(imageManager.tOT, otloc, recti(0, 128, 128, 192), 0, 0, true);
		else if((cp->second.ot & AVAIL_TCG) && !(cp->second.ot & AVAIL_OCG))
			driver->draw2DImage(imageManager.tOT, otloc, recti(0, 192, 128, 256), 0, 0, true);
	} else if(showNotAvail) {
		if(cp->second.ot & AVAIL_OCG)
			driver->draw2DImage(imageManager.tOT, otloc, recti(0, 0, 128, 64), 0, 0, true);
		else if(cp->second.ot & AVAIL_TCG)
			driver->draw2DImage(imageManager.tOT, otloc, recti(0, 64, 128, 128), 0, 0, true);
		else if(!avail)
			driver->draw2DImage(imageManager.tLim, otloc, recti(0, 0, 64, 64), 0, 0, true);
	}
}
void Game::DrawDeckBd() {
	wchar_t textBuffer[64];
	//main deck
	int mainsize = deckManager.current_deck.main.size();
	driver->draw2DRectangle(Resize(310, 137, 410, 157), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
	driver->draw2DRectangleOutline(Resize(309, 136, 410, 157));
	DrawShadowText(textFont, dataManager.GetSysString(deckBuilder.showing_pack ? 1477 : 1330), Resize(315, 137, 410, 157), Resize(1, 1, 1, 1), 0xffffffff, 0xff000000, false, true);
	DrawShadowText(numFont, dataManager.numStrings[mainsize], Resize(380, 138, 440, 158), Resize(1, 1, 1, 1), 0xffffffff, 0xff000000, false, true);
	driver->draw2DRectangle(Resize(310, 160, 797, deckBuilder.showing_pack ? 630 : 436), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
	driver->draw2DRectangleOutline(Resize(309, 159, 797, deckBuilder.showing_pack ? 630 : 436));
	int lx;
	int dy = 68;
	float dx;
	if(mainsize <= 40) {
		dx = 436.0f / 9;
		lx = 10;
	} else if(deckBuilder.showing_pack) {
		lx = 10;
		if(mainsize > 10 * 7)
			lx = 11;
		if(mainsize > 11 * 7)
			lx = 12;
		dx = (mainGame->scrPackCards->isVisible() ? 414.0f : 436.0f) / (lx - 1);
		if(mainsize > 60)
			dy = 66;
	} else {
		lx = (mainsize - 41) / 4 + 11;
		dx = 436.0f / (lx - 1);
	}
	int padding = scrPackCards->getPos() * lx;
	for(int i = 0; i < mainsize - padding && i < 7 * lx; ++i) {
		int j = i + padding;
		DrawThumb(deckManager.current_deck.main[j], position2di(314 + (i % lx) * dx, 164 + (i / lx) * dy), deckBuilder.filterList);
		if(deckBuilder.hovered_pos == 1 && deckBuilder.hovered_seq == j)
			driver->draw2DRectangleOutline(Resize(313 + (i % lx) * dx, 163 + (i / lx) * dy, 359 + (i % lx) * dx, 228 + (i / lx) * dy));
	}
	if(!deckBuilder.showing_pack) {
		//extra deck
		driver->draw2DRectangle(Resize(310, 440, 410, 460), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
		driver->draw2DRectangleOutline(Resize(309, 439, 410, 460));
		DrawShadowText(textFont, dataManager.GetSysString(1331), Resize(315, 440, 410, 460), Resize(1, 1, 1, 1), 0xffffffff, 0xff000000, false, true);
		DrawShadowText(numFont, dataManager.numStrings[deckManager.current_deck.extra.size()], Resize(380, 441, 440, 461), Resize(1, 1, 1, 1), 0xffffffff, 0xff000000, false, true);
		driver->draw2DRectangle(Resize(310, 463, 797, 533), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
		driver->draw2DRectangleOutline(Resize(309, 462, 797, 533));
		if(deckManager.current_deck.extra.size() <= 10)
			dx = 436.0f / 9;
		else dx = 436.0f / (deckManager.current_deck.extra.size() - 1);
		for(size_t i = 0; i < deckManager.current_deck.extra.size(); ++i) {
			DrawThumb(deckManager.current_deck.extra[i], position2di(314 + i * dx, 466), deckBuilder.filterList);
			if(deckBuilder.hovered_pos == 2 && deckBuilder.hovered_seq == (int)i)
				driver->draw2DRectangleOutline(Resize(313 + i * dx, 465, 359 + i * dx, 531));
		}
		//side deck
		driver->draw2DRectangle(Resize(310, 537, 410, 557), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
		driver->draw2DRectangleOutline(Resize(309, 536, 410, 557));
		DrawShadowText(textFont, dataManager.GetSysString(1332), Resize(315, 537, 410, 557), Resize(1, 1, 1, 1), 0xffffffff, 0xff000000, false, true);
		DrawShadowText(numFont, dataManager.numStrings[deckManager.current_deck.side.size()], Resize(380, 538, 440, 558), Resize(1, 1, 1, 1), 0xffffffff, 0xff000000, false, true);
		driver->draw2DRectangle(Resize(310, 560, 797, 630), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
		driver->draw2DRectangleOutline(Resize(309, 559, 797, 630));
		if(deckManager.current_deck.side.size() <= 10)
			dx = 436.0f / 9;
		else dx = 436.0f / (deckManager.current_deck.side.size() - 1);
		for(size_t i = 0; i < deckManager.current_deck.side.size(); ++i) {
			DrawThumb(deckManager.current_deck.side[i], position2di(314 + i * dx, 564), deckBuilder.filterList);
			if(deckBuilder.hovered_pos == 3 && deckBuilder.hovered_seq == (int)i)
				driver->draw2DRectangleOutline(Resize(313 + i * dx, 563, 359 + i * dx, 629));
		}
	}
	//search result
	driver->draw2DRectangle(Resize(805, 137, 926, 157), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
	driver->draw2DRectangleOutline(Resize(804, 136, 926, 157));
	DrawShadowText(textFont, dataManager.GetSysString(1333), Resize(810, 137, 915, 157), Resize(1, 1, 1, 1), 0xffffffff, 0xff000000, false, true);
	DrawShadowText(numFont, deckBuilder.result_string, Resize(875, 137, 935, 157), Resize(1, 1, 1, 1), 0xffffffff, 0xff000000, false, true);
	driver->draw2DRectangle(Resize(805, 160, 1020, 630), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
	driver->draw2DRectangleOutline(Resize(804, 159, 1020, 630));
	for(size_t i = 0; i < 9 && i + scrFilter->getPos() < deckBuilder.results.size(); ++i) {
		code_pointer ptr = deckBuilder.results[i + scrFilter->getPos()];
		if(i >= 7)
		{
			imageManager.GetTextureThumb(ptr->second.code);
			break;
		}
		if(deckBuilder.hovered_pos == 4 && deckBuilder.hovered_seq == (int)i)
			driver->draw2DRectangle(0x80000000, Resize(806, 164 + i * 66, 1019, 230 + i * 66));
		DrawThumb(ptr, position2di(810, 165 + i * 66), deckBuilder.filterList);
		if(ptr->second.type & TYPE_MONSTER) {
			myswprintf(textBuffer, L"%ls", dataManager.GetName(ptr->first));
			DrawShadowText(textFont, textBuffer, Resize(860, 165 + i * 66, 955, 185 + i * 66), Resize(1, 1, 0, 0));
			if(!(ptr->second.type & TYPE_LINK)) {
				const wchar_t* form = L"\u2605";
				if(ptr->second.type & TYPE_XYZ) form = L"\u2606";
				myswprintf(textBuffer, L"%ls/%ls %ls%d", dataManager.FormatAttribute(ptr->second.attribute), dataManager.FormatRace(ptr->second.race), form, ptr->second.level);
				DrawShadowText(textFont, textBuffer, Resize(860, 187 + i * 66, 955, 207 + i * 66), Resize(1, 1, 0, 0));
				if(ptr->second.attack < 0 && ptr->second.defense < 0)
					myswprintf(textBuffer, L"?/?");
				else if(ptr->second.attack < 0)
					myswprintf(textBuffer, L"?/%d", ptr->second.defense);
				else if(ptr->second.defense < 0)
					myswprintf(textBuffer, L"%d/?", ptr->second.attack);
				else myswprintf(textBuffer, L"%d/%d", ptr->second.attack, ptr->second.defense);
			} else {
				myswprintf(textBuffer, L"%ls/%ls LINK-%d", dataManager.FormatAttribute(ptr->second.attribute), dataManager.FormatRace(ptr->second.race), ptr->second.level);
				DrawShadowText(textFont, textBuffer, Resize(860, 187 + i * 66, 955, 207 + i * 66), Resize(1, 1, 0, 0));
				if(ptr->second.attack < 0)
					myswprintf(textBuffer, L"?/-");
				else myswprintf(textBuffer, L"%d/-", ptr->second.attack);
			}
			if(ptr->second.type & TYPE_PENDULUM) {
				wchar_t scaleBuffer[16];
				myswprintf(scaleBuffer, L" %d/%d", ptr->second.lscale, ptr->second.rscale);
				wcscat(textBuffer, scaleBuffer);
			}
			if((ptr->second.ot & AVAIL_OCGTCG) == AVAIL_OCG)
				wcscat(textBuffer, L" [OCG]");
			else if((ptr->second.ot & AVAIL_OCGTCG) == AVAIL_TCG)
				wcscat(textBuffer, L" [TCG]");
			else if((ptr->second.ot & AVAIL_CUSTOM) == AVAIL_CUSTOM)
				wcscat(textBuffer, L" [Custom]");
			DrawShadowText(textFont, textBuffer, Resize(860, 209 + i * 66, 955, 229 + i * 66), Resize(1, 1, 0, 0));
		} else {
			myswprintf(textBuffer, L"%ls", dataManager.GetName(ptr->first));
			DrawShadowText(textFont, textBuffer, Resize(860, 165 + i * 66, 955, 185 + i * 66), Resize(1, 1, 0, 0));
			const wchar_t* ptype = dataManager.FormatType(ptr->second.type);
			DrawShadowText(textFont, ptype, Resize(860, 187 + i * 66, 955, 207 + i * 66), Resize(1, 1, 0, 0));
			textBuffer[0] = 0;
			if((ptr->second.ot & AVAIL_OCGTCG) == AVAIL_OCG)
				wcscat(textBuffer, L"[OCG]");
			else if((ptr->second.ot & AVAIL_OCGTCG) == AVAIL_TCG)
				wcscat(textBuffer, L"[TCG]");
			else if((ptr->second.ot & AVAIL_CUSTOM) == AVAIL_CUSTOM)
				wcscat(textBuffer, L"[Custom]");
			DrawShadowText(textFont, textBuffer, Resize(860, 209 + i * 66, 955, 229 + i * 66), Resize(1, 1, 0, 0));
		}
	}
	if(deckBuilder.is_draging) {
		DrawThumb(deckBuilder.draging_pointer, position2di(deckBuilder.dragx - CARD_THUMB_WIDTH / 2 * mainGame->xScale, deckBuilder.dragy - CARD_THUMB_HEIGHT / 2 * mainGame->yScale), deckBuilder.filterList, true);
	}
}
}
