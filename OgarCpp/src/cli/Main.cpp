#define _HAS_STD_BYTE 0
#define _HAS_STD_BOOLEAN 0

#include <iostream>

#include "../ServerHandle.h"
#include "../Settings.h"

#include "../primitives/Writer.h"

/*
void executor(ServerHandle* handle, ServerHandle* context, vector<string> &tokens) {
	cout << "Tokens: |";
	for_each(tokens.begin(), tokens.end(), [](string token) { cout << token << "|"; });
	cout << endl;
}
*/

int main() {

	Setting* settings = loadConfig();
	ServerHandle handle(settings);

	Router r(&handle.listener);

	r.spawningName = "hello world";
	r.onSpawnRequest();

	handle.start();
	std::this_thread::sleep_for(std::chrono::seconds{ 200 });
	handle.stop();

	/*
	std::thread thread1([] {
		Writer writer1;
		printf("writer1 address: 0x%p\n", writer1.getPool());
	});

	std::thread thread2([] {
		Writer writer2;
		printf("writer2 address: 0x%p\n", writer2.getPool());
	});

	thread1.join();
	thread2.join();
	*/

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

	return EXIT_SUCCESS;
}