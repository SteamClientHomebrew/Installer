#include <imgui.h>
#include <imgui_stdlib.h>
#include <dpi.h>
#include <texture.h>
#include <animate.h>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif
#include <fmt/core.h>
#include <util.h>

using namespace ImGui;

constexpr const char* discordInviteLink   = "https://steambrew.app/discord";
constexpr const char* githubRepositoryUrl = "https://github.com/shdwmtr/millennium";

const void RenderBottomNavBar(const char* identifier, float xPos, std::function<void()> buttonRenderCallback)
{
    ImGuiIO& io = GetIO();
    ImGuiViewport* viewport = GetMainViewport();
    const float BottomNavBarHeight = ScaleY(115);
    const int FooterContainerWidth = ScaleX(300);

    SetCursorPos(ImVec2(xPos, viewport->Size.y - BottomNavBarHeight + 1));

    PushStyleVar  (ImGuiStyleVar_WindowPadding, ImVec2(ScaleX(30), ScaleY(30))     );
    PushStyleColor(ImGuiCol_Border,             ImVec4(0.f, 0.f, 0.f, 0.f)         );
    PushStyleVar  (ImGuiStyleVar_ChildRounding, 0.0f                               );
    PushStyleColor(ImGuiCol_ChildBg,            ImVec4(0.078f, 0.082f, 0.09f, 0.8f));

    BeginChild(fmt::format("##BottomNavBar{}", identifier).c_str(), ImVec2(viewport->Size.x, BottomNavBarHeight), true, ImGuiWindowFlags_NoScrollbar);
    {
        SetCursorPos(ImVec2(ScaleX(45), GetCursorPosY() + ScaleY(12.5)));
        Image((ImTextureID)(intptr_t)infoIconTexture, ImVec2(ScaleX(25), ScaleY(25)));

        SameLine(0, ScaleX(42));
        const float cursorPosSave = GetCursorPosX();

        SetCursorPosY(GetCursorPosY() - ScaleX(12));
        TextColored(ImVec4(0.322f, 0.325f, 0.341f, 1.0f), "Steam Homebrew & Millennium are not affiliated with");

        SetCursorPos(ImVec2(cursorPosSave, GetCursorPosY() - ScaleY(20)));
        TextColored(ImVec4(0.322f, 0.325f, 0.341f, 1.0f), "Steam®, Valve, or any of their partners.");
        
        SameLine(0);
        SetCursorPosY(GetCursorPosY() - ScaleY(25));

        const float buttonPos = GetCursorPosY();

        SetCursorPosX(xPos + GetCursorPosX() + GetContentRegionAvail().x - FooterContainerWidth);
        SetCursorPosY(GetCursorPosY() + ScaleY(10));

        Image((ImTextureID)(intptr_t)discordIconTexture, ImVec2(ScaleX(30), ScaleY(30)));

        
        static bool isDiscordButtonHovered = false;
        float discordIconHoverTransparency = EaseInOutFloat(fmt::format("##DiscordIconHover{}", identifier).c_str(), 0.f, 1.f, isDiscordButtonHovered, 0.3f);

        /** Check if the animation has started */
        if (discordIconHoverTransparency != 0.f)
        {
            SetMouseCursor(ImGuiMouseCursor_Hand);
            PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            PushStyleColor(ImGuiCol_Border, ImVec4(0.18f, 0.184f, 0.192f, 1.0f));
            PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ScaleX(10), ScaleY(10)));
            PushStyleVar(ImGuiStyleVar_WindowRounding, 6);
            PushStyleVar(ImGuiStyleVar_Alpha, discordIconHoverTransparency);
            PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.098f, 0.102f, 0.11f, 1.0f));
            SetTooltip("Join Discord Server");

            if (IsItemClicked())
            {
                OpenUrl(discordInviteLink);
            }

            PopStyleVar(4);
            PopStyleColor(3);
        }

        isDiscordButtonHovered = IsItemHovered() || (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && IsMouseDown(ImGuiMouseButton_Left));


        SameLine(0, ScaleX(25));
        SetCursorPosY(GetCursorPosY() - ScaleY(15));

        Image((ImTextureID)(intptr_t)gtihubIconTexture, ImVec2(ScaleX(30), ScaleY(30)));

        static bool isGithubButtonHovered = false;
        float githubIconHoverTransparency = EaseInOutFloat(fmt::format("##GithubIconHover{}", identifier).c_str(), 0.f, 1.f, isGithubButtonHovered, 0.3f);

        /** Check if the animation has started */
        if (githubIconHoverTransparency != 0.f)
        {
            SetMouseCursor(ImGuiMouseCursor_Hand);
            PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            PushStyleColor(ImGuiCol_Border, ImVec4(0.18f, 0.184f, 0.192f, 1.0f));
            PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ScaleX(10), ScaleY(10)));
            PushStyleVar(ImGuiStyleVar_WindowRounding, 6);
            PushStyleVar(ImGuiStyleVar_Alpha, githubIconHoverTransparency);
            PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.098f, 0.102f, 0.11f, 1.0f));
            SetTooltip("View Source Code");

            if (IsItemClicked())
            {
                OpenUrl(githubRepositoryUrl);
            }

            PopStyleVar(4);
            PopStyleColor(3);
        }

        isGithubButtonHovered = IsItemHovered() || (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && IsMouseDown(ImGuiMouseButton_Left));

        SameLine(0, ScaleX(25));
        SetCursorPosY(buttonPos);

        PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        PushStyleColor(ImGuiCol_Text,          ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        {
            buttonRenderCallback();
        }
        PopStyleColor(2);
    }
    EndChild();

    PopStyleVar(2);
    PopStyleColor(2);
}