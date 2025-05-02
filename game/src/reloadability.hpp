#include "common.hpp"
#include "tinyECS/tiny_ecs.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/registry.hpp"

#include <map>

struct Setting {
	int audio;
	std::map<int, std::vector<std::string>> key_bind;
	bool tutorial_completed;

	std::map<std::string, std::vector<int>> action_key;

	Setting() {}

	void init() {
		audio = 0;
		tutorial_completed = false;
		key_bind = {
			{GLFW_KEY_W, {"player_move_up"}},
			{GLFW_KEY_A, {"player_move_left"}},
			{GLFW_KEY_S, {"player_move_down"}},
			{GLFW_KEY_D, {"player_move_right"}},
			{GLFW_KEY_E, {"interact_relic", "interact_spell", "interact_health_pack", "interact_other"}},
			{GLFW_KEY_LEFT_SHIFT, {"movement_spell"}}
		};

		// Forming action_key
		update_action_key();

		print_key_bind();
	}

	int& get_audio() { return audio; }
	std::map<int, std::vector<std::string>>& get_key_bind() { return key_bind; }
	std::map<std::string, std::vector<int>>& get_action_key() { return action_key; }
	bool get_tutorial_completed() { return tutorial_completed; }
	void set_tutorial_completed(bool completed) { tutorial_completed = completed; }

	void update_action_key()
	{
		action_key.clear();
		for (auto it = key_bind.begin(); it != key_bind.end(); ++it)
			for (auto iit = it->second.begin(); iit != it->second.end(); ++iit)
			{
				action_key[*iit].push_back(it->first);
			}
	}

	// for test
	void print_key_bind() {
		for (auto it = key_bind.begin(); it != key_bind.end(); ++it) {
			std::cout << it->first << ": (";
			for (auto iit = it->second.begin(); iit != it->second.end(); ++iit) {
				std::cout << *iit << " ";
			}
			std::cout << ")" << std::endl;
		}
	}

	void modify_audio_intensity(int intensity) { audio = intensity; }

	std::vector<int> find_key(std::string action)
	{
		std::vector<int> original_keys;
		for (auto it = key_bind.begin(); it != key_bind.end(); ++it)
			for (auto iit = it->second.begin(); iit != it->second.end(); ++iit)
				if (*iit == action) original_keys.push_back(it->first);
		return original_keys; // Mark: if not found return empty vector
	}

	bool key_has_action(int key, std::string action) {

		std::vector<std::string> key_list = key_bind[key];
		for (std::string act : key_list) {
			if (strcmp(act.c_str(), action.c_str()) == 0) return true;
		}

		return false;
	}

	void modify_key_binding(std::string action, std::vector<int> new_keys, std::vector<int> original_keys)
	{
		if (!original_keys.empty()) {
			// remove original binded key first
			for (int original_key : original_keys) {
				std::vector<std::string>& key_list = key_bind[original_key];

				for (auto it = key_list.begin(); it != key_list.end();)
				{
					if (*it == action) {
						it = key_list.erase(it);
					}
					else {
						++it;
					}
				}
			}
		}

		// Add the action to the new key
		for (int new_key : new_keys) {
			key_bind[new_key].push_back(action);
		}

		update_action_key();
		print_key_bind();
	}

	void reset_key_bind() {
		key_bind.clear();
		key_bind = {
			{GLFW_KEY_W, {"player_move_up"}},
			{GLFW_KEY_A, {"player_move_left"}},
			{GLFW_KEY_S, {"player_move_down"}},
			{GLFW_KEY_D, {"player_move_right"}},
			{GLFW_KEY_E, {"interact_relic", "interact_spell", "interact_health_pack", "interact_other"}},
			{GLFW_KEY_LEFT_SHIFT, {"movement_spell"}}
		};
		update_action_key();
	}

	bool save_setting();
	bool load_setting();
};

bool save_scores(int& score, std::vector<int>& top_10_score);

bool load_scores(int& score, std::vector<int>& top_10_score);

//void test_json();

//const UP_KEY = GLFW_KEY_W;


static inline std::map<int, std::string> KEY_TO_STRING = {
	{GLFW_KEY_A, "A"}, {GLFW_KEY_B, "B"},
	{GLFW_KEY_D, "D"}, {GLFW_KEY_E, "E"},
	{GLFW_KEY_F, "F"}, {GLFW_KEY_G, "G"},
	{GLFW_KEY_H, "H"}, {GLFW_KEY_I, "I"},
	{GLFW_KEY_J, "J"}, {GLFW_KEY_K, "K"},
	{GLFW_KEY_M, "M"}, {GLFW_KEY_N, "N"},
	{GLFW_KEY_O, "O"}, {GLFW_KEY_P, "P"},
	{GLFW_KEY_Q, "Q"}, {GLFW_KEY_R, "R"},
	{GLFW_KEY_S, "S"}, {GLFW_KEY_T, "T"},
	{GLFW_KEY_U, "U"}, {GLFW_KEY_V, "V"},
	{GLFW_KEY_W, "W"}, {GLFW_KEY_X, "X"},
	{GLFW_KEY_Y, "Y"}, {GLFW_KEY_Z, "Z"},
	{GLFW_KEY_LEFT_SHIFT, "L_SHIFT"}, {GLFW_KEY_RIGHT_SHIFT, "R_SHIFT"},
};