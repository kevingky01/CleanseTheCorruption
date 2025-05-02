#include "reloadability.hpp"

json initialize_setting_json = {
	{ "audio", 0 },
	{ "key_bind",  },
	{ "tutorial_completed", false }
};

bool Setting::save_setting()
{
	//return false;

	std::string filename = "setting.json";

	std::ifstream in_file(persistance_path(filename));

	if (in_file.is_open())
	{
		json data = in_file.peek() ==
			std::ifstream::traits_type::eof() ? initialize_setting_json : json::parse(in_file);
		//json data = json::parse(in_file);

		std::ofstream out_file(persistance_path(filename));

		in_file.close();

		if (out_file.is_open())
		{
			data["audio"] = audio;

			//for (auto it = key_bind.begin(); it != key_bind.end(); ++it) {
			//	data["key_bind"][it->first] = it->second;
			//}
			data["key_bind"] = key_bind;
			data["tutorial_completed"] = tutorial_completed;
			
			out_file << data.dump(4) << std::endl;

			out_file.close();
			return true;
		}

		return false;
	}

	return false;
}

bool Setting::load_setting()
{
	std::string filename = "setting.json";
	std::ifstream in_file(persistance_path(filename));

	if (in_file.is_open())
	{
		// check whether the file is empty
		json data = in_file.peek() ==
			std::ifstream::traits_type::eof() ? initialize_setting_json : json::parse(in_file);

		audio = data["audio"].template get<int>();
		key_bind = data["key_bind"].template get<std::map<int, std::vector<std::string>>>();
		
		// Check for tutorial key in save data
		if (data.contains("tutorial_completed")) {
			tutorial_completed = data["tutorial_completed"].template get<bool>();
		} else {
			tutorial_completed = false;
		}

		in_file.close();

		update_action_key();
		return true;
	}

	return false;
}


json initialize_high_score_json = {
	{ "highest_score", 0 },
	{ "top_10_highest_score", { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }
};


bool save_scores(int& score, std::vector<int>& top_10_score)
{
	std::ifstream in_file(persistance_path("high_scores.json"));

	if (in_file.is_open())
	{
		// check whether the file is empty
		json data = in_file.peek() ==
			std::ifstream::traits_type::eof() ? initialize_high_score_json : json::parse(in_file);

		std::ofstream out_file(persistance_path("high_scores.json"));

		in_file.close();

		if (out_file.is_open())
		{
			data["highest_score"] = score;
			data["top_10_highest_score"] = top_10_score;

			out_file << data.dump(4) << std::endl;

			out_file.close();
			return true;
		}

		return false;
	}

	return false;
}

bool load_scores(int& score, std::vector<int>& top_10_score)
{
	std::ifstream in_file(persistance_path("high_scores.json"));

	if (in_file.is_open()) 
	{
		// check whether the file is empty
		json data = in_file.peek() == 
			std::ifstream::traits_type::eof() ? initialize_high_score_json : json::parse(in_file);

		score = data["highest_score"];
		// get list of int
		top_10_score = data["top_10_highest_score"].template get<std::vector<int>>();

		in_file.close();
		return true;
	}
	
	return false;
}

/*
void test_json()
{
	// store a string in a JSON value
	json j_string = "this is a string";

	// retrieve the string value
	auto cpp_string = j_string.template get<std::string>();
	// retrieve the string value (alternative when a variable already exists)
	std::string cpp_string2;
	j_string.get_to(cpp_string2);

	// retrieve the serialized value (explicit JSON serialization)
	std::string serialized_string = j_string.dump();

	// output of original string
	std::cout << cpp_string << " == " << cpp_string2 << " == " << j_string.template get<std::string>() << '\n';
	// output of serialized value
	std::cout << j_string << " == " << serialized_string << std::endl;
}
*/