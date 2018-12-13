#pragma once
#include <enet/enet.h>
#include <map>
#include <string>
#include<iostream>
#include<fstream>
#include<vector>
using namespace std;
enum BasicNetworkMessages {
	None,
	Hello,
	Message,
	String,
	Delta_State,	//1 byte per channel since the last state
	Full_State,		//Full transform etc
	Received_State, //received from a client, informs that its received packet n
	Player_Connected,
	Player_Disconnected,
	Shutdown
};

struct GamePacket {
	short size;
	short type;

	GamePacket() {
		type		= BasicNetworkMessages::None;
		size		= 0;
	}

	GamePacket(short type) {
		this->type	= type;
	}

	int GetTotalSize() {
		return sizeof(GamePacket) + size;
	}
};

struct StringPacket : public GamePacket {
	char	stringData[256];

	StringPacket(const std::string& message) {
		type		= BasicNetworkMessages::String;
		size		= (short)message.length();

		memcpy(stringData, message.data(), size);
	};

	std::string GetStringFromData() {
		std::string realString(stringData);
		realString.resize(size);
		return realString;
	}
};

struct NewPlayerPacket : public GamePacket {
	int playerID;
	NewPlayerPacket(int p ) {
		type		= BasicNetworkMessages::Player_Connected;
		playerID	= p;
		size		= sizeof(int);
	}
};

struct PlayerDisconnectPacket : public GamePacket {
	int playerID;
	PlayerDisconnectPacket(int p) {
		type		= BasicNetworkMessages::Player_Disconnected;
		playerID	= p;
		size		= sizeof(int);
	}
};

class PacketReceiver {
public:
	virtual void ReceivePacket(int type, GamePacket* payload, int source = -1) = 0;
};

class NetworkBase	{
public:
	static void Initialise();
	static void Destroy();

	static int GetDefaultPort() {
		return 1234;
	}

	void RegisterPacketHandler(int msgID, PacketReceiver* receiver) {
		packetHandlers.insert(std::make_pair(msgID, receiver));
	}

protected:
	NetworkBase();
	~NetworkBase();

	bool ProcessPacket(GamePacket* p, int peerID = -1);
	typedef std::multimap<int, PacketReceiver*>::const_iterator PacketHandlerIterator;

	bool GetPackethandlers(int msgID, PacketHandlerIterator& first, PacketHandlerIterator& last) const {
		auto range = packetHandlers.equal_range(msgID);

		if (range.first == packetHandlers.end()) {
			return false; //no handlers for this message type!
		}
		first	= range.first;
		last	= range.second;
		return true;
	}

	ENetHost* netHandle;

	std::multimap<int, PacketReceiver*> packetHandlers;
};

class MyServerPacketReceiver : public PacketReceiver {
public:
	MyServerPacketReceiver(std::string name) {
		this->name = name;
	}
	void ReceivePacket(int type, GamePacket* payload, int source) {
		if (type == BasicNetworkMessages::String) {
			StringPacket* realPacket = (StringPacket*)payload;
			std::string msg = realPacket->GetStringFromData();
			vector<string> v = String_Split(msg, ' ');
			int pID = std::stoi(v[0]);
			int pS = std::stoi(v[1]);
			std::cout  << " Server received message: "  << "player" << std::stoi(v[0])<<" UpLoad Score "<< std::stoi(v[1])<< std::endl;
			UpLoadPlayerScore(pID, pS);

		}
	}
	void UpLoadPlayerScore(int playerID, int playerScore) {

		ofstream myfile("example.txt");
		if (myfile.is_open())
		{
			myfile << "playerID" << " " << "playerScore" << endl;
			myfile << std::to_string(playerID) << " " << std::to_string(playerScore) << endl;
			myfile.close();
		}
		else cout << "Unable to open file";
	}

	static vector<string> String_Split(const string& s, const char& c) {
		string buff = "";
		vector<string> v;
		for (auto t : s)
		{
			if (t != c)
			{
				buff += t;
			}
			else if (buff != "")
			{
				v.push_back(buff);
				buff = "";
			}
		}
		if (buff != "")
			v.push_back(buff);
		return v;
	}
	


protected:
	std::string name;
};
class  MyClientPacketReceiver : public PacketReceiver {
public:
	MyClientPacketReceiver(std::string name) {
		this->name = name;
	}
	void ReceivePacket(int type, GamePacket* payload, int source) {
		if (type == BasicNetworkMessages::String) {
			StringPacket* realPacket = (StringPacket*)payload;
			std::string msg = realPacket->GetStringFromData();
			std::cout << name << " received message: " << msg << std::endl;
		}
	}





protected:
	std::string name;
};