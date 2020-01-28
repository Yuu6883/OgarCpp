#include <iostream>
#include "../ServerHandle.h"
#include "../misc/Ticker.h"
#include "../misc/Stopwatch.h"
#include "../primitives/QuadTree.h"

using namespace std;

void executor(ServerHandle* handle, ServerHandle* context, vector<string> &tokens) {
	cout << "Tokens: |";
	for_each(tokens.begin(), tokens.end(), [](string token) { cout << token << "|"; });
	cout << endl;
}

int main() {

	/*
	Command<ServerHandle*> command("test", "test_description1", "blabla", &executor);
	CommandList<ServerHandle*> cl(nullptr);

	cl.registerCommand(command);
	if (cl.execute(nullptr, "test hello world")) {
		cout << "IT WORKED LMAO" << endl;
	} */

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

	Rect border(-100, -100, 100, 100);
	QuadTree tree(border, 16, 2);

	for (int i = -50; i < 50; i += 10) {
		auto rect = new Rect(i, i, 20, 20);
		auto item = new QuadItem(i + 10.0, i + 10.0, *rect);
		rect->print(cout << "Inserting: ");
		tree.insert(item);
		cout << endl;
	}

	/*
	Rect searchRange(-50, 50, 0, 0);
	tree.search(searchRange, [](QuadItem* item) {
		item->range.print(cout);
	});
	*/

	cout << tree;
}