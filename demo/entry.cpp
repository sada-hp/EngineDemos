#include "pch.hpp"
#include "engine.hpp"

GR::Entity object;

TVec3 SunDir = glm::normalize(TVec3(0.5, 0.1, 1.0));
TVec3 CameraPYR = TVec3(0.0);
TVec2 Cursor = TVec2(0.0);

double speed_mult = 1.0;
float Sun = 1.0;

bool MousePressed = false;
std::map<GR::EKey, GR::EAction> KeyStates;

constexpr double StepTime = 1.0 / 60.0;

void Loop(GR::GrayEngine* Context, double Delta)
{
	const double simulation_step = StepTime / Delta;
	const double global_angle = glm::mod(Context->GetTime(), 360.0);
	Context->GetWindow().SetTitle(("Vulkan Application " + std::format("{:.1f}", 1.0 / Delta)).c_str());

	GRComponents::Transform& wld = Context->GetComponent<GRComponents::Transform>(object);
	wld.SetOffset(TVec3(0.0, 50.0 + glm::sin(global_angle) * 2.5, 0.0));
	wld.Rotate(0.0, 0.01, 0.0);

	Context->GetRenderer().SunDirection = glm::normalize(glm::vec3(0.0, Sun * 2.0 - 1.0, Sun));

	TVec3 off = TVec3(0.0);
	if (KeyStates[GR::EKey::A] != GR::EAction::Release) off.x += speed_mult * simulation_step;
	if (KeyStates[GR::EKey::D] != GR::EAction::Release) off.x -= speed_mult * simulation_step;

	if (KeyStates[GR::EKey::W] != GR::EAction::Release) off.z += speed_mult * simulation_step;
	if (KeyStates[GR::EKey::S] != GR::EAction::Release) off.z -= speed_mult * simulation_step;

	if (KeyStates[GR::EKey::PageUp] != GR::EAction::Release) off.y += speed_mult * simulation_step;
	if (KeyStates[GR::EKey::PageDown] != GR::EAction::Release) off.y -= speed_mult * simulation_step;
	
	Context->GetMainCamera().View.Translate(off);
}

void UI(GR::GrayEngine* Context, double Delta)
{
	ImGui::SetCurrentContext(Context->GetGUIContext());

	ImGui::Begin("Settings");
	ImGui::SetWindowPos({ 0, 0 });
	ImGui::SetWindowSize({ 350, 100 });

	GRComponents::RoughnessMultiplier& RM = Context->GetComponent<GRComponents::RoughnessMultiplier>(object);
	ImGui::SliderFloat("Roughness multiplier", &RM.R, 0.0, 1.0);
	ImGui::SliderFloat("Sun position", &Sun, 0.0, 1.0);

	ImGui::End();
}

void KeyPress(GR::GrayEngine* Context, GR::EKey Key, GR::EAction Action)
{
	KeyStates[Key] = Action;
}

void MousePress(GR::GrayEngine* Context, GR::EMouse Button, GR::EAction Action)
{
	Cursor = Context->GetWindow().GetCursorPos();
	MousePressed = (Action != GR::EAction::Release && (Cursor.x > 350.0 || Cursor.y > 100.0));
}

void MouseMove(GR::GrayEngine* Context, GREvent::MousePosition Position)
{
	if (MousePressed && (Position.x > 350.0 || Position.y > 100.0))
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

	Engine->AddInputFunction(UI);
	Engine->AddInputFunction(Loop);
	Engine->GetEventListener().Subscribe(KeyPress);
	Engine->GetEventListener().Subscribe(MouseMove);
	Engine->GetEventListener().Subscribe(MousePress);
	Engine->GetEventListener().Subscribe(MouseScroll);

	GRShape::Sphere Shape;
	Shape.radius = 10.f;
	Shape.rings = 64u;
	Shape.slices = 64u;

	object = Engine->AddShape(Shape);
	Engine->BindImage(Engine->GetComponent<GRComponents::AlbedoMap>(object), "content\\albedo.jpg", GR::EImageType::RGBA_SRGB);
	Engine->BindImage(Engine->GetComponent<GRComponents::NormalMap>(object), "content\\normal.jpg", GR::EImageType::RGBA_UNORM);
	Engine->BindImage(Engine->GetComponent<GRComponents::RoughnessMap>(object), "content\\roughness.jpg", GR::EImageType::RGBA_UNORM);
	Engine->BindImage(Engine->GetComponent<GRComponents::AmbientMap>(object), "content\\ao.jpg", GR::EImageType::RGBA_UNORM);
	//Engine->BindImage(Engine->GetComponent<GRComponents::MetallicMap>(object), "content\\metallic.jpg", GR::EImageType::RGBA_UNORM);

	Engine->GetMainCamera().View.SetOffset({ 0.0, 50.0, -30.0 });
	Engine->GetRenderer().SunDirection = SunDir;
	Engine->StartGameLoop();	

	return 0;
}