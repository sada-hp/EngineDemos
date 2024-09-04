#pragma once
#include "Engine/world.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <btBulletDynamicsCommon.h>
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "BulletDynamics/Character/btKinematicCharacterController.h"
#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "BulletCollision/CollisionShapes/btShapeHull.h"

namespace GR
{
	namespace Components
	{
		struct Body
		{
			btRigidBody* body;
		};

		struct Mass
		{
			float mass = 0.f;
		};
	};

	struct RayCastResult
	{
		glm::vec3 hitPos = glm::vec3(0.0);
		glm::vec3 hitNormal = glm::vec3(0.0);
		Entity id = Entity(-1);
	};

	class PhysicsWorld : public World
	{
	public:
		float gravity = -9.8f;

	public:
		PhysicsWorld(const Renderer& Context);

		virtual ~PhysicsWorld();

		Entity AddShape(const Shapes::Shape& Descriptor) override;

		RayCastResult FirstAtRay(glm::vec3 Origin, glm::vec3 Direction, double Distance = 1e5) const;

		void ResetObject(Entity object);

		void FreezeObject(Entity object);

		void DrawScene(double Delta) override;

		void Clear() override;

	private:
		void _addPlanet(double r, btVector3 origin);

		void _clearPlanet();

	private:
		btAlignedObjectArray<btCollisionShape*> m_CollisionShapes;
		btDefaultCollisionConfiguration* m_CollisionConfiguration;
		btSequentialImpulseConstraintSolver* m_Solver;
		btDiscreteDynamicsWorld* m_DynamicsWorld;
		btCollisionDispatcher* m_Dispatcher;
		btDbvtBroadphase* m_Broadphase;
	};
};