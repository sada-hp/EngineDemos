#include "pch.hpp"
#include "Engine/engine.hpp"
#include "Engine/converter.hpp"

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
		Context->BindImage(Context->GetComponent<GRComponents::NormalDisplacementMap>(object), "content\\brick_nh.png", GR::EImageType::RGBA_UNORM);
		Context->BindImage(Context->GetComponent<GRComponents::AORoughnessMetallicMap>(object), "content\\brick_arm.jpg", GR::EImageType::RGBA_UNORM);
		break;
	case 1:
		Context->BindImage(Context->GetComponent<GRComponents::AlbedoMap>(object), "content\\concrete_albedo.jpg", GR::EImageType::RGBA_SRGB);
		Context->BindImage(Context->GetComponent<GRComponents::NormalDisplacementMap>(object), "content\\concrete_nh.png", GR::EImageType::RGBA_UNORM);
		Context->BindImage(Context->GetComponent<GRComponents::AORoughnessMetallicMap>(object), "content\\concrete_arm.jpg", GR::EImageType::RGBA_UNORM);
		break;
	case 2:
		Context->BindImage(Context->GetComponent<GRComponents::AlbedoMap>(object), "content\\metal_albedo.jpg", GR::EImageType::RGBA_SRGB);
		Context->BindImage(Context->GetComponent<GRComponents::NormalDisplacementMap>(object), "content\\metal_nh.png", GR::EImageType::RGBA_UNORM);
		Context->BindImage(Context->GetComponent<GRComponents::AORoughnessMetallicMap>(object), "content\\metal_arm.jpg", GR::EImageType::RGBA_UNORM);
		break;
	default:
		break;
	}
}

void LoadMaterialWall(GR::GrayEngine* Context)
{
	Context->ClearEntities();

	GRShape::Plane Shape;
	Shape.m_Scale = 20.f;

	object = Context->AddShape(Shape);
	CameraPYR = { 0.0, glm::radians(210.0), 0.0 };
	Context->GetMainCamera().View.SetOffset({ 10.0, 50.0, 20.0 });
	Context->GetMainCamera().View.SetRotation(CameraPYR.x, CameraPYR.y, CameraPYR.z);
}

void LoadSpheresScene(GR::GrayEngine* Context)
{
	Context->ClearEntities();

	GRShape::Sphere Shape;
	Shape.m_Radius = 10.f;
	Shape.m_Rings = 64u;
	Shape.m_Slices = 64u;

	for (uint32_t i = 0; i < 4; i++)
	{
		for (uint32_t j = 0; j < 4; j++)
		{
			spheres[i] = Context->AddShape(Shape);

			Context->GetComponent<GRComponents::Transform<float>>(spheres[i]).SetOffset(TVec3(i * 25.0, j * 25.0 + 20.0, 0.0));
			Context->EmplaceComponent<GRComponents::RGBColor>(spheres[i], GRComponents::RGBColor{ TVec3(1.0, 0.0, 0.0) });
			Context->GetComponent<GRComponents::RoughnessMultiplier>(spheres[i]).Value = (i + 1) * 0.25;
			Context->GetComponent<GRComponents::MetallicOverride>(spheres[i]).Value = (j + 1) * 0.25;
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
	Context->BindImage(Context->GetComponent<GRComponents::NormalDisplacementMap>(object), "content\\graff_nh.png", GR::EImageType::RGBA_UNORM);
	Context->BindImage(Context->GetComponent<GRComponents::AORoughnessMetallicMap>(object), "content\\graff_arm.jpg", GR::EImageType::RGBA_UNORM);
	Context->GetComponent<GRComponents::Transform<float>>(object).SetScale(2.0, 2.0, 2.0);

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
	Context->GetRenderer().m_SunDirection = glm::normalize(glm::vec3(0.0, Sun * 2.0 - 1.0, Sun));

	TVec3 off = TVec3(0.0);
	if (KeyStates[GR::EKey::A] != GR::EAction::Release) off.x += speed_mult * simulation_step;
	if (KeyStates[GR::EKey::D] != GR::EAction::Release) off.x -= speed_mult * simulation_step;

	if (KeyStates[GR::EKey::W] != GR::EAction::Release) off.z += speed_mult * simulation_step;
	if (KeyStates[GR::EKey::S] != GR::EAction::Release) off.z -= speed_mult * simulation_step;

	if (KeyStates[GR::EKey::PageUp] != GR::EAction::Release) off.y += speed_mult * simulation_step;
	if (KeyStates[GR::EKey::PageDown] != GR::EAction::Release) off.y -= speed_mult * simulation_step;
	
	Context->GetMainCamera().View.Translate(off);

	TVec3 U = glm::normalize(TDVec3(0.0, Context->GetRenderer().Rg, 0.0) + Context->GetMainCamera().View.GetOffset());
	TQuat p = glm::rotation(glm::vec3(0.0, 1.0, 0.0), U);

	TQuat q = glm::angleAxis(CameraPYR.y, U);
	q = q * glm::angleAxis(CameraPYR.z, p * glm::vec3(0, 0, 1));
	q = q * glm::angleAxis(-CameraPYR.x, p * glm::vec3(1, 0, 0));

	glm::mat3 M = glm::mat3_cast(q * p);

	Context->GetMainCamera().View.matrix[0] = glm::dvec4(glm::normalize(M[0]), 0.0);
	Context->GetMainCamera().View.matrix[1] = glm::dvec4(glm::normalize(M[1]), 0.0);
	Context->GetMainCamera().View.matrix[2] = glm::dvec4(glm::normalize(M[2]), 0.0);

	if (LoadedScene == 0)
	{
		GRComponents::Transform<float>& wld = Context->GetComponent<GRComponents::Transform<float>>(object);
		wld.SetOffset(TVec3(0.0, 50.0, 0.0));
		wld.SetRotation(glm::radians(-90.0), 0.0, 0.0);
		wld.SetOffset(TVec3(0.0, 50.0 + glm::sin(global_angle) * 2.5, 0.0));
	}
	else if (LoadedScene == 2)
	{
		GRComponents::Transform<float>& wld = Context->GetComponent<GRComponents::Transform<float>>(object);
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
		ImGui::SliderFloat("Roughness mult", &RM.Value, 0.0, 1.0);

		GRComponents::DisplacementScale& HM = Context->GetComponent<GRComponents::DisplacementScale>(object);
		ImGui::SliderFloat("Height mult", &HM.Value, 0.1, 10.0);

		const char* Materials[3] = { "Brick", "Concrete", "Painted metal" };
		ImGui::Combo("Materials", &MaterialID, Materials, 3);
	}
	else if (LoadedScene == 2)
	{
		GRComponents::RoughnessMultiplier& RM = Context->GetComponent<GRComponents::RoughnessMultiplier>(object);
		ImGui::SliderFloat("Roughness mult", &RM.Value, 0.0, 1.0);
	}

	ImGui::End();
}

void KeyPress(GR::GrayEngine* Context, GREvent::KeyPress Event)
{
	KeyStates[Event.key] = Event.action;

	if (Event.action == GR::EAction::Press)
	{
		switch (Event.key)
		{
		case GR::EKey::Key_1:
			Sun = 1.0;
			break;
		case GR::EKey::Key_2:
			Sun = 0.85;
			break;
		case GR::EKey::Key_3:
			Sun = 0.52;
			break;
		case GR::EKey::Key_4:
			Sun = 0.505;
			break;
		default:
			break;
		}
	}
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
		Cursor = { Position.x, Position.y };
	}
}

void MouseScroll(GR::GrayEngine* Context, GREvent::ScrollDelta Delta)
{
	speed_mult = glm::clamp(speed_mult + 2.5f * Delta.y, 1.0, 100.0);
}

void LaunchConvert()
{
	// arm
	GRConvert::ConvertImage_ARM("content\\brick_roughness.jpg", "", "content\\brick_ao.jpg", "content\\brick_arm.jpg");
	GRConvert::ConvertImage_ARM("content\\concrete_roughness.jpg", "", "content\\concrete_ao.jpg", "content\\concrete_arm.jpg");
	GRConvert::ConvertImage_ARM("content\\metal_roughness.jpg", "", "content\\metal_ao.jpg", "content\\metal_arm.jpg");
	GRConvert::ConvertImage_ARM("content\\graff_roughness.jpg", "", "content\\graff_ao.jpg", "content\\graff_arm.jpg");

	// nh
	GRConvert::ConvertImage_NormalHeight("content\\brick_normal.jpg", "content\\brick_height.jpg", "content\\brick_nh.png");
	GRConvert::ConvertImage_NormalHeight("content\\concrete_normal.jpg", "content\\concrete_height.jpg", "content\\concrete_nh.png");
	GRConvert::ConvertImage_NormalHeight("content\\metal_normal.jpg", "content\\metal_height.jpg", "content\\metal_nh.png");
	GRConvert::ConvertImage_NormalHeight("content\\graff_normal.jpg", "", "content\\graff_nh.png");
}

int main(int argc, char** argv)
{
	ApplicationSettings Settings = { "Vulkan Application", { 1024u, 720u } };
	TAuto<GR::GrayEngine> Engine = std::make_unique<GR::GrayEngine>(argc, argv, Settings);

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