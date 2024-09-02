#include "physics_world.hpp"
#include "Vulkan/renderer.hpp"

namespace GR
{
	PhysicsWorld::PhysicsWorld(const Renderer& Context)
		: World(Context)
	{
		broadphase = new btDbvtBroadphase;
		solver = new btSequentialImpulseConstraintSolver;
		collisionConfiguration = new btDefaultCollisionConfiguration;
		dispatcher = new btCollisionDispatcher(collisionConfiguration);
		dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);

		_addPlanet(Renderer::Rg, btVector3(0.0, -Renderer::Rg, 0.0));
	}

	PhysicsWorld::~PhysicsWorld()
	{
		Clear();
		_clearPlanet();

		delete dynamicsWorld;
		delete dispatcher;

		delete collisionConfiguration;
		delete broadphase;
		delete solver;
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
			colShape = new btSphereShape(btScalar(extents.x));
			rollFriction = 0.5;
			mass = Descriptor.GetDimensions().x * Descriptor.GetDimensions().y * 0.75;
		}
		else if (dynamic_cast<const Shapes::Cube*>(&Descriptor))
		{
			colShape = new btBoxShape(btVector3(extents.x, extents.y, extents.z));
			mass = Descriptor.GetDimensions().x* Descriptor.GetDimensions().y;
		}
		else
		{
			colShape = new btBoxShape(btVector3(5.0, 5.0, 5.0));
			mass = Descriptor.GetDimensions().x * Descriptor.GetDimensions().y;
		}

		btTransform startTransform;
		startTransform.setIdentity();
		btVector3 localInertia(0.0, 0.0, 0.0);
		colShape->calculateLocalInertia(mass, localInertia);

		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		body->setSleepingThresholds(0.5, 0.5);
		body->setRollingFriction(rollFriction);

		collisionShapes.push_back(colShape);
		Registry.emplace<Components::Body>(ent, body);
		dynamicsWorld->addRigidBody(body);

		return ent;
	}

	void PhysicsWorld::ResetObject(Entity object)
	{
		Components::WorldMatrix& transform = GetComponent<Components::WorldMatrix>(object);
		btRigidBody* body = GetComponent<Components::Body>(object).body;

		btTransform physTransform;
		physTransform.setFromOpenGLMatrix(glm::value_ptr(transform.matrix));
		body->setWorldTransform(physTransform);
	}

	void PhysicsWorld::DrawScene(double Delta)
	{
		constexpr double fixedStep = 1.0 / 60.0;

		auto view = Registry.view<Components::Body, Components::WorldMatrix>();
		for (const auto& [ent, body, transform] : view.each())
		{
			body.body->getWorldTransform().getOpenGLMatrix(glm::value_ptr(transform.matrix));
			body.body->setGravity((body.body->getWorldTransform().getOrigin() + btVector3(0.0, Renderer::Rg, 0.0)).normalized() * gravity);
		}
		dynamicsWorld->stepSimulation(Delta, 5, fixedStep);

		World::DrawScene(Delta);
	}

	void PhysicsWorld::Clear()
	{
		for (int i = 1; i < dynamicsWorld->getNumCollisionObjects(); ++i)
		{
			btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
			btRigidBody* body = btRigidBody::upcast(obj);
			if (body && body->getMotionState())
			{
				delete body->getMotionState();
			}

			dynamicsWorld->removeCollisionObject(obj);
			delete obj;
		}

		for (int i = 1; i < collisionShapes.size(); ++i)
		{
			delete collisionShapes[i];
		}
		collisionShapes.resize(1);

		World::Clear();
	}

	void PhysicsWorld::_addPlanet(double r, btVector3 origin)
	{
		btCollisionShape* colShape = new btSphereShape(btScalar(r));
		collisionShapes.push_back(colShape);

		btScalar mass(0.0);
		btTransform startTransform;
		btVector3 localInertia(0.0, 0.0, 0.0);
		startTransform.setIdentity();
		startTransform.setOrigin(origin);

		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		dynamicsWorld->addRigidBody(body);
	}

	void PhysicsWorld::_clearPlanet()
	{
		btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[0];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState())
		{
			delete body->getMotionState();
		}

		dynamicsWorld->removeCollisionObject(obj);
		delete obj;
		delete collisionShapes[0];
		collisionShapes.removeAtIndex(0);
	}
};