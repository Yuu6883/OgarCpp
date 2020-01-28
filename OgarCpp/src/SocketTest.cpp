#include <uwebsockets/App.h>
#include <thread>
#include <algorithm>

using namespace std;
using namespace uWS;

int staticID = 0;
/* ws->getUserData returns one of these */
struct UserData {
    int id;
    UserData() : id(staticID++) {}
};

int main() {
    
    unsigned int conc = thread::hardware_concurrency();

    cout << "Starting websockets in " << conc << " threads" << endl;

    /* Simple echo websocket server, using multiple threads */
    vector<thread*> threads(conc);

    transform(threads.begin(), threads.end(), threads.begin(), [](thread* t) {
        return new thread([]() {

            /* Very simple WebSocket echo server */
            App().ws<UserData>("/*", {
                /* Settings */
                .compression = SHARED_COMPRESSOR,
                .maxPayloadLength = 16 * 1024,
                .maxBackpressure = 1 * 1024 * 1204,
                /* Handlers */
                .open = [](auto* ws, auto* req) {
                    UserData* data = (UserData *) ws->getUserData();
                    cout << "Thread " << this_thread::get_id() << " received autism " << data->id << endl;
                },
                .message = [](auto* ws, string_view message, OpCode opCode) {
                    ws->send(message, opCode);
                },
                .drain = [](auto* ws) {
                    /* Check getBufferedAmount here */
                },
                .ping = [](auto* ws) {

                },
                .pong = [](auto* ws) {

                },
                .close = [](auto* ws, int code, string_view message) {

                }
            }).listen(9001, [](auto* token) {
                if (token) {
                    cout << "Thread " << this_thread::get_id() << " listening on port " << 9001 << endl;
                }
                else {
                    cout << "Thread " << this_thread::get_id() << " failed to listen on port 9001" << endl;
                }
            }).run();
        });
    });

    for_each(threads.begin(), threads.end(), [](thread* t) {
        t->join();
    });
}