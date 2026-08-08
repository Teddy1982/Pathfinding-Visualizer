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

void UserInterface::createFrame(RenderData& renderData) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiWindowFlags imguiWindowFlags = 0;
    //imguiWindowFlags |= ImGuiWindowFlags_NoCollapse;
    //imguiWindowFlags |= ImGuiWindowFlags_NoResize;
    //imguiWindowFlags |= ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowBgAlpha(0.8f);
    ImGui::SetNextWindowSize(ImVec2(300, 400));

    ImGui::Begin("Einstellungen", nullptr, imguiWindowFlags);

    if (ImGui::CollapsingHeader("Aktionen")) {
        ImGui::BeginGroup();
        if (ImGui::RadioButton("Startknoten auswaehlen", settings == SET_START_NODE)) {
            settings = SET_START_NODE;
        }
        if (ImGui::RadioButton("Zielknoten auswaehlen", settings == SET_END_NODE)) {
            settings = SET_END_NODE;
        }
        if (ImGui::RadioButton("Hindernisknoten auswaehlen", settings == SET_OBSTACLE_NODE)) {
            settings = SET_OBSTACLE_NODE;
        }
        if (ImGui::RadioButton("Einzelnen Knoten loeschen", settings == SET_ERASE_NODE)) {
            settings = SET_ERASE_NODE;
        }
        if (ImGui::RadioButton("Alle Knoten loeschen", settings == SET_ERASE_ALL_NODES)) {
            settings = SET_ERASE_ALL_NODES;
        }
        if (ImGui::RadioButton("Alle Pfadknoten loeschen", settings == SET_ERASE_ALL_PATH_NODES)) {
            settings = SET_ERASE_ALL_PATH_NODES;
        }
        if (ImGui::RadioButton("Weg suchen", settings == SET_SEARCH_PATH)) {
            settings = SET_SEARCH_PATH;
        }
        if (ImGui::RadioButton("Weg schrittweise suchen", settings == SET_SEARCH_PATH_STEP)) {
            settings = SET_SEARCH_PATH_STEP;
        }       
        if (ImGui::RadioButton("Wegsuche animieren", settings == SET_SEARCH_PATH_PLAY)) {
            settings = SET_SEARCH_PATH_PLAY;
        }
        if (settings == SET_SEARCH_PATH_PLAY) {
            ImGui::PushItemWidth(100.0f);
            ImGui::SliderInt("##AnimationSpeed", &appState.animationSpeed, 1, 10);
            ImGui::PopItemWidth();
            
            int animationKey = gControls.getKeyBindings().play_pause;
            std::string animationKeyName = gControls.getKeyDisplayName(animationKey);
            std::string buttonText = appState.isPlaying ? "Pausieren: " + animationKeyName : "Abspielen: " + animationKeyName;
            if (ImGui::Button(buttonText.c_str(), ImVec2(250, 30))) {
                if (appState.isPlaying == false) {
                    appState.isPlaying = true;
                }
                else {
                    appState.isPlaying = false;
                }
            }
        }

        ImGui::EndGroup();
    }

    if (ImGui::CollapsingHeader("Sucheinstellungen")) {
        ImGui::BeginGroup();
        if (ImGui::RadioButton("6 Suchrichtungen (3D)", searchDirections == SEARCH_6_DIRECTIONS)) {
            searchDirections = SEARCH_6_DIRECTIONS;
            appState.draw3D = true;
        }
        if (ImGui::RadioButton("26 Suchrichtungen (3D)", searchDirections == SEARCH_26_DIRECTIONS)) {
            searchDirections = SEARCH_26_DIRECTIONS;
            appState.draw3D = true;
        }
        if (ImGui::RadioButton("4 Suchrichtungen (2D)", searchDirections == SEARCH_4_DIRECTIONS)) {
            searchDirections = SEARCH_4_DIRECTIONS;
            appState.draw3D = false;
        }
        if (ImGui::RadioButton("8 Suchrichtungen (2D)", searchDirections == SEARCH_8_DIRECTIONS)) {
            searchDirections = SEARCH_8_DIRECTIONS;
            appState.draw3D = false;
        }
        ImGui::EndGroup();
    }

    if (ImGui::CollapsingHeader("Suchalgorithmus")) {
        ImGui::BeginGroup();
        ImGui::Checkbox("Algorithmus vergleichen", &appState.isComparePaths);
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
            ImGui::Text("Taste druecken fuer: %s", rebindContext.actionName.c_str());
        }

        showRebindButton("Kamera aufwaerts bewegen", &editableBindings.cameraUp);
        showRebindButton("Kamera abwaerts bewegen", &editableBindings.cameraDown);
        showRebindButton("Kamera nach links bewegen", &editableBindings.cameraLeft);
        showRebindButton("Kamera nach rechts bewegen", &editableBindings.cameraRight);
        showRebindButton("Kamera vorwaerts bewegen", &editableBindings.cameraForward);
        showRebindButton("Kamera rueckwaerts bewegen", &editableBindings.cameraBackward);
        showRebindButton("aufwaerts bewegen", &editableBindings.up);
        showRebindButton("abwaerts bewegen", &editableBindings.down);
        showRebindButton("nach links bewegen", &editableBindings.left);
        showRebindButton("nach rechts bewegen", &editableBindings.right);
        showRebindButton("vorwaerts bewegen", &editableBindings.forward);
        showRebindButton("rueckwaerts bewegen", &editableBindings.backward);
        showRebindButton("Aktion ausfuehren", &editableBindings.action);
        showRebindButton("naechsten Schritt links", &editableBindings.stepNextLeft);
        showRebindButton("naechsten Schritt rechts", &editableBindings.stepNextRight);
        showRebindButton("letzten Schritt links", &editableBindings.stepPrevLeft);
        showRebindButton("letzten Schritt rechts", &editableBindings.stepPrevRight);
        showRebindButton("Animation abspielen/pausieren", &editableBindings.play_pause);


        if (ImGui::Button("Steuerungeinstellungen speichern", ImVec2(250, 50))) {
            gControls.setKeyBindings(editableBindings);
            gControls.saveBindings();
        }
    }

    if (ImGui::CollapsingHeader("Legende"))
    {
        const ImVec2 rectangleSize(24.0f, 24.0f);

        auto drawLegendEntry = [&](const char* id,
            const ImVec4& color,
            const char* text)
            {
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
    }

    if (ImGui::CollapsingHeader("Zusatzinformationen"))
    {
        if (appState.isComparePaths == true) {
            ImGui::Text("linker Algorithmus:");
            ImGui::Text("-------------------");
        }
        ImGui::Text("Laufzeit:");
        ImGui::Text("----------------------");
        ImGui::Text("Millisekunden: %s", std::to_string(algoLogicLeft.runtime).c_str());
        ImGui::Text("----------------------");
        ImGui::Text("bearbeitete Knoten: %s", std::to_string(algoLogicLeft.nodeSteps.size()).c_str());
        ImGui::Text("----------------------");
        ImGui::Text("Schrittnr. des Knotens: %s", std::to_string(step_left).c_str());
        ImGui::Text("----------------------");
        ImGui::Text("Wegkosten:");
        ImGui::Text("----------------------");

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
            ImGui::Text("----------------------");
            ImGui::Text("rechter Algorithmus:");
            ImGui::Text("----------------------");
            ImGui::Text("Laufzeit:");
            ImGui::Text("----------------------");
            ImGui::Text("Millisekunden: %s", std::to_string(algoLogicRight.runtime).c_str());
            ImGui::Text("----------------------");
            ImGui::Text("bearbeitete Knoten: %s", std::to_string(algoLogicRight.nodeSteps.size()).c_str());
            ImGui::Text("----------------------");
            ImGui::Text("Schrittnr. des Knotens: %s", std::to_string(step_right).c_str());
            ImGui::Text("----------------------");
            ImGui::Text("Wegkosten:");
            ImGui::Text("----------------------");

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

    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(200, 100));
}


void UserInterface::render(RenderData& renderData, int commandBufferIndex) {
    if (isThinking) {
        ImGui::SetNextWindowPos(thinkingWindowPos);
        ImGui::Begin("Berechnet Pfad...");
        ImGui::Text("Bitte warten...");
        ImGui::End();
    }
    
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), renderData.commandBuffers.commandBuffers[commandBufferIndex]);
}

void UserInterface::cleanup(RenderData& renderData) {
    vkDestroyDescriptorPool(renderData.vkInst.device, renderData.imguiDescriptorPool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
