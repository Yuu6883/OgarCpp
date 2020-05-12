#pragma once

#include "../primitives/Logger.h"
#include "uwebsockets/App.h"
#include "AsyncFileReader.h"
#include <filesystem>

using std::to_string;

struct AsyncFileStreamer {

    std::map<std::string_view, AsyncFileReader *> asyncFileReaders;
    std::string root;

    AsyncFileStreamer(std::string root) : root(root) {
        // for all files in this path, init the map of AsyncFileReaders
        updateRootCache();
    }

    ~AsyncFileStreamer() {
        for (auto [k, v] : asyncFileReaders) {
            delete k.data();
            delete v;
        }
    }

    void updateRootCache() {
        try {
            for (auto& p : std::filesystem::recursive_directory_iterator(root)) {
                std::string url = p.path().string().substr(root.length());
                std::replace(url.begin(), url.end(), '\\', '/'); // Windows have autism
                if (p.is_directory()) continue;
                auto size = p.file_size();
                if (!size) continue;
                std::string size_str = size > 1024 * 1024 ? (to_string(size / 1024 / 1024) + "MB") : (to_string(size / 1024) + "KB");
                Logger::verbose("Caching " + url + "    (" + size_str + ")");

                if (url == "/index.html") url = "/";
                char* key = new char[url.length()];
                memcpy(key, url.data(), url.length());
                asyncFileReaders[std::string_view(key, url.length())] = new AsyncFileReader(p.path().string());
            }
        } catch (std::exception& e) {
            Logger::error(e.what());
        }
    }

    template <bool SSL>
    void streamFile(uWS::HttpResponse<SSL> *res, std::string_view url) {
        auto it = asyncFileReaders.find(url);
        if (it == asyncFileReaders.end()) {
            res->writeStatus("404 not found")->end();
        } else {
            streamFile(res, it->second);
        }
    }

    template <bool SSL>
    static void streamFile(uWS::HttpResponse<SSL> *res, AsyncFileReader *asyncFileReader) {
        /* Peek from cache */
        std::string_view chunk = asyncFileReader->peek(res->getWriteOffset());
        if (!chunk.length() || res->tryEnd(chunk, asyncFileReader->getFileSize()).first) {
            /* Request new chunk */
            // todo: we need to abort this callback if peer closed!
            // this also means Loop::defer needs to support aborting (functions should embedd an atomic boolean abort or something)

            // Loop::defer(f) -> integer
            // Loop::abort(integer)

            // hmm? no?

            // us_socket_up_ref eftersom vi delar ägandeskapet

            if (chunk.length() < asyncFileReader->getFileSize()) {
                asyncFileReader->request(res->getWriteOffset(), [res, asyncFileReader](std::string_view chunk) {
                    // check if we were closed in the mean time
                    //if (us_socket_is_closed()) {
                        // free it here
                        //return;
                    //}

                    /* We were aborted for some reason */
                    if (!chunk.length()) {
                        // todo: make sure to check for is_closed internally after all callbacks!
                        res->close();
                    } else {
                        AsyncFileStreamer::streamFile(res, asyncFileReader);
                    }
                });
            }
        } else {
            /* We failed writing everything, so let's continue when we can */
            res->onWritable([res, asyncFileReader](int offset) {

                // här kan skiten avbrytas!

                AsyncFileStreamer::streamFile(res, asyncFileReader);
                // todo: I don't really know what this is supposed to mean?
                return false;
            })->onAborted([]() {
                std::cout << "ABORTED!" << std::endl;
            });
        }
    }
};
