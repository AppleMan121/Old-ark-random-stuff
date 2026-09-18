#include "Cache.h"
#include "../Cheat.h"

ActorCache g_ActorCache;

void ActorCache::Clear() {
    Players.clear();
    Dinos.clear();
    Turrets.clear();
    Containers.clear();
}

void ActorCache::Update() {
    Clear();
    
    if (!UWorld::GWorld || !Cache.LocalPlayer || !Cache.LPC) return;
    
    auto* camera = Cache.LPC->PlayerCameraManager;
    if (!camera) return;
    
    LocalLocation = camera->GetCameraLocation();
    
    auto& actors = UWorld::GWorld->PersistentLevel->Actors;
    if (!actors.Data) return;
    
    Players.reserve(50);
    Dinos.reserve(100);
    
    for (int i = 0; i < actors.Count; i++) {
        auto* actor = actors[i];
        if (!actor || !actor->RootComponent) continue;
        
        FVector actorLoc = actor->RootComponent->GetWorldLocation();
        float dist = LocalLocation.DistTo(actorLoc);
        if (dist > MaxESPDistance) continue;
        
        FVector2D screenPos;
        if (!W2S(actorLoc, screenPos)) continue;
        
        if (actor->IsPlayer() && Settings.Visuals.DrawPlayers) {
            ProcessPlayer(actor, dist);
        }
        else if (actor->IsDino() && (Settings.Visuals.DrawWildCreatures || Settings.Visuals.DrawTamedCreatures)) {
            ProcessDino(actor, dist);
        }
    }
}

void ActorCache::ProcessPlayer(AActor* actor, float dist) {
    auto* character = (APrimalCharacter*)actor;
    if (!character->MyCharacterStatusComponent) return;
    
    CachedActor ca = {};
    ca.Actor = actor;
    ca.Mesh = character->MeshComponent;
    ca.WorldLocation = actor->RootComponent->GetWorldLocation();
    ca.Distance = dist;
    ca.IsPlayer = true;
    ca.IsEnemy = !character->IsPrimalCharFriendly((APrimalCharacter*)Cache.LocalActor);
    ca.IsDead = character->IsDead();
    ca.IsSleeping = !character->IsConscious() && !ca.IsDead;
    ca.Gender = character->RetrievePlayerGender(character->Name.GetName());
    
    auto* status = character->MyCharacterStatusComponent;
    ca.HealthPercent = status->ReplicatedCurrentHealth / status->ReplicatedMaxHealth;
    ca.TorporPercent = status->ReplicatedCurrentTorpor / status->ReplicatedMaxTorpor;
    ca.Level = status->BaseCharacterLevel + status->ExtraCharacterLevel;
    
    if (Settings.Aimbot.VisibleOnly && dist < 10000.0f) {
        ca.IsVisible = Cache.LPC->LineOfSightTo(actor, LocalLocation, false);
    } else {
        ca.IsVisible = true;
    }
    
    if (!W2S(ca.WorldLocation, ca.ScreenPos)) return;
    ca.IsOnScreen = true;
    
    Players.push_back(ca);
}

void ActorCache::ProcessDino(AActor* actor, float dist) {
    auto* dino = (APrimalDinoCharacter*)actor;
    bool isTamed = dino->IsTamed();
    
    if (isTamed && !Settings.Visuals.DrawTamedCreatures) return;
    if (!isTamed && !Settings.Visuals.DrawWildCreatures) return;
    
    CachedActor ca = {};
    ca.Actor = actor;
    ca.Mesh = dino->MeshComponent;
    ca.WorldLocation = actor->RootComponent->GetWorldLocation();
    ca.Distance = dist;
    ca.IsDino = true;
    ca.IsTamed = isTamed;
    ca.IsEnemy = !dino->IsPrimalCharFriendly((APrimalCharacter*)Cache.LocalActor);
    ca.IsDead = dino->IsDead();
    ca.IsSleeping = !dino->IsConscious() && !ca.IsDead;
    
    auto* status = dino->MyCharacterStatusComponent;
    if (status) {
        ca.HealthPercent = status->ReplicatedCurrentHealth / status->ReplicatedMaxHealth;
        ca.TorporPercent = status->ReplicatedCurrentTorpor / status->ReplicatedMaxTorpor;
        ca.Level = status->BaseCharacterLevel + status->ExtraCharacterLevel;
    }
    
    if (dino->DinoNameTag.Data) {
        ca.DinoName = dino->DinoNameTag.ToString();
    }
    
    if (!W2S(ca.WorldLocation, ca.ScreenPos)) return;
    ca.IsOnScreen = true;
    
    Dinos.push_back(ca);
}
