#include "pch.hpp"
#include "engine.hpp"

TVec3 CameraPYR;
TVec2 Cursor = TVec2(0.0);
double speed_mult = 1.0;
float Sun = 1.0;

bool MousePressed = false;
std::map<GR::EKey, GR::EAction> KeyStates;

CloudLayerProfile CloudLayer{};

void Loop(GR::GrayEngine* Context, double Delta)
{
	static CloudLayerProfile CurrentProfile = CloudLayer;
	constexpr double StepTime = 1.0 / 60.0;
	const double simulation_step = StepTime / Delta;
	double angle = glm::mod(Context->GetTime(), 360.0);

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

	if (CurrentProfile != CloudLayer)
	{
		Context->GetRenderer().SetCloudLayerSettings(CloudLayer);
		CurrentProfile = CloudLayer;
	}
}

void UI(GR::GrayEngine* Context, double Delta)
{
	ImGui::SetCurrentContext(Context->GetGUIContext());

	ImGui::Begin("Settings", 0, ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::SetWindowPos({ 0, 0 });

	MousePressed = MousePressed && !ImGui::IsWindowHovered();

	ImGui::SliderFloat("Sun position", &Sun, 0.0, 1.0);
	ImGui::SliderFloat("Coverage", &CloudLayer.Coverage, 0.0, 1.0);
	ImGui::SliderFloat("Vertical span", &CloudLayer.VerticalSpan, 0.0, 1.0);
	ImGui::SliderFloat("Wind speed", &CloudLayer.WindSpeed, 0.0, 1.0);
	ImGui::DragFloat("Absorption", &CloudLayer.Absorption, 1e-5, 0.0, 1.0, "%.5f");

	ImGui::End();
}

void KeyPress(GR::GrayEngine* Context, GREvent::KeyPress Event)
{
	KeyStates[Event.key] = Event.action;

	if (Event.action == GR::EAction::Press)
	{
		CloudLayerProfile Profile{};
		switch (Event.key)
		{
		case GR::EKey::Key_1:
			Sun = 1.0;
			break;
		case GR::EKey::Key_2:
			Sun = 0.495;
			break;
		case GR::EKey::Key_3:
			Sun = 0.52;
			Profile.VerticalSpan = 0.0;
			Profile.Coverage = 0.3;
			break;
		case GR::EKey::Key_4:
			Sun = 0.5;
			Profile.VerticalSpan = 0.49;
			Profile.Coverage = 0.185;
			break;
		default:
			break;
		}

		CloudLayer = Profile;
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
		Context->GetMainCamera().View.SetRotation(CameraPYR.x, CameraPYR.y, CameraPYR.z);
		Cursor = { Position.x, Position.y };
	}
}

int main(int argc, char** argv)
{
	ApplicationSettings Settings = { "Vulkan Application", { 1024, 720 } };
	std::unique_ptr<GR::GrayEngine> Engine = std::make_unique<GR::GrayEngine>(argc, argv, Settings);

	Engine->AddInputFunction(Loop);
	Engine->AddInputFunction(UI);

	Engine->GetEventListener().Subscribe(KeyPress);
	Engine->GetEventListener().Subscribe(MouseMove);
	Engine->GetEventListener().Subscribe(MousePress);

	Engine->GetMainCamera().View.SetOffset({ 0.0, 50.0, 0.0 });
	Engine->GetRenderer().SetCloudLayerSettings(CloudLayer);

	Engine->StartGameLoop();

	return 0;
}