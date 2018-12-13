#include "TutorialGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"

#include "../CSC8503Common/PositionConstraint.h"

using namespace NCL;
using namespace CSC8503;


TutorialGame::TutorialGame()	{
	world		= new GameWorld();
	renderer	= new GameTechRenderer(*world);
	physics		= new PhysicsSystem(*world);
	testMachine = new StateMachine();
	myLevel = 1;

	

	forceMagnitude	= 10.0f;
	useGravity		= false;
	inSelectionMode = false;

	

	Debug::SetRenderer(renderer);

	InitialiseAssets();
	
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it 

*/
void TutorialGame::InitialiseAssets() {
	cubeMesh = new OGLMesh("cube.msh");
	cubeMesh->SetPrimitiveType(GeometryPrimitive::Triangles);
	cubeMesh->UploadToGPU();

	sphereMesh = new OGLMesh("sphere.msh");
	sphereMesh->SetPrimitiveType(GeometryPrimitive::Triangles);
	sphereMesh->UploadToGPU();

	ballTex = (OGLTexture*)TextureLoader::LoadAPITexture("golfball.jpg");
	basicTex = (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	enemyTex = (OGLTexture*)TextureLoader::LoadAPITexture("doge.png");
	terrainTex = (OGLTexture*)TextureLoader::LoadAPITexture("Green.png");
	//floorTex = (OGLTexture*)TextureLoader::LoadAPITexture("grass.bmp");
	floorTex = (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	obsTex = (OGLTexture*)TextureLoader::LoadAPITexture("brown.jpg");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	InitCamera();
	InitWorld();
	InitNetWork();
	
}

void NCL::CSC8503::TutorialGame::InitNetWork()
{
	NetworkBase::Initialise();
	serverReceiver=new MyServerPacketReceiver("Server");
	clientReceiver= new MyClientPacketReceiver("Client");
	int port = NetworkBase::GetDefaultPort();
     server = new GameServer(port, 1);
	 client = new GameClient();
	server->RegisterPacketHandler(String, &(*serverReceiver));
	client->RegisterPacketHandler(String, &(*clientReceiver));
	bool canConnect = client->Connect(127, 0, 0, 1, port);

	bestScore = server->GetHighScore();
	for (int i = 0; i < 4; i++)
	{
			server->SendGlobalMessage(StringPacket("Server spawn!Local Server highest score : " + std::to_string(bestScore)));
			server->UpdateServer();
			client->UpdateClient();
	}
	/*server->SendGlobalMessage(
		StringPacket("Server spawn!Local Server highest score : " + std::to_string(bestScore)));
	client->SendPacket(
		StringPacket("Client spawn!") );
	server->UpdateServer();
	client->UpdateClient();*/
	

	/*for (int i = 0; i < 100; ++i) {
		server->SendGlobalMessage(StringPacket("Server says hello! " + std::to_string(i)));
		client->SendPacket(StringPacket("Client says hello! " + std::to_string(i)));
		server->UpdateServer();
		client->UpdateClient();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}*/
	//NetworkBase::Destroy();
}

TutorialGame::~TutorialGame()	{
	
	


	NetworkBase::Destroy();
	delete serverReceiver;
	delete clientReceiver;
	delete cubeMesh;
	delete sphereMesh;
	delete basicTex;
	delete floorTex;
	delete terrainTex;
	delete obsTex;
	delete ballTex;
	delete basicShader;
	
	delete testMachine;
	delete physics;
	delete renderer;
	delete world;
}

void TutorialGame::UpdateGame(float dt) {
	
	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_O)) {

		if (score > bestScore)
		{
			for (int i = 0; i < 2; i++)
			{

				//server->SendGlobalMessage(StringPacket("Server  OUT"));
				client->SendPacket(StringPacket("1 " + std::to_string(score)));
				server->UpdateServer();
				client->UpdateClient();
			}
		}
	}

	
	
	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}

	UpdateKeys();
	
	Debug::Print("High Score:" + std::to_string(bestScore), Vector2(10, 100));
	Debug::Print("Current Score:"+ std::to_string(score), Vector2(10, 70));
	if (useGravity) {
		Debug::Print("(G)ravity on", Vector2(10, 40));
	}
	 {
		Debug::Print("(G)ravity off", Vector2(10, 40));
	}
	 

	world->myTimer += dt;

	if (score > 0)
	{
		score = 300 - world->myTimer;
		
		//std::cout << score << std::endl;
	}
	

	if (physics->getGravityState()&& world->myTimer>3)
	{
		Enemy_Chase(world->GetEnemy(), world->GetPlayer());
		
	}
	CamFollow(world->GetMainCamera(), world->GetPlayer());
	FSM_MoveWall(mytime, world);
	SelectObject();
	MoveSelectedObject();
	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);


	if (physics->isTouchFloor == true) {
		myLevel = 2; 
		physics->isTouchFloor = false;
	}

	Debug::FlushRenderables();
	renderer->Render();
}

void TutorialGame::UpdateKeys() {

	
	
	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_R)|| myLevel == 2) {
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
		myLevel = 1;
		world->myTimer = 0;
	}

	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::KEYBOARD_G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_F7)) {
		world->ShuffleObjects(true);
	}	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_F8)) {

		world->ShuffleObjects(false);
	}
	//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-100, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(100, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 100, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -100, 0));
		}


		if (Window::GetKeyboard()->KeyDown(KEYBOARD_UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -100));
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 100));
		}
	}

	
}



void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(3.0f);
	world->GetMainCamera()->SetFarPlane(4200.0f);
	world->GetMainCamera()->SetPitch(-35.0f);
	world->GetMainCamera()->SetYaw(320.0f);
	world->GetMainCamera()->SetPosition(Vector3(-500, 1200, 2000));


	//world->GetMainCamera()->SetNearPlane(3.0f);
	//world->GetMainCamera()->SetFarPlane(4200.0f);
	//world->GetMainCamera()->SetPitch(-90.0f);
	//world->GetMainCamera()->SetYaw(0.0f);
	//world->GetMainCamera()->SetPosition(Vector3(100,2000,0));
}







void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();
	//Coursework One
	InitCourt();
	
	//Tutorial Origin
	//InitCubeGridWorld(5, 5, 50.0f, 50.0f, Vector3(10, 10, 10));

	
	
	//InitSphereGridWorld(10, 10, 50.0f, 50.0f, 10.0f);

	//InitSphereGridWorld(w, 10, 10, 50.0f, 50.0f, 10.0f);

	//InitSphereGridWorld(w, 1, 1, 50.0f, 50.0f, 10.0f);
	//InitCubeGridWorld(w,1, 1, 50.0f, 50.0f, Vector3(10, 10, 10));
	//InitCubeGridWorld(w, 1, 1, 50.0f, 50.0f, Vector3(8, 8, 8));

	//InitSphereCollisionTorqueTest(w);
	//InitCubeCollisionTorqueTest(w);

	//InitSphereGridWorld(w, 1, 1, 50.0f, 50.0f, 10.0f);
	//BridgeConstraintTest(w);
	//InitGJKWorld(w);

	//DodgyRaycastTest(w);
	//InitGJKWorld(w);
	//InitSphereAABBTest(w);
	//SimpleGJKTest(w);
	//SimpleAABBTest2(w);

	//InitSphereCollisionTorqueTest(w);
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject();

	Vector3 floorSize = Vector3(1000, 10, 1000);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform().SetWorldScale(floorSize);
	floor->GetTransform().SetWorldPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, floorTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	floor->SetName("floor");

	world->AddGameObject(floor);

	return floor;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass) {
	GameObject* sphere = new GameObject();

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);
	sphere->GetTransform().SetWorldScale(sphereSize);
	sphere->GetTransform().SetWorldPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, ballTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);
	

	return sphere;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject();

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform().SetWorldPosition(position);
	cube->GetTransform().SetWorldScale(dimensions);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

GameObject * NCL::CSC8503::TutorialGame::AddEnemyToWorld(const Vector3 & position, float radius, float inverseMass)
{
	GameObject* sphere = new GameObject();

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);
	sphere->GetTransform().SetWorldScale(sphereSize);
	sphere->GetTransform().SetWorldPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, enemyTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();

	sphere->SetName("enemy");
	world->SetEnemy(sphere);
	world->AddGameObject(sphere);


	return sphere;
}

GameObject* TutorialGame::AddTerrainToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject();

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform().SetWorldPosition(position);
	cube->GetTransform().SetWorldScale(dimensions);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, terrainTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	
	cube->SetName("terrain");

	world->AddGameObject(cube);

	return cube;
}

GameObject * NCL::CSC8503::TutorialGame::AddObstacleToWorld(const Vector3 & position, Vector3 dimensions, float inverseMass)
{
	GameObject* cube = new GameObject();

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform().SetWorldPosition(position);
	cube->GetTransform().SetWorldScale(dimensions);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, obsTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	cube->SetName("wall");

	world->AddGameObject(cube);

	return cube;
}

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, radius, z * rowSpacing);
			AddSphereToWorld(position, radius);
		}
	}
}


void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 10.0f;
	Vector3 cubeDims = Vector3(10, 10, 10);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, cubeDims.y, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
			}
			else {
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}
	AddFloorToWorld(Vector3(0, -100, 0));
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, cubeDims.y, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(10, -100, 1));
}

void TutorialGame::InitTerrain(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3 & cubeDims)
{
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			//if (x == 1 && z == 5) continue;
			Vector3 position = Vector3(x * colSpacing, cubeDims.y, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(10, -100, 1));
}


void TutorialGame::InitCourt() {

	//Terrain
	
		float holeSize = 25.0f;
		float depth = holeSize;
		float blockALong = 1000.0f;
		int unit = (blockALong / holeSize);
		int holePosUnit = 10;
		float blockAWidth = (blockALong - holeSize) *0.5f;
		float blockBLong = holeSize * holePosUnit;
		float blockCLong = holeSize * (unit - holePosUnit - 1);
		float blockBWidth = holeSize;
		float blockCWidth = holeSize;
		Vector3 blockA_Dims = Vector3(blockALong, depth, blockAWidth);
		Vector3 blockB_Dims = Vector3(blockBLong, depth, blockBWidth);
		Vector3 blockC_Dims = Vector3(blockCLong, depth, blockCWidth);

		Vector3 pos1(0, depth, holeSize + blockAWidth);
		Vector3 pos2(0, depth, -(holeSize + blockAWidth));
		Vector3 pos3((unit - holePosUnit)*holeSize, depth, 0);
		Vector3 pos4((-holePosUnit - 1)*holeSize, depth, 0);

		AddTerrainToWorld(pos1, blockA_Dims, 0.0f);
		AddTerrainToWorld(pos2, blockA_Dims, 0.0f);
		AddTerrainToWorld(pos3, blockB_Dims, 0.0f);
		AddTerrainToWorld(pos4, blockC_Dims, 0.0f);
	

	//Edge
	
		Vector3 EA_Dims = Vector3(1000, holeSize*3, 25);
		Vector3 EB_Dims = Vector3(25, holeSize*3, 950);

		Vector3 posA(0, depth*5, 975);
		Vector3 posB(0, depth*5,-975);
		Vector3 posC(975, depth * 5, 0);
		Vector3 posD(-975, depth * 5, 0);

		AddObstacleToWorld(posA, EA_Dims, 0.0f);
		AddObstacleToWorld(posB, EA_Dims, 0.0f);
		AddObstacleToWorld(posC, EB_Dims, 0.0f);
		AddObstacleToWorld(posD, EB_Dims, 0.0f);

	//Obstacle
		Vector3 OBS_LeftRight = Vector3(675, holeSize * 3, 25);
		Vector3 OBS_Top = Vector3(25, holeSize * 3, 675);
		//Vector3 OBS_Door = Vector3(25, holeSize * 3, 200);
		Vector3 OBS_MoveWall = Vector3(25, holeSize * 3, 400);


		Vector3 posR(0, depth * 5, 650);
		Vector3 posL(0, depth * 5, -650);
		Vector3 posT(700, depth * 5, 0);
		Vector3 posRdoor(400, depth * 5, -300);
		Vector3 posLdoor(400, depth * 5, 300);
		Vector3 posMoveWall_A(-400, depth * 5, 0);
		Vector3 posMoveWall_B(200, depth * 5, 0);
		

		AddObstacleToWorld(posR, OBS_LeftRight, 0.0f);
		AddObstacleToWorld(posL, OBS_LeftRight, 0.0f);
		AddObstacleToWorld(posT, OBS_Top, 0.0f);
		//AddObstacleToWorld(posRdoor, OBS_Door, 0.0f);
		//AddObstacleToWorld(posLdoor, OBS_Door, 0.0f);
		world->SetObsMoveWall(AddObstacleToWorld(posMoveWall_A, OBS_MoveWall,0.0f),0);
		world->SetObsMoveWall(AddObstacleToWorld(posMoveWall_B, OBS_MoveWall, 0.0f), 1);





	//PlayerBall
		Vector3 ballPos(-100, depth * 5, 0);
		float radius = (20.0f);
		world->SetPlayer(AddSphereToWorld(ballPos, radius,0.05f));
		world->GetPlayer()->SetName("player");

		Vector3 playerPos = world->GetPlayer()->GetTransform().GetWorldPosition();
		Vector3 newCamPos = Vector3(playerPos.x, playerPos.y+500, playerPos.z+500);
		world->GetMainCamera()->SetPosition(newCamPos);

	//EnemyCube
		Vector3 ePos(0, depth * 5, 0);
		float eRadius = 20.0f;
		world->SetEnemy(AddEnemyToWorld(ePos, eRadius, 1.05f));
		
		

	

	//Floor
	AddFloorToWorld(Vector3(10, -10, 1));


}



void TutorialGame::InitSphereCollisionTorqueTest() {
	AddSphereToWorld(Vector3(15, 0, 0), 10.0f);
	AddSphereToWorld(Vector3(-25, 0, 0), 10.0f);
	AddSphereToWorld(Vector3(-50, 0, 0), 10.0f);

	AddCubeToWorld(Vector3(-50, 0, -50), Vector3(60, 10, 10), 10.0f);

	AddFloorToWorld(Vector3(0, -100, 0));
}


void TutorialGame::InitCubeCollisionTorqueTest() {
	Vector3 cubeSize(10, 10, 10);
	AddCubeToWorld(Vector3(15, 0, 0), cubeSize, 10.0f);
	AddCubeToWorld(Vector3(-25, 0, 0), cubeSize, 10.0f);
	AddCubeToWorld(Vector3(-50, 0, 0), cubeSize, 10.0f);

	AddCubeToWorld(Vector3(-50, 0, -50), Vector3(60, 10, 10), 10.0f);

	AddFloorToWorld(Vector3(0, -100, 0));
}

void TutorialGame::InitSphereAABBTest() {
	Vector3 cubeSize(10, 10, 10);

	AddCubeToWorld(Vector3(0, 0, 0), cubeSize, 10.0f);
	AddSphereToWorld(Vector3(2, 0, 0), 5.0f, 10.0f);
}
						
void TutorialGame::InitGJKWorld() {
	Vector3 dimensions(20, 2, 10);
	float inverseMass = 10.0f;

	for (int i = 0; i < 2; ++i) {
		GameObject* cube = new GameObject();

		OBBVolume* volume = new OBBVolume(dimensions);

		cube->SetBoundingVolume((CollisionVolume*)volume);

		cube->GetTransform().SetWorldPosition(Vector3(0, 0, 0));
		cube->GetTransform().SetWorldScale(dimensions);

		if (i == 1) {
			cube->GetTransform().SetLocalOrientation(Quaternion::AxisAngleToQuaterion(Vector3(1, 0, 0), 90.0f));
		}

		cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
		cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

		cube->GetPhysicsObject()->SetInverseMass(inverseMass);
		cube->GetPhysicsObject()->InitCubeInertia();

		world->AddGameObject(cube);
	}
}

void TutorialGame::BridgeConstraintTest() {
	float sizeMultiplier = 1.0f;

	Vector3 cubeSize = Vector3(8, 8, 8) * sizeMultiplier;

	int numLinks = 5;

	GameObject* start = AddCubeToWorld(Vector3(0, 0, 0), cubeSize, 0);

	GameObject* end = AddCubeToWorld(Vector3((numLinks + 2) * 20 * sizeMultiplier, 0, 0), cubeSize, 0);

	GameObject* previous = start;

	for (int i = 0; i < numLinks; ++i) {
		GameObject* block = AddCubeToWorld(Vector3((i + 1) * 20 * sizeMultiplier, 0, 0), cubeSize, 10.0f);
		PositionConstraint* constraint = new PositionConstraint(previous, block, 30.0f);
		world->AddConstraint(constraint);
		previous = block;
	}

	PositionConstraint* constraint = new PositionConstraint(previous, end, 30.0f);
	world->AddConstraint(constraint);
}

void TutorialGame::SimpleGJKTest() {
	Vector3 dimensions		= Vector3(5, 5, 5);
	Vector3 floorDimensions = Vector3(100, 2, 100);

	GameObject* fallingCube = AddCubeToWorld(Vector3(0, 20, 0), dimensions, 10.0f);
	GameObject* newFloor	= AddCubeToWorld(Vector3(0, 0, 0), floorDimensions, 0.0f);

	delete fallingCube->GetBoundingVolume();
	delete newFloor->GetBoundingVolume();

	fallingCube->SetBoundingVolume((CollisionVolume*)new OBBVolume(dimensions));
	newFloor->SetBoundingVolume((CollisionVolume*)new OBBVolume(floorDimensions));

}

void TutorialGame::SimpleAABBTest() {
	Vector3 dimensions		= Vector3(5, 5, 5);
	Vector3 floorDimensions = Vector3(100, 2, 100);

	GameObject* newFloor	= AddCubeToWorld(Vector3(0, 0, 0), floorDimensions, 0.0f);
	GameObject* fallingCube = AddCubeToWorld(Vector3(10, 20, 0), dimensions, 10.0f);
}

void TutorialGame::SimpleAABBTest2() {
	Vector3 dimensions		= Vector3(5, 5, 5);
	Vector3 floorDimensions = Vector3(8, 2, 8);

	GameObject* newFloor	= AddCubeToWorld(Vector3(0, 0, 0), floorDimensions, 0.0f);
	GameObject* fallingCube = AddCubeToWorld(Vector3(8, 20, 0), dimensions, 10.0f);
}

void NCL::CSC8503::TutorialGame::CamFollow(Camera * c, GameObject * obj)
{
	Vector3 playerPos = obj->GetTransform().GetWorldPosition();
	Vector3 newCamPos = Vector3(playerPos.x, playerPos.y + 500, playerPos.z + 500);
	c->SetPosition(newCamPos);
}

void NCL::CSC8503::TutorialGame::FSM_MoveWall(int  &time, GameWorld * g)
{

	StateFunc AFunc = [](void * data) {
		int* realData = (int *)data;
		(*realData)++;
		//std::cout << "In State A!" << std::endl;
		//std::cout << *realData << std::endl;

	};
	StateFunc BFunc = [](void * data) {
		int * realData = (int *)data;
		(*realData)--;
		//std::cout << "In State B!" << std::endl;
		//std::cout << *realData << std::endl;


	};

	GenericState * stateA = new GenericState(AFunc, (void *)&time);
	GenericState * stateB = new GenericState(BFunc, (void *)&time);
	testMachine->AddState(stateA);
	testMachine->AddState(stateB);

	GenericTransition <int &, int >* transitionA =
		new GenericTransition <int &, int >(
			GenericTransition <int &, int >::GreaterThanTransition, time, 100, stateA, stateB); // if greater than 100 A to B

	GenericTransition <int &, int >* transitionB =
		new GenericTransition <int &, int >(
			GenericTransition <int &, int >::EqualsTransition, time, 0, stateB, stateA); // if equals 0, B to A

	testMachine->AddTransition(transitionA);
	testMachine->AddTransition(transitionB);

	testMachine->Update(); // run the state machine !

	if (time == 0 || time == 101) wallMoveDir *= -1;

	g->GetMoveWall(0)->GetPhysicsObject()->SetLinearVelocity(Vector3(0.0f, 0.0f, wallMoveDir*80.0f));
	g->GetMoveWall(1)->GetPhysicsObject()->SetLinearVelocity(Vector3(0.0f, 0.0f, -wallMoveDir * 80.0f));
}

void NCL::CSC8503::TutorialGame::Enemy_Chase(GameObject * enemy, GameObject * player)
{
	
	

	Vector3 ePos = enemy->GetTransform().GetWorldPosition();
	Vector3 pPos = player->GetTransform().GetWorldPosition();
	Vector3 eVdir = pPos - ePos;
	Vector3 eVdirN = eVdir.Normalised();

	if((int)world->myTimer%1==0)
	{
		rand_int_x = 0+ rand() % 360;
		//rand_int_z = -1 + rand() % 2;
	}

	//if ((int)world->myTimer % 2 == 0)
	//{
	//	rand_int_z = -100 + rand() % 200;
	//	//rand_int_z = -1 + rand() % 2;
	//}


	//std::cout << rand_int << std::endl;
	//std::cout << (int)world->myTimer << std::endl;

	if (eVdir.Length() < 400.0f) {

		enemy->GetPhysicsObject()->AddForceAtPosition(eVdirN*5.0f,ePos);
	}

	
	


	else
	{
		
			
			//std::cout << rand_int_x << std::endl;
			Vector3 surroundPoint(Vector3(ePos.x + cos(rand_int_x), ePos.y, ePos.z + sin(rand_int_x)));
			Vector3 patrollDir = surroundPoint - ePos;
			Vector3 normal = patrollDir.Normalised();
			enemy->GetPhysicsObject()->AddForceAtPosition(normal*100.0f,ePos);
			
		

		//enemy->GetPhysicsObject()->SetLinearVelocity(eVdir*5.0f);
	}
}

/*

Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be 
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around. 

*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		renderer->DrawString("Press Q to change to camera mode!", Vector2(10, 0));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::MOUSE_LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				selectionObject = nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				
			    
				selectionObject = (GameObject*)closestCollision.node;


				if (selectionObject->GetName() == "floor" 
					|| selectionObject->GetName()== "terrain" 
					|| selectionObject->GetName() == "wall"
					|| selectionObject->GetName() == "enemy") return false;

				selectionObject->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));
				return true;
			}
			else {
				return false;
			}
		}
	}
	else {
		renderer->DrawString("Press Q to change to select mode!", Vector2(10, 0));
	}
	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/

void TutorialGame::MoveSelectedObject() {
	renderer->DrawString("Click Force:" + std::to_string(forceMagnitude), Vector2(10, 20));
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 10.0f;

	
	Vector3 forceRayDir;
	Vector3 playerPos(0, 0, 0);
	if(world->GetPlayer()!=NULL)
	Vector3 playerPos = world->GetPlayer()->GetTransform().GetWorldPosition();
	
	
	

	if (!selectionObject) {
		return;//we haven't selected anything!
	}
	//Push the selected object!
	if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::MOUSE_LEFT))
	{
		Ray rayFromScreen = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
		RayCollision closestCollisionFromScreen;
		if (world->Raycast(rayFromScreen, closestCollisionFromScreen, true)) {
			forcePos = closestCollisionFromScreen.collidedAt;
		}
	}


	if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::MOUSE_RIGHT))
	{
		
		//
		Ray rayToFloor = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
		
		RayCollision closestCollisionToFloor;
		if (world->Raycast(rayToFloor, closestCollisionToFloor, true)) {
			
			Vector3 colliidedAt = closestCollisionToFloor.collidedAt;
			axisYoffset = 0.0f;
			dirPoint=Vector3(colliidedAt.x, colliidedAt.y + axisYoffset, colliidedAt.z);
			Debug::DrawLine(forcePos,dirPoint, Vector4(1, 1, 1, 1));
			Ray forceRay(dirPoint, forcePos-dirPoint);
			forceRayDir = forceRay.GetDirection();
		
		}
		
	}

	
    if (Window::GetKeyboard()->KeyPressed(KEYBOARD_P)) {
	//world->GetPlayer()->GetPhysicsObject()->AddForce(-forceRayDir * forceMagnitude);
		
	world->GetPlayer()->GetPhysicsObject()->AddForceAtPosition(forceRayDir * forceMagnitude, forcePos);
	}



	



	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::MOUSE_RIGHT)) {

		//
		//Vector3 playerPos = world->GetPlayer()->GetTransform().GetWorldPosition();

		//Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

		//RayCollision closestCollision;




		//if (world->Raycast(ray, closestCollision, true)) {
		//	if (closestCollision.node == selectionObject) {
		//		//selectionObject->GetPhysicsObject()-> AddForce(ray.GetDirection() * forceMagnitude);
		//		selectionObject ->GetPhysicsObject()->AddForceAtPosition(  ray.GetDirection() * forceMagnitude ,  closestCollision.collidedAt); 
		//	}
		//}
	}
}




