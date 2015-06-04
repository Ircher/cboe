
#include <stack>
#include <set>
#include <map>
#include <numeric>
#include <boost/lexical_cast.hpp>
#include "scen.global.h"
#include "scenario.h"
#include "graphtool.hpp"
#include "scen.graphics.h"
#include "scen.keydlgs.h"
#include "scen.core.h"
#include "dlogutil.hpp"
#include "restypes.hpp"
#include "spell.hpp"

extern short cen_x, cen_y;
extern bool mouse_button_held;
extern short cur_viewing_mode;
extern cTown* town;
extern short mode_count,to_create;
extern ter_num_t template_terrain[64][64];
extern cScenario scenario;
extern cOutdoors* current_terrain;
extern cCustomGraphics spec_scen_g;

std::vector<pic_num_t> field_pics = {0,3,5,6,7,8,9,10,11,12,13,14,15,24,25,26,27,28,29,30,31};
std::vector<pic_num_t> boom_pics = {0,1,2,3,4,8,16,24,32};
std::vector<pic_num_t> lgdlog_pics = {0,72};

size_t num_strs(short str_mode) {
	switch(str_mode) {
		case 0: return scenario.spec_strs.size();
		case 1: return current_terrain->spec_strs.size();
		case 2: return town->spec_strs.size();
		case 3: return scenario.journal_strs.size();
		case 4: return current_terrain->sign_locs.size();
		case 5: return town->sign_locs.size();
	}
	return 0;
}

//cre = check range error
bool cre(short val,short min,short max,std::string text1,std::string text2,cDialog* parent) {
	if((val < min) || (val > max)) {
		giveError(text1,text2,parent);
		return true;
	}
	return false;
}

short choose_background(short cur_choice, cDialog* parent) {
	cDialog bg_dlg("choose-bg",parent);
	auto get_selected = [&]() -> short {
		std::string sel = dynamic_cast<cLedGroup&>(bg_dlg["group"]).getSelected();
		if(sel.empty()) return -1;
		return boost::lexical_cast<int>(sel.substr(3)) - 1;
	};
	auto set_colour_for = [&](int which) {
		sf::Color to = sf::Color::Black;
		if(which == 2)
			to = sf::Color::Red;
		else if(which == 3)
			to = {0x00, 0x00, 0x80};
		else if(which == 11)
			to = {0xC0, 0x40, 0xFF};
		else if(which <= 5 || which == 7 || which == 9 || which == 15)
			to = sf::Color::White;
		bg_dlg["title"].setColour(to);
		for(int i = 0; i < 21; i++)
			bg_dlg["led" + std::to_string(i + 1)].setColour(to);
	};
	bg_dlg["done"].attachClickHandler(std::bind(&cDialog::toast, &bg_dlg, true));
	bg_dlg["cancel"].attachClickHandler(std::bind(&cDialog::toast, &bg_dlg, false));
	bg_dlg["group"].attachFocusHandler([&](cDialog&, std::string, bool) -> bool {
		int which = get_selected();
		bg_dlg.setBg(which);
		set_colour_for(which);
		return true;
	});
	int sel = cur_choice;
	if(sel < 0 || sel >= 21) sel = bg_dlg.getBg();
	dynamic_cast<cLedGroup&>(bg_dlg["group"]).setSelected("led" + std::to_string(sel + 1));
	bg_dlg.setBg(sel);
	set_colour_for(sel);
	bg_dlg.run();
	if(bg_dlg.accepted())
		return get_selected();
	return cur_choice;
}

// TODO: I have two functions that do this. (The other one is pick_picture.)
pic_num_t choose_graphic(short cur_choice,ePicType g_type,cDialog* parent) {
	int i = 0;
	std::vector<pic_num_t> all_pics;
	size_t total_pics = 0;
	cPictChoice* pic_dlg = nullptr;
	switch(g_type + PIC_PRESET) {
		case PIC_TER: total_pics = 859; break;
		case PIC_TER_ANIM: total_pics = 20; break;
		case PIC_DLOG: total_pics = 44; break;
		case PIC_TALK: total_pics = 84; break;
		case PIC_SCEN: total_pics = 30; break;
		case PIC_ITEM: case PIC_TINY_ITEM: total_pics = 123; break;
		case PIC_PC: total_pics = 36; break;
		case PIC_FIELD: all_pics = field_pics; break;
		case PIC_BOOM: all_pics = boom_pics; break;
		case PIC_DLOG_LG: all_pics = lgdlog_pics; break;
		case PIC_MISSILE: total_pics = 16; break;
		case PIC_STATUS: total_pics = 27; break;
		case PIC_SCEN_LG: total_pics = 4; break;
		case PIC_TER_MAP: total_pics = 980; break;
		case PIC_FULL: return NO_PIC; // TODO: Is it even possible to do a pict selection dialog for this?
		default:
			std::cerr << "Picture type " << (g_type + PIC_PRESET) << " is missing case in choose_graphic." << std::endl;
			return NO_PIC;
		case PIC_MONST: case PIC_MONST_WIDE:
		case PIC_MONST_TALL: case PIC_MONST_LG:
			std::vector<std::pair<pic_num_t,ePicType>> pics;
			for(m_pic_index_t m_pic : m_pic_index) {
				// TODO: Put the added monster graphics in m_pic_index to allow picking them
				ePicType type = PIC_MONST;
				if(m_pic.x == 2) type += PIC_WIDE;
				if(m_pic.y == 2) type += PIC_TALL;
				pics.push_back({i++, type});
			}
			for(size_t i = 0; i < scenario.custom_graphics.size(); i++) {
				if(scenario.custom_graphics[i] == PIC_MONST)
					pics.push_back({1000 + i, PIC_CUSTOM_MONST});
				else if(scenario.custom_graphics[i] == PIC_MONST_WIDE)
					pics.push_back({2000 + i, PIC_CUSTOM_MONST_WIDE});
				else if(scenario.custom_graphics[i] == PIC_MONST_TALL)
					pics.push_back({3000 + i, PIC_CUSTOM_MONST_TALL});
				else if(scenario.custom_graphics[i] == PIC_MONST_LG)
					pics.push_back({4000 + i, PIC_CUSTOM_MONST_LG});
				if(cur_choice == pics.back().first)
					cur_choice = pics.size() - 1;
			}
			pic_dlg = new cPictChoice(pics, parent);
			break;
	}
	if(!pic_dlg) {
		if(all_pics.size());
		else if(total_pics > 0) {
			all_pics.resize(total_pics);
			std::iota(all_pics.begin(), all_pics.end(), 0);
		} else return cur_choice;
		// Now add the custom pics
		for(size_t i = 0; i < scenario.custom_graphics.size(); i++) {
			if(scenario.custom_graphics[i] == g_type) {
				if(g_type == PIC_TER_MAP) {
					for(int j = 1; j <= 6; j++)
						all_pics.push_back(j * 1000 + i);
				} else if(g_type == PIC_TER_ANIM)
					all_pics.push_back(2000 + i);
				else all_pics.push_back(1000 + i);
			}
		}
		if(cur_choice >= 1000) {
			auto selected = std::find(all_pics.begin(), all_pics.end(), cur_choice);
			if(selected == all_pics.end()) cur_choice = -1;
			else cur_choice = selected - all_pics.begin();
		}
		pic_dlg = new cPictChoice(all_pics, g_type, parent);
	}
	bool made_choice = pic_dlg->show(cur_choice);
	pic_num_t item_hit = pic_dlg->getPicChosen();
	delete pic_dlg;
	return made_choice ? item_hit : NO_PIC;
}

short choose_text_res(std::string res_list,short first_t,short last_t,unsigned short cur_choice,cDialog* parent,const char *title) {
	location view_loc;
	if((cur_choice < first_t) || (cur_choice > last_t))
		cur_choice = -1;
	else cur_choice -= first_t;
	
	StringRsrc strings = *ResMgr::get<StringRsrc>(res_list);
	cStringChoice dlog(strings.begin() + first_t - 1, strings.begin() + last_t, title, parent);
	
	return dlog.show(cur_choice);
}

short choose_text(eStrType list, unsigned short cur_choice, cDialog* parent, std::string title) {
	location view_loc;
	
	std::vector<std::string> strings;
	switch(list) {
		case STRT_MONST:
			for(cMonster& monst : scenario.scen_monsters) {
				strings.push_back(monst.m_name);
			}
			strings.erase(strings.begin()); // First monster is not a valid monster
			break;
		case STRT_ITEM:
			for(cItem& item : scenario.scen_items) {
				strings.push_back(item.full_name);
			}
			break;
		case STRT_SPEC_ITEM:
			for(cSpecItem& item : scenario.special_items) {
				strings.push_back(item.name);
			}
			break;
		case STRT_QUEST:
			for(cQuest& quest : scenario.quests) {
				strings.push_back(quest.name);
			}
			break;
		case STRT_TER:
			for(cTerrain& ter : scenario.ter_types) {
				strings.push_back(ter.name);
			}
			break;
		case STRT_TOWN:
			for(cTown* town : scenario.towns) {
				strings.push_back(town->town_name);
			}
			break;
		case STRT_SECTOR:
			for(size_t i = 0; i < scenario.outdoors.width(); i++) {
				for(size_t j = 0; j < scenario.outdoors.height(); j++) {
					std::ostringstream name;
					name << '[' << i << ',' << j << ']';
					name << ' ' << scenario.outdoors[i][j]->out_name;
					strings.push_back(name.str());
				}
			}
			break;
		case STRT_MAGE:
			for(int i = 0; i < 62; i++) {
				strings.push_back((*cSpell::fromNum(eSkill::MAGE_SPELLS, i)).name());
			}
			break;
		case STRT_PRIEST:
			for(int i = 0; i < 62; i++) {
				strings.push_back((*cSpell::fromNum(eSkill::PRIEST_SPELLS, i)).name());
			}
			break;
		case STRT_ALCHEMY:
			for(int i = 0; i < 20; i++) {
				strings.push_back(get_str("magic-names", i + 200));
			}
			break;
		case STRT_SKILL:
			for(int i = 0; i < 19; i++) {
				strings.push_back(get_str("skills", i * 2 + 1));
			}
			strings.push_back("Maximum Health");
			strings.push_back("Maximum Spell Points");
			break;
		case STRT_TRAIT:
			for(int i = 0; i < 17; i++) {
				strings.push_back(get_str("traits", i * 2 + 1));
			}
			break;
		case STRT_RACE:
			for(int i = 0; i < 22; i++) {
				strings.push_back(get_str("traits", i * 2 + 35));
			}
			break;
		case STRT_PICT:
			strings = *ResMgr::get<StringRsrc>("picture-types");
			break;
		case STRT_TRAP:
			strings = *ResMgr::get<StringRsrc>("trap-types");
			break;
		case STRT_HEALING:
			strings = *ResMgr::get<StringRsrc>("shop-specials");
			break;
		case STRT_BUTTON:
			for(int btn : available_btns) {
				strings.push_back(basic_buttons[btn].label);
			}
			break;
		case STRT_CMP:
			strings = {"Less Than or Equal", "Less Than", "Equal", "Greater Than", "Grater Than or Equal"};
			break;
		case STRT_ACCUM:
			strings = {"Current PC Only", "Cumulative", "Average", "Minimum", "Maximum"};
			break;
		case STRT_ATTITUDE:
			strings = {"Docile", "Hostile Type A", "Friendly", "Hostile Type B"};
			break;
		case STRT_STAIR:
			strings = {"Stairs Up", "Stairs Down", "Slope Up", "Slope Down", "Slimy Stairs Up", "Slimy Stairs Down", "Dark Slope Up", "Dark Slope Down"};
			break;
		case STRT_STAIR_MODE:
			strings = {"Normal behaviour", "Allow triggering in combat", "Allow triggering when not moving (eg when looking at the space)", "Both of the above"};
			break;
		case STRT_LIGHT:
			strings = {"Fully Lit", "Dark", "Very Dark", "Totally Dark"};
			break;
		case STRT_TALK_NODE:
			for(int i = 0; i < 31; i++) {
				strings.push_back(get_str("talk-node-descs", i * 7 + 1));
			}
			break;
		case STRT_CONTEXT:
			strings = *ResMgr::get<StringRsrc>("special-contexts");
			break;
		case STRT_SHOP:
			for(cShop& shop : scenario.shops) {
				strings.push_back(shop.getName());
			}
			break;
		case STRT_COST_ADJ:
			strings = {"Extremely Cheap", "Very Reasonable", "Pretty Average", "Somewhat Pricey", "Expensive", "Exorbitant", "Utterly Ridiculous"};
			break;
		case STRT_STATUS:
			strings = {"Alive", "Dead", "Dust", "Petrified", "Fled Outdoor Combat", "Absent", "Deleted"};
			break;
		case STRT_SPELL_PAT:
			strings = {"Single Space", "3x3 Square", "2x2 Square", "3x3 Open Square", "Radius 2 Circle", "Radius 3 Circle", "Cross", "Rotateable 2x8 Wall"};
			break;
		case STRT_SUMMON:
			strings = {"0 - no summon (weak)", "1 - weak summoning", "2 - summoning", "3 - major summoning", "4 - no summon (unique/powerful"};
			break;
		case STRT_TALK:
			for(cTown* town : scenario.towns) {
				for(cPersonality who : town->talking.people) {
					strings.push_back(who.title + " (in " + town->town_name + ")");
				}
			}
			break;
		case STRT_ENCHANT:
			strings = {"+1", "+2", "+3", "Shoot Flames", "Flaming", "+5", "Blessed"};
			break;
		case STRT_DIR:
			strings = {"North", "Northease", "East", "Southeast", "South", "Southwest", "West", "Northwest", "None"};
			break;
		case STRT_QUEST_STATUS:
			strings = {"Available", "Started", "Completed", "Failed"};
			break;
		case STRT_TREASURE:
			strings = {"0 - Junk", "1 - Lousy", "2 - So-so", "3 - Good", "4 - Great"};
			break;
		case STRT_MONST_STAT:
			strings = {
				"Maximum Health", "Maximum Magic Points", "Armor", "Skill", "Speed", "Mage Spells", "Priest Spells",
				"Magic Resistance", "Fire Resistance", "Cold Resistance", "Poison Resistance",
			};
			break;
		case STRT_POS_MODE:
			strings = {"Absolute Position", "Move Southeast", "Move Southwest", "Move Northwest", "Move Northeast"};
			break;
	}
	if(cur_choice < 0 || cur_choice >= strings.size())
		cur_choice = -1;
	cStringChoice dlog(strings, title, parent);
	
	return dlog.show(cur_choice);
}

static bool edit_text_event_filter(cDialog& me, std::string item_hit, short& which_str, short str_mode) {
	std::string newVal = me["text"].getText();
	if(str_mode == 0)
		scenario.spec_strs[which_str] = newVal;
	if(str_mode == 1)
		current_terrain->spec_strs[which_str] = newVal;
	if(str_mode == 2)
		town->spec_strs[which_str] = newVal;
	if(str_mode == 3)
		scenario.journal_strs[which_str] = newVal;
	if(str_mode == 4)
		current_terrain->sign_locs[which_str].text = newVal;
	if(str_mode == 5)
		town->sign_locs[which_str].text = newVal;
	if(item_hit == "okay") me.toast(true);
	else if(item_hit == "left" || item_hit == "right") {
		if(item_hit[0] == 'l')
			which_str--;
		else which_str++;
		if(which_str < 0) which_str = num_strs(str_mode) - 1;
		if(which_str >= num_strs(str_mode)) which_str = 0;
	}
	me["num"].setTextToNum(which_str);
	if(str_mode == 0)
		me["text"].setText(scenario.spec_strs[which_str]);
	if(str_mode == 1)
		me["text"].setText(current_terrain->spec_strs[which_str]);
	if(str_mode == 2)
		me["text"].setText(town->spec_strs[which_str]);
	if(str_mode == 3)
		me["text"].setText(scenario.journal_strs[which_str]);
	if(str_mode == 4)
		me["text"].setText(current_terrain->sign_locs[which_str].text);
	if(str_mode == 5)
		me["text"].setText(town->sign_locs[which_str].text);
	return true;
}

// mode 0 - scen 1 - out 2 - town
void edit_text_str(short which_str,short mode) {
	using namespace std::placeholders;
	
	cDialog dlog("edit-text");
	dlog.attachClickHandlers(std::bind(edit_text_event_filter, _1, _2, std::ref(which_str), mode), {"okay", "left", "right"});
	
	dlog["num"].setTextToNum(which_str);
	if(mode == 0)
		dlog["text"].setText(scenario.spec_strs[which_str]);
	if(mode == 1)
		dlog["text"].setText(current_terrain->spec_strs[which_str]);
	if(mode == 2)
		dlog["text"].setText(town->spec_strs[which_str]);
	if(mode == 3)
		dlog["text"].setText(scenario.journal_strs[which_str]);
	if(mode == 4)
		dlog["text"].setText(current_terrain->sign_locs[which_str].text);
	if(mode == 5)
		dlog["text"].setText(town->sign_locs[which_str].text);
	
	dlog.run();
}

static bool edit_area_rect_event_filter(cDialog& me, std::string item_hit, short which_str, short str_mode) {
	if(item_hit == "okay") {
		me.setResult(true);
		me.toast(true);
		std::string str = me["area"].getText().substr(0,29);
		if(str_mode == 0)
			current_terrain->info_rect[which_str].descr = str;
		else town->room_rect[which_str].descr = str;
	} else if(item_hit == "cancel") {
		me.setResult(false);
		me.toast(false);
	}
	return true;
}

// mode 0 - out 1 - town
bool edit_area_rect_str(short which_str,short mode) {
	using namespace std::placeholders;
	
	cDialog dlog("set-area-desc");
	dlog.attachClickHandlers(std::bind(edit_area_rect_event_filter, _1, _2, which_str, mode), {"okay", "cancel"});
	
	if(mode == 0)
		dlog["area"].setText(current_terrain->info_rect[which_str].descr);
	else dlog["area"].setText(town->room_rect[which_str].descr);
	
	dlog.run();
	
	return dlog.getResult<bool>();
}

// MARK: Special node dialog

struct editing_node_t {
	short which, mode;
	cSpecial node;
};

typedef std::stack<editing_node_t> node_stack_t;

static void setup_node_field(cDialog& me, std::string field, short value, std::string label, char buttonType) {
	me[field + "-lbl"].setText(label);
	me[field].setTextToNum(value);
	std::string button = field + "-edit";
	try { // Just make sure the button exists before fiddling with it.
		me.getControl(button);
	} catch(std::invalid_argument) {
		return;
	}
	switch(buttonType) {
		case ' ':
			me[button].hide();
			break;
		case 'm': case 'M': case'$': case 'd': // messages
		case 's': case 'S': // specials
			me[button].show();
			me[button].setText("Create/Edit");
			dynamic_cast<cButton&>(me[button]).setBtnType(BTN_LG);
			break;
		default:
			me[button].show();
			me[button].setText("Choose");
			dynamic_cast<cButton&>(me[button]).setBtnType(BTN_REG);
			break;
	}
}

static void put_spec_enc_in_dlog(cDialog& me, node_stack_t& edit_stack) {
	// These are the node types that should not have a Create/Edit button on the JumpTo
	static std::set<eSpecType> dead_ends = {eSpecType::ENTER_SHOP, eSpecType::START_TALK, eSpecType::TOWN_GENERIC_STAIR, eSpecType::TOWN_STAIR};
	cSpecial& spec = edit_stack.top().node;
	
	// Show which node is being edited and what type of node it is
	switch(edit_stack.top().mode) {
		case 0: me["title"].setText("Edit Scenario Special Node"); break;
		case 1: me["title"].setText("Edit Outdoors Special Node"); break;
		case 2: me["title"].setText("Edit Town Special Node"); break;
	}
	me["num"].setTextToNum(edit_stack.top().which);
	
	// Show the node opcode information
	me["type"].setText((*spec.type).name());
	
	node_properties_t info = *spec.type;
	// Set up the labels, fields, and buttons
	setup_node_field(me, "sdf1", spec.sd1, info.sdf1_lbl(), ' ');
	setup_node_field(me, "sdf2", spec.sd2, info.sdf2_lbl(), ' ');
	
	setup_node_field(me, "msg1", spec.m1, info.msg1_lbl(), info.m1_btn);
	setup_node_field(me, "msg2", spec.m2, info.msg2_lbl(), info.m2_btn);
	setup_node_field(me, "msg3", spec.m3, info.msg3_lbl(), info.m3_btn);
	
	setup_node_field(me, "pict", spec.pic, info.pic_lbl(), info.p_btn);
	setup_node_field(me, "pictype", spec.pictype, info.pt_lbl(), info.pt_btn);
	
	setup_node_field(me, "x1a", spec.ex1a, info.ex1a_lbl(), info.x1a_btn);
	setup_node_field(me, "x1b", spec.ex1b, info.ex1b_lbl(), info.x1b_btn);
	setup_node_field(me, "x1c", spec.ex1c, info.ex1c_lbl(), info.x1c_btn);
	
	setup_node_field(me, "x2a", spec.ex2a, info.ex2a_lbl(), info.x2a_btn);
	setup_node_field(me, "x2b", spec.ex2b, info.ex2b_lbl(), info.x2b_btn);
	setup_node_field(me, "x2c", spec.ex2c, info.ex2c_lbl(), info.x2c_btn);
	
	setup_node_field(me, "jump", spec.jumpto, info.jmp_lbl(), dead_ends.count(spec.type) ? ' ' : 's');
}

static void save_spec_enc(cDialog& me, node_stack_t& edit_stack) {
	cSpecial& the_node = edit_stack.top().node;
	the_node.sd1 = me["sdf1"].getTextAsNum();
	the_node.sd2 = me["sdf2"].getTextAsNum();
	the_node.m1 = me["msg1"].getTextAsNum();
	the_node.m2 = me["msg2"].getTextAsNum();
	the_node.m3 = me["msg3"].getTextAsNum();
	the_node.pic = me["pict"].getTextAsNum();
	the_node.pictype = me["pictype"].getTextAsNum();
	the_node.ex1a = me["x1a"].getTextAsNum();
	the_node.ex1b = me["x1b"].getTextAsNum();
	the_node.ex1c = me["x1c"].getTextAsNum();
	the_node.ex2a = me["x2a"].getTextAsNum();
	the_node.ex2b = me["x2b"].getTextAsNum();
	the_node.ex2c = me["x2c"].getTextAsNum();
	the_node.jumpto = me["jump"].getTextAsNum();
}

static bool commit_spec_enc(cDialog& me, std::string item_hit, node_stack_t& edit_stack) {
	if(item_hit != "unwind")
		save_spec_enc(me, edit_stack);
	int mode = edit_stack.top().mode, node = edit_stack.top().which;
	switch(mode) {
		case 0: scenario.scen_specials[node] = edit_stack.top().node; break;
		case 1: current_terrain->specials[node] = edit_stack.top().node; break;
		case 2: town->specials[node] = edit_stack.top().node; break;
	}
	edit_stack.pop();
	if(item_hit == "okay") {
		while(!edit_stack.empty())
			commit_spec_enc(me, "unwind", edit_stack);
		me.toast(true);
		me.setResult(true);
	} else if(item_hit == "back") {
		put_spec_enc_in_dlog(me, edit_stack);
		if(edit_stack.size() == 1)
			me["back"].hide();
	}
	return true;
}

static bool discard_spec_enc(cDialog& me, node_stack_t& edit_stack) {
	if(edit_stack.size() > 1) {
		std::string action = cChoiceDlog("discard-special-node", {"save", "cancel", "revert"}).show();
		if(action == "save") return true;
		if(action == "cancel") return me.toast(false);
		edit_stack.pop();
		if(edit_stack.size() == 1)
			me["back"].hide();
	} else me.toast(false);
	return true;
}

static bool edit_spec_enc_type(cDialog& me, std::string item_hit, node_stack_t& edit_stack) {
	save_spec_enc(me, edit_stack);
	eSpecCat category = eSpecCat::INVALID;
	if(item_hit == "general") category = eSpecCat::GENERAL;
	else if(item_hit == "oneshot") category = eSpecCat::ONCE;
	else if(item_hit == "affectpc") category = eSpecCat::AFFECT;
	else if(item_hit == "ifthen") category = eSpecCat::IF_THEN;
	else if(item_hit == "town") category = eSpecCat::TOWN;
	else if(item_hit == "out") category = eSpecCat::OUTDOOR;
	else if(item_hit == "rect") category = eSpecCat::RECT;
	int start = -1, finish = -1, current = int(edit_stack.top().node.type);
	for(int i = 0; i < std::numeric_limits<unsigned short>::max(); i++) {
		eSpecType check = eSpecType(i);
		eSpecCat checkCat = getNodeCategory(check);
		if(start >= 0 && checkCat == eSpecCat::INVALID) {
			finish = i - 1;
			break;
		} else if(checkCat == category && start < 0)
			start = i;
	}
	if(start < 0 || finish < 0) return true;
	std::vector<std::string> choices;
	for(int i = start; i <= finish; i++) {
		eSpecType node = eSpecType(i);
		std::string name = (*node).name();
		if(name.empty()) choices.push_back("Unused Node");
		else choices.push_back(name);
	}
	size_t cancelled = -1, result;
	cStringChoice choose(choices, "Select a special node type:",&me);
	result = choose.show((current < start || current > finish) ? cancelled : current - start);
	if(result != cancelled) {
		edit_stack.top().node.type = eSpecType(result + start);
		put_spec_enc_in_dlog(me, edit_stack);
	}
	return true;
}

short choose_field_type(short cur, cDialog* parent, bool includeSpec) {
	static const char*const fieldNames[] = {
		"Wall of Force", "Wall of Fire", "Antimagic Field", "Stinking Cloud", "Wall of Ice", "Wall of Blades", "Sleep Cloud",
		"Stone Block", "Webs", "Crate", "Barrel", "Fire Barrier", "Force Barrier", "Quickfire",
		"Small Blood", "Medium Blood", "Large Blood", "Small Slime", "Large Slime", "Ash", "Bones", "Rubble",
		"Force Cage", "Cleanse Space", "Crumble Walls",
	};
	std::vector<std::pair<pic_num_t,ePicType>> pics = {
		{8, PIC_FIELD}, {9, PIC_FIELD}, {10, PIC_FIELD}, {11, PIC_FIELD},
		{12, PIC_FIELD}, {13, PIC_FIELD}, {14, PIC_FIELD},
		{3, PIC_FIELD}, {5, PIC_FIELD}, {6, PIC_FIELD}, {7, PIC_FIELD},
		{14, PIC_TER_ANIM}, {14, PIC_TER_ANIM}, {15, PIC_FIELD},
		{24, PIC_FIELD}, {25, PIC_FIELD}, {26, PIC_FIELD}, {27, PIC_FIELD},
		{28, PIC_FIELD}, {29, PIC_FIELD}, {30, PIC_FIELD}, {31, PIC_FIELD},
		{0, PIC_FIELD},
	};
	if(includeSpec) {
		pics.push_back({23, PIC_FIELD});
		pics.push_back({95, PIC_TER});
	}
	short prev = cur;
	cur--;
	if(cur >= SPECIAL_SPOT) cur--;
	if(cur >= 25) cur -= 7;
	if(cur < 0 || cur >= pics.size()) cur = 0;
	cPictChoice pic_dlg(pics, parent);
	pic_dlg->getControl("prompt").setText("Select a field type:");
	pic_dlg->getControl("help").setText(fieldNames[cur]);
	pic_dlg.attachSelectHandler([](cPictChoice& me, int n) {
		me->getControl("help").setText(fieldNames[n]);
	});
	bool made_choice = pic_dlg.show(cur);
	size_t item_hit = pic_dlg.getSelected();
	item_hit++;
	if(item_hit >= SPECIAL_SPOT)
		item_hit++;
	if(item_hit >= 25)
		item_hit += 7;
	return made_choice ? item_hit : prev;
}

pic_num_t choose_damage_type(short cur, cDialog* parent) {
	static const char*const damageNames[] = {"Weapon", "Fire", "Poison", "Magic", "Unblockable", "Cold", "Undead", "Demon", "Special"};
	static const std::vector<pic_num_t> pics = {3,0,2,1,1,4,3,3,1};
	short prev = cur;
	if(cur < 0 || cur >= pics.size()) cur = 0;
	cPictChoice pic_dlg(pics, PIC_BOOM, parent);
	pic_dlg->getControl("prompt").setText("Select a damage type:");
	pic_dlg->getControl("help").setText(damageNames[cur]);
	pic_dlg.attachSelectHandler([](cPictChoice& me, int n) {
		me->getControl("help").setText(damageNames[n]);
	});
	bool made_choice = pic_dlg.show(cur);
	size_t item_hit = pic_dlg.getSelected();
	return made_choice ? item_hit : prev;
}

static pic_num_t choose_boom_type(short cur, cDialog* parent) {
	static const char*const boomNames[] = {"Fire", "Teleport", "Magic/Cold/Electricity", "Magic/Electricity", "Custom Explosion"};
	static const int preset_booms = 4;
	std::vector<pic_num_t> pics;
	for(int i = 0; i < preset_booms; i++) pics.push_back(i + 8);
	for(int i = 0; i < scenario.custom_graphics.size(); i++) {
		if(scenario.custom_graphics[i] == PIC_BOOM)
			pics.push_back(1000 + i);
	}
	short prev = cur;
	if(cur < 0 || (cur >= preset_booms && cur < 1000) || (cur >= 1000 && cur - 1000 < pics.size() - preset_booms))
	   cur = 0;
	cPictChoice pic_dlg(pics, PIC_BOOM, parent);
	pic_dlg->getControl("prompt").setText("Select a boom type:");
	pic_dlg->getControl("help").setText(boomNames[std::min<short>(cur,4)]);
	pic_dlg.attachSelectHandler([](cPictChoice& me, int n) {
		me->getControl("help").setText(boomNames[std::min<short>(n,4)]);
	});
	bool made_choice = pic_dlg.show(cur);
	size_t item_hit = pic_dlg.getPicChosen();
	return made_choice ? item_hit : prev;
}

pic_num_t choose_status_effect(short cur, bool party, cDialog* parent) {
	static const char*const status[] = {
		"Poisoned Weapon", "Bless/Curse", "Poison", "Haste/Slow", "Invulnerable",
		"Magic Resist/Amplify", "Webs", "Disease", "Sanctuary", "Dumbfounding/Enlightening",
		"Martyr's Shield", "Sleep/Hyperactivity", "Paralysis", "Acid"
	};
	static const char*const pstatus[] = {"Stealth", "Flight", "Detect Life", "Firewalk"};
	static const std::vector<pic_num_t> status_pics = {4,2,0,6,5,9,10,11,12,13,14,15,16,17};
	static const std::vector<pic_num_t> pstatus_pics = {25,23,24,26};
	short prev = cur;
	if(cur < 0 || cur >= (party ? pstatus_pics : status_pics).size()) cur = 0;
	cPictChoice pic_dlg(party ? pstatus_pics : status_pics, PIC_STATUS, parent);
	pic_dlg->getControl("prompt").setText("Select a status effect:");
	pic_dlg->getControl("help").setText((party ? pstatus : status)[cur]);
	pic_dlg.attachSelectHandler([party](cPictChoice& me, int n) {
		me->getControl("help").setText((party ? pstatus : status)[n]);
	});
	bool made_choice = pic_dlg.show(cur);
	size_t item_hit = pic_dlg.getSelected();
	return made_choice ? item_hit : prev;
}

snd_num_t choose_sound(short cur, cDialog* parent, std::string title) {
	// TODO: Somehow find a way to include custom sounds in this list
	if(cur < 0) cur = 0;
	cStringChoice snd_dlg(*ResMgr::get<StringRsrc>("sound-names"), title, parent);
	snd_dlg.attachSelectHandler([](cStringChoice&, int n) {
		play_sound(-n);
	});
	return snd_dlg.show(cur);
}

static bool edit_spec_enc_value(cDialog& me, std::string item_hit, node_stack_t& edit_stack) {
	static const ePicType pics[10] = {
		PIC_TER, PIC_MONST, PIC_DLOG, PIC_TALK, PIC_ITEM,
		PIC_PC, PIC_FIELD, PIC_BOOM, PIC_MISSILE, PIC_STATUS
	};
	std::string field = item_hit.substr(0, item_hit.find_first_of('-'));
	char btn;
	eSpecType type = edit_stack.top().node.type;
	if(field.substr(0,3) == "sdf") return true;
	else if(field == "msg1") btn = (*type).m1_btn;
	else if(field == "msg2") btn = (*type).m2_btn;
	else if(field == "msg3") btn = (*type).m3_btn;
	else if(field == "pict") btn = (*type).p_btn;
	else if(field == "pictype") btn = (*type).pt_btn;
	else if(field == "x1a") btn = (*type).x1a_btn;
	else if(field == "x1b") btn = (*type).x1b_btn;
	else if(field == "x1c") btn = (*type).x1c_btn;
	else if(field == "x2a") btn = (*type).x2a_btn;
	else if(field == "x2b") btn = (*type).x2b_btn;
	else if(field == "x2c") btn = (*type).x2c_btn;
	else if(field == "jump") {
		if(type == eSpecType::CALL_GLOBAL) btn = 'S';
		else btn = 's';
	}
	short val = me[field].getTextAsNum(), mode = (btn == 'S' || btn == '$') ? 0 : edit_stack.top().mode, store;
	short pictype = me["pictype"].getTextAsNum();
	bool choose_string = true;
	eStrType strt;
	short str_adj = 0;
	const char* title;
	cSpecial* node_to_change_to;
	switch(btn) {
		case 'm':
			choose_string = false;
			store = me["msg2"].getTextAsNum();
			edit_spec_text(mode, &val, &store, &me);
			me["msg2"].setTextToNum(store);
			store = val;
			break;
		case 'M': case '$':
			choose_string = false;
			edit_spec_text(mode, &val, nullptr, &me);
			store = val;
			break;
		case 'd':
			choose_string = false;
			store = val;
			edit_dialog_text(mode, &store, &me);
			break;
		case 's': case 'S':
			choose_string = false;
			store = val < 0 ? get_fresh_spec(mode) : val;
			me[field].setTextToNum(store);
			save_spec_enc(me, edit_stack);
			if(mode == 0)
				node_to_change_to = &scenario.scen_specials[store];
			else if(mode == 1)
				node_to_change_to = &current_terrain->specials[store];
			else if(mode == 2)
				node_to_change_to = &town->specials[store];
			if(node_to_change_to->pic < 0)
				node_to_change_to->pic = 0;
			edit_stack.push({store,mode,*node_to_change_to});
			put_spec_enc_in_dlog(me, edit_stack);
			me["back"].show();
			return true;
		case 'p':
			choose_string = false;
			if(pictype < 0) me["pictype"].setTextToNum(PIC_DLOG);
			store = choose_graphic(val, pictype < 0 ? PIC_DLOG : ePicType(pictype), &me);
			if(store < 0) store = val;
			break;
		case 'f': case 'F': choose_string = false; store = choose_field_type(val, &me, btn == 'F'); break;
		case 'D': choose_string = false; store = choose_damage_type(val, &me); break;
		case '!': choose_string = false; store = choose_boom_type(val, &me); break;
		case 'e': choose_string = false; store = choose_status_effect(val, false, &me); break;
		case 'E': choose_string = false; store = choose_status_effect(val, true, &me); break;
		case '{': case '}': strt = STRT_SPELL_PAT; title = "Which spell pattern?"; break;
		case 'i': strt = STRT_ITEM; title = "Which item?"; break;
		case 'I': strt = STRT_SPEC_ITEM; title = "Which special item?"; break;
		case 't': strt = STRT_TER; title = "Which terrain?"; break;
		case 'c': strt = STRT_MONST; title = "Which monster?"; str_adj = -1; break;
		case 'C': strt = STRT_MONST_STAT; title = "Which statistic?"; break;
		case 'a': strt = STRT_ALCHEMY; title = "Which recipe?"; break;
		case 'A': strt = STRT_MAGE; title = "Which spell?"; break;
		case 'P': strt = STRT_PRIEST; title = "Which spell?"; break;
		case 'k': case 'K': strt = STRT_SKILL; title = "Which statistic?"; break;
		case 'q': strt = STRT_TRAIT; title = "Which trait?"; break;
		case 'Q': strt = STRT_RACE; title = "Which species?"; break;
		case 'T': strt = STRT_TOWN; title = "Which town?"; break;
		case 'b': strt = STRT_BUTTON; title = "Which button?"; break;
		case '?': strt = STRT_PICT; title = "Which picture type?"; str_adj = -1; break;
		case 'x': choose_string = false; store = choose_sound(val, &me); break;
		case 'X': strt = STRT_TRAP; title = "What trap type?"; break;
		case '=': strt = STRT_CMP; title = "What comparison method?"; str_adj = 2; break;
		case '+': strt = STRT_ACCUM; title = "What accumulation method?"; str_adj = 1; break;
		case '@': strt = STRT_ATTITUDE; title = "What attitude?"; break;
		case '/': strt = STRT_STAIR; title = "Which stairway text?"; break;
		case 'L': strt = STRT_LIGHT; title = "What lighting type?"; break;
		case '&': strt = STRT_SHOP; title = "Which shop?"; break;
		case '%': strt = STRT_COST_ADJ; title = "What cost adjust?"; break;
		case '*': strt = STRT_CONTEXT; title = "What context?"; break;
		case '^': strt = STRT_POS_MODE; title = "Select positioning mode:"; break;
		case ':': strt = STRT_STAIR_MODE; title = "Select trigger limitations:"; break;
		case 'w': strt = STRT_STATUS; title = "Select status:"; str_adj = 1; break;
		case 'j': strt = STRT_QUEST; title = "Select a quest:"; break;
		case 'J': strt = STRT_QUEST_STATUS; title = "Select the quest's status:"; break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			choose_string = false;
			store = choose_graphic(val, pics[btn - '0'], &me);
			if(store < 0) store = val;
			break;
		default:
			choose_string = false;
			store = val;
			giveError("Whoops! Unknown edit type! (" + (btn + std::string(")")), "", &me);
			break;
	}
	if(choose_string)
		store = choose_text(strt, val + str_adj, &me, title) - str_adj;
	me[field].setTextToNum(store);
	return true;
}

// mode - 0 scen 1 - out 2 - town
bool edit_spec_enc(short which_node,short mode,cDialog* parent) {
	using namespace std::placeholders;
	cSpecial the_node;
	node_stack_t edit_stack;
	
	if(mode == 0) {
		if(which_node >= scenario.scen_specials.size()) {
			giveError("That special node does not exist. You can create a new node by setting the field to -1 and trying again.", parent);
			return false;
		}
		the_node = scenario.scen_specials[which_node];
	} else if(mode == 1) {
		if(which_node >= current_terrain->specials.size()) {
			giveError("That special node does not exist. You can create a new node by setting the field to -1 and trying again.", parent);
			return false;
		}
		the_node = current_terrain->specials[which_node];
	} else if(mode == 2) {
		if(which_node >= town->specials.size()) {
			giveError("That special node does not exist. You can create a new node by setting the field to -1 and trying again.", parent);
			return false;
		}
		the_node = town->specials[which_node];
	}

	cDialog special("edit-special-node",parent);
	special.attachClickHandlers(std::bind(commit_spec_enc, _1, _2, std::ref(edit_stack)), {"okay", "back"});
	special.attachClickHandlers(std::bind(edit_spec_enc_type, _1, _2, std::ref(edit_stack)), {
		"general", "oneshot", "affectpc", "ifthen", "town", "out", "rect"
	});
	special.attachClickHandlers(std::bind(edit_spec_enc_value, _1, _2, std::ref(edit_stack)), {
		"msg1-edit", "msg2-edit", "msg3-edit", "pict-edit", "pictype-edit", "jump-edit",
		"x1a-edit", "x1b-edit", "x1c-edit", "x2a-edit", "x2b-edit", "x2c-edit",
	});
	special["cancel"].attachClickHandler(std::bind(discard_spec_enc, _1, std::ref(edit_stack)));
	
	special["back"].hide();
	edit_stack.push({which_node,mode,the_node});
	put_spec_enc_in_dlog(special, edit_stack);
	
	special.setResult(false);
	special.run();
	return special.getResult<bool>();
}

short get_fresh_spec(short which_mode) {
	cSpecial store_node;
	size_t num_specs;
	switch(which_mode) {
		case 0: num_specs = scenario.scen_specials.size(); break;
		case 1: num_specs = current_terrain->specials.size(); break;
		case 2: num_specs = town->specials.size(); break;
	}
	
	for(size_t i = 0; i < num_specs; i++) {
		if(which_mode == 0)
			store_node = scenario.scen_specials[i];
		if(which_mode == 1)
			store_node = current_terrain->specials[i];
		if(which_mode == 2)
			store_node = town->specials[i];
		if(store_node.type == eSpecType::NONE && store_node.jumpto == -1 && store_node.pic < 0)
			return i;
	}
	
	switch(which_mode) {
		case 0: scenario.scen_specials.emplace_back(); break;
		case 1: current_terrain->specials.emplace_back(); break;
		case 2: town->specials.emplace_back(); break;
	}
	
	return num_specs;
}

static bool edit_spec_text_event_filter(cDialog& me, std::string item_hit, short str_mode, short* str1, short* str2) {
	std::string str;
	size_t i;
	
	if(item_hit == "okay") {
		str = me["str1"].getText();
		if(!str.empty()) {
			if(*str1 < 0) {
				size_t n = num_strs(str_mode);
				for(i = 0; i < n; i++)
					switch(str_mode) {
						case 0:
							// TODO: This could overwrite a string if it's unlucky enough to start with *
							if(scenario.spec_strs[i][0] == '*') {
								*str1 = i;
								i = 500;
							}
							break;
						case 1:
							if(current_terrain->spec_strs[i][0] == '*') {
								*str1 = i;
								i = 500;
							}
							break;
						case 2:
							if(town->spec_strs[i][0] == '*') {
								*str1 = i;
								i = 500;
							}
							break;
						case 3:
							if(scenario.journal_strs[i][0] == '*') {
								*str1 = i;
								i = 500;
							}
							break;
						case 4:
							if(current_terrain->sign_locs[i].text[0] == '*') {
								*str1 = i;
								i = 500;
							}
							break;
						case 5:
							if(town->sign_locs[i].text[0] == '*') {
								*str1 = i;
								i = 500;
							}
							break;
					}
				if(i < 500) {
					giveError("There are no more free message slots.",
							  "To free up some slots, go into Edit Town/Out/Scenario Text to clear some messages.", &me);
					return true;
				}
			}
			if(*str1 >= 0) {
				switch(str_mode) {
					case 0:
						scenario.spec_strs[*str1] = str;
						break;
					case 1:
						current_terrain->spec_strs[*str1] = str;
						break;
					case 2:
						town->spec_strs[*str1] = str;
						break;
					case 3:
						scenario.journal_strs[*str1] = str;
						break;
					case 4:
						current_terrain->sign_locs[*str1].text = str;
						break;
					case 5:
						town->sign_locs[*str1].text = str;
						break;
				}
			}
		}
		str = str2 == nullptr ? "" : me["str2"].getText();
		if(!str.empty()) {
			if(*str2 < 0) {
				size_t n = num_strs(str_mode);
				for(i = 160; i < n; i++)
					switch(str_mode) {
						case 0:
							if(scenario.spec_strs[i][0] == '*') {
								*str2 = i;
								i = 500;
							}
							break;
						case 1:
							if(current_terrain->spec_strs[i][0] == '*') {
								*str2 = i;
								i = 500;
							}
							break;
						case 2:
							if(town->spec_strs[i][0] == '*') {
								*str2 = i;
								i = 500;
							}
							break;
						case 3:
							if(scenario.journal_strs[i][0] == '*') {
								*str2 = i;
								i = 500;
							}
							break;
						case 4:
							if(current_terrain->sign_locs[i].text[0] == '*') {
								*str2 = i;
								i = 500;
							}
							break;
						case 5:
							if(town->sign_locs[i].text[0] == '*') {
								*str2 = i;
								i = 500;
							}
							break;
							
					}
				if(i < 500) {
					giveError("There are no more free message slots.",
							  "To free up some slots, go into Edit Town/Out/Scenario Text to clear some messages.", &me);
					return true;
				}
			}
			if(*str2 >= 0) {
				switch(str_mode) {
					case 0:
						scenario.spec_strs[*str2] = str;
						break;
					case 1:
						current_terrain->spec_strs[*str2] = str;
						break;
					case 2:
						town->spec_strs[*str2] = str;
						break;
					case 3:
						scenario.journal_strs[*str2] = str;
						break;
					case 4:
						current_terrain->sign_locs[*str2].text = str;
						break;
					case 5:
						town->sign_locs[*str2].text = str;
						break;
				}
			}
		}
		me.toast(true);
	} else me.toast(false);
	return true;
}

// mode 0 - scen 1 - out 2 - town
void edit_spec_text(short mode,short *str1,short *str2,cDialog* parent) {
	using namespace std::placeholders;
	short num_s_strs[3] = {100,90,100};
	
	cDialog edit(str2 ? "edit-special-text" : "edit-special-text-sm", parent);
	edit.attachClickHandlers(std::bind(edit_spec_text_event_filter, _1, _2, mode, str1, str2), {"okay", "cancel"});
	
	if(*str1 >= num_s_strs[mode])
		*str1 = -1;
	if(*str1 >= 0){
		if(mode == 0)
			edit["str1"].setText(scenario.spec_strs[*str1]);
		if(mode == 1)
			edit["str1"].setText(current_terrain->spec_strs[*str1]);
		if(mode == 2)
			edit["str1"].setText(town->spec_strs[*str1]);
		if(mode == 3)
			edit["str1"].setText(scenario.journal_strs[*str1]);
		if(mode == 4)
			edit["str1"].setText(current_terrain->sign_locs[*str1].text);
		if(mode == 5)
			edit["str1"].setText(town->sign_locs[*str1].text);
	}
	if(str2 != nullptr) {
		if(*str2 >= num_s_strs[mode])
			*str2 = -1;
		if(*str2 >= 0){
			if(mode == 0)
				edit["str2"].setText(scenario.spec_strs[*str2]);
			if(mode == 1)
				edit["str2"].setText(current_terrain->spec_strs[*str2]);
			if(mode == 2)
				edit["str2"].setText(town->spec_strs[*str2]);
			if(mode == 3)
				edit["str2"].setText(scenario.journal_strs[*str2]);
			if(mode == 4)
				edit["str2"].setText(current_terrain->sign_locs[*str2].text);
			if(mode == 5)
				edit["str2"].setText(town->sign_locs[*str2].text);
		}
	}
	edit.run();
}

static bool edit_dialog_text_event_filter(cDialog& me, std::string item_hit, short str_mode, short* str1){
	std::string str;
	short i;
	
	if(item_hit == "okay") {
		for(i = 0; i < 6; i++) {
			std::string id = "str" + std::to_string(i + 1);
			str = me[id].getText();
			if(i == 0 && str.empty()) break;
			switch(str_mode) {
				case 0:
					scenario.spec_strs[*str1 + i] = str;
					break;
				case 1:
					current_terrain->spec_strs[*str1 + i] = str;
					break;
				case 2:
					town->spec_strs[*str1 + i] = str;
					break;
				case 3:
					scenario.journal_strs[*str1 + i] = str;
					break;
				case 4:
					current_terrain->sign_locs[*str1 + i].text = str;
					break;
				case 5:
					town->sign_locs[*str1 + i].text = str;
					break;
			}
		}
		me.toast(true);
	} else me.toast(false);
	return true;
}

// mode 0 - scen 1 - out 2 - town
void edit_dialog_text(short mode,short *str1,cDialog* parent) {
	size_t i,j;
	short num_s_strs[3] = {100,90,100};
	
	if(*str1 >= num_s_strs[mode] - 6)
		*str1 = -1;
	// first, assign the 6 strs for the dialog.
	if(*str1 < 0) {
		size_t n = num_strs(mode);
		for(i = 160; i < n - 6; i++) {
			for(j = i; j < i + 6; j++)
				switch(mode) {
					case 0:
						if(scenario.spec_strs[j][0] != '*')
							j = 500;
						break;
					case 1:
						if(current_terrain->spec_strs[j][0] != '*')
							j = 500;
						break;
					case 2:
						if(town->spec_strs[j][0] != '*')
							j = 500;
						break;
					case 3:
						if(scenario.journal_strs[j][0] != '*')
							j = 500;
						break;
					case 4:
						if(current_terrain->sign_locs[j].text[0] != '*')
							j = 500;
						break;
					case 5:
						if(town->sign_locs[j].text[0] != '*')
							j = 500;
						break;
				}
			if(j < 500) {
				*str1 = i;
				i = 500;
			}
		}
		if(*str1 >= 0)
			for(short i = *str1; i < *str1 + 6; i++) {
				switch(mode) {
					case 0:
						scenario.spec_strs[i] = "";
						break;
					case 1:
						current_terrain->spec_strs[i] = "";
						break;
					case 2:
						town->spec_strs[i] = "";
						break;
					case 3:
						scenario.journal_strs[i] = "";
						break;
					case 4:
						current_terrain->sign_locs[i].text = "";
						break;
					case 5:
						town->sign_locs[i].text = "";
						break;
				}
			}
	}
	if(*str1 < 0) {
		giveError("To create a dialog, you need 6 consecutive unused messages. To free up 6 messages, select Edit Out/Town/Scenario Text from the menus.","",parent);
		return;
	}
	
	using namespace std::placeholders;
	cDialog edit("edit-dialog-text",parent);
	edit.attachClickHandlers(std::bind(edit_dialog_text_event_filter, _1, _2, mode, str1), {"okay", "cancel"});
	
	if(*str1 >= 0){
		for(i = 0; i < 6; i++) {
			std::string id = "str" + std::to_string(i + 1);
			if(mode == 0)
				edit[id].setText(scenario.spec_strs[*str1 + i]);
			if(mode == 1)
				edit[id].setText(current_terrain->spec_strs[*str1 + i]);
			if(mode == 2)
				edit[id].setText(town->spec_strs[*str1 + i]);
			if(mode == 3)
				edit[id].setText(scenario.journal_strs[*str1 + i]);
			if(mode == 4)
				edit[id].setText(current_terrain->sign_locs[*str1 + i].text);
			if(mode == 5)
				edit[id].setText(town->sign_locs[*str1 + i].text);
		}
	}
	
	edit.run();
}

static bool edit_special_num_event_filter(cDialog& me, std::string item_hit, short spec_mode) {
	short i;
	size_t num_specs;
	switch(spec_mode) {
		case 0: num_specs = scenario.scen_specials.size();
		case 1: num_specs = current_terrain->specials.size();
		case 2: num_specs = town->specials.size();
	}
	
	if(item_hit == "cancel") me.setResult<short>(-1);
	else if(item_hit == "okay") {
		i = me["num"].getTextAsNum();
		if(i < 0 || i >= num_specs) {
			giveError("There is no special node with that number. The available range is 0 to " + std::to_string(num_specs - 1) + ".",&me);
		}
		me.setResult(i);
		me.toast(true);
	} me.toast(false);
	return true;
}

short edit_special_num(short mode,short what_start) {
	using namespace std::placeholders;
	
	cDialog edit("edit-special-assign");
	edit.attachClickHandlers(std::bind(edit_special_num_event_filter, _1, _2, mode), {"okay", "cancel"});
	
	edit["num"].setTextToNum(what_start);
	
	edit.run();
	
	return edit.getResult<short>();
}

static bool edit_scen_intro_event_filter(cDialog& me, std::string item_hit, eKeyMod) {
	short i;
	
	if(item_hit == "okay") {
		scenario.intro_pic = me["picnum"].getTextAsNum();
		if(scenario.intro_pic > 29) {
			giveError("Intro picture number is out of range.","",&me);
			return true;
		}
		for(i = 0; i < 6; i++) {
			std::string id = "str" + std::to_string(i + 1);
			scenario.intro_strs[i] = me[id].getText();
		}
		me.toast(true);
	} else if(item_hit == "cancel") {
		me.toast(false);
	} else if(item_hit == "choose") {
		pic_num_t pic = me["picnum"].getTextAsNum();
		i = choose_graphic(pic,PIC_SCEN,&me);
		if(i != NO_PIC) {
			me["picnum"].setTextToNum(i);
			dynamic_cast<cPict&>(me["pic"]).setPict(i);
		}
	}
	return true;
}

void edit_scen_intro() {
	short i;
	
	cDialog edit("edit-intro");
	edit.attachClickHandlers(edit_scen_intro_event_filter, {"okay", "cancel", "choose"});
	
	edit["picnum"].setTextToNum(scenario.intro_pic);
	for(i = 0; i < 6; i++) {
		std::string id = "str" + std::to_string(i + 1);
		edit[id].setText(scenario.intro_strs[i]);
	}
	dynamic_cast<cPict&>(edit["pic"]).setPict(scenario.intro_pic);
	
	edit.run();
}

void make_cursor_sword() {
	set_cursor(wand_curs);
}
