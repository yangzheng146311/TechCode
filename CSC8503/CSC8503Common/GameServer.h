#pragma once
#include <thread>
#include <atomic>

#include "NetworkBase.h"
#include <iostream>
#include <fstream>
#include<vector>
using namespace std;
namespace NCL {
	namespace CSC8503 {
		class GameWorld;
		class GameServer : public NetworkBase {
		public:
			GameServer(int onPort, int maxClients);
			~GameServer();

			vector<string> String_Split(const string& s, const char& c);
			int GetHighScore();
			void UpLoadPlayerScore(int playerID,int playerScore);
			bool Initialise();
			void Shutdown();

			virtual void UpdateServer();
			void SetGameWorld(GameWorld &g);

			void ThreadedUpdate();

			bool SendGlobalMessage(int msgID);
			bool SendGlobalMessage(GamePacket& packet);

			void BroadcastSnapshot(bool deltaFrame);
			void UpdateMinimumState();

			

		protected:
			int			port;
			int			clientMax;
			int			clientCount;
			GameWorld*	gameWorld;
			int bestRecord;

			std::atomic<bool> threadAlive;

			

			std::thread updateThread;

			int incomingDataRate;
			int outgoingDataRate;

			std::map<int, int> stateIDs;
		};
	}
}
