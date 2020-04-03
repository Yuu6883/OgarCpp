#include <libconfig.h++>
#include <iostream>
#include <filesystem>
#include "primitives/Logger.h"

using namespace libconfig;

static const char* configPath = "game.cfg";
static const char* defaultConfigPath = "src/cli/default.cfg";

static Config* cfg = nullptr;

inline Setting* loadConfig() {
	if (cfg) delete cfg;
	cfg = new Config();

	cfg->setOptions(Config::OptionFsync
		| Config::OptionSemicolonSeparators
		| Config::OptionColonAssignmentForGroups
		| Config::OptionOpenBraceOnSeparateLine);

	if (std::filesystem::exists(configPath)) {
		try {
			cfg->readFile(configPath);
		} catch (const ParseException & pex) {
			Logger::error(std::string("Parse error at ") + pex.getFile() + ":" + 
				std::to_string(pex.getLine()) + " - " + pex.getError());
		}
	} else {
		Logger::info("Writing default config to \"game.cfg\"");
		cfg->readFile(defaultConfigPath);
		cfg->writeFile(configPath);
	}
	return &cfg->getRoot();
}

inline bool saveConfig() {
	if (cfg) {
		cfg->writeFile(configPath);
		return true;
	}
	return false;
}

inline bool unloadConfig() {
	if (cfg) {
		delete cfg;
		cfg = nullptr;
		return true;
	}
	return false;
}