#include "gui.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "../interface/interface.h"

#include <d3d11.h>
#include <tchar.h>
#include <filesystem>

std::string path = "";

int panel = 0;
int selected_func = 0;
char func_name[1024];

std::vector<pdbparser::sym_func>funcs;
std::vector<pdbparser::sym_func>funcs_to_obfuscate;
std::vector<std::string>logs;
bool obf_entry_point;

bool b_show_file_selection_menu = true;

bool b_is_style_initialized = false;
void init_style()
{
	ImGuiStyle& style = ImGui::GetStyle();

	style.WindowRounding = 0.0f;
	style.FramePadding = ImVec2(6, 6);
	style.FrameRounding = 10.0f;

	style.Colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);

	// ImGuiIO& io = ImGui::GetIO();
	// io.Fonts->AddFontFromFileTTF("path_to_your_font.ttf", 16.0f);
}

bool b_is_compiling = false;
void gui::render_interface()
{
	if (!b_is_style_initialized)
		init_style();

	if (b_show_file_selection_menu)
	{
		ImGui::SetNextWindowSize(ImVec2(1200, 800));
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::Begin("Alcatraz - Select File", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove);
		{
			ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 100.0f, ImGui::GetContentRegionAvail().y * 0.5f - 60.0f));
			if (ImGui::Button("Select File...", ImVec2(200.0f, 60.0f)))
			{
				bool b_valid_file = true;

char filename[MAX_PATH];

OPENFILENAMEA ofn;
ZeroMemory(&filename, sizeof(filename));
ZeroMemory(&ofn, sizeof(ofn));
ofn.lStructSize = sizeof(ofn);
ofn.hwndOwner = NULL;
ofn.lpstrFilter = "Executables\0*.exe\0Dynamic Link Libraries\0*.dll\0Drivers\0*.sys";
ofn.lpstrFile = filename;
ofn.nMaxFile = MAX_PATH;
ofn.lpstrTitle = "Select your file.";
ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
GetOpenFileNameA(&ofn);

if (!std::filesystem::exists(filename)) {
	MessageBoxA(0, "Couldn't find file!", "Error", 0);

	b_valid_file = false;
}
else
{
	path = filename;
	try
	{
		funcs = inter::load_context(path);
	}
	catch (std::runtime_error e)
	{
		MessageBoxA(0, e.what(), "Exception", 0);
		path = "";

		b_valid_file = false;
	}
	selected_func = 0;
	funcs_to_obfuscate.clear();
}

if (b_valid_file)
b_show_file_selection_menu = false;
			}
		}
		ImGui::End();
	}
	else
	{
		ImGui::SetNextWindowSize(ImVec2(1200, 800));
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::Begin("Alcatraz", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove);
		{
			ImGui::BeginChild("##selection_panel", ImVec2(1200, 75), false, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			{
				const ImVec2 buttonSize = ImVec2(580.0f, 70.0f);
				const char* buttons[2] = { "Protection", "About" };

				for (int i = 0; i < 2; ++i)
				{
					if (panel == i)
					{
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.35f, 0.39f, 1.00f));
					}

					if (i > 0) ImGui::SameLine();
					if (ImGui::Button(buttons[i], buttonSize))
					{
						panel = i;
					}

					if (panel == i)
					{
						ImGui::PopStyleColor();
					}
				}
			}
			ImGui::EndChild();
			ImGui::Separator();
			ImGui::BeginChild("##content_panel", ImVec2(ImGui::GetContentRegionAvail().x - 15.0f, ImGui::GetContentRegionAvail().y - 40.0f), false, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			{
				switch (panel)
				{
				case 0: //protection
					ImGui::BeginChild("##left_side", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, ImGui::GetContentRegionAvail().y));
					{
						static float panel_size = ImGui::GetContentRegionAvail().y * 0.45f;
						ImGui::BeginChild("##added_functions_panel", ImVec2(ImGui::GetContentRegionAvail().x, panel_size), true, ImGuiWindowFlags_HorizontalScrollbar);
						{
							ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x * 0.5f) - (ImGui::CalcTextSize("Added Functions").x * 0.5f));
							ImGui::Text("Added Functions");
							if (ImGui::TreeNode("##Added functions")) 
							{
								for (int i = 0; i < funcs_to_obfuscate.size(); i++)
								{
									if (ImGui::Button(funcs_to_obfuscate.at(i).name.c_str()))
										selected_func = funcs_to_obfuscate.at(i).id;
								}
								ImGui::TreePop();
							}
						}
						ImGui::EndChild();
						ImGui::BeginChild("##functions_panel", ImVec2(ImGui::GetContentRegionAvail().x, panel_size), true, ImGuiWindowFlags_HorizontalScrollbar);
						{
							ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x * 0.5f) - (ImGui::CalcTextSize("Functions").x * 0.5f));
							ImGui::Text("Functions");
							if (ImGui::TreeNode("##Functions")) 
							{
								for (int i = 0; i < funcs.size(); i++)
								{
									if (funcs.at(i).size >= 5 && (funcs.at(i).name.find(func_name) != std::string::npos)) 
									{
										if (ImGui::Button(funcs.at(i).name.c_str()))
											selected_func = funcs.at(i).id;
									}
								}
								ImGui::TreePop();
							}
						}
						ImGui::EndChild();
						if (ImGui::Button("Add All", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetContentRegionAvail().y)))
						{
							while (funcs.size() != 0) 
							{
								funcs_to_obfuscate.push_back(funcs.front());
								funcs.erase(funcs.begin());
							}
						}
						ImGui::SameLine();
						if (ImGui::Button("Compile", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)))
						{
							try
							{
								inter::run_obfuscator(funcs_to_obfuscate, obf_entry_point);
							}
							catch (std::runtime_error e)
							{
								MessageBoxA(0, e.what(), "exception", 0);
								path = "";
								exit(EXIT_FAILURE);
							}
							MessageBoxA(0, "file has been compiled.", "success", 0);
							exit(EXIT_SUCCESS);
						}
					}
					ImGui::EndChild();
					ImGui::SameLine();
					ImGui::BeginChild("##right_side", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y));
					{
						ImGui::BeginChild("##function_config_panel", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.5f));
						{
							ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x * 0.5f) - (ImGui::CalcTextSize("Function").x * 0.5f));
							ImGui::Text("Function");

							auto already_added = std::find_if(funcs_to_obfuscate.begin(), funcs_to_obfuscate.end(), [&](const pdbparser::sym_func infunc) { return infunc.id == selected_func; });
							if (already_added != funcs_to_obfuscate.end())
							{
								auto func = already_added;
								ImGui::Text("Name :     %s", func->name.c_str());
								ImGui::Text("Address :  %x", func->offset);
								ImGui::Text("Size :     %i bytes", func->size);

								ImGui::Checkbox("Control flow flattening", &func->ctfflattening);
								ImGui::Checkbox("Immediate MOV obfuscation", &func->movobf);
								ImGui::Checkbox("Mutate", &func->mutateobf);
								ImGui::Checkbox("LEA obfuscation", &func->leaobf);
								ImGui::Checkbox("Anti disassembly", &func->antidisassembly);
							}
							else
							{
								auto func = std::find_if(funcs.begin(), funcs.end(), [&](const pdbparser::sym_func infunc) { return infunc.id == selected_func; });
								ImGui::Text("Name :     %s", func->name.c_str());
								ImGui::Text("Address :  %x", func->offset);
								ImGui::Text("Size :     %i bytes", func->size);

								ImGui::Checkbox("Control flow flattening", &func->ctfflattening);
								ImGui::Checkbox("Immediate MOV obfuscation", &func->movobf);
								ImGui::Checkbox("Mutate", &func->mutateobf);
								ImGui::Checkbox("LEA obfuscation", &func->leaobf);
								ImGui::Checkbox("Anti disassembly", &func->antidisassembly);

								if (ImGui::Button("Add to list"))
								{
									if (std::find_if(funcs_to_obfuscate.begin(), funcs_to_obfuscate.end(), [&](const pdbparser::sym_func infunc) {return infunc.id == func->id; }) == funcs_to_obfuscate.end())
									{
										funcs_to_obfuscate.push_back(*func);
										funcs.erase(funcs.begin() + selected_func);
									}
								}
							}
						}
						ImGui::EndChild();
						ImGui::BeginChild("##misc_panel", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y));
						{
							ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x * 0.5f) - (ImGui::CalcTextSize("Misc").x * 0.5f));
							ImGui::Text("Misc");

							ImGui::Checkbox("Obfuscate entry point", &obf_entry_point);
						}
						ImGui::EndChild();
					}
					ImGui::EndChild();
					break;
				case 1: //about
					ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x * 0.5f) - (ImGui::CalcTextSize("developed & published by weak1337").x * 0.5f));
					ImGui::Text("developed & published by weak1337");
					ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x * 0.5f) - (ImGui::CalcTextSize("contributers: raigorx, FigmaFan").x * 0.5f));
					ImGui::Text("contributers: raigorx, FigmaFan");

					break;
				default:
					break;
				}
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

	/*
	ImGui::SetNextWindowSize(ImVec2(1280, 800));
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::Begin("Alcaztaz", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollWithMouse);

	if (ImGui::BeginMenuBar()) {

		if (ImGui::BeginMenu("File")) {

			if (ImGui::MenuItem("Open")) {

				char filename[MAX_PATH];

				OPENFILENAMEA ofn;
				ZeroMemory(&filename, sizeof(filename));
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = NULL; 
				ofn.lpstrFilter = "Executables\0*.exe\0Dynamic Link Libraries\0*.dll\0Drivers\0*.sys";
				ofn.lpstrFile = filename;
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrTitle = "Select your file.";
				ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
				GetOpenFileNameA(&ofn);

				if (!std::filesystem::exists(filename)) {
					MessageBoxA(0, "Couldn't find file!", "Error", 0);
				}
				else {
					path = filename;
					try {
						funcs = inter::load_context(path);
					}
					catch (std::runtime_error e)
					{
						MessageBoxA(0, e.what(), "Exception", 0);
						path = "";
					}
					selected_func = 0;
					funcs_to_obfuscate.clear();
					
				}

			}

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	if (path.size()) {

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.12f, 0.94f));
		ImGui::BeginChild("selectionpanel", ImVec2(100, 800), true, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		if (ImGui::Button("Protection", ImVec2(100, 100)))
			panel = 0;

		ImGui::EndChild();


		if (panel == 0) 
		{
			ImGui::SetNextWindowPos(ImVec2(100, 25));

			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.24f, 0.24f, 0.24f, 0.94f));
			if (ImGui::BeginChild("optionpanel", ImVec2(300, 775), true, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove )) {

			
				ImGui::SetNextItemWidth(300);
				ImGui::InputText("##treeAddFuncs", func_name, 1024);

				if (ImGui::TreeNode("Added functions")) {
					for (int i = 0; i < funcs_to_obfuscate.size(); i++)
					{
						if (ImGui::Button(funcs_to_obfuscate.at(i).name.c_str()))
							selected_func = funcs_to_obfuscate.at(i).id;

					}

					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Functions")) {
					for (int i = 0; i < funcs.size(); i++)
					{
						if (funcs.at(i).size >= 5 && (funcs.at(i).name.find(func_name) != std::string::npos)) {
							if (ImGui::Button(funcs.at(i).name.c_str()))
								selected_func = funcs.at(i).id;
						}
					
					}

					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Misc")) {
					ImGui::Checkbox("Obfuscate entry point", &obf_entry_point);
					ImGui::TreePop();
				}

				
				if (ImGui::Button("Add all")) {

					while (funcs.size() != 0) {
						funcs_to_obfuscate.push_back(funcs.front());
						funcs.erase(funcs.begin());
					}

				}

				if (ImGui::Button("Compile")) {
					try {
						inter::run_obfuscator(funcs_to_obfuscate, obf_entry_point);
						MessageBoxA(0, "Compiled", "Success", 0);
					}
					catch (std::runtime_error e)
					{
						MessageBoxA(0, e.what(), "Exception", 0);
						path = "";
						//std::cout << "Runtime error: " << e.what() << std::endl;
					}
				}
			}
			ImGui::EndChild();

			
			ImGui::SetNextWindowPos(ImVec2(400, 25));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.48f, 0.48f, 0.48f, 0.94f));
			ImGui::BeginChild("functionpanel", ImVec2(880,775), true, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);


			
			auto already_added = std::find_if(funcs_to_obfuscate.begin(), funcs_to_obfuscate.end(), [&](const pdbparser::sym_func infunc) { return infunc.id == selected_func; });
			if (already_added != funcs_to_obfuscate.end()) {

				auto func = already_added;
				ImGui::Text("Name : %s", func->name.c_str());
				ImGui::Text("Address : %x", func->offset);
				ImGui::Text("Size : %i bytes", func->size);

				ImGui::Checkbox("Control flow flattening", &func->ctfflattening);
				ImGui::Checkbox("Immediate MOV obfuscation", &func->movobf);
				ImGui::Checkbox("Mutate", &func->mutateobf);
				ImGui::Checkbox("LEA obfuscation", &func->leaobf);
				ImGui::Checkbox("Anti disassembly", &func->antidisassembly);
			}
			else {

				auto func = std::find_if(funcs.begin(), funcs.end(), [&](const pdbparser::sym_func infunc) { return infunc.id == selected_func; });
				ImGui::Text("Name : %s", func->name.c_str());
				ImGui::Text("Address : %x", func->offset);
				ImGui::Text("Size : %i bytes", func->size);

				ImGui::Checkbox("Control flow flattening", &func->ctfflattening);
				ImGui::Checkbox("Immediate MOV obfuscation", &func->movobf);
				ImGui::Checkbox("Mutate", &func->mutateobf);
				ImGui::Checkbox("LEA obfuscation", &func->leaobf);
				ImGui::Checkbox("Anti disassembly", &func->antidisassembly);

				if (ImGui::Button("Add to list")) {

					if (std::find_if(funcs_to_obfuscate.begin(), funcs_to_obfuscate.end(), [&](const pdbparser::sym_func infunc) {return infunc.id == func->id; }) == funcs_to_obfuscate.end()) {
						funcs_to_obfuscate.push_back(*func);
						funcs.erase(funcs.begin() + selected_func);
					}
				}
			}
			
			ImGui::SetCursorPosY(700);
			ImGui::Text(path.c_str());

			ImGui::EndChild();

			ImGui::PopStyleColor();

			ImGui::PopStyleColor();

		}
		

		ImGui::PopStyleColor();

		
	}

	ImGui::End();
	*/
}
