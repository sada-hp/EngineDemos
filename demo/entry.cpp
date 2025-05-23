#include "pch.hpp"
#include "imgui/imgui.h"
#include "Engine/utils.hpp"
#include "Engine/window.hpp"
#include "Engine/event_listener.hpp"

using namespace GR;

glm::vec3 CameraPYR;
glm::vec3 CameraPYRTarget;
glm::vec2 Cursor = glm::vec2(0.0);
std::map<Enums::EKey, Enums::EAction> KeyStates;
CloudLayerProfile CloudLayer{};
CloudLayerProfile CloudLayer_Old{};

float LayerScale = 1e1;
float LayerScale_Old = 1e1;
TerrainLayerProfile TerrainLayers[3];
TerrainLayerProfile TerrainLayers_Old[3];

int LayerOperations[3] = { 1, 3, 2 };

bool MousePressed = false;
double speed_mult = 5000.0;
float Sun = 1.0;

glm::dvec4 CameraGeo = glm::dvec4(0.f, 90.f, 2500.f, Renderer::Rg);

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
		CameraPYRTarget += glm::radians(glm::vec3 (Cursor.y - Event.y, Cursor.x - Event.x, 0.0));
		Cursor = { Event.x, Event.y };
	}
};

void MouseScroll(Events::ScrollDelta Event, void* Data)
{
	speed_mult = glm::clamp(speed_mult + 10000.0 * Event.y, 1.0, 1000000.0);
};

void KeyPress(Events::KeyPress Event, void* Data)
{
	Window* wnd = static_cast<Window*>(Data);
	KeyStates[Event.key] = Event.action;

	if (Event.action == Enums::EAction::Press)
	{
		Window* wnd = static_cast<Window*>(Data);
		switch (Event.key)
		{
		case Enums::EKey::Key_1:
			CameraGeo = glm::vec4(0.f, 90.f, 7000.f, Renderer::Rg);
			wnd->GetRenderer().m_Camera.Transform.offset = Utils::CartesianFromGeo(CameraGeo.x, CameraGeo.y, CameraGeo.z, CameraGeo.w);
			break;
		case Enums::EKey::Key_2:
			CameraGeo = glm::vec4(45.f, 90.f, 7000.f, Renderer::Rg);
			wnd->GetRenderer().m_Camera.Transform.offset = Utils::CartesianFromGeo(CameraGeo.x, CameraGeo.y, CameraGeo.z, CameraGeo.w);
			break;
		case Enums::EKey::Key_3:
			CameraGeo = glm::vec4(0.f, 45.f, 7000.f, Renderer::Rg);
			wnd->GetRenderer().m_Camera.Transform.offset = Utils::CartesianFromGeo(CameraGeo.x, CameraGeo.y, CameraGeo.z, CameraGeo.w);
			break;
		case Enums::EKey::Key_4:
			CameraGeo = glm::vec4(-95.f, -45.f, 7000.f, Renderer::Rg);
			wnd->GetRenderer().m_Camera.Transform.offset = Utils::CartesianFromGeo(CameraGeo.x, CameraGeo.y, CameraGeo.z, CameraGeo.w);
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

	ImGui::Text("Camera settings");
	ImGui::SliderFloat("Exposure", &renderer.m_Camera.Exposure, 0.0, 5.0);
	ImGui::SliderFloat("Gamma", &renderer.m_Camera.Gamma, 0.0, 5.0);

	ImGui::Separator();
	ImGui::Text("World settings");
	ImGui::Separator();
	ImGui::SliderFloat("Sun position", &Sun, 0.0, 1.0);
	ImGui::SliderFloat("Wind speed", &renderer.WindSpeed, 0.0, 1.0);

	ImGui::Separator();
	ImGui::Text("Clouds settings");
	ImGui::Separator();

	ImGui::SliderFloat("Coverage", &CloudLayer.Coverage, 0.0, 1.0);
	ImGui::DragFloat("Density", &CloudLayer.Density, 1e-5, 0.0, 1.0, "%.5f");

	ImGui::Separator();
	ImGui::Text("Terrain settings");
	ImGui::Separator();

	ImGui::SliderFloat("Terrain biome scale", &LayerScale, 1.0, 1000.0);

	ImGui::Separator();
	ImGui::Text("Terrain layer 1 settings");
	ImGui::Separator();

	ImGui::SliderFloat("Layer1 Ea", &TerrainLayers[0].AltitudeF, 0.0, 1.0);
	ImGui::SliderFloat("Layer1 Es", &TerrainLayers[0].SlopeF, 0.0, 1.0);
	ImGui::SliderFloat("Layer1 Ec", &TerrainLayers[0].ConcavityF, 0.0, 1.0);
	ImGui::SliderInt("Layer1 Octaves", &TerrainLayers[0].Octaves, 1, 50);
	ImGui::SliderFloat("Layer1 Sharp", &TerrainLayers[0].Sharpness, -1.0, 1.0);
	ImGui::SliderFloat("Layer1 Freq", &TerrainLayers[0].Frequency, 100.0, 1000.0);
	ImGui::SliderFloat("Layer1 Offset", &TerrainLayers[0].Offset, -1.0, 1.0);

	bool Inverse1 = LayerOperations[0] < 0;
	LayerOperations[0] = glm::abs(LayerOperations[0]);
	ImGui::Combo("Processing 1", &LayerOperations[0], "None\0Default\0Smoothstep\0Ridge");
	ImGui::Checkbox("Inverse 1", &Inverse1);
	LayerOperations[0] = Inverse1 ? -LayerOperations[0] : LayerOperations[0];
	TerrainLayers[0].Op = LayerOperations[0];

	ImGui::Separator();
	ImGui::Text("Terrain layer 2 settings");
	ImGui::Separator();

	ImGui::SliderFloat("Layer2 Ea", &TerrainLayers[1].AltitudeF, 0.0, 1.0);
	ImGui::SliderFloat("Layer2 Es", &TerrainLayers[1].SlopeF, 0.0, 1.0);
	ImGui::SliderFloat("Layer2 Ec", &TerrainLayers[1].ConcavityF, 0.0, 1.0);
	ImGui::SliderInt("Layer2 Octaves", &TerrainLayers[1].Octaves, 1, 50);
	ImGui::SliderFloat("Layer2 Sharp", &TerrainLayers[1].Sharpness, -1.0, 1.0);
	ImGui::SliderFloat("Layer2 Freq", &TerrainLayers[1].Frequency, 100.0, 1000.0);
	ImGui::SliderFloat("Layer2 Offset", &TerrainLayers[1].Offset, -1.0, 1.0);

	bool Inverse2 = LayerOperations[1] < 0;
	LayerOperations[1] = glm::abs(LayerOperations[1]);
	ImGui::Combo("Processing 2", &LayerOperations[1], "None\0Default\0Smoothstep\0Ridge");
	ImGui::Checkbox("Inverse 2", &Inverse2);
	LayerOperations[1] = Inverse2 ? -LayerOperations[1] : LayerOperations[1];
	TerrainLayers[1].Op = LayerOperations[1];

	ImGui::Separator();
	ImGui::Text("Terrain layer 3 settings");
	ImGui::Separator();

	ImGui::SliderFloat("Layer3 Ea", &TerrainLayers[2].AltitudeF, 0.0, 1.0);
	ImGui::SliderFloat("Layer3 Es", &TerrainLayers[2].SlopeF, 0.0, 1.0);
	ImGui::SliderFloat("Layer3 Ec", &TerrainLayers[2].ConcavityF, 0.0, 1.0);
	ImGui::SliderInt("Layer3 Octaves", &TerrainLayers[2].Octaves, 1, 50);
	ImGui::SliderFloat("Layer3 Sharp", &TerrainLayers[2].Sharpness, -1.0, 1.0);
	ImGui::SliderFloat("Layer3 Freq", &TerrainLayers[2].Frequency, 100.0, 1000.0);
	ImGui::SliderFloat("Layer3 Offset", &TerrainLayers[2].Offset, -1.0, 1.0);

	bool Inverse3 = LayerOperations[2] < 0;
	LayerOperations[2] = glm::abs(LayerOperations[2]);
	ImGui::Combo("Processing 3", &LayerOperations[2], "None\0Default\0Smoothstep\0Ridge");
	ImGui::Checkbox("Inverse 3", &Inverse3);
	LayerOperations[2] = Inverse3 ? -LayerOperations[2] : LayerOperations[2];
	TerrainLayers[2].Op = LayerOperations[2];

	ImGui::End();
};

inline void ControlCamera(GR::Camera& camera, double delta)
{
	CameraPYR = glm::mix(CameraPYR, CameraPYRTarget, glm::clamp(delta * 40.0, 0.0, 1.0));

	glm::vec3 off = glm::dvec3(0.0);
	if (KeyStates[Enums::EKey::A] != Enums::EAction::Release) off.x += speed_mult * delta;
	if (KeyStates[Enums::EKey::D] != Enums::EAction::Release) off.x -= speed_mult * delta;
	if (KeyStates[Enums::EKey::W] != Enums::EAction::Release) off.z += speed_mult * delta;
	if (KeyStates[Enums::EKey::S] != Enums::EAction::Release) off.z -= speed_mult * delta;
	if (KeyStates[Enums::EKey::PageUp] != Enums::EAction::Release) off.y += speed_mult * delta;
	if (KeyStates[Enums::EKey::PageDown] != Enums::EAction::Release) off.y -= speed_mult * delta;

	camera.Transform.Translate(off);
	CameraGeo = Utils::CartesianToGeo(camera.Transform.offset, double(Renderer::Rg));

	glm::vec3 U = glm::normalize(camera.Transform.GetOffset());
	glm::quat p = Utils::OrientationFromNormal(U);

	glm::quat q = angleAxis(CameraPYR.y, U);
	q = q * glm::angleAxis(CameraPYR.z, p * glm::vec3(0, 0, 1));
	q = q * glm::angleAxis(-CameraPYR.x, p * glm::vec3(1, 0, 0));

	glm::mat3 M = glm::mat3_cast(q * p);
	camera.Transform.SetRotation(M);
};

inline void ControlWorld(Renderer& renderer, double delta)
{
	if (CloudLayer_Old != CloudLayer)
	{
		renderer.SetCloudLayerSettings(CloudLayer);
	}
	
	if (LayerScale != LayerScale || TerrainLayers != TerrainLayers_Old)
	{
		renderer.SetTerrainLayerSettings(LayerScale, 3, TerrainLayers);
		memcpy(TerrainLayers_Old, TerrainLayers, sizeof(TerrainLayerProfile) * 3);
	}

	CloudLayer_Old = CloudLayer;
	LayerScale_Old = LayerScale;
	renderer.m_SunDirection = glm::normalize(glm::vec3(0.0, Sun * 2.0 - 1.0, 1.0));
};

int main(int argc, const char** argv)
{
	// Systems setup
	Window window(1280, 720, "Procedural planet demo ");
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
	camera.Transform.offset = Utils::CartesianFromGeo(CameraGeo.x, CameraGeo.y, CameraGeo.z, CameraGeo.w);
	camera.Projection.SetDepthRange(0.01, 1e9);
	renderer.WindSpeed = 0.1;
		
	CloudLayer.Coverage = 0.545;
	Sun = 0.95;

	Shapes::GeoClipmap Terrain;
	Terrain.m_Rings = 9u;
	Terrain.m_Scale = 25.f;
	Terrain.m_VerPerRing = 511u;
	Terrain.m_MinHeight = 1500.f;
	Terrain.m_MaxHeight = 35000.f;
#if 0
	Terrain.m_NoiseSeed = uint32_t(&Terrain);
#else
	Terrain.m_NoiseSeed = 1u;
#endif

	LayerScale = 40.0;
	
	TerrainLayers[0].AltitudeF = 0.25;
	TerrainLayers[0].SlopeF = 0.4;
	TerrainLayers[0].ConcavityF = 0.985;
	TerrainLayers[0].Sharpness = 1.0;
	TerrainLayers[0].Frequency = 190.0;
	TerrainLayers[0].Offset = 0.82;

	TerrainLayers[1].AltitudeF = 0.75;
	TerrainLayers[1].SlopeF = 0.15;
	TerrainLayers[1].ConcavityF = 0.0;
	TerrainLayers[1].Sharpness = 0.0;
	TerrainLayers[1].Frequency = 500.0;
	TerrainLayers[1].Offset = 1.0;
	TerrainLayers[1].Op = 3;

	TerrainLayers[2].AltitudeF = 0.1;
	TerrainLayers[2].SlopeF = 0.375;
	TerrainLayers[2].ConcavityF = 0.5;
	TerrainLayers[2].Sharpness = 0.0;
	TerrainLayers[2].Frequency = 350.0;
	TerrainLayers[2].Offset = -0.15;

	Shapes::Sphere shape;
	shape.m_Radius = 100.f;
	shape.m_Rings = 64u;
	shape.m_Slices = 64u;

	for (uint32_t i = 0; i <= 4; i++)
	{
		for (uint32_t j = 0; j <= 4; j++)
		{
			Entity ent = world.AddShape(shape);
			world.GetComponent<Components::WorldMatrix>(ent).SetOffset(glm::dvec3(i * 250.0, Renderer::Rg + Terrain.m_MinHeight + j * 250.0 + 750.0, 0.0));
			world.GetComponent<Components::RGBColor>(ent).Value = glm::vec3(1.0, 0.0, 0.0);
			world.GetComponent<Components::RoughnessMultiplier>(ent).Value = i * 0.25;
			world.GetComponent<Components::MetallicOverride>(ent).Value = j * 0.25;
		}
	}

	Entity TerrainEntity = world.AddShape(Terrain);
	world.BindTexture(world.GetComponent<Components::AlbedoMap>(TerrainEntity), std::vector<std::string>{ "content\\moss_albedo.jpg", "content\\rock_albedo.jpg", "content\\sand_albedo.jpg", "content\\snow_albedo.jpg " });
	world.BindTexture(world.GetComponent<Components::AORoughnessMetallicMapTransmittance>(TerrainEntity), std::vector<std::string>{ "content\\moss_arm.png", "content\\rock_arm.png", "content\\sand_arm.png", "content\\snow_arm.png" });
	world.BindTexture(world.GetComponent<Components::NormalDisplacementMap>(TerrainEntity), std::vector<std::string>{ "content\\moss_nh.png", "content\\rock_nh.png", "content\\sand_nh.png", "content\\snow_nh.png" });
	world.GetComponent<Components::TerrainGrassRings>(TerrainEntity).Count = 4;

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