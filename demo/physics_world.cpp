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
	}

	PhysicsWorld::~PhysicsWorld()
	{
		Clear();

		delete m_DynamicsWorld;
		delete m_Dispatcher;

		delete m_CollisionConfiguration;
		delete m_Broadphase;
		delete m_Solver;
	}

	Entity PhysicsWorld::AddShape(const Shapes::GeoClipmap& Descriptor)
	{
		double r = Renderer::Rg;
		btVector3 origin = btVector3(0.0, -r, 0.0);
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

		return World::AddShape(Descriptor);
	}

	Entity PhysicsWorld::AddShape(const Shapes::Cube& Descriptor)
	{
		Entity ent = World::AddShape(Descriptor);

		btScalar rollFriction = 1.0;
		glm::vec3 extents = Descriptor.GetDimensions();
		btScalar mass = extents.x * extents.y;
		btCollisionShape* colShape = new btBoxShape(btVector3(extents.x, extents.y, extents.z));

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

	Entity PhysicsWorld::AddShape(const Shapes::Sphere& Descriptor)
	{
		Entity ent = World::AddShape(Descriptor);

		btScalar rollFriction = 0.25;
		glm::vec3 extents = Descriptor.GetDimensions();
		btScalar mass = extents.x * extents.y * 0.5;
		btCollisionShape* colShape = colShape = new btSphereShape(btScalar(extents.x));

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

	void PhysicsWorld::ObjectContactPoints(Entity object, std::vector<RayCastResult>& out)
	{
		btRigidBody* body = GetComponent<Components::Body>(object).body;
		CollisionTest ctAll(out);

		m_DynamicsWorld->getCollisionWorld()->contactTest(body, ctAll);
	}

	double PhysicsWorld::DeepestContactPoint(Entity object, RayCastResult& out)
	{
		btRigidBody* body = GetComponent<Components::Body>(object).body;
		CollisionTestDeepest ctDeep(out);

		m_DynamicsWorld->getCollisionWorld()->contactTest(body, ctDeep);

		return ctDeep.d;
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
		body->setLinearVelocity(btVector3(0.0, 0.0, 0.0));
		body->setAngularVelocity(btVector3(0.0, 0.0, 0.0));

		btVector3 localInertia(0.0, 0.0, 0.0);
		body->getCollisionShape()->calculateLocalInertia(body->getMass(), localInertia);
		body->setMassProps(body->getMass(), localInertia);
	}

	void PhysicsWorld::ResetPosition(Entity object)
	{
		Components::WorldMatrix& transform = GetComponent<Components::WorldMatrix>(object);
		btRigidBody* body = GetComponent<Components::Body>(object).body;

		btTransform physTransform;
		physTransform.setFromOpenGLMatrix(glm::value_ptr(transform.matrix));
		body->setWorldTransform(physTransform);
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
		for (int i = 0; i < m_DynamicsWorld->getNumCollisionObjects(); ++i)
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

		for (int i = 0; i < m_CollisionShapes.size(); ++i)
		{
			delete m_CollisionShapes[i];
		}
		m_CollisionShapes.resize(0);

		World::Clear();
	}
};