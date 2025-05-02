#include "dialogue.hpp"

#include <map>
#include <vector>
#include <string>

std::string getDialogue(NPC_NAME name, NPC_CONVERSATION npc_conversation, int index) {
	std::map<NPC_CONVERSATION, std::vector<std::string>> conversations = NPC_DIALOGUES.at(name);
	std::vector<std::string> conversation = conversations[npc_conversation];

	if (index >= conversation.size()) {
		std::string end = "END";
		return end;
	}

	return conversation[index];
}