#pragma once

#include "Protocol.h"
class Protocol;

class ProtocolStore {
public:
	vector<Protocol*> store;
	ProtocolStore() {};
	void registerProtocol(Protocol* p) {
		store.push_back(p);
	}
	Protocol* decide(Connection* connection, Reader& reader) {
		for (auto protocol : store) {
			auto copy = new Protocol(*protocol);
			if (!copy->distinguishes(reader)) {
				reader.reset();
				continue;
			}
			return connection->socketDisconnected ? nullptr : copy;
		}
		return nullptr;
	}
};