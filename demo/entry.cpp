#include "pch.hpp"
#include "imgui/imgui.h"
#include "Engine/utils.hpp"
#include "Engine/window.hpp"
#include "physics_world.hpp"
#include "Engine/event_listener.hpp"

using namespace GR;

struct EventData
{
	Window* window;
	PhysicsWorld* world;
	Camera* camera;
};

glm::vec3 CameraPYR(0.0);
glm::vec3 ColorModifier(1.0);
glm::vec2 Cursor = glm::vec2(0.0);
std::map<Enums::EKey, Enums::EAction> KeyStates;
bool MousePressed = false;
double speed_mult = 1.0;
float ObjectScale = 1.0;
float Sun = 1.0;

Entity Selection = Entity(-1);

inline glm::vec3 GetCursorDirection(Window& window, Camera& camera)
{
	glm::vec2 ScreeUV = Cursor / glm::vec2(window.GetWindowSize());
	glm::dvec4 pixelPosition = glm::vec4(2.0f * ScreeUV - 1.0f, 1.0f, 1.0f);
	pixelPosition = glm::inverse(camera.get_projection_matrix()) * pixelPosition;
	pixelPosition = glm::inverse(camera.get_view_matrix()) * glm::dvec4(glm::dvec2(pixelPosition), -1.0, 0.0);

	return glm::normalize(glm::dvec3(pixelPosition));
};

void MousePress(Events::MousePress Event, void* Data)
{
	EventData* data = static_cast<EventData*>(Data);
	PhysicsWorld& world = *data->world;
	Window& window = *data->window;
	Camera& camera = *data->camera;

	Cursor = window.GetCursorPos();
	MousePressed = Event.key == Enums::EMouse::Left && (Event.action != Enums::EAction::Release);

	if (Event.key == Enums::EMouse::Right && Event.action == Enums::EAction::Press)
	{
		glm::vec3 dir = GetCursorDirection(window, camera);
		RayCastResult rayCast = world.FirstAtRay(camera.View.GetOffset(), dir);
		Selection = rayCast.id;

		//printf("%d \n", int(rayCast.id));
	}
	else if (Selection != Entity(-1) && Event.key == Enums::EMouse::Right && Event.action == Enums::EAction::Release)
	{
		world.ResetObject(Selection);
		Selection = Entity(-1);
	}
};

void MouseMove(Events::MousePosition Event, void* Data)
{
	EventData* data = static_cast<EventData*>(Data);
	PhysicsWorld& world = *data->world;
	Window& window = *data->window;
	Camera& camera = *data->camera;

	if (MousePressed)
	{
		CameraPYR += glm::radians(glm::vec3 (Cursor.y - Event.y, Cursor.x - Event.x, 0.0));
	}

	if (Selection != Entity(-1))
	{
		glm::vec3 dir = GetCursorDirection(window, camera);
		Components::WorldMatrix& T = world.GetComponent<Components::WorldMatrix>(Selection);

		glm::vec3 offset = glm::vec3(camera.View.GetOffset()) + glm::length(T.GetOffset() - glm::vec3(camera.View.GetOffset())) * dir;
		T.SetOffset(offset);
		world.ResetObject(Selection);

		int iter = 0;
		RayCastResult hit;
		float deepestContact = world.DeepestContactPoint(Selection, hit);
		while (deepestContact > 0.0 && iter < 10)
		{
			offset += hit.hitNormal * float(deepestContact + 0.01);

			T.SetOffset(offset);
			world.ResetObject(Selection);

			deepestContact = world.DeepestContactPoint(Selection, hit);
			iter++;
		}
	}

	Cursor = { Event.x, Event.y };
};

void MouseScroll(Events::ScrollDelta Event, void* Data)
{
	EventData* data = static_cast<EventData*>(Data);
	PhysicsWorld& world = *data->world;
	Window& window = *data->window;
	Camera& camera = *data->camera;

	if (Selection != Entity(-1))
	{
		glm::vec3 dir = GetCursorDirection(window, camera);
		Components::WorldMatrix& T = world.GetComponent<Components::WorldMatrix>(Selection);

		glm::vec3 offset = glm::vec3(camera.View.GetOffset()) + float(1.0 + Event.y * 0.1) * glm::length(T.GetOffset() - glm::vec3(camera.View.GetOffset())) * dir;
		T.SetOffset(offset);
		world.ResetObject(Selection);

		int iter = 0;
		RayCastResult hit;
		float deepestContact = world.DeepestContactPoint(Selection, hit);
		while (deepestContact > 0.0 && iter < 10)
		{
			offset += hit.hitNormal * float(deepestContact + 0.01);

			T.SetOffset(offset);
			world.ResetPosition(Selection);

			deepestContact = world.DeepestContactPoint(Selection, hit);
			iter++;
		}
	}
	else
	{
		speed_mult = glm::clamp(speed_mult + 10.0 * Event.y, 1.0, 10000.0);
	}
};

void KeyPress(Events::KeyPress Event, void* Data)
{
	KeyStates[Event.key] = Event.action;
};

void SpawnSphere(Renderer& renderer, PhysicsWorld& world)
{
	Camera& camera = renderer.m_Camera;

	Shapes::Sphere sphere{};
	sphere.m_Radius = ObjectScale * 15.f;
	sphere.m_Rings = 64u;
	sphere.m_Slices = 64u;

	Entity object = world.AddShape(sphere);
	world.GetComponent<Components::WorldMatrix>(object).SetOffset(camera.View.GetOffset() + camera.View.GetForward() * (50.0 * ObjectScale));
	world.GetComponent<Components::RGBColor>(object).Value = ColorModifier;
	world.ResetPosition(object);
};

void SpawnBox(Renderer& renderer, PhysicsWorld& world)
{
	Camera& camera = renderer.m_Camera;

	Shapes::Cube sphere{};
	sphere.m_Scale = ObjectScale * 30.f;

	Entity object = world.AddShape(sphere);
	world.GetComponent<Components::WorldMatrix>(object).SetOffset(camera.View.GetOffset() + camera.View.GetForward() * (50.0 * ObjectScale));
	world.GetComponent<Components::RGBColor>(object).Value = ColorModifier;
	world.ResetPosition(object);
};

inline void UpdateUI(Renderer& renderer, PhysicsWorld& world)
{
	ImGui::SetCurrentContext(renderer.GetImguiContext());

	ImGui::Begin("Settings", 0, ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::SetWindowPos({ 0, 0 });

	ImGui::SliderFloat("Sun position", &Sun, 0.0, 1.0);

	ImGui::Separator();
	ImGui::Text("Spawn menu:");

	ImGui::ColorEdit3("Color", glm::value_ptr(ColorModifier));

	ImGui::SliderFloat("Object scale", &ObjectScale, 0.1, 2.0);

	ImGui::Button("Spawn ball");
	if (ImGui::IsItemClicked())
	{
		SpawnSphere(renderer, world);
	}

	ImGui::Button("Spawn box");
	if (ImGui::IsItemClicked())
	{
		SpawnBox(renderer, world);
	}

	ImGui::Separator();
	ImGui::Text("RMB - Select and move object");
	ImGui::Text("Scroll - Move selected object closer/further");

	ImGui::End();

	MousePressed = MousePressed && !ImGui::IsAnyItemHovered();
};

inline void ControlCamera(Camera& camera, double delta)
{
	glm::dvec3 off = glm::dvec3(0.0);
	if (KeyStates[Enums::EKey::A] != Enums::EAction::Release) off.x += speed_mult * delta;
	if (KeyStates[Enums::EKey::D] != Enums::EAction::Release) off.x -= speed_mult * delta;

	if (KeyStates[Enums::EKey::W] != Enums::EAction::Release) off.z += speed_mult * delta;
	if (KeyStates[Enums::EKey::S] != Enums::EAction::Release) off.z -= speed_mult * delta;

	if (KeyStates[Enums::EKey::PageUp] != Enums::EAction::Release) off.y += speed_mult * delta;
	if (KeyStates[Enums::EKey::PageDown] != Enums::EAction::Release) off.y -= speed_mult * delta;

	camera.View.Translate(off);

	glm::vec3 U = glm::normalize(glm::dvec3(0.0, Renderer::Rg, 0.0) + camera.View.GetOffset());
	glm::quat p = glm::rotation(glm::vec3(0.0, 1.0, 0.0), U);

	glm::quat q = angleAxis(CameraPYR.y, U);
	q = q * glm::angleAxis(CameraPYR.z, p * glm::vec3(0, 0, 1));
	q = q * glm::angleAxis(-CameraPYR.x, p * glm::vec3(1, 0, 0));

	glm::mat3 M = glm::mat3_cast(q * p);

	camera.View.matrix[0] = glm::dvec4(glm::normalize(M[0]), 0.0);
	camera.View.matrix[1] = glm::dvec4(glm::normalize(M[1]), 0.0);
	camera.View.matrix[2] = glm::dvec4(glm::normalize(M[2]), 0.0);
};

inline void ControlWorld(Renderer& renderer, PhysicsWorld& world, double delta)
{
	renderer.m_SunDirection = glm::normalize(glm::vec3(0.0, Sun * 2.0 - 1.0, 1.0));

	if (Selection != Entity(-1))
	{
		world.ResetPosition(Selection);
	}
};

int main(int argc, const char** argv)
{
	// Systems setup
	Window window(1024, 720, "Bullet physics demo");
	Renderer& renderer = window.GetRenderer();
	Camera& camera = renderer.m_Camera;
	EventListener listener = {};
	PhysicsWorld world(renderer);

	EventData Data{ &window, &world, &camera };

	// Events setup
	window.SetUpEvents(listener);
	listener.SetUserPointer(&Data);
	listener.Subscribe(MouseScroll);
	listener.Subscribe(MousePress);
	listener.Subscribe(MouseMove);
	listener.Subscribe(KeyPress);

	// World setup
	renderer.m_Camera.View.SetOffset({ 0.0, 25.0, 0.0 });

	Shapes::GeoClipmap Terrain{};
	Terrain.m_Scale = 20.f;
	Terrain.m_Rings = 10u;
	Entity terrain = world.AddShape(Terrain);
	world.GetComponent<Components::RGBColor>(terrain).Value = glm::vec3(0.0, 0.35, 0.0);

	camera.Projection.SetDepthRange(0.01, 1e5);

	// Rendering
	double delta = 0.0;
	auto last_time = Utils::GetTime();
	constexpr double fixedStep = 1.0 / 60.0;

	// Simulation loop
	while (window.IsAlive())
	{
		// Update delta
		auto time = Utils::GetTime();
		delta = time - last_time;
		window.SetTitle(("Bullet physics demo " + std::format("{:.1f}", 1.0 / delta)).c_str());
		last_time = time;

		// Update simulation
		window.ProcessEvents();
		ControlCamera(camera, fixedStep / delta);
		ControlWorld(renderer, world, delta);

		// Render frame
		renderer.BeginFrame();

		world.DrawScene(delta);
		UpdateUI(renderer, world);
		
		renderer.EndFrame();
	}
};