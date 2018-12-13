#include "GameServer.h"
#include "GameWorld.h"
#include <iostream>
using namespace NCL;
using namespace CSC8503;

GameServer::GameServer(int onPort, int maxClients)	{
	port		= onPort;
	clientMax	= maxClients;
	clientCount = 0;
	netHandle	= nullptr;
	threadAlive = false;
	bestRecord = 0;
	Initialise();
}

GameServer::~GameServer()	{
	Shutdown();
}

void GameServer::Shutdown() {
	SendGlobalMessage(BasicNetworkMessages::Shutdown);

	threadAlive = false;
	updateThread.join();

	enet_host_destroy(netHandle);
	netHandle = nullptr;
}



int NCL::CSC8503::GameServer::GetHighScore()
{
	string line;
	ifstream myfile("example.txt");
	if (myfile.is_open())
	{
		while (getline(myfile, line))
		{
			vector<string> v = MyServerPacketReceiver::String_Split(line, ' ');
			if (v[0] != "playerID")
			{
				bestRecord = std::stoi(v[1]);
			}
		}
		myfile.close();
		
	}

	else cout << "Unable to open file";

	return bestRecord;
}



bool GameServer::Initialise() {
	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = port; 
	netHandle = enet_host_create(&address, clientMax, 1, 0, 0); 
	if (!netHandle) { 
		std::cout << __FUNCTION__ <<  " failed to create network handle!" << std::endl;
		return false; 
	} 
	return true;
}

bool GameServer::SendGlobalMessage(int msgID) {
	GamePacket packet;
	packet.type = msgID; 
	return SendGlobalMessage(packet);
}

bool GameServer::SendGlobalMessage(GamePacket& packet) {
	ENetPacket* dataPacket = enet_packet_create(&packet,  packet.GetTotalSize(), 0); 
	enet_host_broadcast(netHandle, 0, dataPacket); 
	return true;
}

void GameServer::UpdateServer() {
	if (!netHandle) { return; } 
	ENetEvent event; 
	while (enet_host_service(netHandle, &event, 0) > 0) { 
		int type = event.type; 
		ENetPeer* p = event.peer; 
		int peer = p->incomingPeerID; 
		if (type == ENetEventType::ENET_EVENT_TYPE_CONNECT) {
			std::cout << "Server: New client connected" << std::endl; 
		} 
		else if (type == ENetEventType::ENET_EVENT_TYPE_DISCONNECT) { 
			std::cout << "Server: A client has disconnected" << std::endl; 
		} 
		else if (type == ENetEventType::ENET_EVENT_TYPE_RECEIVE) { 
			GamePacket* packet = (GamePacket*)event.packet->data; 
			ProcessPacket(packet, peer); 
		} 
		enet_packet_destroy(event.packet);
	}
}

void GameServer::ThreadedUpdate() {
	while (threadAlive) {
		UpdateServer();
		std::this_thread::yield();
	}
}

//Second networking tutorial stuff

void GameServer::SetGameWorld(GameWorld &g) {
	gameWorld = &g;
}

void GameServer::BroadcastSnapshot(bool deltaFrame) {

}

void GameServer::UpdateMinimumState() {

}