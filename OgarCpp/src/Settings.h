#include <libconfig.h++>
#include <iostream>
#include <filesystem>
#include "primitives/Logger.h"

using namespace libconfig;

static const char* configPath = "game.cfg";
static const char* defaultConfigPath = "src/cli/default.cfg";

Setting* loadConfig() {
	Config* cfg = new Config();

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
