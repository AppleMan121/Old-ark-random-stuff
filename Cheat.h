// In your Cheat.h or SDK.h - UPDATE THESE STRUCTURES:

struct ULevel {
    char pad_0[0xB8];
    class UWorld* OwningWorld;                    // 0xB8
    char pad_C0[0x18];
    class ALevelScriptActor* LevelScriptActor;   // 0xD8
    char pad_E0[0x230];
    // Actors array is in ULevelBase or use PersistentLevel->Actors
};

struct UWorld {
    char pad_0[0xF8];
    class ULevel* PersistentLevel;                // 0xF8 - CORRECT
    char pad_100[0x28];
    class AGameState* GameState;                  // 0x128 - CORRECT
    char pad_130[0x168];
    class UGameInstance* OwningGameInstance;      // 0x298 - CORRECT
};

struct AActor {
    char pad_0[0x98];
    class AActor* Owner;                          // 0x98
    char pad_A0[0x1B0];
    class USceneComponent* RootComponent;         // 0x250 - WAS WRONG (was 0x158 in old versions)
    char pad_258[0x40];
    // ... rest of actor
};
