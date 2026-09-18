#include "SDK.h"


bool InitSDK(const std::string& moduleName, const uintptr_t gObjectsOffset, const uintptr_t gNamesOffset)
{
	auto mBaseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleA(moduleName.c_str()));
	if (mBaseAddress == 0x00)
		return false;

	UObject::GObjects = reinterpret_cast<decltype(UObject::GObjects)>(mBaseAddress + gObjectsOffset);
	FName::GNames = *reinterpret_cast<decltype(FName::GNames)*>(mBaseAddress + gNamesOffset);
	//UWorld::GWorld = reinterpret_cast<UWorld*>(mBaseAddress + gWorldOffset);
	return true;
}

bool InitSDK()
{
	return InitSDK("ShooterGame.exe", GObjects_Offset, GNames_Offset);
}

void CleanupSDK()
{
	UObject::GObjects = nullptr;
	FName::GNames = nullptr;
}

std::string UObject::GetName() const
{
	std::string name(Name.GetName());
	if (Name.Number > 0)
	{
		name += '_' + std::to_string(Name.Number);
	}
	auto pos = name.rfind('/');
	if (pos == std::string::npos)
	{
		return name;
	}
	return name.substr(pos + 1);
}

std::string UObject::GetFullName() const
{	
	std::string name;
	if (Class != nullptr)
	{
		std::string temp;
		for (auto p = Outer; p; p = p->Outer)
		{
			temp = p->GetName() + "." + temp;
		}

		name = Class->GetName();
		name += " ";
		name += temp;
		name += GetName();
	}
	return name;
}

bool UObject::IsA(UClass* cmp) const
{
	
	for (auto super = Class; super; super = static_cast<UClass*>(super->SuperField))
	{
		if (super == cmp)
		{
			return true;
		}
	}
	return false;
}
/*
UClass* AInfo::StaticClass()
{
	static UClass* ptr = nullptr;
	if (!ptr)
		ptr = UObject::FindClass("Class Engine.Info");
	return ptr;
}*/


/*
void AShooterPlayerState::ServerRequestCreateNewPlayer(const AcLSFNivGXo5DnoPqAdVxHB7qQrUvxjgXxmrGhEFH6v4& PlayerCharacterConfig)
{
	static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterPlayerState.ServerRequestCreateNewPlayer");


	AShooterPlayerState_ServerRequestCreateNewPlayer_Params params{};
	params.PlayerCharacterConfig = PlayerCharacterConfig;

	auto flags = fn->FunctionFlags;
	fn->FunctionFlags |= 0x00000400;
	ProcessEvent(this, fn, &params);
	fn->FunctionFlags = flags;
}
*/

struct APrimalCharacter_GetHealthPercentage_Params
{
public:
	float                                                      ReturnValue;                                             // 0x0000(0x0004)  (Parm, OutParm, ZeroConstructor, ReturnParm, IsPlainOldData, NoDestructor)
};

float AShooterCharacter::GetHealthPercentage()
{
	static UFunction* fn = nullptr;
	if (!fn)
		fn = UObject::FindObject<UFunction>(("Function ShooterGame.PrimalCharacter.GetHealthPercentage"));

	APrimalCharacter_GetHealthPercentage_Params params{};

	auto flags = fn->FunctionFlags;
	fn->FunctionFlags |= 0x00000400;
	ProcessEvent(this, fn, &params);
	fn->FunctionFlags = flags;

	return params.ReturnValue;
}
