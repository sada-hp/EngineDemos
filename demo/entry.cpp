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
double speed_mult = 25000.0;
float Sun = 1.0;

glm::vec3 CameraGeo = glm::vec3(0.0f, glm::radians(90.f), 5000.f);

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
	speed_mult = glm::clamp(speed_mult + 10000.0 * Event.y, 1.0, 1000000.0);
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
			CloudLayer.Coverage = 0.5;
			break;
		case Enums::EKey::Key_2:
			Sun = 0.495;
			CloudLayer.Coverage = 0.4;
			break;
		case Enums::EKey::Key_3:
			Sun = 0.52;
			CloudLayer.Coverage = 0.75;
			break;
		case Enums::EKey::Key_4:
			Sun = 0.6;
			CloudLayer.Coverage = 0.6;
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
	ImGui::SliderFloat("Wind speed", &CloudLayer.WindSpeed, 0.0, 1.0);
	ImGui::DragFloat("Density", &CloudLayer.Density, 1e-5, 0.0, 1.0, "%.5f");

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
	
	camera.Transform.Translate(off);

	glm::vec3 U = glm::normalize(camera.Transform.GetOffset());
	glm::quat p = glm::rotation(glm::vec3(0.0, 1.0, 0.0), U);

	glm::quat q = angleAxis(CameraPYR.y, U);
	q = q * glm::angleAxis(CameraPYR.z, p * glm::vec3(0, 0, 1));
	q = q * glm::angleAxis(-CameraPYR.x, p * glm::vec3(1, 0, 0));

	glm::mat3 M = glm::mat3_cast(q * p);
	camera.Transform.SetRotation(M);
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
	Window window(1024, 720, "Procedural planet demo ");
	Renderer& renderer = window.GetRenderer();
	Camera& camera = renderer.m_Camera;
	std::unique_ptr<EventListener> listener = std::make_unique<EventListener>();
	World world(renderer);

	// Events setup
	window.SetUpEvents(*listener);
	listener->SetUserPointer(&window);
	listener->Subscribe(MouseScroll);
	listener->Subscribe(MousePress);
	listener->Subscribe(MouseMove);
	listener->Subscribe(KeyPress);

	// World setup
	// renderer.m_Camera.Transform.SetOffset(0.0, Renderer::Rg + 5000.0, 0.0);
	renderer.m_Camera.Transform.SetOffsetFromGeo(CameraGeo.x, CameraGeo.y, CameraGeo.z, Renderer::Rg);
	camera.Projection.SetDepthRange(0.01, 1e9);

	CloudLayer.Coverage = 0.0;

	Shapes::GeoClipmap Terrain;
	Terrain.m_Rings = 8u;
	Terrain.m_Scale = 100.f;
	Terrain.m_VerPerRing = 1023u;
	Terrain.m_MinHeight = 3000.f;
	Terrain.m_MaxHeight = 35000.f;
#if 1
	Terrain.m_NoiseSeed = uint32_t(&Terrain);
#else
	Terrain.m_NoiseSeed = 1u;
#endif

	// GR::Utils::ConvertImage_ARMT("content\\moss_r.jpg", "", "content\\moss_ao.jpg", "content\\moss_t.jpg", "content\\moss_arm.png");
	// GR::Utils::ConvertImage_ARMT("content\\snow_r.jpg", "", "content\\snow_ao.jpg", "content\\snow_t.jpg", "content\\snow_arm.png");
	// GR::Utils::ConvertImage_ARMT("content\\rock_r.jpg", "", "content\\rock_ao.jpg", "", "content\\rock_arm.png");
	// GR::Utils::ConvertImage_ARMT("content\\sand_r.jpg", "", "content\\sand_ao.jpg", "", "content\\sand_arm.png");

	// GR::Utils::ConvertImage_NormalHeight("content\\moss_n.jpg", "", "content\\moss_nh.png");
	// GR::Utils::ConvertImage_NormalHeight("content\\snow_n.jpg", "", "content\\snow_nh.png");
	// GR::Utils::ConvertImage_NormalHeight("content\\rock_n.jpg", "", "content\\rock_nh.png");
	// GR::Utils::ConvertImage_NormalHeight("content\\sand_n.jpg", "", "content\\sand_nh.png");

	Entity TerrainEntity = world.AddShape(Terrain);
	world.BindTexture(world.GetComponent<Components::AlbedoMap>(TerrainEntity), std::vector<std::string>{ "content\\moss_albedo.jpg", "content\\rock_albedo.jpg", "content\\sand_albedo.jpg", "content\\snow_albedo.jpg " });
	world.BindTexture(world.GetComponent<Components::AORoughnessMetallicMapTransmittance>(TerrainEntity), std::vector<std::string>{ "content\\moss_arm.png", "content\\rock_arm.png", "content\\sand_arm.png", "content\\snow_arm.png" });
	world.BindTexture(world.GetComponent<Components::NormalDisplacementMap>(TerrainEntity), std::vector<std::string>{ "content\\moss_nh.png", "content\\rock_nh.png", "content\\sand_nh.png", "content\\snow_nh.png" });

	// Rendering
	double delta = 0.0;
	auto last_time = Utils::GetTime();
	while (window.IsAlive())
	{
		// Update delta
		auto time = Utils::GetTime();
		delta = time - last_time;
		window.SetTitle(("Procedural planet demo " + std::format("{:.1f}", 1.0 / delta)).c_str());
		last_time = time;

		// Update simulation
		window.ProcessEvents();
		ControlCamera(camera, delta);
		ControlWorld(renderer, delta);

		// Render frame
		if (renderer.BeginFrame())
		{
			UpdateUI(renderer);

			world.DrawScene(delta);

			renderer.EndFrame();
		}
	}
};