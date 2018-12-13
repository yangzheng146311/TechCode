#include "NetworkBase.h"

NetworkBase::NetworkBase()
{
	netHandle = nullptr;
}

NetworkBase::~NetworkBase()
{
	if (netHandle) {
		enet_host_destroy(netHandle);
	}
}

bool NetworkBase::ProcessPacket(GamePacket * p, int peerID)
{
	PacketHandlerIterator firstHandler; 
	PacketHandlerIterator lastHandler; 
	bool canHandle = GetPackethandlers(p->type,  firstHandler, lastHandler);
	if (canHandle) { 
		for (auto i = firstHandler; i != lastHandler; ++i) { 
			i->second->ReceivePacket(p->type, p, peerID); 
			
		} 
		return true; 
	} 
	std::cout<< __FUNCTION__ << " no handler for packet type "  << p->type << std::endl;
	return false;
}

void NetworkBase::Initialise() {
	enet_initialize();
}

void NetworkBase::Destroy() {
	enet_deinitialize();
}