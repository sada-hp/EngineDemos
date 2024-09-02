#include "pch.hpp"
#include "Engine/utils.hpp"
#include "Engine/world.hpp"
#include "Engine/window.hpp"
#include "Engine/event_listener.hpp"

std::string path;

using namespace GR;

std::map<GR::EKey, GR::EAction> KeyStates;
glm::vec3 CameraPYR = glm::vec3(0.0);
glm::vec2 Cursor = glm::vec2(0.0);

bool MousePressed = false;
double speed_mult = 25.0;
float Sun = 1.0;

int MaterialID = 0;
int LoadedMaterial = -1;

int SceneID = 0;
int LoadedScene = -1;

void MousePress(GREvent::MousePress Event, void* Data)
{
	Window* wnd = static_cast<Window*>(Data);
	Cursor = wnd->GetCursorPos();
	MousePressed = (Event.action != EAction::Release);
};

void MouseMove(GREvent::MousePosition Event, void* Data)
{
	if (MousePressed)
	{
		CameraPYR += glm::radians(glm::vec3(Cursor.y - Event.y, Cursor.x - Event.x, 0.0));
		Cursor = { Event.x, Event.y };
	}
};

void MouseScroll(GREvent::ScrollDelta Event, void* Data)
{
	speed_mult = glm::clamp(speed_mult + 100.0 * Event.y, 1.0, 10000.0);
};

void KeyPress(GREvent::KeyPress Event, void* Data)
{
	KeyStates[Event.key] = Event.action;
};

void LoadMaterial(World& world, int ID)
{
	Entity ent = world.Registry.view<Entity>().front();
	switch (ID)
	{
	case 0:
		world.BindTexture(world.GetComponent<GRComponents::AlbedoMap>(ent), "content\\brick_albedo.jpg");
		world.BindTexture(world.GetComponent<GRComponents::NormalDisplacementMap>(ent), "content\\brick_nh.png");
		world.BindTexture(world.GetComponent<GRComponents::AORoughnessMetallicMap>(ent), "content\\brick_arm.jpg");
		break;
	case 1:
		world.BindTexture(world.GetComponent<GRComponents::AlbedoMap>(ent), "content\\concrete_albedo.jpg");
		world.BindTexture(world.GetComponent<GRComponents::NormalDisplacementMap>(ent), "content\\concrete_nh.png");
		world.BindTexture(world.GetComponent<GRComponents::AORoughnessMetallicMap>(ent), "content\\concrete_arm.jpg");
		break;
	case 2:
		world.BindTexture(world.GetComponent<GRComponents::AlbedoMap>(ent), "content\\metal_albedo.jpg");
		world.BindTexture(world.GetComponent<GRComponents::NormalDisplacementMap>(ent), "content\\metal_nh.png");
		world.BindTexture(world.GetComponent<GRComponents::AORoughnessMetallicMap>(ent), "content\\metal_arm.jpg");
		break;
	default:
		break;
	}
};

void LoadSpheresScene(Camera& camera, World& world)
{
	world.Clear();

	Sphere shape;
	shape.m_Radius = 10.f;
	shape.m_Rings = 64u;
	shape.m_Slices = 64u;

	for (uint32_t i = 0; i < 4; i++)
	{
		for (uint32_t j = 0; j < 4; j++)
		{
			Entity ent = world.AddShape(shape);
			world.GetComponent<GRComponents::Transform<float>>(ent).SetOffset(glm::vec3(i * 25.0, j * 25.0 + 20.0, 0.0));
			world.GetComponent<GRComponents::RGBColor>(ent).Value = glm::vec3(1.0, 0.0, 0.0);
			world.GetComponent<GRComponents::RoughnessMultiplier>(ent).Value = (i + 1) * 0.25;
			world.GetComponent<GRComponents::MetallicOverride>(ent).Value = (j + 1) * 0.25;
		}
	}

	CameraPYR = { 0.0, glm::radians(180.0), 0.0 };
	camera.View.SetOffset({ 35.0, 55.0, 150.0 });
	camera.View.SetRotation(CameraPYR.x, CameraPYR.y, CameraPYR.z);
};

void LoadGRaff(Camera& camera, World& world)
{
	world.Clear();

	Mesh shape;
	shape.path = path + "content\\graff.obj";
	Entity ent = world.AddShape(shape);

	world.BindTexture(world.GetComponent<GRComponents::AlbedoMap>(ent), "content\\graff_albedo.jpg");
	world.BindTexture(world.GetComponent<GRComponents::NormalDisplacementMap>(ent), "content\\graff_nh.png");
	world.BindTexture(world.GetComponent<GRComponents::AORoughnessMetallicMap>(ent), "content\\graff_arm.jpg");
	world.GetComponent<GRComponents::Transform<float>>(ent).SetScale(2.0, 2.0, 2.0);

	CameraPYR = { 0.0, glm::radians(180.0), 0.0 };
	camera.View.SetOffset({ -2.5, 55.0, 35.0 });
	camera.View.SetRotation(CameraPYR.x, CameraPYR.y, CameraPYR.z);
};

void LoadMaterialWall(Camera& camera, World& world)
{
	world.Clear();

	Plane shape;
	shape.m_Scale = 20.f;

	Entity ent = world.AddShape(shape);
	CameraPYR = { 0.0, glm::radians(210.0), 0.0 };
	camera.View.SetOffset({ 10.0, 50.0, 20.0 });
	camera.View.SetRotation(CameraPYR.x, CameraPYR.y, CameraPYR.z);
};

inline void UpdateResources(Camera& camera, World& world)
{
	if (LoadedScene != SceneID)
	{
		if (SceneID == 0)
		{
			LoadMaterialWall(camera, world);
			LoadMaterial(world, MaterialID);
			LoadedMaterial = MaterialID;
		}
		else if (SceneID == 1)
		{
			LoadSpheresScene(camera, world);
		}
		else
		{
			LoadGRaff(camera, world);
		}

		LoadedScene = SceneID;
	}

	if (LoadedMaterial != MaterialID)
	{
		LoadMaterial(world, MaterialID);
		LoadedMaterial = MaterialID;
	}
};

inline void ControlCamera(GR::Camera& camera, double delta)
{
	glm::dvec3 off = glm::dvec3(0.0);
	if (KeyStates[EKey::A] != EAction::Release) off.x += speed_mult * delta;
	if (KeyStates[EKey::D] != EAction::Release) off.x -= speed_mult * delta;

	if (KeyStates[EKey::W] != EAction::Release) off.z += speed_mult * delta;
	if (KeyStates[EKey::S] != EAction::Release) off.z -= speed_mult * delta;

	if (KeyStates[EKey::PageUp] != EAction::Release) off.y += speed_mult * delta;
	if (KeyStates[EKey::PageDown] != EAction::Release) off.y -= speed_mult * delta;

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

inline void ControlWorld(Renderer& renderer, World& world, double Delta)
{
	const double global_angle = glm::mod(GetTime(), 360.0);
	renderer.m_SunDirection = glm::normalize(glm::vec3(0.0, Sun * 2.0 - 1.0, Sun));

	if (LoadedScene == 0)
	{
		GRComponents::Transform<float>& wld = world.GetComponent<GRComponents::Transform<float>>(world.Registry.view<Entity>().front());
		wld.SetOffset(glm::vec3(0.0, 50.0, 0.0));
		wld.SetRotation(glm::radians(-90.0), 0.0, 0.0);
		wld.SetOffset(glm::vec3(0.0, 50.0 + glm::sin(global_angle) * 2.5, 0.0));
	}
	else if (LoadedScene == 2)
	{
		GRComponents::Transform<float>& wld = world.GetComponent<GRComponents::Transform<float>>(world.Registry.view<Entity>().front());
		wld.SetOffset(glm::vec3(0.0, 50.0 + glm::sin(global_angle) * 2.5, 0.0));
		wld.Rotate(0.0, 0.01, 0.0);
	}
};

inline void UpdateUI(Renderer& renderer, World& world)
{
	if (LoadedScene == -1 || LoadedScene != SceneID) return;

	ImGui::SetCurrentContext(renderer.GetImguiContext());

	ImGui::Begin("Settings", 0, ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::SetWindowPos({ 0, 0 });
	MousePressed = MousePressed && !ImGui::IsWindowHovered();

	ImGui::Text("Common settings");
	ImGui::Separator();

	const char* Materials[3] = { "Material wall", "R-M Spheres", "Model" };
	ImGui::Combo("Scene", &SceneID, Materials, 3);
	ImGui::SliderFloat("Sun position", &Sun, 0.0, 1.0);

	ImGui::NewLine();
	ImGui::Text("Scene settings");
	ImGui::Separator();

	if (LoadedScene == 0)
	{
		GRComponents::RoughnessMultiplier& RM = world.GetComponent<GRComponents::RoughnessMultiplier>(world.Registry.view<Entity>().front());
		ImGui::SliderFloat("Roughness mult", &RM.Value, 0.0, 1.0);

		GRComponents::DisplacementScale& HM = world.GetComponent<GRComponents::DisplacementScale>(world.Registry.view<Entity>().front());
		ImGui::SliderFloat("Height mult", &HM.Value, 0.1, 10.0);

		const char* Materials[3] = { "Brick", "Concrete", "Painted metal" };
		ImGui::Combo("Materials", &MaterialID, Materials, 3);
	}
	else if (LoadedScene == 2)
	{
		GRComponents::RoughnessMultiplier& RM = world.GetComponent<GRComponents::RoughnessMultiplier>(world.Registry.view<Entity>().front());
		ImGui::SliderFloat("Roughness mult", &RM.Value, 0.0, 1.0);
	}

	ImGui::End();
}

int main(int argc, const char** argv)
{
	if (argc > 0)
	{
		path = argv[0];
		path = path.substr(0, path.find_last_of('\\') + 1);
	}

	// Systems setup
	Window window(1024, 720, "PBR materials demo");
	Renderer& renderer = window.GetRenderer();
	Camera& camera = renderer.m_Camera;
	EventListener listener = {};
	World world(renderer);

	// Events setup
	window.SetUpEvents(listener);
	listener.SetUserPointer(&window);
	listener.Subscribe(MouseScroll);
	listener.Subscribe(MousePress);
	listener.Subscribe(MouseMove);
	listener.Subscribe(KeyPress);

	// Rendering
	double delta = 0.0;
	auto last_time = GetTime();
	while (window.IsAlive())
	{
		// Update delta
		auto time = GetTime();
		delta = time - last_time;
		window.SetTitle(("PBR materials demo " + std::format("{:.1f}", 1.0 / delta)).c_str());
		last_time = time;

		// Update simulation
		window.ProcessEvents();
		ControlCamera(camera, delta);
		UpdateResources(camera, world);
		ControlWorld(renderer, world, delta);

		// Render frame
		renderer.BeginFrame(delta);

		world.DrawScene();
		UpdateUI(renderer, world);

		renderer.EndFrame();
	}
	world.Clear();
};