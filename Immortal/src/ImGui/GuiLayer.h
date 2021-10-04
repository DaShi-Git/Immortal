#pragma once

#include <imgui.h>

#include "Framework/Layer.h"

#include "Event/KeyEvent.h"
#include "Event/MouseEvent.h"

namespace Immortal
{

class RenderContext;
class IMMORTAL_API GuiLayer : public Layer
{
public:
    static GuiLayer *Create(RenderContext *context);

public:
    GuiLayer();

    ~GuiLayer();

    void OnUpdate() { };

    virtual void OnAttach() override;

    virtual void OnEvent(Event &e) override;

    virtual void OnGuiRender() override;

    inline virtual void GuiLayer::OnDetach() override
    {
        ImGui::DestroyContext();
    }

    inline void GuiLayer::Begin()
    {
        ImGui::NewFrame();
    }

    inline void GuiLayer::End()
    {
        ImGui::Render();
    }

    void BlockEvent(bool block) { blockEvents = block;  }

    void SetTheme();

    ImFont *DemiLight()
    {
        return fonts.demilight;
    }

    ImFont *Bold()
    {
        return fonts.bold;
    }

private:
    bool blockEvents = true;
    
    float time = 0.0f;

    struct
    {
        ImFont *demilight{ nullptr };
        ImFont *bold{ nullptr };
    } fonts;
};

using SuperGuiLayer = GuiLayer;
}
