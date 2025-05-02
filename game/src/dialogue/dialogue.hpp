#pragma once

#include <map>
#include <vector>
#include <string>

enum class NPC_NAME {
	OLD_MAN = 0,
	SHOP_KEEPER = OLD_MAN + 1,
};

enum class NPC_CONVERSATION {
	OLD_MAN_1 = 0,
	OLD_MAN_2 = OLD_MAN_1 + 1,
	OLD_MAN_3 = OLD_MAN_2 + 1,
	OLD_MAN_4 = OLD_MAN_3 + 1,
	OLD_MAN_5 = OLD_MAN_4 + 1,
};

///////////////////////////
// OLD MAN CONVERSATIONS //
///////////////////////////

const std::vector<std::string> old_man_1 = {
	"Heya, pal!",
	"The Grand Wizard, huh?",
	"I have no idea where he is.",
	"Hyuk hyuk hyuk!",
};
const std::vector<std::string> old_man_2 = {
	"Hello, friend!",
	"The corruption turned some wizards to slime.",
	"Not me though!", 
	"Hyuk hyuk hyuk!",
};
const std::vector<std::string> old_man_3 = {
	"Hi, little wizard!",
	"How am I not corrupted?",
	"Either I have the strongest willpower...",
	"Or I have no brain to corrupt!",
	"Hyuk hyuk hyuk!",
};
const std::vector<std::string> old_man_4 = {
	"Hey, purple hat guy!",
	"If you ever see a red fountain...",
	"Do not drink it! It hurts real bad!",
	"At least it gave me some shiny goods...",
	"Hyuk hyuk hyuk!",
};
const std::vector<std::string> old_man_5 = {
	"Hello, stranger!",
	"It is dangerous to go alone...",
	"Take this!",
	"...",
	"......",
	".........",
	"I dont actually have anything to give you.",
	"Hyuk hyuk hyuk!",
};


// Map of old man Conversations
const std::map<NPC_CONVERSATION, std::vector<std::string>> old_man_conversations = {
	{NPC_CONVERSATION::OLD_MAN_1, old_man_1},
	{NPC_CONVERSATION::OLD_MAN_2, old_man_2},
	{NPC_CONVERSATION::OLD_MAN_3, old_man_3},
	{NPC_CONVERSATION::OLD_MAN_4, old_man_4},
	{NPC_CONVERSATION::OLD_MAN_5, old_man_5},
};


//////////////////////////////
// SHOPKEEPER CONVERSATIONS //
//////////////////////////////

const std::vector<std::string> shop_keeper_1 = {
	"Another wizard...",
	"You are looking for the Grand Wizard?",
	"Keep going down. You will find him.",
	"Assuming he doesnt find you first...",
	"Good luck.",
};
const std::vector<std::string> shop_keeper_2 = {
	"You arent corrupted like others."
	"You must be strong, then.",
	"Strong enough to get here, at least",
	"Sadly, I have nothing to give you.",
	"Farewell, and good luck."
};
const std::vector<std::string> shop_keeper_3 = {
	"Hello, little wizard.",
	"I used to run a shop here, before the corruption.",
	"But the Grand Wizard took all my goods when he came.",
	"And the corrupted took the rest of my belongings.",
	"The economy really is rough these days.",
	"Good luck."
};
const std::vector<std::string> shop_keeper_4 = {
	"A wizard? I wish I had some spells to sell to you.",
	"You see, I used to be a wizard like you.",
	"But then I took a fireball in the knee.",
	"So I have nothing for you.",
	"Good luck.",
};

// Map of old man Conversations
const std::map<NPC_CONVERSATION, std::vector<std::string>> shop_keeper_conversations = {
	{NPC_CONVERSATION::OLD_MAN_1, shop_keeper_1},
	{NPC_CONVERSATION::OLD_MAN_2, shop_keeper_2},
	{NPC_CONVERSATION::OLD_MAN_3, shop_keeper_3},
	{NPC_CONVERSATION::OLD_MAN_4, shop_keeper_4},
};




// Map of Map of Conversations
const std::map<
	NPC_NAME, 
	std::map<
		NPC_CONVERSATION, 
		std::vector<std::string>
	>
> NPC_DIALOGUES = {
	{NPC_NAME::OLD_MAN, old_man_conversations},
	{NPC_NAME::SHOP_KEEPER, shop_keeper_conversations},
};


std::string getDialogue(NPC_NAME name, NPC_CONVERSATION npc_conversation, int index);