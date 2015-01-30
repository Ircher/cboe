/*
 *  talking.cpp
 *  BoE
 *
 *  Created by Celtic Minstrel on 22/04/09.
 *
 */

#include "talking.h"

#include <string>
#include <vector>
#include <map>
#include <sstream>

#include "oldstructs.h"

void cSpeech::append(legacy::talking_record_type& old){
	int i,j;
	for(i = 0; i < 200; i++)
		strlens[i] = old.strlens[i];
	for(i = 0; i < 60; i++){
		talk_nodes[i].personality = old.talk_nodes[i].personality;
		for(j = 0; j < 4; j++){
			talk_nodes[i].link1[j] = old.talk_nodes[i].link1[j];
			talk_nodes[i].link2[j] = old.talk_nodes[i].link2[j];
			talk_nodes[i].extras[j] = old.talk_nodes[i].extras[j];
		}
		// Now, convert data if necessary
		switch(old.talk_nodes[i].type) {
			case 0: // normal speech
				talk_nodes[i].type = eTalkNode::REGULAR;
				break;
			case 1: // depend on SDF
				talk_nodes[i].type = eTalkNode::DEP_ON_SDF;
				break;
			case 2: // set SDF
				talk_nodes[i].type = eTalkNode::SET_SDF;
				break;
			case 3: // inn
				talk_nodes[i].type = eTalkNode::INN;
				break;
			case 4: // depend on time
				talk_nodes[i].type = eTalkNode::DEP_ON_TIME;
				break;
			case 5: // depend on event
				talk_nodes[i].type = eTalkNode::DEP_ON_TIME_AND_EVENT;
				break;
			case 6: // depend on town
				talk_nodes[i].type = eTalkNode::DEP_ON_TOWN;
				break;
			case 7: // Item shop
				talk_nodes[i].type = eTalkNode::BUY_ITEMS;
				break;
			case 8: // Training
				talk_nodes[i].type = eTalkNode::TRAINING;
				break;
			case 9: // Spell shops
				talk_nodes[i].type = eTalkNode::BUY_MAGE;
			if(false) // Intentional fallthrough, but suppress first line
			case 10:
				talk_nodes[i].type = eTalkNode::BUY_PRIEST;
				talk_nodes[i].extras[1] += 30;
				break;
			case 11: // Alchemy shop
				talk_nodes[i].type = eTalkNode::BUY_ALCHEMY;
				break;
			case 12: // Healer
				talk_nodes[i].type = eTalkNode::BUY_HEALING;
				break;
			case 13: // Sell weapons
				talk_nodes[i].type = eTalkNode::SELL_WEAPONS;
				break;
			case 14: // Sell armor
				talk_nodes[i].type = eTalkNode::SELL_ARMOR;
				break;
			case 15: // Sell any
				talk_nodes[i].type = eTalkNode::SELL_ITEMS;
				break;
			case 16: // Identification
				talk_nodes[i].type = eTalkNode::IDENTIFY;
				break;
			case 17: // Augmentation
				talk_nodes[i].type = eTalkNode::ENCHANT;
				break;
			case 18: // buy response
				talk_nodes[i].type = eTalkNode::BUY_INFO;
				break;
			case 19: // buy SDF
				talk_nodes[i].type = eTalkNode::BUY_SDF;
				break;
			case 20: // buy boat
				talk_nodes[i].type = eTalkNode::BUY_SHIP;
				break;
			case 21: // buy horse
				talk_nodes[i].type = eTalkNode::BUY_HORSE;
				break;
			case 22: // buy special item
				talk_nodes[i].type = eTalkNode::BUY_SPEC_ITEM;
				break;
			case 23: // Junk shop
				talk_nodes[i].type = eTalkNode::BUY_JUNK;
				break;
			case 24: // buy town location
				talk_nodes[i].type = eTalkNode::BUY_TOWN_LOC;
				break;
			case 25: // force conversation end
				talk_nodes[i].type = eTalkNode::END_FORCE;
				break;
			case 26: // end and fight
				talk_nodes[i].type = eTalkNode::END_FIGHT;
				break;
			case 27: // end and raise alarm
				talk_nodes[i].type = eTalkNode::END_ALARM;
				break;
			case 28: // end and die
				talk_nodes[i].type = eTalkNode::END_DIE;
				break;
			case 29: // call town special
				talk_nodes[i].type = eTalkNode::CALL_TOWN_SPEC;
				break;
			case 30: // call scenario special
				talk_nodes[i].type = eTalkNode::CALL_SCEN_SPEC;
				break;
		}
	}
}
