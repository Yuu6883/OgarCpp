#include <libconfig.h++>
#include <iostream>
#include <filesystem>

using namespace libconfig;
using namespace std;

static const char* configPath = "game.cfg";
static const char* defaultConfigPath = "src/cli/default.cfg";

Config* loadConfig() {
	Config* cfg = new Config();

	cfg->setOptions(Config::OptionFsync
		| Config::OptionSemicolonSeparators
		| Config::OptionColonAssignmentForGroups
		| Config::OptionOpenBraceOnSeparateLine);

	if (filesystem::exists(configPath)) {
		try {
			cfg->readFile(configPath);
		} catch (const ParseException & pex) {
			cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
				<< " - " << pex.getError() << endl;
		}
	} else {
		cfg->readFile(defaultConfigPath);
		cfg->writeFile(configPath);
	}

	return cfg;
}
