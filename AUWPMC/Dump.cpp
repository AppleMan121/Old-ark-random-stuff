#include "Dump.h"
#include "Detours/detours.h"
#include "Logger.h"
#include <sstream>

static int PAD_INDEX = 0;

bool OffsetDumper::Init()
{
	if (!Logger::Init2() || !InitSDK()) return false;
}

bool OffsetDumper::Eject()
{
	if (!Logger::Remove2()) return false;
	CleanupSDK();
	while (true)
	{
		FreeLibraryAndExitThread(LoadLibraryA("ArkOffsetDumper.dll"), 0);
		Sleep(20);
	}
}

std::string OffsetDumper::PADString()
{
	PAD_INDEX += 1;
	return std::to_string(PAD_INDEX);
}

int OffsetDumper::FindClassPropertyOffset(const char* Class, const char* Property)
{
	auto ClassPointer = UObject::FindClass(Class);
	for (UProperty* Child = (UProperty*)ClassPointer->Children; Child; Child = (UProperty*)Child->Next)
	{
		if (Child->GetName() == Property)
		{
			auto Offset = (int)Child->Offset;
			return Offset;
		}
	}
	return -1;
}

auto Offset = OffsetDumper::FindFunctionOffset("UObject::ProcessEvent");
int OffsetDumper::FindFunctionOffset(const char* FunctionName)
{
	auto GameBase = (uintptr_t)GetModuleHandleA(nullptr);

	//auto FunctionAddress = DetourFindFunction("ShooterGame.exe", FunctionName);
	auto FunctionAddress = DetourFindFunction("ShooterGame.exe", "UObject::ProcessEvent");
	if (FunctionAddress && GameBase)
	{
		DWORD FunctionOffset = (uintptr_t)FunctionAddress - GameBase;
		if (FunctionOffset >= 1) return FunctionOffset;
	}
	return -1;
}

void OffsetDumper::DumpClassProperty(PropertyInfo Property, PropertyInfo PreviousProperty, bool FirstProperty)
{
	std::stringstream Stream;
	std::transform(Property.OffsetString.begin(), Property.OffsetString.end(), Property.OffsetString.begin(), ::toupper);
	if (FirstProperty)
	{
		Logger::Print(std::string("unsigned char     PAD_" + OffsetDumper::PADString()).c_str());
		Logger::Print(std::string("[0x" + Property.OffsetString + std::string("];\n")).c_str());
	}
	else if (PreviousProperty.PropertyPointer)
	{
		const int PropertyDifference = (Property.PropertyOffset - PreviousProperty.PropertyOffset) - PreviousProperty.PropertyPointer->ElementSize;
		Stream << std::hex << PropertyDifference;
		std::string OffsetString = Stream.str();
		std::transform(OffsetString.begin(), OffsetString.end(), OffsetString.begin(), ::toupper);
		Logger::Print(std::string("unsigned char     PAD_" + OffsetDumper::PADString()).c_str());
		Logger::Print(std::string("[0x" + OffsetString + std::string("];\n")).c_str());
	}
	Logger::Print(Property.Type.c_str());
	Logger::Print("                        ");
	Logger::Print((Property.Name + std::string("; // 0x") + Property.OffsetString + std::string("\n")).c_str());
}

void OffsetDumper::DumpPartialClass(const char* Class, std::vector<std::string> ClassProperties, std::vector<std::string> ClassPropertyTypes)
{
	bool FirstProperty = true;
	UClass* ClassPointer = UObject::FindClass(Class);
	PropertyInfo PreviousProperty = PropertyInfo(nullptr, std::string(), std::string(), std::string(), 0, 0);
	Logger::Print("class "), Logger::Print(ClassPointer->GetName().c_str());
	Logger::Print("\n{\npublic:\n");
	for (size_t s = 0; s < ClassProperties.size(); s++)
	{
		for (UProperty* Child = (UProperty*)ClassPointer->Children; Child; Child = (UProperty*)Child->Next)
		{
			if (Child->GetName() == ClassProperties[s])
			{
				std::stringstream Stream;
				Stream << std::hex << Child->Offset;
				PropertyInfo CurrentInfo = PropertyInfo(Child, ClassProperties[s], ClassPropertyTypes[s], std::string(Stream.str()), Child->ElementSize, Child->Offset);
				OffsetDumper::DumpClassProperty(CurrentInfo, PreviousProperty, FirstProperty);
				PreviousProperty = CurrentInfo, FirstProperty = false;
			}
		}
	}
	Logger::Print("};\n\n");
}

void OffsetDumper::DumpRequestedOffsets()
{
	OffsetDumper::Init();
	OffsetDumper::DumpPartialClass("ScriptStruct ShooterGame.PrimalPlayerData.PrimalPlayerDataStruct", std::vector<std::string>{ "SavedNetworkAddress", "LastPinCodeUsed", "TribeId", "NextAllowedRespawnTime", "LoginTime", "LastLoginTime", }, std::vector<std::string>{ "struct FString ", "int", "int", "double", "double", "double", });
	OffsetDumper::DumpPartialClass("Class ShooterGame.PrimalInventoryComponent", std::vector<std::string>{ "InventoryItems", "EquippedItems", "MaxInventoryItems", "MaxInventoryWeight", "NumSlots", "MaxRemoteInventoryViewingDistance", "AbsoluteMaxInventoryItems", "MaxInventoryAccessDistance", }, std::vector<std::string>{ "TArray<class UPrimalItem*> ", "TArray<class UPrimalItem*> ", "int", "float", "int", "float", "int", "float", });
	OffsetDumper::DumpPartialClass("Class Engine.Actor", std::vector<std::string>{ "TargetingTeam", "CreationTime", "RootComponent", "LastRenderTime", "bForceNonBlockingHits", "DescriptiveName",}, std::vector<std::string>{ "int", "double", "USceneComponent*", "double", "bool", "FString", });
	OffsetDumper::DumpPartialClass("Class ShooterGame.PrimalPlayerData", std::vector<std::string>{ "MyData", }, std::vector<std::string>{ "FPrimalPlayerDataStruct", });
	OffsetDumper::DumpPartialClass("Class ShooterGame.PrimalTargetableActor", std::vector<std::string>{ "DescriptiveName", "ReplicatedHealth", "Health", "MaxHealth", "bForceFloatingDamageNumbers", }, std::vector<std::string>{ "struct FString", "float", "float", "float", "bool", });
	OffsetDumper::DumpPartialClass("Class ShooterGame.PrimalStructure", std::vector<std::string>{ "StructureTag", "LastHealthPercentage", "OwningPlayerName", "OwnerName", }, std::vector<std::string>{ "struct FName*", "float", "struct FString", "struct FString", });
	OffsetDumper::DumpPartialClass("Class ShooterGame.PrimalStructureItemContainer", std::vector<std::string>{ "CurrentItemCount", "MaxItemCount", "PoweredNearbyStructureRange", }, std::vector<std::string>{ "int", "int", "float", });
	OffsetDumper::DumpPartialClass("ScriptStruct ShooterGame.ShooterWeapon.WeaponData", std::vector<std::string>{ "TimeBetweenShots", "ReloadDurationPerAmmoCount", }, std::vector<std::string>{ "float", "float", });
	OffsetDumper::DumpPartialClass("ScriptStruct ShooterGame.ShooterWeapon.InstantWeaponData", std::vector<std::string>{ "WeaponSpread", "TargetingSpreadMod", "FinalWeaponSpreadMultiplier", "FiringSpreadIncrement", "FiringSpreadMax", "WeaponRange", }, std::vector<std::string>{ "float", "float", "float", "float", "float", "float", });
	OffsetDumper::DumpPartialClass("ScriptStruct ShooterGame.PrimalItem.ItemNetInfo", std::vector<std::string>{ "CustomItemName", "WeaponClipAmmo", "ItemDurability", "EggGenderOverride", "EggRandomMutationsFemale", "EggRandomMutationsMale", }, std::vector<std::string>{ "FString", "uint32_t", "float", "int", "int", "int", });
	OffsetDumper::DumpPartialClass("Class ShooterGame.PrimalCharacterStatusComponent", std::vector<std::string>{ "BaseCharacterLevel", "ExtraCharacterLevel", "KnockedOutTorpidityRecoveryRateMultiplier", "MinInventoryWeight", "DinoImprintingQuality", }, std::vector<std::string>{ "int", "uint16_t", "float", "float", "float", });
	OffsetDumper::DumpPartialClass("Class Engine.PlayerState", std::vector<std::string>{ "PlayerName", "PlayerId", }, std::vector<std::string>{ "struct FString", "int", });
	OffsetDumper::DumpPartialClass("Class Engine.Pawn", std::vector<std::string>{ "AIControllerClass", "PlayerState", }, std::vector<std::string>{ "UClass*", "class APlayerState*", });
	OffsetDumper::DumpPartialClass("Class ShooterGame.PrimalItem", std::vector<std::string>{ "MyItemType", "MyEquipmentType", "SlotIndex", "ItemId", "DescriptiveNameBase", "ItemDurability", "ItemQuantity", "MaxItemQuantity", "DroppedItemTemplateOverride", }, std::vector<std::string>{ "EPrimalItemType*", "EPrimalItemType", "int", "struct FItemNetID", "struct FString", "float", "int", "int", "class UClass*" });
	OffsetDumper::DumpPartialClass("Class ShooterGame.ShooterWeapon", std::vector<std::string>{ "AssociatedPrimalItem", "TargetingDelayTime", "AimDriftYawFrequency", "AimDriftPitchFrequency", "GlobalFireCameraShakeScale", "GlobalFireCameraShakeScaleTargeting", "ReloadCameraShakeSpeedScale", "bUseFireCameraShakeScale", "InstantConfig", }, std::vector<std::string>{ "UPrimalItem*", "float", "float", "float", "float", "float", "float", "bool", "struct FInstantWeaponData" });
	OffsetDumper::DumpPartialClass("Class ShooterGame.ShooterCharacter", std::vector<std::string>{ "PlatformProfileName", "CurrentWeapon", "bIsControllingBallista", "ReplicatedWeight", "PlayerName", }, std::vector<std::string>{ "FString", "AShooterWeapon*", "bool", "float", "FString", });
	OffsetDumper::DumpPartialClass("Class ShooterGame.PrimalCharacter", std::vector<std::string>{ "TribeName", "ReplicatedCurrentHealth", "ReplicatedMaxHealth", "ReplicatedCurrentTorpor", "ReplicatedMaxTorpor", "RunningSpeedModifier", "FallDamageMultiplier", "MyCharacterStatusComponent", "OrbitCamMinZoomLevel", "OrbitCamMaxZoomLevel", "ExtraMaxSpeedModifier", "ExtraRotationRateModifier", "AdditionalMaxUseDistance", }, std::vector<std::string>{ "FString", "float", "float", "float", "float", "float", "float", "UPrimalCharacterStatusComponent*", "float", "float", "float", "float", "float", });
	OffsetDumper::DumpPartialClass("Class ShooterGame.PrimalDinoCharacter", std::vector<std::string>{ "MyCharacterStatusComponent", "ChargeSpeedMultiplier", "AllowRidingMaxDistance", "DinoNameTag", "FlyingRunSpeedModifier", "ExtraRunningSpeedModifier", "ScaleExtraRunningSpeedModifierMin", "ScaleExtraRunningSpeedModifierMax", "ScaleExtraRunningSpeedModifierSpeed", "RiderMovementSpeedScalingRotationRatePowerMultiplier", "RiderFlyingRotationRateModifier", "WalkingRotationRateModifier", "RidingSwimmingRunSpeedModifier", "bBPLimitPlayerRotation", }, std::vector<std::string>{ "UPrimalCharacterStatusComponent*", "float", "float", "FName", "float", "float", "float", "float", "float", "float", "float", "float", "float", "unsigned char", });
	OffsetDumper::DumpPartialClass("Class Engine.Controller", std::vector<std::string>{ "ControlRotation", }, std::vector<std::string>{ "FRotator", });
	OffsetDumper::DumpPartialClass("Class Engine.PlayerController", std::vector<std::string>{ "PlayerCameraManager", "MaxUseDistance", "MyHUD", }, std::vector<std::string>{ "struct APlayerCameraManager*", "float", "struct AHUD*", });
	OffsetDumper::DumpPartialClass("Class ShooterGame.ShooterPlayerController", std::vector<std::string>{ "MaxUseDistance", }, std::vector<std::string>{ "float", });
	OffsetDumper::DumpPartialClass("Class Engine.Player", std::vector<std::string>{ "PlayerController", }, std::vector<std::string>{ "APlayerController*", });
	OffsetDumper::DumpPartialClass("Class Engine.LocalPlayer", std::vector<std::string>{ "LastViewLocation", }, std::vector<std::string>{ "FVector*", });

	OffsetDumper::DumpPartialClass("Class Engine.Level", std::vector<std::string>{ "Actors", }, std::vector<std::string>{ "TTransArray<AActor*>", });
	OffsetDumper::DumpPartialClass("Class Engine.Level", std::vector<std::string>{ "LocalPlayers", }, std::vector<std::string>{ "TArray<ULocalPlayer*>", });

	OffsetDumper::DumpPartialClass("Class Engine.World", std::vector<std::string>{ "PersistentLevel", "GameState", "OwningGameInstance", }, std::vector<std::string>{ "ULevel*", "class AGameState*", "UGameInstance*", });
	OffsetDumper::DumpPartialClass("Class ShooterGame.ShooterGameState", std::vector<std::string>{ "NumPlayerConnected", "NumTamedDinos", "LimitTurretsNum", }, std::vector<std::string>{ "int", "int", "int", });
	OffsetDumper::DumpPartialClass("ScriptStruct Engine.MinimalViewInfo", std::vector<std::string>{ "FOV", }, std::vector<std::string>{ "float", });
	OffsetDumper::DumpPartialClass("Class Engine.PlayerCameraManager", std::vector<std::string>{ "DefaultFOV", }, std::vector<std::string>{ "float", });
	OffsetDumper::DumpPartialClass("ScriptStruct Engine.EngineTypes.HitResult", std::vector<std::string>{ "BoneName", }, std::vector<std::string>{ "struct FName", });
	OffsetDumper::DumpPartialClass("Class Engine.MovementComponent", std::vector<std::string>{ "Velocity", }, std::vector<std::string>{ "struct FVector", });
	OffsetDumper::DumpPartialClass("Class Engine.CharacterMovementComponent", std::vector<std::string>{ "MaxStepHeight", "GravityScale", "MaxWalkSpeed", "MaxWalkSpeedCrouched", "MaxWalkSpeedProne", "MaxCustomMovementSpeed", "MaxSwimSpeed", "MaxFlySpeed", "MaxAcceleration", "Buoyancy", "RotationRate", "MaxOutOfWaterStepHeight", "InitialPushForceFactor", "PushForceFactor", "CrouchedSpeedMultiplier", "BackwardsMaxSpeedMultiplier", "Acceleration", "RotationAcceleration", "SwimmingAccelZMultiplier", "CurrentRotationSpeed", }, std::vector<std::string>{ "float", "float", "float", "float", "float", "float", "float", "float", "float", "float", "struct FRotator", "float", "float", "float", "float", "float", "struct FVector", "float", "float", "struct FRotator", });
	OffsetDumper::DumpPartialClass("Class ShooterGame.OnlineSessionEntryButton", std::vector<std::string>{ "NumPlayers", }, std::vector<std::string>{ "uint32_t", });
	OffsetDumper::DumpPartialClass("Class ShooterGame.ShooterWeapon", std::vector<std::string>{ "WeaponConfig", "LastFireTime", "LastNotifyShotTime", }, std::vector<std::string>{ "FWeaponData", "long double", "long double", });

	//Class ShooterGame.ShooterWeapon

}


/*

*/