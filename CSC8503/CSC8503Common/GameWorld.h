#pragma once
#include <vector>
#include "Ray.h"
#include "CollisionDetection.h"
#include "QuadTree.h"
namespace NCL {
		class Camera;
		using Maths::Ray;
	namespace CSC8503 {
		class GameObject;
		class Constraint;

		class GameWorld	{
		public:
			GameWorld();
			~GameWorld();

			void Clear();
			void ClearAndErase();

			void AddGameObject(GameObject* o);
			void RemoveGameObject(GameObject* o);

			void AddConstraint(Constraint* c);
			void RemoveConstraint(Constraint* c);

			Camera* GetMainCamera() const {
				return mainCamera;
			}

			GameObject* GetPlayer() const {
				return playerObj;
			}
			void SetPlayer(GameObject* player)
			{
				playerObj = player;

			}
			void SetEnemy(GameObject* enemy)
			{
				enemyObj = enemy;

			}

			GameObject* GetEnemy() const {
				return enemyObj;
			}

			


			GameObject* GetMoveWall(int index ) const {
				return obsMoveWall[index];
			}

			void SetObsMoveWall(GameObject* wall,int index)
			{
				obsMoveWall[index]=wall;

			}


			void ShuffleConstraints(bool state) {
				shuffleConstraints = state;
			}

			void ShuffleObjects(bool state) {
				shuffleObjects = state;
			}

			bool Raycast(Ray& r, RayCollision& closestCollision, bool closestObject = false) const;

			virtual void UpdateWorld(float dt);

			void GetObjectIterators(
				std::vector<GameObject*>::const_iterator& first,
				std::vector<GameObject*>::const_iterator& last) const;

			void GetConstraintIterators(
				std::vector<Constraint*>::const_iterator& first,
				std::vector<Constraint*>::const_iterator& last) const;


			float myTimer = 0.0f;
		protected:
			void UpdateTransforms();
			void UpdateQuadTree();

			std::vector<GameObject*> gameObjects;

			std::vector<Constraint*> constraints;

			QuadTree<GameObject*>* quadTree;

			Camera* mainCamera;

			GameObject* playerObj=NULL;
			GameObject* enemyObj = NULL;

			GameObject* obsMoveWall[2];

			

			bool shuffleConstraints;
			bool shuffleObjects;
		};
	}
}

