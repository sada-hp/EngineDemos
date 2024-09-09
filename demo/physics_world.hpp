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

	struct CollisionTest : public btCollisionWorld::ContactResultCallback
	{
		std::vector<RayCastResult>& out;

		CollisionTest(std::vector<RayCastResult>& output)
			: out(output)
		{
		}

		btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override
		{
			RayCastResult r{};
			r.hitPos = glm::vec3(cp.getPositionWorldOnB().x(), cp.getPositionWorldOnB().y(), cp.getPositionWorldOnB().z());
			r.hitNormal = glm::vec3(cp.m_normalWorldOnB.x(), cp.m_normalWorldOnB.y(), cp.m_normalWorldOnB.z());
			r.id = Entity(colObj1Wrap->getCollisionObject()->getUserIndex());
			out.push_back(r);

			return 1;
		}
	};

	struct CollisionTestDeepest : public btCollisionWorld::ContactResultCallback
	{
		RayCastResult& out;
		btScalar d2 = 0.0;
		btScalar d = 0.0;

		CollisionTestDeepest(RayCastResult& output)
			: out(output)
		{
		}

		btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override
		{
			btScalar nd = cp.getPositionWorldOnB().distance2(cp.getPositionWorldOnA());
			if (nd > d2)
			{
				out.hitPos = glm::vec3(cp.getPositionWorldOnB().x(), cp.getPositionWorldOnB().y(), cp.getPositionWorldOnB().z());
				out.hitNormal = glm::vec3(cp.m_normalWorldOnB.x(), cp.m_normalWorldOnB.y(), cp.m_normalWorldOnB.z());
				out.id = Entity(colObj1Wrap->getCollisionObject()->getUserIndex());

				d2 = nd;
				d = glm::sqrt(nd);
			}

			return 1;
		}
	};

	class PhysicsWorld : public World
	{
	public:
		float gravity = -9.8f;

	public:
		PhysicsWorld(const Renderer& Context);

		virtual ~PhysicsWorld();

		Entity AddShape(const Shapes::GeoClipmap& Descriptor) override;

		Entity AddShape(const Shapes::Cube& Descriptor);

		Entity AddShape(const Shapes::Sphere& Descriptor);

		RayCastResult FirstAtRay(glm::vec3 Origin, glm::vec3 Direction, double RayLen = 1e5) const;

		void ObjectContactPoints(Entity object, std::vector<RayCastResult>& out);

		double DeepestContactPoint(Entity object, RayCastResult& out);

		void ResetObject(Entity object);

		void ResetPosition(Entity object);

		void FreezeObject(Entity object);

		void DrawScene(double Delta) override;

		void Clear() override;

	private:
		btAlignedObjectArray<btCollisionShape*> m_CollisionShapes;
		btDefaultCollisionConfiguration* m_CollisionConfiguration;
		btSequentialImpulseConstraintSolver* m_Solver;
		btDiscreteDynamicsWorld* m_DynamicsWorld;
		btCollisionDispatcher* m_Dispatcher;
		btDbvtBroadphase* m_Broadphase;
	};
};