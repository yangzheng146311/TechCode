#pragma once
#include "GameTechRenderer.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/State.h"
#include "NetworkedGame.h"
#include "../CSC8503Common/GameServer.h"
#include "../CSC8503Common/GameClient.h"
#include"../CSC8503Common/NetworkBase.h"

using namespace std;
namespace NCL {
	namespace CSC8503 {
		class TutorialGame		{
		public:
			TutorialGame();
			~TutorialGame();

			virtual void UpdateGame(float dt);
			
			
			int myLevel;
			int mytime=1;
			int wallMoveDir = 1;
			int rand_int_x = 0;
			int score = 300;
			int bestScore = 0;
			
			GameServer* server;
			GameClient* client;
			
			MyServerPacketReceiver *serverReceiver;
			MyClientPacketReceiver *clientReceiver;
			StateMachine *testMachine;
		protected:
			void InitialiseAssets();
			void InitNetWork();
			void InitCamera();
			void UpdateKeys();
			
			void InitWorld();

			/*
			These are some of the world/object creation functions I created when testing the functionality
			in the module. Feel free to mess around with them to see different objects being created in different
			test scenarios (constraints, collision types, and so on). 
			*/
			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			void InitTerrain(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			void InitCourt();
			void InitSphereCollisionTorqueTest();
			void InitCubeCollisionTorqueTest();
			void InitSphereAABBTest();
			void InitGJKWorld();
			void BridgeConstraintTest();
			void SimpleGJKTest();
			void SimpleAABBTest();
			void SimpleAABBTest2();

			void CamFollow(Camera *c, GameObject *obj);
			void FSM_MoveWall(int &time, GameWorld *g);
			void Enemy_Chase(GameObject *enemy, GameObject *player);
			bool SelectObject();
			void MoveSelectedObject();
		
			

			GameObject* AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);
			GameObject* AddEnemyToWorld(const Vector3& position, float radius, float inverseMass = 10.0f);
			GameObject* AddTerrainToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);
			GameObject* AddObstacleToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);

			GameTechRenderer*	renderer;
			PhysicsSystem*		physics;
			GameWorld*			world;
			
			Vector3 forcePos;
			Vector3 dirPoint;
			float axisYoffset;
			bool useGravity;
			bool inSelectionMode;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;

			OGLMesh*	cubeMesh	= nullptr;
			OGLMesh*	sphereMesh	= nullptr;
			OGLTexture* ballTex = nullptr;
			OGLTexture* basicTex	= nullptr;
			OGLTexture* enemyTex = nullptr;
			OGLTexture* terrainTex = nullptr;
			OGLTexture* obsTex = nullptr;
			OGLTexture* floorTex = nullptr;
			OGLShader*	basicShader = nullptr;
		};


		
	}
}

