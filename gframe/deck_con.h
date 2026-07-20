#ifndef DECK_CON_H
#define DECK_CON_H

#include <string>
#include <vector>
#include <random>
#include <irrlicht.h>

namespace ygo {

struct CardDataC;
struct LFList;
class Game;

class DeckBuilder: public irr::IEventReceiver {
public:
	DeckBuilder(Game* game);
	bool OnEvent(const irr::SEvent& event) override;
	void ButtonHandler(const irr::SEvent& event);
	void ComboBoxHandler(const irr::SEvent& event);
	void Initialize();
	void Terminate();
	void GetHoveredCard();
	void FilterCards();
	void StartFilter();
	void ClearFilter();
	void InstantSearch();
	void ClearSearch();
	void SortList();
	void RefreshCurrentPoint();

	void RefreshDeckList();
	void RefreshReadonly(int catesel);
	void RefreshPackListScroll();
	void ChangeCategory(const wchar_t* deck_name = nullptr);
	void ShowDeckManage();
	void ShowBigCard(int code, float zoom);
	void ZoomBigCard(float delta, irr::s32 centerx = -1, irr::s32 centery = -1);
	void CloseBigCard();
	void EnableEditWindow(bool enabled);
	void EnableManageWindow(bool enabled);

	unsigned long long filter_effect{};
	unsigned int filter_type{};
	unsigned int filter_type2{};
	unsigned int filter_attrib{};
	unsigned int filter_race{};
	unsigned int filter_atktype{};
	int filter_atk{};
	unsigned int filter_deftype{};
	int filter_def{};
	unsigned int filter_lvtype{};
	unsigned int filter_lv{};
	unsigned int filter_scltype{};
	unsigned int filter_scl{};
	unsigned int filter_marks{};
	int filter_lm{};
	irr::core::vector2di mouse_pos;
	int hovered_code{};
	int hovered_pos{};
	int hovered_seq{ -1 };
	int is_lastcard{};
	int click_pos{};
	bool is_draging{};
	bool is_starting_dragging{};
	int dragx{};
	int dragy{};
	int bigcard_code{};
	float bigcard_zoom{};
	size_t pre_mainc{};
	size_t pre_extrac{};
	size_t pre_sidec{};
	int current_point{};
	const CardDataC* draging_pointer{};
	irr::s32 prev_operation{};
	irr::s32 dmquery_operation{};
	bool is_modified{};
	bool readonly{};
	bool showing_pack{};
	std::mt19937 rnd;

	const LFList* filterList{};
	std::vector<const CardDataC*> results;
	wchar_t result_string[8]{};
	std::vector<std::wstring> expansionPacks;

private:
	bool push_main(const CardDataC* pointer, int seq = -1);
	bool push_extra(const CardDataC* pointer, int seq = -1);
	bool push_side(const CardDataC* pointer, int seq = -1);
	void pop_main(int seq);
	void pop_extra(int seq);
	void pop_side(int seq);
	bool check_limit(const CardDataC* pointer);

	Game* game_{ nullptr };
};

}

#endif //DECK_CON_H
