#include "pch.hpp"
#include "imgui/imgui.h"
#include "Engine/utils.hpp"
#include "Engine/window.hpp"
#include "Engine/event_listener.hpp"

using namespace GR;

glm::vec3 CameraPYR;
glm::vec2 Cursor = glm::vec2(0.0);
std::map<Enums::EKey, Enums::EAction> KeyStates;
CloudLayerProfile CloudLayer{};
CloudLayerProfile CloudLayer_Old{};
bool MousePressed = false;
double speed_mult = 100.0;
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
	speed_mult = glm::clamp(speed_mult + 100.0 * Event.y, 1.0, 10000.0);
};

void KeyPress(Events::KeyPress Event, void* Data)
{
	KeyStates[Event.key] = Event.action;

	if (Event.action == Enums::EAction::Press)
	{
		Window* wnd = static_cast<Window*>(Data);
		switch (Event.key)
		{
		case Enums::EKey::Key_1:
			Sun = 1.0;
			CloudLayer.Coverage = 0.185;
			CloudLayer.VerticalSpan = 0.2;
			break;
		case Enums::EKey::Key_2:
			Sun = 0.495;
			CloudLayer.Coverage = 0.2;
			CloudLayer.VerticalSpan = 0.5;
			break;
		case Enums::EKey::Key_3:
			Sun = 0.52;
			CloudLayer.Coverage = 0.25;
			CloudLayer.VerticalSpan = 0.0;
			break;
		case Enums::EKey::Key_4:
			Sun = 0.45;
			CloudLayer.Coverage = 0.185;
			CloudLayer.VerticalSpan = 0.49;
			break;
		default:
			break;
		}
	}
};

inline void UpdateUI(Renderer& renderer)
{
	ImGui::SetCurrentContext(renderer.GetImguiContext());

	ImGui::Begin("Settings", 0, ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::SetWindowPos({ 0, 0 });

	MousePressed = MousePressed && !ImGui::IsWindowHovered();

	ImGui::SliderFloat("Sun position", &Sun, 0.0, 1.0);
	ImGui::SliderFloat("Coverage", &CloudLayer.Coverage, 0.0, 1.0);
	ImGui::SliderFloat("Vertical span", &CloudLayer.VerticalSpan, 0.0, 1.0);
	ImGui::SliderFloat("Wind speed", &CloudLayer.WindSpeed, 0.0, 1.0);
	ImGui::DragFloat("Absorption", &CloudLayer.Absorption, 1e-5, 0.0, 1.0, "%.5f");

	ImGui::End();
};

inline void ControlCamera(GR::Camera& camera, double delta)
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

	if (CloudLayer_Old != CloudLayer)
	{
		renderer.SetCloudLayerSettings(CloudLayer);
		CloudLayer_Old = CloudLayer;
	}
};

int main(int argc, const char** argv)
{
	// Systems setup
	Window window(1024, 720, "Volumetric clouds demo");
	Renderer& renderer = window.GetRenderer();
	Camera& camera = renderer.m_Camera;
	EventListener listener = {};

	// Events setup
	window.SetUpEvents(listener);
	listener.SetUserPointer(&window);
	listener.Subscribe(MouseScroll);
	listener.Subscribe(MousePress);
	listener.Subscribe(MouseMove);
	listener.Subscribe(KeyPress);

	// World setup
	renderer.m_Camera.View.SetOffset({ 0.0, 50.0, 0.0 });

	// Rendering
	double delta = 0.0;
	auto last_time = Utils::GetTime();
	while (window.IsAlive())
	{
		// Update delta
		auto time = Utils::GetTime();
		delta = time - last_time;
		window.SetTitle(("Volumetric clouds demo " + std::format("{:.1f}", 1.0 / delta)).c_str());
		last_time = time;

		// Update simulation
		window.ProcessEvents();
		ControlCamera(camera, delta);
		ControlWorld(renderer, delta);

		// Render frame
		renderer.BeginFrame();
	
		UpdateUI(renderer);
		
		renderer.EndFrame();
	}
};