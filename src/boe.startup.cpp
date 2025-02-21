
#include "boe.global.hpp"

#include "universe.hpp"
#include "boe.newgraph.hpp"
#include "boe.graphics.hpp"
#include "boe.fileio.hpp"
#include "boe.actions.hpp"
#include "boe.dlgutil.hpp"
#include "boe.text.hpp"
#include "boe.graphutil.hpp"
#include "boe.items.hpp"
#include "boe.party.hpp"
#include "boe.main.hpp"
#include "soundtool.hpp"
#include "fileio.hpp"
#include "dlogutil.hpp"
#include "winutil.hpp"
#include "boe.menus.hpp"
#include "mathutil.hpp"

#include <vector>
using std::vector;

extern bool play_sounds,party_in_memory;
extern long register_flag;
extern sf::RenderWindow mainPtr;
extern location ul;
extern std::vector<scen_header_type> scen_headers;
extern cUniverse univ;
extern eGameMode overall_mode;

rectangle startup_button[6];

// TODO: Always returns false, so make it void
bool handle_startup_press(location the_point) {
	using kb = sf::Keyboard;
	std::string scen_name;
	short i,scen;
	bool force_party = false;
	
	the_point.x -= ul.x;
	the_point.y -= ul.y;
	
	for(i = 0; i < 5; i++)
		if(the_point.in(startup_button[i])) {
			draw_start_button(i,5);
			mainPtr.display(); // TODO: I suspect this won't work
			if(play_sounds)
				play_sound(37);
			else sf::sleep(time_in_ticks(5));
			draw_start_button(i,0);
			switch(i) {
				case STARTBTN_LOAD:
					do_load();
					break;
					
				case STARTBTN_NEW:
					draw_startup(0);
					start_new_game();
					make_cursor_sword();
					draw_startup(0);
					break;
					
				case STARTBTN_ORDER:
					// TODO: Figure out something to put here.
//					give_reg_info();
//					draw_startup(0);
					break;
					
				case STARTBTN_JOIN: // regular scen
					if(!party_in_memory) {
						if(kb::isKeyPressed(kb::LAlt) || kb::isKeyPressed(kb::RAlt)) {
							force_party = true;
							start_new_game(true);
						} else {
							cChoiceDlog("need-party").show();
							break;
						}
					}
					scen = pick_prefab_scen();
					if(scen < 0) {
						if(force_party)
							party_in_memory = false;
						break;
					}
					
					switch(scen) {
						case 0: scen_name = "valleydy.exs"; break;
							// if not reg, rub out
						case 1: scen_name = "stealth.exs"; break;
						case 2: scen_name = "zakhazi.exs"; break;
							//case 3: sprintf(univ.party.scen_name,"busywork.exs"); break;
					}
					put_party_in_scen(scen_name);
					if(force_party && scen_name != univ.party.scen_name)
						party_in_memory = false;
					break;
					
				case STARTBTN_CUSTOM: // custom
					if(!party_in_memory) {
						if(kb::isKeyPressed(kb::LAlt) || kb::isKeyPressed(kb::RAlt)) {
							force_party = true;
							start_new_game(true);
						} else {
							cChoiceDlog("need-party").show();
							break;
						}
					}
					
					scen = pick_a_scen();
					if(scen < 0) {
						if(force_party)
							party_in_memory = false;
						break;
					}
					if(scen_headers[scen].prog_make_ver[0] > 2 || scen_headers[scen].prog_make_ver[1] > 0) {
						if(force_party)
							party_in_memory = false;
						cChoiceDlog("scen-version-mismatch").show();
						break;
					}
					scen_name = scen_headers[scen].file;
					put_party_in_scen(scen_name);
					if(force_party && scen_name != univ.party.scen_name)
						party_in_memory = false;
					break;
			}
		}
	return false;
}

/*
void start_game () {
	init_party(0);
	
	setup_outdoors(party.p_loc);
	
	load_area_graphics();
	
	draw_main_screen();
	
	in_startup_mode = false;
	
	adjust_monst_menu();
	adjust_spell_menus();
	
}*/
