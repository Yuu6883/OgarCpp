#include <iostream>
#include "../ServerHandle.h"
#include "../Settings.h"

using namespace std;

/*
void executor(ServerHandle* handle, ServerHandle* context, vector<string> &tokens) {
	cout << "Tokens: |";
	for_each(tokens.begin(), tokens.end(), [](string token) { cout << token << "|"; });
	cout << endl;
}
*/

int main() {

	/*
	Command<ServerHandle*> command("test", "test_description1", "blabla", &executor);
	CommandList<ServerHandle*> cl(nullptr);

	cl.registerCommand(command);
	if (cl.execute(nullptr, "test hello world")) {
		cout << "IT WORKED LMAO" << endl;
	} */

	Setting* settings = loadConfig();
	ServerHandle handle(settings);

	int playerMaxSize = handle.getSettingInt("playerMaxSize");
	cout << "playerMaxSize: " << playerMaxSize << endl;

	string serverName = handle.getSettingString("serverName");
	cout << "serverName: " << serverName << endl;

	handle.start();

	this_thread::sleep_for(seconds{ 1 });
	cout << "Program ending" << endl;

	/*
	Ticker ticker(40);
	Stopwatch watch;
	watch.begin();

	ticker.add([&]() {
		cout << "lap: " << watch.lap() << ", elapesd: " << watch.elapsed() << endl;
	});

	ticker.start();

	this_thread::sleep_for(milliseconds{ 10000 });
	ticker.stop();

	cout << "ticker has stopped" << endl;
	this_thread::sleep_for(milliseconds{ 5000 });
	*/

	/*
	Rect border(-100, -100, 100, 100);
	QuadTree tree(border, 16, 2);
	vector<QuadItem*> items;

	for (int i = -90; i < 90; i += 20) {
		auto rect = new Rect(i, i, 20, 20);
		auto item = new QuadItem(i, i, *rect);
		// rect->print(cout << "Inserting: ");
		tree.insert(item);
		items.push_back(item);
		// cout << endl;
	}
	*/

	/* 
	Rect searchRange(50, 50, 40, 40);
	tree.search(searchRange, [](QuadItem* item) {
		item->range.print(cout);
	});
	*/

	/*
	for (auto item : items) {
		tree.remove(item);
		delete &item->range;
		delete item;
	}

	cout << tree;
	*/

	/*
	Writer writer;

	writer.writeStringUTF8("pi is: ");
	writer.writeFloat32(3.1415926);
	writer.writeStringUTF8("Hello world");
	writer.writeUInt32(69420);

	string_view buffer = writer.finalize();

	cout << "Buffer size is: " << buffer.size() << endl;

	Reader reader(buffer);

	cout << reader.readStringUTF8() << endl;
	cout << reader.readFloat32() << endl;
	cout << reader.readStringUTF8() << endl;
	cout << reader.readUInt32() << endl;
	*/

	return 0;
}