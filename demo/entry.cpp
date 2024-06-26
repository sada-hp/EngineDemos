#include "pch.hpp"
#include "engine.hpp"

GR::Entity object;
GR::Entity spheres[16];

TVec3 CameraPYR = TVec3(0.0);
TVec2 Cursor = TVec2(0.0);

double speed_mult = 1.0;
float Sun = 1.0;

int MaterialID = 0;
int LoadedMaterial = -1;

int SceneID = 0;
int LoadedScene = -1;

bool MousePressed = false;
std::map<GR::EKey, GR::EAction> KeyStates;

constexpr double StepTime = 1.0 / 60.0;

void LoadMaterial(GR::GrayEngine* Context, int ID)
{
	switch (ID)
	{
	case 0:
		Context->BindImage(Context->GetComponent<GRComponents::AlbedoMap>(object), "content\\brick_albedo.jpg", GR::EImageType::RGBA_SRGB);
		Context->BindImage(Context->GetComponent<GRComponents::NormalMap>(object), "content\\brick_normal.jpg", GR::EImageType::RGBA_UNORM);
		Context->BindImage(Context->GetComponent<GRComponents::RoughnessMap>(object), "content\\brick_roughness.jpg", GR::EImageType::RGBA_UNORM);
		Context->BindImage(Context->GetComponent<GRComponents::AmbientMap>(object), "content\\brick_ao.jpg", GR::EImageType::RGBA_UNORM);
		Context->BindImage(Context->GetComponent<GRComponents::DisplacementMap>(object), "content\\brick_height.jpg", GR::EImageType::RGBA_UNORM);
		break;
	case 1:
		Context->BindImage(Context->GetComponent<GRComponents::AlbedoMap>(object), "content\\concrete_albedo.jpg", GR::EImageType::RGBA_SRGB);
		Context->BindImage(Context->GetComponent<GRComponents::NormalMap>(object), "content\\concrete_normal.jpg", GR::EImageType::RGBA_UNORM);
		Context->BindImage(Context->GetComponent<GRComponents::RoughnessMap>(object), "content\\concrete_roughness.jpg", GR::EImageType::RGBA_UNORM);
		Context->BindImage(Context->GetComponent<GRComponents::AmbientMap>(object), "content\\concrete_ao.jpg", GR::EImageType::RGBA_UNORM);
		Context->BindImage(Context->GetComponent<GRComponents::DisplacementMap>(object), "content\\concrete_height.jpg", GR::EImageType::RGBA_UNORM);
		break;
	case 2:
		Context->BindImage(Context->GetComponent<GRComponents::AlbedoMap>(object), "content\\metal_albedo.jpg", GR::EImageType::RGBA_SRGB);
		Context->BindImage(Context->GetComponent<GRComponents::NormalMap>(object), "content\\metal_normal.jpg", GR::EImageType::RGBA_UNORM);
		Context->BindImage(Context->GetComponent<GRComponents::RoughnessMap>(object), "content\\metal_roughness.jpg", GR::EImageType::RGBA_UNORM);
		Context->BindImage(Context->GetComponent<GRComponents::AmbientMap>(object), "content\\metal_ao.jpg", GR::EImageType::RGBA_UNORM);
		Context->BindImage(Context->GetComponent<GRComponents::DisplacementMap>(object), "content\\metal_height.jpg", GR::EImageType::RGBA_UNORM);
		break;
	default:
		break;
	}
}

void LoadMaterialWall(GR::GrayEngine* Context)
{
	Context->ClearEntities();

	GRShape::Plane Shape;
	Shape.scale = 20.f;

	object = Context->AddShape(Shape);
	CameraPYR = { 0.0, glm::radians(210.0), 0.0 };
	Context->GetMainCamera().View.SetOffset({ 10.0, 50.0, 20.0 });
	Context->GetMainCamera().View.SetRotation(CameraPYR.x, CameraPYR.y, CameraPYR.z);
}

void LoadSpheresScene(GR::GrayEngine* Context)
{
	Context->ClearEntities();

	GRShape::Sphere Shape;
	Shape.radius = 10.f;
	Shape.rings = 65u;
	Shape.slices = 65u;

	for (uint32_t i = 0; i < 4; i++)
	{
		for (uint32_t j = 0; j < 4; j++)
		{
			spheres[i] = Context->AddShape(Shape);

			Context->GetComponent<GRComponents::Transform>(spheres[i]).SetOffset(TVec3(i * 25.0, j * 25.0 + 20.0, 0.0));
			Context->EmplaceComponent<GRComponents::Color>(spheres[i], GRComponents::Color{ TVec3(1.0, 0.0, 0.0) });
			Context->GetComponent<GRComponents::RoughnessMultiplier>(spheres[i]).R = (i + 1) * 0.25;
			Context->GetComponent<GRComponents::MetallicOverride>(spheres[i]).M = (j + 1) * 0.25;
		}
	}

	CameraPYR = { 0.0, glm::radians(180.0), 0.0 };
	Context->GetMainCamera().View.SetOffset({ 35.0, 55.0, 150.0 });
	Context->GetMainCamera().View.SetRotation(CameraPYR.x, CameraPYR.y, CameraPYR.z);
}

void LoadGRaff(GR::GrayEngine* Context)
{
	Context->ClearEntities();
	object = Context->AddMesh("content\\graff.obj");

	Context->BindImage(Context->GetComponent<GRComponents::AlbedoMap>(object), "content\\graff_albedo.jpg", GR::EImageType::RGBA_SRGB);
	Context->BindImage(Context->GetComponent<GRComponents::NormalMap>(object), "content\\graff_normal.jpg", GR::EImageType::RGBA_UNORM);
	Context->BindImage(Context->GetComponent<GRComponents::RoughnessMap>(object), "content\\graff_roughness.jpg", GR::EImageType::RGBA_UNORM);
	Context->BindImage(Context->GetComponent<GRComponents::AmbientMap>(object), "content\\graff_ao.jpg", GR::EImageType::RGBA_UNORM);
	Context->GetComponent<GRComponents::Transform>(object).SetScale(2.0, 2.0, 2.0);

	CameraPYR = { 0.0, glm::radians(180.0), 0.0 };
	Context->GetMainCamera().View.SetOffset({ -2.5, 55.0, 35.0 });
	Context->GetMainCamera().View.SetRotation(CameraPYR.x, CameraPYR.y, CameraPYR.z);
}

void UpdateResources(GR::GrayEngine* Context, double Delta)
{
	if (LoadedScene != SceneID)
	{
		if (SceneID == 0)
		{
			LoadMaterialWall(Context);
			LoadMaterial(Context, MaterialID);
			LoadedMaterial = MaterialID;
		}
		else if (SceneID == 1)
		{
			LoadSpheresScene(Context);
		}
		else
		{
			LoadGRaff(Context);
		}

		LoadedScene = SceneID;
	}

	if (LoadedMaterial != MaterialID)
	{
		LoadMaterial(Context, MaterialID);
		LoadedMaterial = MaterialID;
	}
}

void Loop(GR::GrayEngine* Context, double Delta)
{
	const double simulation_step = StepTime / Delta;
	const double global_angle = glm::mod(Context->GetTime(), 360.0);
	Context->GetWindow().SetTitle(("Vulkan Application " + std::format("{:.1f}", 1.0 / Delta)).c_str());
	Context->GetRenderer().SunDirection = glm::normalize(glm::vec3(0.0, Sun * 2.0 - 1.0, Sun));

	TVec3 off = TVec3(0.0);
	if (KeyStates[GR::EKey::A] != GR::EAction::Release) off.x += speed_mult * simulation_step;
	if (KeyStates[GR::EKey::D] != GR::EAction::Release) off.x -= speed_mult * simulation_step;

	if (KeyStates[GR::EKey::W] != GR::EAction::Release) off.z += speed_mult * simulation_step;
	if (KeyStates[GR::EKey::S] != GR::EAction::Release) off.z -= speed_mult * simulation_step;

	if (KeyStates[GR::EKey::PageUp] != GR::EAction::Release) off.y += speed_mult * simulation_step;
	if (KeyStates[GR::EKey::PageDown] != GR::EAction::Release) off.y -= speed_mult * simulation_step;
	
	Context->GetMainCamera().View.Translate(off);

	if (LoadedScene == 0)
	{
		GRComponents::Transform& wld = Context->GetComponent<GRComponents::Transform>(object);
		wld.SetOffset(TVec3(0.0, 50.0, 0.0));
		wld.SetRotation(glm::radians(-90.0), 0.0, 0.0);
		wld.SetOffset(TVec3(0.0, 50.0 + glm::sin(global_angle) * 2.5, 0.0));
	}
	else if (LoadedScene == 2)
	{
		GRComponents::Transform& wld = Context->GetComponent<GRComponents::Transform>(object);
		wld.SetOffset(TVec3(0.0, 50.0 + glm::sin(global_angle) * 2.5, 0.0));
		wld.Rotate(0.0, 0.01, 0.0);
	}
}

void UI(GR::GrayEngine* Context, double Delta)
{
	if (LoadedScene == -1 || LoadedScene != SceneID) return;

	ImGui::SetCurrentContext(Context->GetGUIContext());

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
		GRComponents::RoughnessMultiplier& RM = Context->GetComponent<GRComponents::RoughnessMultiplier>(object);
		ImGui::SliderFloat("Roughness mult", &RM.R, 0.0, 1.0);

		GRComponents::DisplacementScale& HM = Context->GetComponent<GRComponents::DisplacementScale>(object);
		ImGui::SliderFloat("Height mult", &HM.H, 0.1, 10.0);

		const char* Materials[3] = { "Brick", "Concrete", "Painted metal" };
		ImGui::Combo("Materials", &MaterialID, Materials, 3);
	}
	else if (LoadedScene == 2)
	{
		GRComponents::RoughnessMultiplier& RM = Context->GetComponent<GRComponents::RoughnessMultiplier>(object);
		ImGui::SliderFloat("Roughness mult", &RM.R, 0.0, 1.0);
	}

	ImGui::End();
}

void KeyPress(GR::GrayEngine* Context, GREvent::KeyPress Event)
{
	KeyStates[Event.key] = Event.action;
}

void MousePress(GR::GrayEngine* Context, GREvent::MousePress Event)
{
	Cursor = Context->GetWindow().GetCursorPos();
	MousePressed = (Event.action != GR::EAction::Release);
}

void MouseMove(GR::GrayEngine* Context, GREvent::MousePosition Position)
{
	if (MousePressed)
	{
		CameraPYR += glm::radians(TVec3(Cursor.y - Position.y, Cursor.x - Position.x, 0.0));
		Context->GetMainCamera().View.SetRotation(CameraPYR.x, CameraPYR.y, CameraPYR.z);
		Cursor = { Position.x, Position.y };
	}
}

void MouseScroll(GR::GrayEngine* Context, GREvent::ScrollDelta Delta)
{
	speed_mult = glm::clamp(speed_mult + 2.5f * Delta.y, 1.0, 100.0);
}

int main(int argc, char** argv)
{
	std::string exec_path = "";

	if (argc > 0)
	{
		exec_path = argv[0];
		exec_path = exec_path.substr(0, exec_path.find_last_of('\\') + 1);
	}

	ApplicationSettings Settings = { "Vulkan Application", { 1024u, 720u } };
	TAuto<GR::GrayEngine> Engine = std::make_unique<GR::GrayEngine>(exec_path, Settings);

	Engine->AddInputFunction(UpdateResources);
	Engine->AddInputFunction(UI);
	Engine->AddInputFunction(Loop);
	Engine->GetEventListener().Subscribe(KeyPress);
	Engine->GetEventListener().Subscribe(MouseMove);
	Engine->GetEventListener().Subscribe(MousePress);
	Engine->GetEventListener().Subscribe(MouseScroll);
	Engine->StartGameLoop();

	return 0;
}