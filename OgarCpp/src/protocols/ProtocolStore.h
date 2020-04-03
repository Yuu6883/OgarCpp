#pragma once

#include "Protocol.h"
class Protocol;

class ProtocolStore {
public:
	vector<Protocol*> store;

	ProtocolStore() {};
	~ProtocolStore() {
		while (store.size()) {
			delete store.back();
			store.pop_back();
		}
	}

	void registerProtocol(Protocol* p) {
		if (p) store.push_back(p);
	}

	Protocol* decide(Connection* connection, Reader& reader) {
		for (auto protocol : store) {
			auto copy = protocol->clone();
			copy->connection = connection;
			if (!copy->distinguishes(reader)) {
				reader.reset();
				continue;
			}
			return connection->socketDisconnected ? nullptr : copy;
		}
		return nullptr;
	}
};