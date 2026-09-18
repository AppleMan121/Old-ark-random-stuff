#pragma once
#include "../Cheat.h"

namespace BoneIndices {
    constexpr int Head = 8;
    constexpr int Neck = 6;
    constexpr int Spine = 4;
    constexpr int Pelvis = 1;
    constexpr int LeftShoulder = 33;
    constexpr int LeftElbow = 36;
    constexpr int LeftWrist = 38;
    constexpr int RightShoulder_Male = 57;
    constexpr int RightShoulder_Female = 59;
    constexpr int RightElbow_Male = 60;
    constexpr int RightElbow_Female = 62;
    constexpr int RightWrist_Male = 62;
    constexpr int RightWrist_Female = 64;
    constexpr int LeftKnee_Male = 82;
    constexpr int LeftKnee_Female = 84;
    constexpr int LeftAnkle_Male = 84;
    constexpr int LeftAnkle_Female = 86;
    constexpr int RightKnee_Male = 88;
    constexpr int RightKnee_Female = 90;
    constexpr int RightAnkle_Male = 90;
    constexpr int RightAnkle_Female = 92;
}

struct CachedActor {
    AActor* Actor;
    USkeletalMeshComponent* Mesh;
    FVector WorldLocation;
    FVector2D ScreenPos;
    float Distance;
    float HealthPercent;
    float TorporPercent;
    int Level;
    bool IsPlayer : 1;
    bool IsDino : 1;
    bool IsEnemy : 1;
    bool IsDead : 1;
    bool IsSleeping : 1;
    bool IsTamed : 1;
    bool IsVisible : 1;
    bool IsOnScreen : 1;
    int Gender;
    std::string DinoName;
    
    std::string GetNameString() const {
        if (IsPlayer && Actor) {
            auto* player = (APrimalCharacter*)Actor;
            if (player->PlayerName.Data) {
                return player->PlayerName.ToString();
            }
        }
        return DinoName;
    }
};

class ActorCache {
public:
    std::vector<CachedActor> Players;
    std::vector<CachedActor> Dinos;
    std::vector<CachedActor> Turrets;
    std::vector<CachedActor> Containers;
    FVector LocalLocation;
    float MaxESPDistance = 50000.0f;
    
    void Update();
    void Clear();
    
private:
    void ProcessPlayer(AActor* actor, float dist);
    void ProcessDino(AActor* actor, float dist);
};

extern ActorCache g_ActorCache;
