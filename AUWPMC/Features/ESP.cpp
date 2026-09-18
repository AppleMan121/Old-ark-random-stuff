#include "ESP.h"
#include "../Cheat.h"

static char g_TextBuffer[512];
static char g_HpBuffer[32];
static char g_DistBuffer[32];

void ESP::Render() {
    auto* drawList = ImGui::GetBackgroundDrawList();
    
    for (const auto& player : g_ActorCache.Players) {
        if (player.IsDead && !Settings.Visuals.DrawDeadPlayers) continue;
        if (player.IsSleeping && !Settings.Visuals.DrawSleepingPlayers) continue;
        if (!player.IsEnemy && Settings.Visuals.HideTeamPlayers) continue;
        RenderPlayer(drawList, player);
    }
    
    for (const auto& dino : g_ActorCache.Dinos) {
        if (dino.IsDead && !Settings.Visuals.DinoDead) continue;
        if (Settings.Visuals.HideTeamDinos && !dino.IsEnemy) continue;
        RenderDino(drawList, dino);
    }
    
    if (Settings.Visuals.Radar2D) RenderRadar(drawList);
    if (Settings.Visuals.DrawLine) RenderSnaplines(drawList);
}

void ESP::RenderPlayer(ImDrawList* drawList, const CachedActor& player) {
    ImColor color;
    if (player.IsDead) color = ImColor(255, 153, 8, 255);
    else if (player.IsSleeping) color = ImColor(255, 255, 255, 255);
    else if (!player.IsEnemy) color = ImColor(60, 255, 0, 255);
    else color = ImColor(255, 0, 0, 255);
    
    if (Settings.Visuals.RenderPlayerName) {
        const char* name = player.GetNameString().c_str();
        sprintf(g_TextBuffer, "%s lvl %d", name, player.Level);
        ImVec2 pos(player.ScreenPos.X, player.ScreenPos.Y - 60.0f);
        RenderTextOutlined(drawList, pos, color, g_TextBuffer);
    }
    
    if (Settings.Visuals.DrawPlayerDistance) {
        int meters = (int)(player.Distance * 0.01f);
        sprintf(g_DistBuffer, "%d M", meters);
        ImVec2 pos(player.ScreenPos.X, player.ScreenPos.Y - 75.0f);
        RenderTextOutlined(drawList, pos, color, g_DistBuffer);
    }
    
    if (Settings.Visuals.DrawPlayerHP && !player.IsDead) {
        RenderHealthBar(drawList, player.ScreenPos, player.HealthPercent, player.TorporPercent, 
                       Settings.Visuals.PlayerTorp && !player.IsSleeping);
    }
    
    if (Settings.Visuals.DrawPlayerBones && player.Mesh) {
        RenderSkeleton(drawList, player);
    }
}

void ESP::RenderDino(ImDrawList* drawList, const CachedActor& dino) {
    ImColor color = dino.IsTamed ? 
        ImColor((int)(TamedDinoColor1[0] * 255), (int)(TamedDinoColor1[1] * 255), (int)(TamedDinoColor1[2] * 255)) :
        ImColor((int)(WildDinoColor1[0] * 255), (int)(WildDinoColor1[1] * 255), (int)(WildDinoColor1[2] * 255));
    
    if (dino.IsDead) color = ImColor(235, 119, 0, 255);
    else if (dino.IsSleeping) color = ImColor(255, 237, 237, 255);
    else if (!dino.IsEnemy) color = ImColor(2, 125, 0, 255);
    
    sprintf(g_TextBuffer, "%s lvl %d", dino.DinoName.c_str(), dino.Level);
    ImVec2 pos(dino.ScreenPos.X + 100, dino.ScreenPos.Y - 60.0f);
    RenderTextOutlined(drawList, pos, color, g_TextBuffer);
    
    if (Settings.Visuals.DrawDinoDistance && !dino.IsSleeping) {
        int meters = (int)(dino.Distance * 0.01f);
        sprintf(g_DistBuffer, "%d M", meters);
        ImVec2 dpos(dino.ScreenPos.X + 100, dino.ScreenPos.Y - 75.0f);
        RenderTextOutlined(drawList, dpos, color, g_DistBuffer);
    }
    
    if (!dino.IsDead) {
        RenderHealthBar(drawList, ImVec2(dino.ScreenPos.X + 100, dino.ScreenPos.Y), 
                       dino.HealthPercent, dino.TorporPercent, 
                       Settings.Visuals.TamedDinoTorp && !dino.IsSleeping);
    }
}

void ESP::RenderHealthBar(ImDrawList* drawList, const ImVec2& pos, float hpPct, float torpPct, bool showTorp) {
    const float barWidth = 40.0f;
    const float barHeight = 6.0f;
    const float yOffset = 40.0f;
    
    ImVec2 barPos(pos.x - barWidth/2, pos.y + yOffset);
    
    drawList->AddRectFilled(barPos, ImVec2(barPos.x + barWidth, barPos.y + barHeight), 
                           IM_COL32(255, 0, 0, 255));
    
    float hpWidth = barWidth * hpPct;
    drawList->AddRectFilled(barPos, ImVec2(barPos.x + hpWidth, barPos.y + barHeight), 
                           IM_COL32(0, 255, 0, 255));
    
    int hp = (int)(hpPct * 100);
    sprintf(g_HpBuffer, "%d", hp);
    ImVec2 textPos(barPos.x + barWidth/2 - 8, barPos.y - 12);
    drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), g_HpBuffer);
    
    if (showTorp && torpPct > 0.01f) {
        ImVec2 torpPos(barPos.x, barPos.y + barHeight + 2);
        drawList->AddRectFilled(torpPos, ImVec2(torpPos.x + barWidth, torpPos.y + barHeight), 
                               IM_COL32(128, 128, 128, 255));
        float torpWidth = barWidth * torpPct;
        drawList->AddRectFilled(torpPos, ImVec2(torpPos.x + torpWidth, torpPos.y + barHeight), 
                               IM_COL32(128, 0, 128, 255));
    }
}

void ESP::RenderTextOutlined(ImDrawList* drawList, const ImVec2& pos, ImColor color, const char* text) {
    drawList->AddText(ImVec2(pos.x + 1, pos.y + 1), IM_COL32(0, 0, 0, 255), text);
    drawList->AddText(ImVec2(pos.x - 1, pos.y - 1), IM_COL32(0, 0, 0, 255), text);
    drawList->AddText(pos, color, text);
}

void ESP::RenderSkeleton(ImDrawList* drawList, const CachedActor& player) {
    if (!player.Mesh) return;
    
    FVector2D head, neck, spine, pelvis;
    FVector2D leftShoulder, leftElbow, leftWrist;
    FVector2D rightShoulder, rightElbow, rightWrist;
    FVector2D leftKnee, leftAnkle, rightKnee, rightAnkle;
    
    FVector loc;
    bool ok = true;
    auto* mesh = player.Mesh;
    
    ok &= Settings.GetBoneLocation(mesh, &loc, BoneIndices::Head, 0) && W2S(loc, head);
    ok &= Settings.GetBoneLocation(mesh, &loc, BoneIndices::Neck, 0) && W2S(loc, neck);
    ok &= Settings.GetBoneLocation(mesh, &loc, BoneIndices::Spine, 0) && W2S(loc, spine);
    ok &= Settings.GetBoneLocation(mesh, &loc, BoneIndices::Pelvis, 0) && W2S(loc, pelvis);
    ok &= Settings.GetBoneLocation(mesh, &loc, BoneIndices::LeftShoulder, 0) && W2S(loc, leftShoulder);
    ok &= Settings.GetBoneLocation(mesh, &loc, BoneIndices::LeftElbow, 0) && W2S(loc, leftElbow);
    ok &= Settings.GetBoneLocation(mesh, &loc, BoneIndices::LeftWrist, 0) && W2S(loc, leftWrist);
    
    int rShoulderIdx = (player.Gender == 2) ? BoneIndices::RightShoulder_Female : BoneIndices::RightShoulder_Male;
    int rElbowIdx = (player.Gender == 2) ? BoneIndices::RightElbow_Female : BoneIndices::RightElbow_Male;
    int rWristIdx = (player.Gender == 2) ? BoneIndices::RightWrist_Female : BoneIndices::RightWrist_Male;
    int lKneeIdx = (player.Gender == 2) ? BoneIndices::LeftKnee_Female : BoneIndices::LeftKnee_Male;
    int lAnkleIdx = (player.Gender == 2) ? BoneIndices::LeftAnkle_Female : BoneIndices::LeftAnkle_Male;
    int rKneeIdx = (player.Gender == 2) ? BoneIndices::RightKnee_Female : BoneIndices::RightKnee_Male;
    int rAnkleIdx = (player.Gender == 2) ? BoneIndices::RightAnkle_Female : BoneIndices::RightAnkle_Male;
    
    ok &= Settings.GetBoneLocation(mesh, &loc, rShoulderIdx, 0) && W2S(loc, rightShoulder);
    ok &= Settings.GetBoneLocation(mesh, &loc, rElbowIdx, 0) && W2S(loc, rightElbow);
    ok &= Settings.GetBoneLocation(mesh, &loc, rWristIdx, 0) && W2S(loc, rightWrist);
    ok &= Settings.GetBoneLocation(mesh, &loc, lKneeIdx, 0) && W2S(loc, leftKnee);
    ok &= Settings.GetBoneLocation(mesh, &loc, lAnkleIdx, 0) && W2S(loc, leftAnkle);
    ok &= Settings.GetBoneLocation(mesh, &loc, rKneeIdx, 0) && W2S(loc, rightKnee);
    ok &= Settings.GetBoneLocation(mesh, &loc, rAnkleIdx, 0) && W2S(loc, rightAnkle);
    
    if (!ok) return;
    
    ImColor color = player.IsEnemy ? ImColor(255, 0, 0) : ImColor(60, 255, 0);
    
    drawList->AddLine(ImVec2(leftAnkle.X, leftAnkle.Y), ImVec2(leftKnee.X, leftKnee.Y), color);
    drawList->AddLine(ImVec2(rightAnkle.X, rightAnkle.Y), ImVec2(rightKnee.X, rightKnee.Y), color);
    drawList->AddLine(ImVec2(leftKnee.X, leftKnee.Y), ImVec2(pelvis.X, pelvis.Y), color);
    drawList->AddLine(ImVec2(rightKnee.X, rightKnee.Y), ImVec2(pelvis.X, pelvis.Y), color);
    drawList->AddLine(ImVec2(pelvis.X, pelvis.Y), ImVec2(spine.X, spine.Y), color);
    drawList->AddLine(ImVec2(spine.X, spine.Y), ImVec2(neck.X, neck.Y), color);
    drawList->AddLine(ImVec2(neck.X, neck.Y), ImVec2(head.X, head.Y), color);
    drawList->AddLine(ImVec2(leftWrist.X, leftWrist.Y), ImVec2(leftElbow.X, leftElbow.Y), color);
    drawList->AddLine(ImVec2(leftElbow.X, leftElbow.Y), ImVec2(leftShoulder.X, leftShoulder.Y), color);
    drawList->AddLine(ImVec2(leftShoulder.X, leftShoulder.Y), ImVec2(neck.X, neck.Y), color);
    drawList->AddLine(ImVec2(rightWrist.X, rightWrist.Y), ImVec2(rightElbow.X, rightElbow.Y), color);
    drawList->AddLine(ImVec2(rightElbow.X, rightElbow.Y), ImVec2(rightShoulder.X, rightShoulder.Y), color);
    drawList->AddLine(ImVec2(rightShoulder.X, rightShoulder.Y), ImVec2(neck.X, neck.Y), color);
}

void ESP::RenderRadar(ImDrawList* drawList) {
    const float radarSize = 200.0f;
    const float margin = 100.0f;
    ImVec2 radarPos(ImGui::GetIO().DisplaySize.x - radarSize - margin, margin);
    ImVec2 center(radarPos.x + radarSize/2, radarPos.y + radarSize/2);
    
    drawList->AddRectFilled(radarPos, ImVec2(radarPos.x + radarSize, radarPos.y + radarSize), 
                           IM_COL32(95, 95, 95, 95));
    drawList->AddRect(radarPos, ImVec2(radarPos.x + radarSize, radarPos.y + radarSize), 
                     IM_COL32(0, 0, 0, 255));
    drawList->AddLine(ImVec2(center.x, radarPos.y), ImVec2(center.x, radarPos.y + radarSize), 
                     IM_COL32(0, 0, 0, 255));
    drawList->AddLine(ImVec2(radarPos.x, center.y), ImVec2(radarPos.x + radarSize, center.y), 
                     IM_COL32(0, 0, 0, 255));
    
    float camYaw = Cache.LPC->PlayerCameraManager->GetCameraRotation().Yaw;
    float yawRad = camYaw * M_PI / 180.0f;
    
    for (const auto& player : g_ActorCache.Players) {
        if (player.IsDead && !Settings.Visuals.Radar2DDead) continue;
        if (!player.IsEnemy && !Settings.Visuals.Radar2DAlly) continue;
        
        FVector relative = player.WorldLocation - g_ActorCache.LocalLocation;
        float dist = relative.Size() / 33000.0f;
        if (dist > 1.0f) dist = 1.0f;
        
        float angle = atan2f(relative.Y, relative.X) - yawRad;
        float dotX = center.x + dist * (radarSize/2) * sinf(-angle);
        float dotY = center.y + dist * (radarSize/2) * cosf(-angle);
        
        ImU32 color = player.IsEnemy ? IM_COL32(255, 0, 0, 255) : IM_COL32(60, 255, 0, 255);
        if (player.IsDead) color = IM_COL32(255, 165, 0, 255);
        if (!player.IsConscious() && !player.IsDead) color = IM_COL32(255, 255, 255, 255);
        
        drawList->AddCircleFilled(ImVec2(dotX, dotY), 3.0f, color);
        
        if (Settings.Visuals.DrawLineRadar2D && player.IsEnemy && !player.IsDead) {
            ImU32 lineColor = player.IsVisible ? IM_COL32(0, 255, 0, 255) : IM_COL32(128, 128, 128, 255);
            drawList->AddLine(center, ImVec2(dotX, dotY), lineColor);
        }
    }
}

void ESP::RenderSnaplines(ImDrawList* drawList) {
    FVector2D localScreen;
    if (!W2S(Cache.LocalActor->K2_GetActorLocation(), localScreen)) return;
    
    for (const auto& player : g_ActorCache.Players) {
        if (!player.IsEnemy) continue;
        if (player.IsDead && !Settings.Visuals.DEnemy) continue;
        if (!player.IsConscious() && !player.IsDead && !Settings.Visuals.SEnemy) continue;
        
        ImU32 color;
        if (player.IsDead) color = IM_COL32(255, 165, 0, 255);
        else if (!player.IsConscious()) color = IM_COL32(255, 255, 255, 255);
        else {
            bool visible = Cache.LPC->LineOfSightTo(player.Actor, 
                Cache.LPC->PlayerCameraManager->GetCameraLocation(), false);
            color = visible ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255);
        }
        
        if (Settings.Visuals.TopScreen) {
            ImVec2 start(localScreen.X, 50.0f);
            drawList->AddLine(start, ImVec2(player.ScreenPos.X, player.ScreenPos.Y), color);
        } else {
            drawList->AddLine(ImVec2(localScreen.X, localScreen.Y), 
                            ImVec2(player.ScreenPos.X, player.ScreenPos.Y), color);
        }
    }
}
