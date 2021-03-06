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
	useGravity = false;
	inSelectionMode = false;
	GameRunning = false;

	

	Debug::SetRenderer(renderer);

	InitialiseAssets();
	
}


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
	InitNetWork();
	InitWorld();


	
	
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

	if (!inSelectionMode&&GameRunning) {
		world->GetMainCamera()->UpdateCamera(dt);
	}

	UpdateKeys();
	UI();
    ScoreCalculating();

	
	CamFollow(world->GetMainCamera(), world->GetPlayer());
	if (GameRunning)
	{
		TimeCalculating(dt);
		UpdateEnemy();
		UpdateWall();
		SelectObject();
		MoveSelectedObject();
	}
	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);

	Debug::FlushRenderables();
	renderer->Render();

	LevelSwitch();
}

void TutorialGame::UpdateKeys() {

	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_O)) {
		if (!GameRunning) GameRunning = true;
		else GameRunning = false;
	}
	
	
	
	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_R)) {
		//InitialiseAssets();
		InitWorld(); //We can reset the simulation at any time with F1
		
		selectionObject = nullptr;
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


}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();
	//Coursework One
	InitCourt();
	

}

void NCL::CSC8503::TutorialGame::UI()
{

	if (GameRunning)
	{

		Debug::Print("Level:" + std::to_string(myLevel), Vector2(10, 600));
		Debug::Print("Press 'O' to Pause Game", Vector2(10, 130));
		Debug::Print("High Score:" + std::to_string(bestScore), Vector2(10, 100));
		Debug::Print("Current Score:" + std::to_string(score), Vector2(10, 70));

		if (useGravity) {
			Debug::Print("(G)ravity on", Vector2(10, 40));
		}
		else

		{
			Debug::Print("(G)ravity off", Vector2(10, 40));
		}
	}
	else
	{
		Debug::Print("Press 'O' to Start or Resume  ", Vector2(0, 300));
		Debug::Print("Click Left Button on the ball ", Vector2(0, 200));
		Debug::Print("Keep Pressing Right Button to on the floor ", Vector2(0, 100));
		Debug::Print("Then Press 'P' to Add Force", Vector2(0, 0));
	}
}

void NCL::CSC8503::TutorialGame::ScoreCalculating()
{
	if (score > 0)
	{
		score = 300 - world->myTimer;

	}
}

void NCL::CSC8503::TutorialGame::TimeCalculating(float dt)
{

	if (!ifWin)
		world->myTimer += dt;

}

void NCL::CSC8503::TutorialGame::LevelSwitch()
{
	if (ifGoToLevel_2 == true)
	{
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
		//world->myTimer = 0;
		ifGoToLevel_2 = false;


	}

	if (physics->enemyTouchFloor == true)
	{

		physics->enemyTouchFloor = false;
		world->GetEnemy()->GetTransform().SetWorldPosition(Vector3(0, 100, 0));
	}


	if (physics->isTouchFloor == true) {
		if (myLevel == 1)
		{
			myLevel = 2;
			physics->isTouchFloor = false;
			ifGoToLevel_2 = true;
			return;
		}

		if (myLevel == 2)
		{


			Debug::Print("You are Win", Vector2(400, 300));
			ifWin = true;
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
	}
}

void NCL::CSC8503::TutorialGame::UpdateEnemy()
{
	if (world->GetEnemy())
	{
		if (physics->getGravityState() && world->myTimer > 3 && !ifWin)
		{
			Enemy_Chase(world->GetEnemy(), world->GetPlayer());
			//cout << world->GetEnemy()->GetTransform().GetWorldPosition() << endl;
			Vector3 ePos = world->GetEnemy()->GetTransform().GetWorldPosition();
			
			server->SendGlobalMessage(StringPacket("Enemy Position:(" + to_string((int)ePos.x) + "," + to_string((int)ePos.y) + "," + to_string((int)ePos.z) + ")"));
			
			server->UpdateServer();

			client->UpdateClient();

		}
	}
}

void NCL::CSC8503::TutorialGame::UpdateWall()
{
	if (world->GetMoveWall(0) && world->GetMoveWall(1))
		FSM_MoveWall(mytime, world);
}


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

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, obsTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);
	cube->SetName("cube");
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

	if (myLevel == 1)
	{
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

		Vector3 EA_Dims = Vector3(1000, holeSize * 3, 25);
		Vector3 EB_Dims = Vector3(25, holeSize * 3, 950);

		Vector3 posA(0, depth * 5, 975);
		Vector3 posB(0, depth * 5, -975);
		Vector3 posC(975, depth * 5, 0);
		Vector3 posD(-975, depth * 5, 0);

		AddObstacleToWorld(posA, EA_Dims, 0.0f);
		AddObstacleToWorld(posB, EA_Dims, 0.0f);
		AddObstacleToWorld(posC, EB_Dims, 0.0f);
		AddObstacleToWorld(posD, EB_Dims, 0.0f);

		//PlayerBall
		Vector3 ballPos(-100, depth * 5, 0);
		float radius = (20.0f);
		world->SetPlayer(AddSphereToWorld(ballPos, radius, 0.05f));
		world->GetPlayer()->SetName("player");

		Vector3 playerPos = world->GetPlayer()->GetTransform().GetWorldPosition();
		Vector3 newCamPos = Vector3(playerPos.x, playerPos.y + 500, playerPos.z + 500);
		world->GetMainCamera()->SetPosition(newCamPos);
		//Floor
		AddFloorToWorld(Vector3(10, -10, 1));

		BridgeConstraintTest();
	}

	if (myLevel == 2)
	{

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

		Vector3 EA_Dims = Vector3(1000, holeSize * 3, 25);
		Vector3 EB_Dims = Vector3(25, holeSize * 3, 950);

		Vector3 posA(0, depth * 5, 975);
		Vector3 posB(0, depth * 5, -975);
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
		world->SetObsMoveWall(AddObstacleToWorld(posMoveWall_A, OBS_MoveWall, 0.0f), 0);
		world->SetObsMoveWall(AddObstacleToWorld(posMoveWall_B, OBS_MoveWall, 0.0f), 1);

		//EnemyCube
		Vector3 ePos(0, depth * 2, 0);
		float eRadius = 20.0f;
		world->SetEnemy(AddEnemyToWorld(ePos, eRadius, 1.05f));


		//PlayerBall
		Vector3 ballPos(-500, depth * 5, 0);
		float radius = (20.0f);
		world->SetPlayer(AddSphereToWorld(ballPos, radius, 0.05f));
		world->GetPlayer()->SetName("player");

		Vector3 playerPos = world->GetPlayer()->GetTransform().GetWorldPosition();
		Vector3 newCamPos = Vector3(playerPos.x, playerPos.y + 500, playerPos.z + 500);
		world->GetMainCamera()->SetPosition(newCamPos);

		//Floor
		AddFloorToWorld(Vector3(10, -10, 1));
	}
}


void NCL::CSC8503::TutorialGame::CamFollow(Camera * c, GameObject * obj)
{
	Vector3 playerPos = obj->GetTransform().GetWorldPosition();
	//Vector3 newCamPos = Vector3(playerPos.x, playerPos.y + 500, playerPos.z + 500);
	Vector3 newCamPos = Vector3(playerPos.x, playerPos.y + 500, playerPos.z+500);
	c->SetPosition(newCamPos);


	
		


}

void NCL::CSC8503::TutorialGame::FSM_MoveWall(int  &time, GameWorld * g)
{

	StateFunc AFunc = [](void * data) {
		int* realData = (int *)data;
		(*realData)++;


	};
	StateFunc BFunc = [](void * data) {
		int * realData = (int *)data;
		(*realData)--;
	


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
	
	if ((int)world->myTimer % 1 == 0)
		rand_int_x = 0 + rand() % 360;
	GameObject* obj=nullptr;
	Vector3 ePos = enemy->GetTransform().GetWorldPosition();
	Vector3 pPos = player->GetTransform().GetWorldPosition();
	Vector3 eEyePos(ePos.x,ePos.y+100,ePos.z);
	Vector3 pEyePos(pPos.x, pPos.y, pPos.z);
	Vector3 eVdir = pPos - ePos;
	Vector3 eVdirN = eVdir.Normalised();
	Ray ray_EP(eEyePos,(pEyePos-eEyePos).Normalised());
	//Debug::DrawLine(eEyePos, pEyePos, Vector4(1, 0, 1, 1));

	RayCollision closestCollision;
	if (world->Raycast(ray_EP, closestCollision, true)) {

		 obj = (GameObject * )closestCollision.node;
		 //std::cout << obj->GetName() << std::endl;
		 if (obj&&obj->GetName() != "wall") {
			 //std::cout << "I can see u" << std::endl;
			 enemy->GetPhysicsObject()->AddForceAtPosition(eVdirN*5.0f, ePos);
		 }


		 else
		 {
			 //std::cout << "go arround" << std::endl;
			 Vector3 surroundPoint(Vector3(ePos.x + cos(rand_int_x), ePos.y, ePos.z + sin(rand_int_x)));
			 Vector3 patrollDir = surroundPoint - ePos;
			 Vector3 normal = patrollDir.Normalised();
			 enemy->GetPhysicsObject()->AddForceAtPosition(normal*100.0f, ePos);
		 }

	}

	




	

	
	
}

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
	if (inSelectionMode&&GameRunning) {
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
					|| selectionObject->GetName() == "enemy"
					|| selectionObject->GetName() == "cube") return false;

				selectionObject->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));
				return true;
			}
			else {
				return false;
			}
		}
	}
	else {
		if(GameRunning)
		renderer->DrawString("Press Q to change to select mode!", Vector2(10, 0));
	}
	return false;
}


void TutorialGame::MoveSelectedObject() {
	if (GameRunning)
	{
		renderer->DrawString("Click Force:" + std::to_string(forceMagnitude), Vector2(10, 20));
		forceMagnitude += Window::GetMouse()->GetWheelMovement() * 10.0f;
	}

	
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

void NCL::CSC8503::TutorialGame::BridgeConstraintTest()
{
	Vector3 cubeSize = Vector3(15, 15, 800); 
	float invCubeMass = 0; //how heavy the middle pieces are 
	int numLinks = 5; 
	float maxDistance = 70; //constraint distance
	float cubeDistance = 40; //distance between links
	Vector3 startPos = Vector3(100, 25, 0); 
	GameObject* start = AddCubeToWorld(startPos + Vector3(0, 0, 0) , cubeSize , 0); 
	GameObject* end = AddCubeToWorld(startPos + Vector3((numLinks + 1)  * cubeDistance , 0, 0), cubeSize , 0);
	GameObject* previous = start; 
	for (int i = 0; i < numLinks; ++i) { 
		GameObject* block = AddCubeToWorld(startPos + Vector3((i+1) * cubeDistance , i*25, 0), cubeSize , invCubeMass); 
		PositionConstraint* constraint =new PositionConstraint(previous ,  block , maxDistance); 
		world ->AddConstraint(constraint); 
		previous = block; 
	} 
	PositionConstraint* constraint = new PositionConstraint(previous ,  end, maxDistance); 
	world ->AddConstraint(constraint); 
}




