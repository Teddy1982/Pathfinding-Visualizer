#include <string>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "../globals/global_constants.h"
#include "../tools/logger.h"
#include "../tools/controls.h"
#include "../application/app_state.h"
#include "user_interface.h"
#include "ui_command_buffer.h"

Controls gControls;
RebindContext rebindContext;

UserInterface::UserInterface(AppState& state, AlgoLogic& logicLeft, AlgoLogic& logicRight) : appState(state), algoLogicLeft(logicLeft), algoLogicRight(logicRight) {}

// Initialisiert Benutzermenü 
bool UserInterface::init(RenderData& renderData) {
  gControls.loadBindings();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  VkDescriptorPoolSize imguiPoolSizes[] =
  {
    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
  };

  VkDescriptorPoolCreateInfo imguiPoolInfo{};
  imguiPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  imguiPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  imguiPoolInfo.maxSets = 1000;
  imguiPoolInfo.poolSizeCount = std::size(imguiPoolSizes);
  imguiPoolInfo.pPoolSizes = imguiPoolSizes;

  if (vkCreateDescriptorPool(renderData.vkInst.device, &imguiPoolInfo, nullptr, &renderData.imguiDescriptorPool)) {
    Logger::log(1, "%s error: could not init ImGui descriptor pool \n", __FUNCTION__);
    return false;
  }

  ImGui_ImplGlfw_InitForVulkan(renderData.window, true);

  ImGui_ImplVulkan_InitInfo imguiIinitInfo{};
  imguiIinitInfo.Instance = renderData.vkInst.instance;
  imguiIinitInfo.PhysicalDevice = renderData.vkInst.physicalDevice;
  imguiIinitInfo.Device = renderData.vkInst.device;
  imguiIinitInfo.Queue = renderData.vkInst.graphicsQueue;
  imguiIinitInfo.DescriptorPool = renderData.imguiDescriptorPool;
  imguiIinitInfo.MinImageCount = 2;
  imguiIinitInfo.ImageCount = renderData.swapchain.images.size();
  imguiIinitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

  ImGui_ImplVulkan_Init(&imguiIinitInfo, renderData.renderPass.renderPass);

  VkCommandBuffer imguiCommandBuffer;

  if (!CommandBuffer::init(renderData, imguiCommandBuffer)) {
    Logger::log(1, "%s error: could not create texture upload command buffers\n", __FUNCTION__);
    return false;
  }

  if (vkResetCommandBuffer(imguiCommandBuffer, 0) != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to reset imgui command buffer\n", __FUNCTION__);
    return false;
  }

  VkCommandBufferBeginInfo cmdBeginInfo{};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if(vkBeginCommandBuffer(imguiCommandBuffer, &cmdBeginInfo) != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to begin imgui command buffer\n", __FUNCTION__);
    return false;
  }

  ImGui_ImplVulkan_CreateFontsTexture(imguiCommandBuffer);

  if (vkEndCommandBuffer(imguiCommandBuffer) != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to end staging command buffer\n", __FUNCTION__);
    return false;
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pWaitDstStageMask = nullptr;
  submitInfo.waitSemaphoreCount = 0;
  submitInfo.pWaitSemaphores = nullptr;
  submitInfo.signalSemaphoreCount = 0;
  submitInfo.pSignalSemaphores = nullptr;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &imguiCommandBuffer;;

  VkFence imguiBufferFence;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  if (vkCreateFence(renderData.vkInst.device, &fenceInfo, nullptr, &imguiBufferFence) != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to imgui buffer fence\n", __FUNCTION__);
    return false;
  }

  if (vkResetFences(renderData.vkInst.device, 1, &imguiBufferFence) != VK_SUCCESS) {
    Logger::log(1, "%s error: imgui buffer fence reset failed", __FUNCTION__);
    return false;
  }

  if (vkQueueSubmit(renderData.vkInst.graphicsQueue, 1, &submitInfo, imguiBufferFence) != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to imgui init command buffer\n", __FUNCTION__);
    return false;
  }

  if (vkWaitForFences(renderData.vkInst.device, 1, &imguiBufferFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
    Logger::log(1, "%s error: waiting for imgui init fence failed", __FUNCTION__);
    return false;
  }

  vkDestroyFence(renderData.vkInst.device, imguiBufferFence, nullptr);
  CommandBuffer::cleanup(renderData, imguiCommandBuffer);

  ImGui_ImplVulkan_DestroyFontUploadObjects();

  ImGui::StyleColorsDark();

  return true;
}

// Erstellt Ansicht des Benutzermenüs
void UserInterface::createFrame(RenderData& renderData) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiWindowFlags imguiWindowFlags = 0;
    //imguiWindowFlags |= ImGuiWindowFlags_NoCollapse;
    //imguiWindowFlags |= ImGuiWindowFlags_NoResize;
    //imguiWindowFlags |= ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowBgAlpha(0.8f);
    ImGui::SetNextWindowSize(ImVec2(350, 400));

    ImGui::Begin("Einstellungen", nullptr, imguiWindowFlags);

    if (ImGui::CollapsingHeader("Anleitung"))
    {
        ImGui::Text(
            "Steuerung: Oeffnen Sie den Reiter 'Steuerung'\n"
            "und machen Sie sich mit der Steuerung fuer die\n"
            "Anwendung vertraut.\n"
            "Wenn Sie in der Lage sind das markierte Feld\n"
            "innerhalb des Suchraums zu wechseln und die\n"
            "Kamera zu steuern, koennen Sie mit der\n" 
            "Gestaltung des Suchraums beginnen.\n\n"
            "Start und Ziel festlegen : Oeffnen Sie\n" 
            "den Reiter 'Aktionen' um den Startknoten\n" 
            "und den Zielknoten in verschiedene Felder zu\n"
            "platzieren. Sie koennen auch bei Bedarf\n"
            "mehrere Hindernisse setzen. Bitte beachten\n"
            "Sie, dass zum Start des Suchalgorithmus\n"
            "mindestens ein Start- und ein Zielknoten im\n"
            "Suchraum vorhanden sein muss.\n\n"
            "Pfadsuche starten: Fuehren Sie die Aktion\n" 
            "'Pfad suchen' aus, um die Pfadsuche zu beginnen.\n"
            "Alternativ stehen Ihnen fuer die Pfadsuche die\n"
            "Optionen 'Pfad schrittweise suchen' und\n"
            "'Pfadsuche animieren' zur Verfuegung.\n"
            "Hinweis: Wenn der Algorithmus stoppt und kein\n"
            "gelber Pfad erscheint, ist das Ziel\n"
            "vollstaendig von Hindernissen eingeschlossen\n"
            "und unerreichbar.\n\n"
            "Sucheinstellungen: Wenn Sie den Reiter\n"
            "'Sucheinstellungen' oeffnen, koennen Sie\n"
            "zukuenftige Pfadsuchen beeinflussen, indem\n"
            "Sie entscheiden, ob die Suche in einem\n"
            "dreidimensionalen oder zweidimensionalen\n"
            "Suchraum und ob die Suche nur in horizontaler\n"
            "und vertikaler Richtung oder zusaetzlich\n"
            "diagonal stattfinden soll.\n\n"
            "Suchalgorithmus: Nach dem Oeffnen des Reiters\n"
            "'Suchalgorithmus' sind Sie in der Lage den\n"
            "Suchalgorithmus fuer die Pfadfindung zu\n"
            "wechseln. Auch koennen Sie zwei Such-\n"
            "algorithmen bei ihrem Vorgehen vergleichen.\n"
            "Bei der Gestaltung der beiden Suchraeume ist\n"
            "zu beachten, dass diese identisch aufgesetzt\n"
            "werden.\n\n"
            "Grid-Groesse: Nach dem Oeffnen des Reiters\n"
            "'Grid-Groesse' koennen Sie die Dimension des\n"
            "Suchraums skalieren. Dies gilt fuer den drei-\n"
            "dimensionalen sowie dem zweidimensionalen\n"
            "Suchraum, nur dass beim zweidimensionalem\n"
            "Suchraum die Eingabe zur Z-Achse ignoriert\n"
            "wird.\n\n"
            "Zum Verstaendnis des Vorgehens des Algorithmus\n"
            "werden Daten zur Pfadsuche nach dem Oeffnen\n"
            "des Reiters 'Bewertungsinformationen'\n"
            "sichtbar. Unter anderem werden Informationen\n"
            "zur Pfadsuche durch das Auswaehlen eines\n"
            "bereits bearbeiteten Knotens angezeigt. Auch\n"
            "koennen hier Informationen beider Algorithmen\n"
            "beim Algorithmenvergleich eingesehen werden.\n\n"
            "Der Reiter 'Legende & Info' beinhaltet\n"
            "Informationen zur Suchraumdarstellung\n"
            "und den Bewertungsinformationen.\n"
            "Durch das Bewegen der Maus ueber das\n" 
            "Symbol [i] erscheinen in Form eines Tooltips\n"
            "weitere Informationen zum entsprechenden\n"
            "Eintrag."
        );
    }

    if (ImGui::CollapsingHeader("Steuerung")) {
        static KeyBindings editableBindings = gControls.getKeyBindings();

        auto showRebindButton = [&](const char* label, int* keyPtr) {
            std::string buttonLabel = std::string(label) + ": " + gControls.getKeyDisplayName(*keyPtr);
            if (ImGui::Button(buttonLabel.c_str(), ImVec2(250, 30))) {
                rebindContext.awaitingKey = true;
                rebindContext.actionName = label;
                rebindContext.targetKey = keyPtr;
            }
            };

        if (rebindContext.awaitingKey) {
            isSetting = true;
            //ImGui::Text("Taste druecken fuer: %s", rebindContext.actionName.c_str());
            settingKeyName = rebindContext.actionName.c_str();
        }
        else {
            isSetting = false;
        }
        ImGui::Text("Kamerarotation:");
        ImGui::Text("rechte Maustaste gedrueckt halten");
        ImGui::Text("Kamera bewegen:");
        showRebindButton("Kamera aufwaerts bewegen", &editableBindings.cameraUp);
        showRebindButton("Kamera abwaerts bewegen", &editableBindings.cameraDown);
        showRebindButton("Kamera nach links bewegen", &editableBindings.cameraLeft);
        showRebindButton("Kamera nach rechts bewegen", &editableBindings.cameraRight);
        showRebindButton("Kamera vorwaerts bewegen", &editableBindings.cameraForward);
        showRebindButton("Kamera rueckwaerts bewegen", &editableBindings.cameraBackward);
        ImGui::Text("Zelle wechseln:");
        showRebindButton("aufwaerts bewegen", &editableBindings.up);
        showRebindButton("abwaerts bewegen", &editableBindings.down);
        showRebindButton("nach links bewegen", &editableBindings.left);
        showRebindButton("nach rechts bewegen", &editableBindings.right);
        showRebindButton("vorwaerts bewegen", &editableBindings.forward);
        showRebindButton("rueckwaerts bewegen", &editableBindings.backward);
        ImGui::Text("Aktion ausfuehren/auswaehlen");
        showRebindButton("Aktion ausfuehren", &editableBindings.action);
        ImGui::Text("schrittweise Suche (links):");
        showRebindButton("naechster Schritt", &editableBindings.stepNextLeft);
        showRebindButton("vorheriger Schritt", &editableBindings.stepPrevLeft);
        ImGui::Text("schrittweise Suche (rechts):");
        showRebindButton("naechster Schritt", &editableBindings.stepNextRight);
        showRebindButton("vorheriger Schritt", &editableBindings.stepPrevRight);
        ImGui::Text("Animation:");
        showRebindButton("Animation abspielen/pausieren", &editableBindings.play_pause);

        ImGui::Text("Einstellungen speichern:");
        if (ImGui::Button("Steuerungeinstellungen speichern", ImVec2(250, 50))) {
            gControls.setKeyBindings(editableBindings);
            gControls.saveBindings();
        }
    }

    if (ImGui::CollapsingHeader("Aktionen")) {
        ImGui::BeginGroup();
        int actionKey = gControls.getKeyBindings().action;
        std::string actionKeyName = gControls.getKeyDisplayName(actionKey);
        std::string actionText = "Aktion auswaehlen mit Taste: " + actionKeyName;
        
        if (ImGui::RadioButton("Startknoten setzen", settings == SET_START_NODE)) {
            settings = SET_START_NODE;
        }
        if (settings == SET_START_NODE) {
            ImGui::Text(actionText.c_str());
        }

        if (ImGui::RadioButton("Zielknoten setzen", settings == SET_END_NODE)) {
            settings = SET_END_NODE;
        }
        if (settings == SET_END_NODE) {
            ImGui::Text(actionText.c_str());
        }

        if (ImGui::RadioButton("Hindernisknoten setzen", settings == SET_OBSTACLE_NODE)) {
            settings = SET_OBSTACLE_NODE;
        }
        if (settings == SET_OBSTACLE_NODE) {
            ImGui::Text(actionText.c_str());
        }

        if (ImGui::RadioButton("Einzelnen Knoten loeschen", settings == SET_ERASE_NODE)) {
            settings = SET_ERASE_NODE;
        }
        if (settings == SET_ERASE_NODE) {
            ImGui::Text(actionText.c_str());
        }

        if (ImGui::RadioButton("Alle Knoten loeschen", settings == SET_ERASE_ALL_NODES)) {
            settings = SET_ERASE_ALL_NODES;
        }
        if (settings == SET_ERASE_ALL_NODES) {
            ImGui::Text(actionText.c_str());
        }

        if (ImGui::RadioButton("Alle Pfadknoten loeschen", settings == SET_ERASE_ALL_PATH_NODES)) {
            settings = SET_ERASE_ALL_PATH_NODES;
        }
        if (settings == SET_ERASE_ALL_PATH_NODES) {
            ImGui::Text(actionText.c_str());
        }

        if (ImGui::RadioButton("Weg suchen", settings == SET_SEARCH_PATH)) {
            settings = SET_SEARCH_PATH;
        }
        if (settings == SET_SEARCH_PATH) {
            ImGui::Text(actionText.c_str());
        }

        if (ImGui::RadioButton("Weg schrittweise suchen", settings == SET_SEARCH_PATH_STEP)) {
            settings = SET_SEARCH_PATH_STEP;
        }
        if (settings == SET_SEARCH_PATH_STEP) {
            int nextLeftKey = gControls.getKeyBindings().stepNextLeft;
            std::string nextLeftKeyName = gControls.getKeyDisplayName(nextLeftKey);
            int prevLeftKey = gControls.getKeyBindings().stepPrevLeft;
            std::string prevLeftKeyName = gControls.getKeyDisplayName(prevLeftKey);
            std::string strLeft = "Einzelschritte mit Tasten: " + nextLeftKeyName + ", " + prevLeftKeyName.c_str();

            int nextRightKey = gControls.getKeyBindings().stepNextRight;
            std::string nextRightKeyName = gControls.getKeyDisplayName(nextRightKey);
            int prevRightKey = gControls.getKeyBindings().stepPrevRight;
            std::string prevRightKeyName = gControls.getKeyDisplayName(prevLeftKey);
            std::string strRight = "Einzelschritte mit Tasten: " + nextRightKeyName + ", " + prevRightKeyName.c_str();

            ImGui::Text((appState.isComparePaths ? "(links:) %s" : "%s"), strLeft.c_str());
            if (appState.isComparePaths) {
                ImGui::Text("(rechts:) %s", strRight.c_str());
            }
        }

        if (ImGui::RadioButton("Wegsuche animieren", settings == SET_SEARCH_PATH_PLAY)) {
            settings = SET_SEARCH_PATH_PLAY;
        }
        if (settings == SET_SEARCH_PATH_PLAY) {
            ImGui::PushItemWidth(100.0f);
            ImGui::Text("Geschwindigkeit:");
            ImGui::SliderInt("##AnimationSpeed", &appState.animationSpeed, 1, 10);
            ImGui::PopItemWidth();
            
            int animationKey = gControls.getKeyBindings().play_pause;
            std::string animationKeyName = gControls.getKeyDisplayName(animationKey);
            std::string buttonText = appState.isPlaying ? "Pausieren mit Taste: " + animationKeyName : "Abspielen mit Taste: " + animationKeyName;
            ImGui::Text(buttonText.c_str());
        }

        ImGui::EndGroup();
    }

    if (ImGui::CollapsingHeader("Sucheinstellungen")) {
        ImGui::BeginGroup();
        if (ImGui::RadioButton("6 Suchrichtungen (3D)", searchDirections == SEARCH_6_DIRECTIONS)) {
            searchDirections = SEARCH_6_DIRECTIONS;
            appState.draw3D = true;
        }
        if (searchDirections == SEARCH_6_DIRECTIONS) {
            ImGui::Text("Suchrichtungen: horizontal / vertikal");
        }

        if (ImGui::RadioButton("26 Suchrichtungen (3D)", searchDirections == SEARCH_26_DIRECTIONS)) {
            searchDirections = SEARCH_26_DIRECTIONS;
            appState.draw3D = true;
        }
        if (searchDirections == SEARCH_26_DIRECTIONS) {
            ImGui::Text("Suchrichtungen: horizontal / vertikal / diagonal");
        }

        if (ImGui::RadioButton("4 Suchrichtungen (2D)", searchDirections == SEARCH_4_DIRECTIONS)) {
            searchDirections = SEARCH_4_DIRECTIONS;
            appState.draw3D = false;
        }
        if (searchDirections == SEARCH_4_DIRECTIONS) {
            ImGui::Text("Suchrichtungen: horizontal / vertikal");
        }

        if (ImGui::RadioButton("8 Suchrichtungen (2D)", searchDirections == SEARCH_8_DIRECTIONS)) {
            searchDirections = SEARCH_8_DIRECTIONS;
            appState.draw3D = false;
        }
        if (searchDirections == SEARCH_8_DIRECTIONS) {
            ImGui::Text("Suchrichtungen: horizontal / vertikal / diagonal");
        }

        ImGui::EndGroup();
    }

    if (ImGui::CollapsingHeader("Suchalgorithmus")) {
        ImGui::BeginGroup();
        ImGui::Checkbox("Algorithmus vergleichen", &appState.isComparePaths);
        ImGui::Text("Algorhitmus auswaehlen:");
        if (ImGui::BeginCombo(appState.isComparePaths ? "Links##AlgorithmusLinks" : "##", algoLogicLeft.algorithms.at(algorithmusTypeLeft)->title.c_str())) {
            for (int i = 0; i < algoLogicLeft.algorithms.size(); ++i) {
                const bool isSelected = (algorithmusTypeLeft == i);

                if (ImGui::Selectable(algoLogicLeft.algorithms.at(i)->title.c_str(), isSelected)) {
                    algorithmusTypeLeft = i;
                }

                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            
            ImGui::EndCombo();
        }
        
        if (appState.isComparePaths) {
            if (ImGui::BeginCombo("Rechts##AlgorithmusRechts", algoLogicRight.algorithms.at(algorithmusTypeRight)->title.c_str())) {
                for (int i = 0; i < algoLogicRight.algorithms.size(); ++i) {
                    const bool isSelected = (algorithmusTypeRight == i);

                    if (ImGui::Selectable(algoLogicRight.algorithms.at(i)->title.c_str(), isSelected)) {
                        algorithmusTypeRight = i;
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }
        }

        ImGui::EndGroup();
    }

    if (ImGui::CollapsingHeader("Grid-Groesse")) {
        ImGui::BeginGroup();
        ImGui::Text("Anzahl der Zellen festlegen:");
        ImGui::PushItemWidth(100.0f);
        ImGui::SliderInt("X-Achse (Einheiten)", &xSize, 1, 10);
        ImGui::SliderInt("Y-Achse (Einheiten)", &ySize, 1, 10);
        ImGui::SliderInt("Z-Achse (Einheiten)", &zSize, 1, 10);
        ImGui::PopItemWidth();
        if (ImGui::Button("Grid anpassen", ImVec2(250, 30))) {
            appState.xSize = xSize;
            appState.ySize = ySize;
            appState.zSize = zSize;
            if (appState.selX >= xSize) {
                appState.selX = xSize - 1;
            }
            if (appState.selY >= ySize) {
                appState.selY = ySize - 1;
            }
            if (appState.selZ >= zSize) {
                appState.selZ = zSize - 1;
            }
            appState.initArray = true;
        }
        ImGui::EndGroup();
    }

    if (ImGui::CollapsingHeader("Bewertungsinformationen"))
    {
        if (appState.isComparePaths == true) {
            ImGui::Text("linker Algorithmus:");
            ImGui::Text("---------------------------");
        }
        ImGui::Text("Laufzeit in");
        ImGui::Text("Millisekunden: %s", std::to_string(algoLogicLeft.runtime).c_str());
        ImGui::Text("---------------------------");
        ImGui::Text("Schrittnummer des");
        ImGui::Text("ausgewaehlten Knotens: %s", std::to_string(step_left).c_str());
        ImGui::Text("---------------------------");
        ImGui::Text("bearbeitete Knoten: %s", std::to_string(algoLogicLeft.nodeSteps.size()).c_str());
        ImGui::Text("---------------------------");
        ImGui::Text("Pfadlaenge: %s", std::to_string(appState.pathLeft.size()).c_str());
        ImGui::Text("---------------------------");
        ImGui::Text("Wegkosten:");
        ImGui::Text("");

        if (algoLogicLeft.algorithms[algorithmusTypeLeft]->isFCost) {
            ImGui::Text("F-Kosten: %s", std::to_string(f_costs_left).c_str());
        }
        if (algoLogicLeft.algorithms[algorithmusTypeLeft]->isGCost) {
            ImGui::Text("G-Kosten: %s", std::to_string(g_costs_left).c_str());
        }
        if (algoLogicLeft.algorithms[algorithmusTypeLeft]->isHCost) {
            ImGui::Text("H-Kosten: %s", std::to_string(h_costs_left).c_str());
        }
        if (algoLogicLeft.algorithms[algorithmusTypeLeft]->isFLimit) {
            ImGui::Text("F-Limit: %s", std::to_string(fLimit_left).c_str());
        }
        if (algoLogicLeft.algorithms[algorithmusTypeLeft]->isBound) {
            ImGui::Text("Bound: %s", std::to_string(bound_left).c_str());
        }
        if (appState.isComparePaths == true) {
            ImGui::Text("---------------------------");
            ImGui::Text("rechter Algorithmus:");
            ImGui::Text("---------------------------");
            ImGui::Text("Laufzeit in");
            ImGui::Text("Millisekunden: %s", std::to_string(algoLogicRight.runtime).c_str());
            ImGui::Text("---------------------------");
            ImGui::Text("Schrittnummer des");
            ImGui::Text("ausgewaehlten Knotens: %s", std::to_string(step_right).c_str());
            ImGui::Text("---------------------------");
            ImGui::Text("bearbeitete Knoten: %s", std::to_string(algoLogicRight.nodeSteps.size()).c_str());
            ImGui::Text("---------------------------");
            ImGui::Text("Pfadlaenge: %s", std::to_string(appState.pathRight.size()).c_str());
            ImGui::Text("---------------------------");
            ImGui::Text("Wegkosten:");
            ImGui::Text("");

            if (algoLogicRight.algorithms[algorithmusTypeRight]->isFCost) {
                ImGui::Text("F-Kosten: %s", std::to_string(f_costs_right).c_str());
            }
            if (algoLogicRight.algorithms[algorithmusTypeRight]->isGCost) {
                ImGui::Text("G-Kosten: %s", std::to_string(g_costs_right).c_str());
            }
            if (algoLogicRight.algorithms[algorithmusTypeRight]->isHCost) {
                ImGui::Text("H-Kosten: %s", std::to_string(h_costs_right).c_str());
            }
            if (algoLogicRight.algorithms[algorithmusTypeRight]->isFLimit) {
                ImGui::Text("F-Limit: %s", std::to_string(fLimit_right).c_str());
            }
            if (algoLogicLeft.algorithms[algorithmusTypeLeft]->isBound) {
                ImGui::Text("Bound: %s", std::to_string(bound_right).c_str());
            }
        }
    }

    if (ImGui::CollapsingHeader("Legende & Info"))
    {
        const ImVec2 rectangleSize(24.0f, 24.0f);

        ImGui::Text(
            "Um eine Suche starten zu koennen,\n"
            "muss mindestens der Start- sowie\n"
            "der Zielknoten gesetzt worden sein."
        );

        ImGui::Text("\nLegende:");
        auto drawLegendEntry = [&](const char* id,
            const ImVec4& color,
            const char* text)
            {
                ImGui::SmallButton("?");
                if (ImGui::IsItemHovered() && id == "##StartNodeColor") {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(
                        "Die Pfadsuche startet von diesem Knoten aus."
                    );
                    ImGui::EndTooltip();
                }
                if (ImGui::IsItemHovered() && id == "##EndNodeColor") {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(
                        "Das Ziel der Pfadsuche ist es einen Pfad\n"
                        "zu diesem Knoten zu finden."
                    );
                    ImGui::EndTooltip();
                }
                if (ImGui::IsItemHovered() && id == "##ObstacleColor") {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(
                        "Dieser Knoten stellt ein Hindernis dar.\n"
                        "Fuer die Pfadsuche gilt: Diese Zelle ist\n"
                        "unueberwindbar und wird nicht untersucht."
                    );
                    ImGui::EndTooltip();
                }
                if (ImGui::IsItemHovered() && id == "##OpenNodeColor") {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(
                        "Offener Knoten\n"
                        "Dieser Knoten wurde bereits entdeckt,\n"
                        "aber noch nicht abschliessend untersucht."
                    );
                    ImGui::EndTooltip();
                }
                if (ImGui::IsItemHovered() && id == "##VisitedColor") {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(
                        "Dieser Knoten wurde bereits untersucht."
                    );
                    ImGui::EndTooltip();
                }
                if (ImGui::IsItemHovered() && id == "##PathColor") {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(
                        "Dieser Knoten stellt einen Teil des\n"
                        "gefundenen Pfads dar."
                    );
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();
                ImGui::ColorButton(
                    id,
                    color,
                    ImGuiColorEditFlags_NoTooltip |
                    ImGuiColorEditFlags_NoDragDrop |
                    ImGuiColorEditFlags_NoBorder,
                    rectangleSize
                );

                ImGui::SameLine();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(text);
            };

        drawLegendEntry("##StartNodeColor", ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Startknoten");
        drawLegendEntry("##EndNodeColor", ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Zielknoten");
        drawLegendEntry("##ObstacleColor", ImVec4(0.0f, 0.0f, 0.0f, 1.0f), "Hindernis");
        drawLegendEntry("##OpenNodeColor", ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Offener Knoten");
        drawLegendEntry("##VisitedColor", ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Bearbeiteter Knoten");
        drawLegendEntry("##PathColor", ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Gefundener Pfad");

        auto drawCostEntry = [&](const char* id, const char* text) {
            ImGui::SmallButton("?");
            if (ImGui::IsItemHovered() && id == "##Kosten") {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(
                    "Die Kosten geben an, wie aufwendig ein Weg ist.\n"
                    "Jede Bewegung von einem Feld zum naechsten erhoeht die Kosten.\n"
                    "Je niedriger die Gesamtkosten sind, desto guenstiger ist der gefundene Weg."
                );
                ImGui::EndTooltip();
            }
            if (ImGui::IsItemHovered() && id == "##G-Kosten") {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(
                    "Bisheriger Aufwand: Gibt an, wie aufwendig der Weg vom Start\n"
                    "bis zu diesem Feld bereits war."
                );
                ImGui::EndTooltip();
            }
            if (ImGui::IsItemHovered() && id == "##H-Kosten") {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(
                    "Geschaetzter Restaufwand: Schaetzt, wie aufwendig der verbleibende\n"
                    "Weg von diesem Feld bis zum Ziel sein koennte."
                );
                ImGui::EndTooltip();
            }
            if (ImGui::IsItemHovered() && id == "##F-Kosten") {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(
                    "Geschaetzter Gesamtaufwand: Verbindet den bisherigen Aufwand mit\n"
                    "dem geschaetzten Restaufwand (f = g + h)."
                );
                ImGui::EndTooltip();
            }
            if (ImGui::IsItemHovered() && id == "##F-Limit") {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(
                    "Aktuelle Suchgrenze: Felder mit einem hoeheren geschaetzten\n"
                    "Gesamtaufwand werden in diesem Suchdurchlauf noch nicht untersucht."
                );
                ImGui::EndTooltip();
            }
            if (ImGui::IsItemHovered() && id == "##Bound") {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(
                    "Aktuelle Suchgrenze: Felder, die ueber dieser Grenze liegen,\n"
                    "werden zunaechst zurueckgestellt und moeglicherweise spaeter untersucht."
                );
                ImGui::EndTooltip();
            }

            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(text);
            };
        ImGui::Text("");
        drawCostEntry("##Kosten", "Kosten:");
        drawCostEntry("##G-Kosten", "G-Kosten");
        drawCostEntry("##H-Kosten", "H-Kosten");
        drawCostEntry("##F-Kosten", "F-Kosten");
        drawCostEntry("##F-Limit", "F-Limit");
        drawCostEntry("##Bound", "Bound");
    }

    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(200, 100));
}

// stellt alles zum Rendern des Benutzermenüs ein
void UserInterface::render(RenderData& renderData, int commandBufferIndex) {
    // Falls die Steuerung verändert wird, erscheint dieses Fenster
    if (isSetting) {
        ImGui::SetNextWindowPos(thinkingWindowPos);
        ImGui::Begin("Steuerung:");
        ImGui::Text("Taste druecken fuer Funktion:\n");
        ImGui::Text(settingKeyName);
        ImGui::End();
    }

    // Falls der Pfad des Algorithmus noch berechnet wird, erscheint dieses Fenster
    if (isThinking) {
        ImGui::SetNextWindowPos(thinkingWindowPos);
        ImGui::Begin("Berechnet Pfad...");
        ImGui::Text("Bitte warten...");
        ImGui::End();
    }
    
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), renderData.commandBuffers.commandBuffers[commandBufferIndex]);
}

// Aufräumfunktion. Gibt Speicher von ImGui-Komponenten frei
void UserInterface::cleanup(RenderData& renderData) {
    vkDestroyDescriptorPool(renderData.vkInst.device, renderData.imguiDescriptorPool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
