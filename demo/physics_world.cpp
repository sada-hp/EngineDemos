#include "physics_world.hpp"
#include "Vulkan/renderer.hpp"

namespace GR
{
	PhysicsWorld::PhysicsWorld(const Renderer& Context)
		: World(Context)
	{
		m_Broadphase = new btDbvtBroadphase;
		m_Solver = new btSequentialImpulseConstraintSolver;
		m_CollisionConfiguration = new btDefaultCollisionConfiguration;
		m_Dispatcher = new btCollisionDispatcher(m_CollisionConfiguration);
		m_DynamicsWorld = new btDiscreteDynamicsWorld(m_Dispatcher, m_Broadphase, m_Solver, m_CollisionConfiguration);

		_addPlanet(Renderer::Rg, btVector3(0.0, -Renderer::Rg, 0.0));
	}

	PhysicsWorld::~PhysicsWorld()
	{
		Clear();
		_clearPlanet();

		delete m_DynamicsWorld;
		delete m_Dispatcher;

		delete m_CollisionConfiguration;
		delete m_Broadphase;
		delete m_Solver;
	}

	Entity PhysicsWorld::AddShape(const Shapes::Shape& Descriptor)
	{
		Entity ent = World::AddShape(Descriptor);

		btScalar mass = 0.0;
		btScalar rollFriction = 0.0;
		btCollisionShape* colShape = nullptr;
		glm::vec3 extents = Descriptor.GetDimensions();
		if (dynamic_cast<const Shapes::Sphere*>(&Descriptor))
		{
			rollFriction = 0.25;
			colShape = new btSphereShape(btScalar(extents.x));
			mass = Descriptor.GetDimensions().x * Descriptor.GetDimensions().y * 0.5;
		}
		else if (dynamic_cast<const Shapes::Cube*>(&Descriptor))
		{
			rollFriction = 1.0;
			colShape = new btBoxShape(btVector3(extents.x, extents.y, extents.z));
			mass = Descriptor.GetDimensions().x* Descriptor.GetDimensions().y;
		}
		else
		{
			rollFriction = 1.0;
			colShape = new btBoxShape(btVector3(5.0, 5.0, 5.0));
			mass = 25.0;
		}

		btScalar damping = glm::min(rollFriction * mass * 0.01, 0.25);
		btTransform startTransform;
		startTransform.setIdentity();

		btVector3 localInertia(0.0, 0.0, 0.0);
		colShape->calculateLocalInertia(mass, localInertia);

		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		body->setSleepingThresholds(0.05, 0.05);
		body->setRollingFriction(rollFriction);
		body->setDamping(damping, damping);
		body->setUserIndex(int(ent));

		m_CollisionShapes.push_back(colShape);
		Registry.emplace<Components::Body>(ent, body);
		Registry.emplace<Components::Mass>(ent, mass);
		m_DynamicsWorld->addRigidBody(body);

		return ent;
	}

	RayCastResult PhysicsWorld::FirstAtRay(glm::vec3 Origin, glm::vec3 Direction, double Distance) const
	{
		RayCastResult res{};
		btVector3 toWorld = btVector3(Origin.x, Origin.y, Origin.z);
		btVector3 fromWorld = toWorld + btVector3(Direction.x * Distance, Direction.y * Distance, Direction.z * Distance);
		btCollisionWorld::ClosestRayResultCallback callback(toWorld, fromWorld);

		m_DynamicsWorld->rayTest(toWorld, fromWorld, callback);

		if (callback.hasHit())
		{
			res.hitPos = glm::vec3(callback.m_hitPointWorld.x(), callback.m_hitPointWorld.y(), callback.m_hitPointWorld.z());
			res.hitNormal = glm::vec3(callback.m_hitNormalWorld.x(), callback.m_hitNormalWorld.y(), callback.m_hitNormalWorld.z());
			res.id = Entity(callback.m_collisionObject->getUserIndex());
		}

		return res;
	}

	void PhysicsWorld::ResetObject(Entity object)
	{
		Components::WorldMatrix& transform = GetComponent<Components::WorldMatrix>(object);
		btRigidBody* body = GetComponent<Components::Body>(object).body;

		btTransform physTransform;
		physTransform.setFromOpenGLMatrix(glm::value_ptr(transform.matrix));
		body->setWorldTransform(physTransform);
		body->clearForces();
		body->clearGravity();
		body->activate(true);
	}

	void PhysicsWorld::FreezeObject(Entity object)
	{
		btRigidBody* body = GetComponent<Components::Body>(object).body;
		body->clearForces();
		body->clearGravity();
		body->forceActivationState(0);
	}

	void PhysicsWorld::DrawScene(double Delta)
	{
		constexpr double fixedStep = 1.0 / 60.0;
		constexpr double rSq = Renderer::Rg * Renderer::Rg;
		constexpr double rSqInv = 1.0 / rSq;
		constexpr double Me = rSq * 0.1;

		auto view = Registry.view<Components::Body, Components::WorldMatrix>();
		for (const auto& [ent, body, transform] : view.each())
		{
			if (body.body->getActivationState() == ACTIVE_TAG)
			{
				const double Fg = glm::sqrt(body.body->getMass()) * gravity; // made up
				body.body->getWorldTransform().getOpenGLMatrix(glm::value_ptr(transform.matrix));
				body.body->setGravity((body.body->getWorldTransform().getOrigin() + btVector3(0.0, Renderer::Rg, 0.0)).normalized() * Fg);
			}
		}
		m_DynamicsWorld->stepSimulation(Delta, 10, fixedStep);

		World::DrawScene(Delta);
	}

	void PhysicsWorld::Clear()
	{
		for (int i = 1; i < m_DynamicsWorld->getNumCollisionObjects(); ++i)
		{
			btCollisionObject* obj = m_DynamicsWorld->getCollisionObjectArray()[i];
			btRigidBody* body = btRigidBody::upcast(obj);
			if (body && body->getMotionState())
			{
				delete body->getMotionState();
			}

			m_DynamicsWorld->removeCollisionObject(obj);
			delete obj;
		}

		for (int i = 1; i < m_CollisionShapes.size(); ++i)
		{
			delete m_CollisionShapes[i];
		}
		m_CollisionShapes.resize(1);

		World::Clear();
	}

	void PhysicsWorld::_addPlanet(double r, btVector3 origin)
	{
		btCollisionShape* colShape = new btSphereShape(btScalar(r));
		m_CollisionShapes.push_back(colShape);

		btScalar mass(0.0);
		btTransform startTransform;
		btVector3 localInertia(0.0, 0.0, 0.0);
		startTransform.setIdentity();
		startTransform.setOrigin(origin);

		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		m_DynamicsWorld->addRigidBody(body);
	}

	void PhysicsWorld::_clearPlanet()
	{
		btCollisionObject* obj = m_DynamicsWorld->getCollisionObjectArray()[0];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState())
		{
			delete body->getMotionState();
		}

		m_DynamicsWorld->removeCollisionObject(obj);
		delete obj;
		delete m_CollisionShapes[0];
		m_CollisionShapes.removeAtIndex(0);
	}
};