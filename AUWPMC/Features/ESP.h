#pragma once
#include "../Core/Cache.h"

namespace ESP {
    void Render();
    void RenderPlayer(ImDrawList* drawList, const CachedActor& player);
    void RenderDino(ImDrawList* drawList, const CachedActor& dino);
    void RenderHealthBar(ImDrawList* drawList, const ImVec2& pos, float hpPct, float torpPct, bool showTorp);
    void RenderTextOutlined(ImDrawList* drawList, const ImVec2& pos, ImColor color, const char* text);
    void RenderSkeleton(ImDrawList* drawList, const CachedActor& player);
    void RenderRadar(ImDrawList* drawList);
    void RenderSnaplines(ImDrawList* drawList);
    void RenderBox(ImDrawList* drawList, const CachedActor& actor);
    void RenderHeadDot(ImDrawList* drawList, const CachedActor& actor);
}
