#pragma once
#include "SDK/Offsets.h"
#include "MemHelper.h"
#include <vector>
#include <string>

// Forward declarations
class UWorld;
class APlayerController;
class AShooterCharacter;
class USkeletalMeshComponent;

struct FVector {
    float X, Y, Z;
    
    FVector operator-(const FVector& other) const {
        return { X - other.X, Y - other.Y, Z - other.Z };
    }
    
    FVector operator+(const FVector& other) const {
        return { X + other.X, Y + other.Y, Z + other.Z };
    }
    
    FVector operator*(float scalar) const {
        return { X * scalar, Y * scalar, Z * scalar };
    }
    
    float Length() const {
        return sqrtf(X * X + Y * Y + Z * Z);
    }
    
    float Dot(const FVector& other) const {
        return X * other.X + Y * other.Y + Z * other.Z;
    }
};

struct FRotator {
    float Pitch, Yaw, Roll;
};

struct FTransform {
    FVector Location;
    FRotator Rotation;
    FVector Scale;
};

// ============================================
// CORE SDK CLASSES
// ============================================

class UObject {
public:
    void** VTable;
    
    void ProcessEvent(class UFunction* func, void* params) {
        auto vtable = *reinterpret_cast<void***>(this);
        auto processEventFn = reinterpret_cast<void(*)(UObject*, UFunction*, void*)>(
            reinterpret_cast<uintptr_t>(vtable[Offsets::ProcessEventIndex])
        );
        processEventFn(this, func, params);
    }
};

class UWorld : public UObject {
public:
    static UWorld* GetWorld() {
        // Pattern scan for GWorld if not cached
        static UWorld** GWorld = nullptr;
        if (!GWorld) {
            // TODO: Implement pattern scan or use your existing GWorld resolution
            // Fallback: read from known offset if base is known
            GWorld = reinterpret_cast<UWorld**>(GetModuleHandle(nullptr) + Offsets::GObjects - 0x1000000); // Adjust
        }
        return GWorld ? *GWorld : nullptr;
    }
    
    class ULevel* GetPersistentLevel() {
        return *reinterpret_cast<ULevel**>(
            reinterpret_cast<uintptr_t>(this) + Offsets::UWorld::PersistentLevel
        );
    }
    
    class AGameStateBase* GetGameState() {
        return *reinterpret_cast<AGameStateBase**>(
            reinterpret_cast<uintptr_t>(this) + Offsets::UWorld::GameState
        );
    }
    
    class UGameInstance* GetGameInstance() {
        return *reinterpret_cast<UGameInstance**>(
            reinterpret_cast<uintptr_t>(this) + Offsets::UWorld::OwningGameInstance
        );
    }
};

class AActor : public UObject {
public:
    class USceneComponent* GetRootComponent() {
        return *reinterpret_cast<USceneComponent**>(
            reinterpret_cast<uintptr_t>(this) + Offsets::AActor::RootComponent  // 0x250 - FIXED
        );
    }
    
    FVector GetLocation() {
        auto root = GetRootComponent();
        if (!root) return { 0, 0, 0 };
        return root->GetLocation();
    }
};

class APrimalCharacter : public AActor {
public:
    float GetCurrentHealth() {
        return *reinterpret_cast<float*>(
            reinterpret_cast<uintptr_t>(this) + Offsets::APrimalCharacter::ReplicatedCurrentHealth  // 0x94C
        );
    }
    
    float GetMaxHealth() {
        return *reinterpret_cast<float*>(
            reinterpret_cast<uintptr_t>(this) + Offsets::APrimalCharacter::ReplicatedMaxHealth  // 0x950
        );
    }
    
    float GetCurrentTorpor() {
        return *reinterpret_cast<float*>(
            reinterpret_cast<uintptr_t>(this) + Offsets::APrimalCharacter::ReplicatedCurrentTorpor  // 0x954
        );
    }
    
    float GetMaxTorpor() {
        return *reinterpret_cast<float*>(
            reinterpret_cast<uintptr_t>(this) + Offsets::APrimalCharacter::ReplicatedMaxTorpor  // 0x958
        );
    }
    
    float GetHealthPercent() {
        float max = GetMaxHealth();
        return max > 0 ? GetCurrentHealth() / max : 0.0f;
    }
    
    bool IsKnockedOut() {
        float maxTorpor = GetMaxTorpor();
        float currentTorpor = GetCurrentTorpor();
        return maxTorpor > 0 && currentTorpor >= maxTorpor * 0.99f;
    }
};

class AShooterWeapon : public AActor {
public:
    int GetCurrentAmmo() {
        return *reinterpret_cast<int*>(
            reinterpret_cast<uintptr_t>(this) + Offsets::AShooterWeapon::CurrentAmmo  // 0xAC0
        );
    }
};

class USkeletalMeshComponent : public UObject {
public:
    void* GetBodySetup() {
        return *reinterpret_cast<void**>(
            reinterpret_cast<uintptr_t>(this) + Offsets::USkeletalMeshComponent::BodySetup  // 0xA50
        );
    }
    
    // Get bone location by index
    FVector GetBoneLocation(int boneIndex) {
        // Implementation depends on your existing bone reading method
        // This typically involves reading the bone array at a specific offset
        // and performing matrix transformations
        return { 0, 0, 0 }; // Placeholder - use your existing implementation
    }
};

// ============================================
// CHEAT FEATURES
// ============================================

class Cheat {
public:
    bool Initialize();
    void Run();
    void Shutdown();
    
    // ESP
    void DrawESP();
    void DrawPlayerESP(APrimalCharacter* player, FVector screenPos);
    void DrawDinoESP(APrimalCharacter* dino, FVector screenPos);
    
    // Aimbot
    void RunAimbot();
    FVector GetAimbotTarget();
    FVector PredictTargetPosition(APrimalCharacter* target, float bulletSpeed);
    
    // Weapon
    void NoSway();
    void RapidFire();
    void InstantReload();
    
    // Misc
    void NoRecoil();
    void FlyHack();
    void SpeedHack(float multiplier);
    
private:
    UWorld* m_World = nullptr;
    APlayerController* m_LocalController = nullptr;
    APrimalCharacter* m_LocalPlayer = nullptr;
    
    bool m_ESP = true;
    bool m_Aimbot = false;
    bool m_NoSway = true;
    bool m_RapidFire = false;
    bool m_NoRecoil = true;
    
    std::vector<APrimalCharacter*> m_Entities;
};

// ============================================
// UTILITY FUNCTIONS
// ============================================

namespace Utils {
    FVector WorldToScreen(UWorld* world, FVector worldPos);
    float GetDistance(FVector a, FVector b);
    bool IsVisible(AActor* target);
    int GetBestBoneIndex(APrimalCharacter* target, bool maleCharacter);
    
    template<typename T>
    T* GetObjectFromId(uint32_t id) {
        // UObject array traversal using GObjects
        auto objects = reinterpret_cast<T**>(GetModuleHandle(nullptr) + Offsets::GObjects);
        // Implementation depends on your object array structure
        return nullptr;
    }
}
