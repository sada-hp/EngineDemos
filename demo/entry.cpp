#include "pch.hpp"
#include "imgui/imgui.h"
#include "Engine/utils.hpp"
#include "Engine/window.hpp"
#include "physics_world.hpp"
#include "Engine/event_listener.hpp"

using namespace GR;

glm::vec3 CameraPYR(0.0);
glm::vec3 ColorModifier(1.0);
glm::vec2 Cursor = glm::vec2(0.0);
std::map<Enums::EKey, Enums::EAction> KeyStates;
bool MousePressed = false;
double speed_mult = 1.0;
float Sun = 1.0;

void MousePress(Events::MousePress Event, void* Data)
{
	Window* wnd = static_cast<Window*>(Data);
	Cursor = wnd->GetCursorPos();
	MousePressed = (Event.action != Enums::EAction::Release);
};

void MouseMove(Events::MousePosition Event, void* Data)
{
	if (MousePressed)
	{
		CameraPYR += glm::radians(glm::vec3 (Cursor.y - Event.y, Cursor.x - Event.x, 0.0));
		Cursor = { Event.x, Event.y };
	}
};

void MouseScroll(Events::ScrollDelta Event, void* Data)
{
	speed_mult = glm::clamp(speed_mult + 10.0 * Event.y, 1.0, 10000.0);
};

void KeyPress(Events::KeyPress Event, void* Data)
{
	KeyStates[Event.key] = Event.action;
};

void SpawnSphere(Renderer& renderer, PhysicsWorld& world)
{
	Camera& camera = renderer.m_Camera;

	Shapes::Sphere sphere{};
	sphere.m_Radius = 15.f;
	sphere.m_Rings = 64u;
	sphere.m_Slices = 64u;

	Entity object = world.AddShape(sphere);
	world.GetComponent<Components::WorldMatrix>(object).SetOffset(camera.View.GetOffset() + camera.View.GetForward() * 50.0);
	world.GetComponent<Components::RGBColor>(object).Value = ColorModifier;
	world.ResetObject(object);
};

void SpawnBox(Renderer& renderer, PhysicsWorld& world)
{
	Camera& camera = renderer.m_Camera;

	Shapes::Cube sphere{};
	sphere.m_Scale = 30.f;

	Entity object = world.AddShape(sphere);
	world.GetComponent<Components::WorldMatrix>(object).SetOffset(camera.View.GetOffset() + camera.View.GetForward() * 50.0);
	world.GetComponent<Components::RGBColor>(object).Value = ColorModifier;
	world.ResetObject(object);
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

inline void ControlWorld(Renderer& renderer, double delta)
{
	renderer.m_SunDirection = glm::normalize(glm::vec3(0.0, Sun * 2.0 - 1.0, 1.0));
};

int main(int argc, const char** argv)
{
	// Systems setup
	Window window(1024, 720, "Volumetric clouds demo");
	Renderer& renderer = window.GetRenderer();
	Camera& camera = renderer.m_Camera;
	EventListener listener = {};
	PhysicsWorld world(renderer);

	// Events setup
	window.SetUpEvents(listener);
	listener.SetUserPointer(&window);
	listener.Subscribe(MouseScroll);
	listener.Subscribe(MousePress);
	listener.Subscribe(MouseMove);
	listener.Subscribe(KeyPress);

	// World setup
	renderer.m_Camera.View.SetOffset({ 0.0, 10.0, 0.0 });

	// Rendering
	double delta = 0.0;
	auto last_time = Utils::GetTime();
	constexpr double fixedStep = 1.0 / 60.0;

	while (window.IsAlive())
	{
		// Update delta
		auto time = Utils::GetTime();
		delta = time - last_time;
		window.SetTitle(("Volumetric clouds demo " + std::format("{:.1f}", 1.0 / delta)).c_str());
		last_time = time;

		// Update simulation
		window.ProcessEvents();
		ControlCamera(camera, fixedStep / delta);
		ControlWorld(renderer, delta);

		// Render frame
		renderer.BeginFrame();

		world.DrawScene(delta);
		UpdateUI(renderer, world);
		
		renderer.EndFrame();
	}
};