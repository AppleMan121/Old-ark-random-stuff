#pragma once
#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)
#define PAD(SIZE) BYTE MACRO_CONCAT(_pad, __COUNTER__)[SIZE];

#include <Windows.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <locale>
#include <array>
#include "UE4/UE4.h"


// Offset to update here
#define GObjects_Offset								0x438C890
#define GNames_Offset								0x43D4790
#define uWorld_Offset								0x43A5758//  0x43D2C18 //+43A5758
#define PROCESS_EVENT_INDEX		                    0x4E
#define ADJUSTED_AIM_INDEX		                    0x00
bool InitSDK(const std::string& moduleName, const uintptr_t gObjectsOffset, const uintptr_t gNamesOffset);
bool InitSDK();
void CleanupSDK();
class UObject;
template<class T>
class TArray
{
	friend struct FString;
public:
	TArray() { Count = Max = 0; };

	T& operator[](int i) const { return Data[i]; };

	T* Data;
	int Count; // int32_t
	int Max; // int32_t
};
struct FNameEntry
{
	uint32_t Index;
	uint32_t Pad;
	FNameEntry* HashNext;

	union
	{
		char AnsiName[1024];
		wchar_t WideName[1024];
	};
	const int GetIndex() const { return Index >> 1; }
	const char* GetAnsiName() const { return AnsiName; }
};
class TNameEntryArray
{
public:
	bool IsValidIndex(uint32_t index) const { return index < NumElements; }

	FNameEntry const* GetByID(uint32_t index) const { return *GetItemPtr(index); }

	FNameEntry const* const* GetItemPtr(uint32_t Index) const
	{
		const auto ChunkIndex = Index / 16384;
		const auto WithinChunkIndex = Index % 16384;
		const auto Chunk = Chunks[ChunkIndex];

		return Chunk + WithinChunkIndex;
	}

	FNameEntry** Chunks[256]; // ChunkTableSize 
	uint32_t NumElements = 0;
	uint32_t NumChunks = 0;
};
struct FName
{
	int ComparisonIndex = 0;
	int Number = 0;

	static inline TNameEntryArray* GNames = nullptr;

	static const char* GetNameByIDFast(int ID)
	{
		auto NameEntry = GNames->GetByID(ID);
		if (!NameEntry) return nullptr;
		return NameEntry->AnsiName;
	}
	static std::string GetNameByID(int ID)
	{
		auto NameEntry = GNames->GetByID(ID);
		if (!NameEntry) return std::string();
		return NameEntry->AnsiName;
	}
	const char* GetNameFast() const
	{
		auto NameEntry = GNames->GetByID(ComparisonIndex);
		if (!NameEntry) return nullptr;
		return NameEntry->AnsiName;
	}
	const std::string GetName() const
	{
		auto NameEntry = GNames->GetByID(ComparisonIndex);
		if (!NameEntry) return std::string();
		return NameEntry->AnsiName;
	}
	inline bool operator==(const FName& other) const
	{
		return ComparisonIndex == other.ComparisonIndex;
	}

	FName() {}
	FName(const char* nameToFind)
	{
		for (int i = 1000u; i < GNames->NumElements; i++)
		{
			auto Name = GetNameByIDFast(i);
			if (!Name) continue;
			if (strcmp(Name, nameToFind) == 0)
			{
				ComparisonIndex = i;
				return;
			}
		}
	}
};
class FUObjectItem
{
public:
	class UObject* Object;
	int32_t SerialNumber;
	unsigned char pad_C1AOV13XBK[0x4];

	enum class ObjectFlags : int32_t
	{
		None = 0,
		Native = 1 << 25,
		Async = 1 << 26,
		AsyncLoading = 1 << 27,
		Unreachable = 1 << 28,
		PendingKill = 1 << 29,
		RootSet = 1 << 30,
		NoStrongReference = 1 << 31
	};
};
class TUObjectArray
{
	enum
	{
		NumElementsPerChunk = 64 * 1024,
	};
public:
	inline int32_t Num() const
	{
		return NumElements;
	}
	inline int32_t Max() const
	{
		return MaxElements;
	}
	inline bool IsValidIndex(int32_t Index) const
	{
		return Index < Num() && Index >= 0;
	}
	inline FUObjectItem* GetObjectPtr(int32_t Index) const
	{
		const int32_t ChunkIndex = Index / NumElementsPerChunk;
		const int32_t WithinChunkIndex = Index % NumElementsPerChunk;
		if (!IsValidIndex(Index)) return nullptr;
		if (ChunkIndex > NumChunks) return nullptr;
		if (Index > MaxElements) return nullptr;
		FUObjectItem* Chunk = Objects[ChunkIndex];
		if (!Chunk) return nullptr;
		return Chunk + WithinChunkIndex;
	}
	inline UObject* GetByIndex(int32_t index) const
	{
		FUObjectItem* ItemPtr = GetObjectPtr(index);
		if (!ItemPtr) return nullptr;

		return (*ItemPtr).Object;
	}
	inline FUObjectItem* GetItemByIndex(int32_t index) const
	{
		FUObjectItem* ItemPtr = GetObjectPtr(index);
		if (!ItemPtr) return nullptr;
		return ItemPtr;
	}
private:
	FUObjectItem** Objects;
	FUObjectItem* PreAllocatedObjects;
	int32_t MaxElements;
	int32_t NumElements;
	int32_t MaxChunks;
	int32_t NumChunks;
};
class UClass;
class UObject
{
public:
	static inline TUObjectArray* GObjects = nullptr;												   // 0x0000
	void* VfTable;                                                   // 0x0000
	int32_t                                             Flags;                                                     // 0x0008
	int32_t                                             InternalIndex;                                             // 0x000C
	UClass* Class;                                                     // 0x0010
	FName                                               Name;                                                      // 0x0018
	UObject* Outer;                                                     // 0x0020

	static inline TUObjectArray& GetGlobalObjects()
	{
		return *GObjects;
	}
	std::string GetName() const;

	std::string GetFullName() const;

	template<typename T>
	static T* FindObject(const std::string& name)
	{
		for (int i = 0; i < GetGlobalObjects().Num(); ++i)
		{
			auto object = GetGlobalObjects().GetByIndex(i);
			if (object == nullptr)
			{
				continue;
			}
			if (object->GetFullName() == name)
			{
				return static_cast<T*>(object);
			}
		}
		return nullptr;
	}


	template<typename T>
	static T* FindObject()
	{
		auto v = T::StaticClass();
		for (int i = 0; i < UObject::GetGlobalObjects().Num(); ++i)
		{
			auto object = UObject::GetGlobalObjects().GetByIndex(i);
			if (object == nullptr)
			{
				continue;
			}
			if (object->IsA(v))
			{
				return static_cast<T*>(object);
			}
		}
		return nullptr;
	}

	template<typename T>
	static std::vector<T*> FindObjects(const std::string& name)
	{
		std::vector<T*> ret;
		for (int i = 0; i < GetGlobalObjects().Num(); ++i)
		{
			auto object = GetGlobalObjects().GetByIndex(i);
			if (object == nullptr)
			{
				continue;
			}
			if (object->GetFullName() == name)
			{
				ret.push_back(static_cast<T*>(object));
			}
		}
		return ret;
	}

	template<typename T>
	static std::vector<T*> FindObjects()
	{
		std::vector<T*> ret;
		auto v = T::StaticClass();
		for (int i = 0; i < UObject::GetGlobalObjects().Num(); ++i)
		{
			auto object = UObject::GetGlobalObjects().GetByIndex(i);
			if (object == nullptr)
			{
				continue;
			}
			if (object->IsA(v))
			{
				ret.push_back(static_cast<T*>(object));
			}
		}
		return ret;
	}
	static UClass* FindClass(const std::string& name)
	{
		return FindObject<UClass>(name);
	}

	template<typename T>
	static T* GetObjectCasted(std::size_t index)
	{
		return static_cast<T*>(GetGlobalObjects().GetByIndex(index));
	}
	bool IsA(UClass* cmp) const;

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class CoreUObject.Object");
		return ptr;
	}
};
class UField : public UObject
{
public:
	class UField* Next;                                        // 0x0028

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class CoreUObject.Field");
		return ptr;
	}
};
class UProperty : public UField
{
public:
	unsigned char								pad_8F5a5TE4IdwQ[0x4];
	int32_t										ElementSize;				// 0x0034(0x0004)
	unsigned char								pad_8F55TE4ITQ[0xC];
	FName										RepNotifyFunc;				// 0x0044(0x0010)
	int32_t										Offset;						// 0x004C(0x0004)
public:
	unsigned char                                      UnknownData_URFE[0x40];

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class CoreUObject.Property");
		return ptr;
	}
};
class UStruct : public UField
{
public:
	class UStruct* SuperField;                                                // 0x0030
	class UField* Children;                                                  // 0x0038
	int32_t                                             PropertySize;                                              // 0x0040
	unsigned char                                       pad_8F55TE4ITQ[0x4];
	TArray<unsigned char>								Script;													   // 0x0048
	int32_t                                             MinAlignment;                                              // 0x0058
	unsigned char                                       pad_PVICHD1SYI[0x4];
	class UProperty* PropertyLink;                                              // 0x0060
	class UProperty* RefLink;                                                   // 0x0068
	class UProperty* DestructorLink;                                            // 0x0070
	class UProperty* PostConstructLink;                                         // 0x0078
	TArray<UObject*>                                    ScriptAndPropertyObjectReferences;                         // 0x0080

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class CoreUObject.Struct");
		return ptr;
	}
};
class UFunction : public UStruct
{
public:
	uint32_t										   FunctionFlags;                                             // 0x0090
	uint16_t                                           RepOffset;                                                 // 0x0094
	unsigned char                                      pad_NRXEECH4MB[0x1];
	unsigned char                                      pad_DW5RJEN1QQ[0x1];
	uint16_t                                           ParmsSize;                                                 // 0x0098
	uint16_t                                           ReturnValueOffset;                                         // 0x009A
	uint16_t                                           RPCId;                                                     // 0x009C
	uint16_t                                           RPCResponseId;                                             // 0x009E
	class UProperty* FirstPropertyToInit;                                                                         // 0x00A0
	void* Func;                                                                                                   // 0x00A8

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class CoreUObject.Function");
		return ptr;
	}
};
template<typename Fn>
Fn GetVFunction(const void* instance, size_t index)
{
	auto vtable = *static_cast<const void***>(const_cast<void*>(instance));
	return reinterpret_cast<Fn>(const_cast<void(*)>(vtable[index]));

	//Logger::Log("[GObjects]: %p\n", vtable[index]);
}
inline void ProcessEvent(void* obj, UFunction* function, void* parms)
{
	UObject* object = static_cast<UObject*>(obj);
	GetVFunction<void(*)(UObject*, UFunction*, void*)>(object, PROCESS_EVENT_INDEX)(object, function, parms);
}
class UClass : public UStruct
{
public:
	unsigned char                                      UnknownData_BH42[0xF8];

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class CoreUObject.Class");
		return ptr;
	}
public:
	template<typename T>
	T* CreateDefaultObjectOfType()
	{
		return static_cast<T*>(CreateDefaultObject());
	}

	UObject* CreateDefaultObject();
	//static UClass* StaticClass();
};
struct FString : public TArray<wchar_t>
{
	FString() = default;
	explicit FString(const wchar_t* other)
	{
		Max = Count = *other ? std::wcslen(other) + 1 : 0;

		if (Count)
		{
			Data = const_cast<wchar_t*>(other);
		}
	};
	inline bool IsValid() const
	{
		return Data != nullptr;
	}
	inline const wchar_t* cw_str() const
	{
		return Data;
	}
	inline const char* c_str() const
	{
		return (const char*)Data;
	}
	std::string ToString() const
	{
		size_t length = std::wcslen(Data);
		std::string str(length, '\0');
		std::use_facet<std::ctype<wchar_t>>(std::locale()).narrow(Data, Data + length, '?', &str[0]);

		return str;
	}
	std::wstring ToWString() const
	{
		std::wstring str(Data);
		return str;
	}
	int Multi(char* name, int size) const
	{
		return WideCharToMultiByte(CP_UTF8, 0, Data, Count, name, size, nullptr, nullptr) - 1;
	}
};;
template<typename T>
struct TTransArray : public TArray<T>
{
	UObject* Owner;
};
struct FKey
{
	struct FName								       KeyName;															// 0x0000
	unsigned char                                      UnknownData_QTRK[0x10] = {};										// 0x0010
	FKey() {};
	FKey(const char* InName) : KeyName(FName(InName)) {}
};
struct FWeaponAnim
{
	class UAnimMontage* Pawn1P;                                                    // 0x0000(0x0008)
	class UAnimMontage* Pawn3P;                                                    // 0x0008(0x0008)
};
struct  USceneComponent /*: public UActorComponent*/
{
	FVector GetWorldLocation()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.SceneComponent.GetWorldLocation");
		struct {
			FVector ReturnValue;
		} Params;

		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	void SetRelativeScale3D(const struct FVector& NewScale3D)
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function Engine.SceneComponent.SetRelativeScale3D");
		struct {
			FVector NewScale3D;
		}Params;
		Params.NewScale3D = NewScale3D;
		ProcessEvent(this, fn, &Params);
	}
	static UClass* StaticClass()
	{
		static UClass* ptr = nullptr;
		if (!ptr)
			ptr = UObject::FindClass("Class Engine.SceneComponent");
		return ptr;
	}
	/*void ToggleVisibility(bool bPropagateToChildren);*/
	//void SetWorldRotation(const struct FRotator& NewRotation, bool bSweep);
	//void SetVisibility(bool bNewVisibility, bool bPropagateToChildren);
};
struct USkeletalMeshComponent /* public USkinnedMeshComponent*/
{
	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class Engine.SkeletalMeshComponent");
		return ptr;
	}
	FName GetBoneName(int BoneIndex)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.SkinnedMeshComponent.GetBoneName");
		struct {
			int BoneIndex;
			FName ReturnValue;
		} Params;
		Params.BoneIndex = BoneIndex;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	int32_t GetNumBones()
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function Engine.SkinnedMeshComponent.GetNumBones");
		struct {
			int32_t ReturnValue;
		}Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
};
struct FPrimalPlayerDataStruct
{
	PAD(0x18);
	struct FString                                     SavedNetworkAddress;                                       // 0x0018(0x0010)
	PAD(0xF8); //ORI 0xF0
	int                                                LastPinCodeUsed;                                           // 0x0118(0x0004) //need update here        0x0120);
	PAD(0x2D8);//ORI 0x2D0
	int                                                TribeId;                                                   // 0x03EC(0x0004) //need update here        0x03F4);
	PAD(0x20);//ORI 0x18
	double                                             NextAllowedRespawnTime;                                    // 0x0408(0x0008) //need update here        0x0410);
	PAD(0x10);//ORI 0x8
	double                                             LoginTime;                                                 // 0x0418(0x0008) //need update here        0x0420);
	double                                             LastLoginTime;                                             // 0x0420(0x0008) //need update here        
};
struct FItemNetID
{
	uint32_t                                           ItemID1;                                                   // 0x0000(0x0004) (ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, NativeAccessSpecifierPublic)
	uint32_t                                           ItemID2;                                                   // 0x0004(0x0004) (ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, NativeAccessSpecifierPublic)

};
struct FVector_NetQuantizeNormal : public FVector
{	};
struct UPrimalInventoryComponent /*: public UActorComponent*/
{
	PAD(0x118); //118
	TArray<class UPrimalItem*>                         InventoryItems;                                            // 0x0118(0x0010) (BlueprintVisible, BlueprintReadOnly, ZeroConstructor, Transient, SaveGame)
	TArray<class UPrimalItem*>                         EquippedItems;                                             // 0x0128(0x0010) 128
	PAD(0x80);
	int                                                MaxInventoryItems;                                         // 0x01B8(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor)
	float                                              MaxInventoryWeight;                                        // 0x01BC(0x0004) (Edit, Net, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, UObjectWrapper)
	PAD(0x4);
	int                                                NumSlots;                                                  // 0x01C4(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor)
	PAD(0x180);
	float                                              MaxRemoteInventoryViewingDistance;                         // 0x0348(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor)
	int                                                AbsoluteMaxInventoryItems;                                 // 0x0350(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor)
	PAD(0x144);
	float                                              MaxInventoryAccessDistance;                                // 0x0498(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor)

	struct UPrimalItem* GetEquippedItemOfType(uint8_t Type)
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalInventoryComponent.GetEquippedItemOfType");
		struct {
			uint8_t Type;
			UPrimalItem* ReturnValue;
		} Params;
		Params.Type = Type;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class ShooterGame.PrimalInventoryComponent");
		return ptr;
	}

	class ADroppedItem* EjectItem(const struct FItemNetID& ItemId, bool bPreventImpule, bool bForceEject, bool bSetItemLocation, const struct FVector& LocationOverride, bool showHUDMessage, class UClass* TheDroppedTemplateOverride, bool bAssignToTribeForPickup, int AssignedTribeID)
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalInventoryComponent.EjectItem");

		//UPrimalInventoryComponent_EjectItem_Params params;
		struct {
			struct FItemNetID              ItemId;                        //(ConstParm, Parm, OutParm, ZeroConstructor, ReferenceParm, IsPlainOldData, NoDestructor)
			bool                           bPreventImpule;                 //(Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
			bool                           bForceEject;                   // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
			bool                           bSetItemLocation;             // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
			struct FVector                 LocationOverride;           //  (ConstParm, Parm, OutParm, ZeroConstructor, ReferenceParm, IsPlainOldData, NoDestructor)
			bool                           showHUDMessage;           // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
			class UClass* TheDroppedTemplateOverride;   // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
			bool                           bAssignToTribeForPickup;      // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
			int                            AssignedTribeID;        // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
			class ADroppedItem* ReturnValue;        // (Parm, OutParm, ZeroConstructor, ReturnParm, IsPlainOldData, NoDestructor)
		} Params;

		Params.ItemId = ItemId;
		Params.bPreventImpule = bPreventImpule;
		Params.bForceEject = bForceEject;
		Params.bSetItemLocation = bSetItemLocation;
		Params.LocationOverride = LocationOverride;
		Params.showHUDMessage = showHUDMessage;
		Params.TheDroppedTemplateOverride = TheDroppedTemplateOverride;
		Params.bAssignToTribeForPickup = bAssignToTribeForPickup;
		Params.AssignedTribeID = AssignedTribeID;

		//auto flags = fn->FunctionFlags;
		//fn->FunctionFlags |= 0x00000400;

		ProcessEvent(this, fn, &Params);
		//fn->FunctionFlags = flags;


		return Params.ReturnValue;
	}

	/*void InitializeInventory()
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalInventoryComponent.InitializeInventory");
		struct {
		}Params;
		ProcessEvent(this, fn, &Params);
	}*/

	void InitializeInventory()
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalInventoryComponent.InitializeInventory");
		struct {

		}Params;
		ProcessEvent(this, fn, &Params);
	}

	//void TransferAllItemsToInventory(class UPrimalInventoryComponent* toInventory);
	//void TransferAllItemsOfClassToInventory(class UPrimalInventoryComponent* toInventory, class UClass* OfItemClass, bool bAllowSubclasses);
	//void TransferAllItemsOfClassesToInventory(class UPrimalInventoryComponent* toInventory, TArray<class UClass*> OfItemClasses, bool bAllowSubclasses);
	//bool IsLocalToPlayer(class AShooterPlayerController* ForPC);
	//bool IsLocal();
	//float GetTotalEquippedItemStat(TEnumAsByte<ShooterGame_EPrimalItemStat> statType);
	//class UPrimalItem* GetEquippedItemOfType(TEnumAsByte<Engine_EPrimalEquipmentType> aType);
	//float GetEquippedArmorRating(TEnumAsByte<Engine_EPrimalEquipmentType> equipmentType);
	//class ADroppedItem* EjectItem(const struct FItemNetID& ItemId, bool bPreventImpule, bool bForceEject, bool bSetItemLocation, const struct FVector& LocationOverride, bool showHUDMessage, class UClass* TheDroppedTemplateOverride, bool bAssignToTribeForPickup, int AssignedTribeID);
};
struct UActorComponent : public UObject
{
public:
	static UClass* StaticClass()
	{
		static UClass* ptr = nullptr;
		if (!ptr)
			ptr = UObject::FindClass("Class Engine.ActorComponent");
		return ptr;
	}

};
struct AActor : public UObject
{
	PAD(0x240); // ORI 0x230
	USceneComponent* RootComponent;                // 0x0258                                              // need update here             0x0268); 268 0x8 correct

	PAD(0x290); // ORI 0x290
	FString												DescriptiveName;              // 0x04F0(0x0010)   // need update here        ???  0x0500);  0x0500
	USkeletalMeshComponent* MeshComponent;				  // 0x0500                           // USkeletalMeshComponent, 0x0C10);      //    ADefaultPawn, MeshComponent, 0x0528);
	//USceneComponent* Mesh;				  // 0x0500                           // USkeletalMeshComponent, 0x0C10);      //    ADefaultPawn, MeshComponent, 0x0528);
	class UCharacterMovementComponent* CharacterMovement;            // 0x0508(0x0008) 0x0518
	PAD(0x290); // ORI 0x290
	FString												TribeName;					  // 0x07A0(0x0010)   // need update here             0x07B0); 0x07B0
	PAD(0x1A4); // ORI 0x18C Correct  0x1A1
	float                                               ReplicatedCurrentHealth;      // 0x093C(0x0004)   // need update here            0x0964);  = 28  correct = 13 0x0964
	float                                               ReplicatedMaxHealth;          // 0x0940(0x0004)
	float                                              ReplicatedCurrentTorpor;                                   // 0x0944(0x0004) 
	float                                              ReplicatedMaxTorpor;                                       // 0x0948(0x0004)  0x0970
	PAD(0x2A8);  // ORI  0x2C0 // 2C0 a 2D6 Tested  0x2C0-before
	int                                                 CurrentItemCount;             // 0x0C0C(0x0004)   // need update here            0x0C1C); +10  0x0C1C
	int                                                 MaxItemCount;                 // 0x0C10(0x0004)   0x0C20
	PAD(0xEC); // ORI 0xCC correct = D4 D4-before
	struct UPrimalCharacterStatusComponent* MyCharacterStatusComponent;	              // 0x0CE0(0x0008)   // need update here            0x0D10);  +30 0x0D10
	PAD(0x8);  // ORI 0x8 before0x8
	class UPrimalInventoryComponent* MyInventoryComponent;                            // 0x0CF0(0x0008)   // need update here            0x0D20);0x0D20 0x0D20
	PAD(0x11C); // ORI 0x11C before11c
	float                                              OrbitCamMinZoomLevel;          // 0x0E14(0x0004)   // need update here            0x0E44); 0x0E44
	float                                              OrbitCamMaxZoomLevel;          // 0x0E18(0x0004)											0x0E48
	PAD(0x7C); // ORI  0x9C before 9c
	int                                                 NumBullets;                   // 0x0EB8(0x0004)   // need update here            0x0EC8); 0x0EC8
	PAD(0x664); // ORI 0x5F4 before 644
	FString												PlayerName;					  // 0x14B0           // need update here            0x1530); +80  correct = + 50  0x1530
	PAD(0xB8); // ORI 0xB8  // 126 - 0x10B before b8
	FString												PlatformProfileName;          // 0x1578(0x0010)   // need update here            0x15F8);  +80 Correct = + 0  0x15F8
	PAD(0x180); // ORI 0x180 before 188
	struct AShooterWeapon* CurrentWeapon;                // 0x1708(0x0008)   // need update here            0x1788); 0x1788
	PAD(0x248);  // ORI 0x270 289 - 290 a 294 = none before 290
	FName											    DinoNameTag;                  // 0x1980(0x0008)   // need update here            0x19D8);0x19D8
	PAD(0xE5);
	bool                                                       bIsControllingBallista;                                  // 0x1AC5(0x0001) BlueprintVisible, BlueprintReadOnly, ZeroConstructor, IsPlainOldData, NoDestructor
	PAD(0x18E);
	float                                                      ReplicatedWeight;                                        // 0x1C54(0x0004) Net, ZeroConstructor, Transient, IsPlainOldData, NoDestructor






	class UPrimalPlayerData* GetPlayerData()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterCharacter.GetPlayerData");
		struct {
			UPrimalPlayerData* ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	FVector GetActorViewDirection()
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetActorViewDirection");
		struct {
			FVector ReturnValue;
		}Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	inline bool IsDino()
	{
		static auto obj = UObject::FindClass("Class ShooterGame.PrimalDinoCharacter");
		return IsA(obj);
	}
	inline bool IsPlayer()
	{
		static auto obj = UObject::FindClass("Class ShooterGame.ShooterCharacter");
		return IsA(obj);
	}
	inline bool IsWeapon()
	{
		static auto obj = UObject::FindClass("Class ShooterGame.ShooterWeapon");
		return IsA(obj);
	}
	inline bool IsDroppedItem()
	{
		static auto obj = UObject::FindClass("Class ShooterGame.DroppedItem");
		return IsA(obj);

	}
	inline bool IsTurret(std::string Name, std::string& ShortName)
	{
		if (Name == "StructureTurretBaseBP_C")
		{
			ShortName = "AT";
			return true;
		}
		if (Name == "StructureTurretBaseBP_Heavy_C")
		{
			ShortName = "HT";
			return true;
		}
		if (Name == "StructureTurretTek_C")
		{
			ShortName = "TT";
			return true;
		}
		static auto Obj = UObject::FindClass("Class ShooterGame.PrimalStructureTurretPlant");
		if (IsA(Obj))
		{
			ShortName = "XPlant";
			return true;
		}
		return false;
	}
	inline bool IsFish(std::string DinoName)
	{
		if (DinoName == "Coel") return true;
		if (DinoName == "Trilobite") return true;
		if (DinoName == "Salmon") return true;
		return false;
	}

	inline bool IsAlpha(std::string DinoName)
	{
		if (DinoName == "Elite Raptor") return true;
		if (DinoName == "Elite Mega") return true;
		if (DinoName == "Elite Rex") return true;
		if (DinoName == "Elite Carno") return true;
		return false;
	}

	inline bool IsManta(std::string DinoName)
	{
		if (DinoName == "Manta") return true;
		return false;
	}

	inline int RetrievePlayerGender(std::string Name)
	{
		if (Name == "PlayerPawnTest_Male_C") return 1;
		if (Name == "PlayerPawnTest_Female_C") return 2;
		return 0;
	}
	inline struct AController* GetCharacterController()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetCharacterController");
		struct {
			struct AController* ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	void GetActorBounds(bool bOnlyCollidingComponents, struct FVector& Origin, struct FVector& BoxExtent)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetActorBounds");
		struct {
			bool bOnlyCollidingComponents;
			FVector Origin;
			FVector BoxExtent;
		} Params;

		Params.bOnlyCollidingComponents = bOnlyCollidingComponents;
		ProcessEvent(this, fn, &Params);
		Origin = Params.Origin;
		BoxExtent = Params.BoxExtent;
	}
	bool IsAlliedWith(int OtherTeamID)
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalCharacter.IsAlliedWithOtherTeam");
		struct {
			int OtherTeamID;
			bool ReturnValue;
		} Params;
		Params.OtherTeamID = OtherTeamID;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	struct AShooterWeapon* GetWeapon()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterCharacter.GetWeapon");
		struct {
			struct AShooterWeapon* ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	bool IsPlayingInitialSpawnAnim()
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterCharacter.IsPlayingInitialSpawnAnim");
		struct {
			bool ReturnValue;
		}Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	FVector GetVelocity(bool ForRagDoll)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetVelocity");
		struct {
			bool ForRagDoll;
			FVector ReturnValue;
		} Params;
		Params.ForRagDoll = ForRagDoll;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	FVector K2_GetActorLocation()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.K2_GetActorLocation");
		struct
		{
			FVector ReturnVector;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnVector;
	}
	FRotator K2_GetActorRotation()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.K2_GetActorRotation");
		struct {
			FRotator ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	TArray<class UActorComponent*> GetComponentsByClass(class UClass* ComponentClass)
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetComponentsByClass");
		struct {
			UClass* ComponentClass;
		}Params;
		Params.ComponentClass = ComponentClass;
		ProcessEvent(this, fn, &Params);
	}
	bool IsTamed()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalDinoCharacter.BPIsTamed");
		struct {
			bool ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	bool IsLocalPlayer()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalCharacter.IsOwningClient");
		struct {
			bool ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	struct APrimalDinoCharacter* GetBasedOrSeatingOnDino()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalCharacter.GetBasedOrSeatingOnDino");
		struct {
			struct APrimalDinoCharacter* ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	APrimalDinoCharacter* GetBasedOnDino()
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalCharacter.GetBasedOnDino");
		struct {
			APrimalDinoCharacter* ReturnValue;
		}Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	//struct UOnlineSessionEntryButton* OnlineSessionEntryButton()
	//{
	//	static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.OnlineSessionEntryButton");
	//	struct {
	//		struct UOnlineSessionEntryButton* ReturnValue;
	//	} Params;
	//	ProcessEvent(this, fn, &Params);
	//	return Params.ReturnValue;
	//}
	bool IsConscious()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalCharacter.BPIsConscious");
		struct {
			bool ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	bool IsFriendly(int OtherTeam)
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterPlayerState.IsFriendly");
		struct {
			int OtherTeam;
			bool ReturnValue;
		} Params;
		//auto flags = fn->FunctionFlags;
		//fn->FunctionFlags |= 0x00000400;
		Params.OtherTeam = OtherTeam;
		ProcessEvent(this, fn, &Params);
		//fn->FunctionFlags = flags;
		return Params.ReturnValue;
	}

	bool IsPrimalCharFriendly(class APrimalCharacter* primalChar)
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalCharacter.IsPrimalCharFriendly");

		struct {
			class APrimalCharacter* primalChar;
			bool ReturnValue;
		}params;
		params.primalChar = primalChar;

		//auto flags = fn->FunctionFlags;
		//fn->FunctionFlags |= 0x00000400;

		ProcessEvent(this, fn, &params);
		//fn->FunctionFlags = flags;


		return params.ReturnValue;
	}
	void DisableInput(class APlayerController* PlayerController)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.DisableInput");

		struct {
			class APlayerController* PlayerController;
		}params;
		params.PlayerController = PlayerController;

		//auto flags = fn->FunctionFlags;
		//fn->FunctionFlags |= 0x00000400;

		ProcessEvent(this, fn, &params);
		//fn->FunctionFlags = flags;

	}
	bool IsDead()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.IsDead");
		struct {
			bool ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	bool IsStructure()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.IsPrimalStructure");

		struct {
			bool                           ReturnValue;
		}params;

		ProcessEvent(this, fn, &params);

		return params.ReturnValue;
	}
	bool IsItemContainer()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.IsPrimalStructureItemContainer");
		struct {
			bool ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	bool IsExcludedContainer(std::string DescriptiveName)
	{
		std::array<std::string, 6> ExcludedContainerNames = { "Automated Turret", "Heavy Automated Turret", "Tek Turret", "Small Crop Plot", "Medium Crop Plot", "Large Crop Plot" };
		for (int i = 0; i < ExcludedContainerNames.size(); i++)
		{
			if (DescriptiveName == ExcludedContainerNames[i]) return true;
		}
		return false;
	}
	bool IsExcludedContainer2(std::string DescriptiveName)
	{
		std::array<std::string, 65> ExcludedContainerNames = { "Tek Terminal", "Terminal", "Artifact", "Tek Transmitter", "Green Obelisk Terminal", "Red Obelisk Terminal", "Blue Obelisk Terminal", "Supply Crate", "Bee Hive", "Artifact Container", "Loot Crate", "Tribute Terminal", "Tek Teleporter", "Tek Generator", "Bed", "Bunk Bed", "Sleeping Bag", "Tek Sleeping Pod", "Tek Replicator", "Tek Cloning Chamber", "Tek Trough", "Tek Forcefield", "Dedicated Storage", "Canoe", "Placed Taxidermy Base", "Storage Box", "Mortar and Pestle", "Cooking Pot", "Fireplace", "Loadout Manneqiun", "Smithy", "Refining Forge", "Bookshelf", "Preserving Bin", "Large Storage Box", "Beer Barrel", "Toilet", "Catapult Turret", "Cryofridge", "Minigun Turret", "Rocket-Launcher Turret", "Ammo Box", "Fabricator", "Power Generator", "Industrial Grill", "Vault", "Chemistry Bench", "Egg Incubator", "Industrial Cooker", "Item Cache", "Tek Sensor", "Vacuum Compartment Moonpool", "Vacuum Compartment", "Vessel", "Wardrums", "Water Well", "Delivery Crate", "Water Tank", "Wood Elevator Track", "Oil Pump", "Air Conditioner", "Tek Bridge", "Tek Jump Pad", "Tek Surveillance Console", "Elevator Track" };
		for (int i = 0; i < ExcludedContainerNames.size(); i++)
		{
			if (DescriptiveName == ExcludedContainerNames[i]) return true;
		}
		return false;
	}
	bool SetActorRotation(const FRotator& NewRotation)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.SetActorRotation");
		struct {
			FRotator NewRotation;
			bool ReturnValue;
		} Params;
		Params.NewRotation = NewRotation;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}

	void TurnAtRate(float val)
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalCharacter.TurnAtRate");
		struct {
			float val;
			//bool ReturnValue;
		} Params;
		Params.val = val;
		//auto flags = fn->FunctionFlags;
		//fn->FunctionFlags |= 0x00000400;
		ProcessEvent(this, fn, &Params);
	}
	void SetActorEnableCollision(bool bNewActorEnableCollision)
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function Engine.Actor.SetActorEnableCollision");
		struct {
			bool bNewActorEnableCollision;
		}Params;
		Params.bNewActorEnableCollision = bNewActorEnableCollision;
		ProcessEvent(this, fn, &Params);
	}
	//TArray<class UActorComponent*> GetComponentsByClass(class UClass* ComponentClass)
	//{
	//	static UFunction* fn = nullptr;
	//	if (!fn)
	//		fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetComponentsByClass");
	//	struct {
	//		UClass* ComponentClass;
	//	}Params;
	//	Params.ComponentClass = ComponentClass;
	//	ProcessEvent(this, fn, &Params);
	//}
	void ServerFireBallistaProjectile(const struct FVector& Origin, const struct FVector_NetQuantizeNormal& ShootDir)
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterCharacter.ServerFireBallistaProjectile");
		struct {
			FVector Origin;
			FVector_NetQuantizeNormal ShootDir;
		}Params;
		Params.Origin = Origin;
		Params.ShootDir = ShootDir;
		ProcessEvent(this, fn, &Params);
	}

};
struct AController
{
	//PAD(0x498);
	//FRotator                                    ControlRotation;                                           // 0x0498(0x000C)
};
struct UPrimalPlayerData
{
	PAD(0x28);
	FPrimalPlayerDataStruct                     MyData;                                                    // 0x0028(0x0448)
};
struct APrimalTargetableActor : public AActor
{
	PAD(0x90);  // ORI 0x80
	struct FString                                     DescriptiveName;                                           // 0x04F0(0x0010)   // need update here    0x0500);
	PAD(0x20);  // ORI 0x10
	float                                              ReplicatedHealth;                                          // 0x0510(0x0004)   // need update here    0x0520);
	float                                              Health;                                                    // 0x0514(0x0004)
	float                                              MaxHealth;                                                 // 0x0518(0x0004)
	//PAD(0x2A);
	//bool                                                     bForceFloatingDamageNumbers : 1;                         // 0x04ED(0x0001) BIT_FIELD Edit, DisableEditOnInstance, NoDestructor  // bShowFloatingDamageText, 0x21C8);

};
struct APrimalStructure : public APrimalTargetableActor
{
	PAD(0x48);  // ORI 0x38
	struct FName                                       StructureTag;                                              // 0x05B8(0x0008)  // need update here    0x05C8);
	PAD(0x168);  // ORI 0x158
	float                                              LastHealthPercentage;                                      // 0x0718(0x0004)  // need update here    0x0728);
	PAD(0xA4);  // ORI 0x94
	struct FString                                     OwningPlayerName;                                          // 0x07B0(0x0010)  // need update here    0x07C0);
	PAD(0xFB);  // ORI  0xEB
	struct FString                                     OwnerName;                                                 // 0x08A8(0x0010)  // need update here    0x08B8);

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class ShooterGame.PrimalStructure");
		return ptr;
	}
	//void StartRepair();
	//class UPrimalItem* PickupStructure(bool bIsQuickPickup, class AShooterPlayerController* PC);
	//bool IsLinkedToWaterOrPowerSource();
	//int GetPinCode();
	//bool HandleBedFastTravel(class AShooterPlayerController* ForPC, class APrimalStructure* ToBed);
};
struct APrimalStructureItemContainer : public APrimalStructure
{
	PAD(0x134); // ORI  0x124
	int                                                CurrentItemCount;                                          // 0x0C0C(0x0004)  // need update here    0x0C1C);
	int                                                MaxItemCount;                                              // 0x0C10(0x0004)
	PAD(0x8C); // ORI  0x7C
	float                                              PoweredNearbyStructureRange;                               // 0x0C90(0x0004)  // need update here    0x0CA0)

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class ShooterGame.PrimalStructureItemContainer");
		return ptr;
	}
	void RefreshItemCounts()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalStructureItemContainer.RefreshInventoryItemCounts");
		struct {
		} Params;
		ProcessEvent(this, fn, &Params);
	}
	//bool VerifyPinCode(int pinCode);
	//bool HasSamePinCode(class APrimalStructureItemContainer* otherContainer);
	//int GetQuantityOfItemWithoutCheckingInventory(class UClass* ItemToCheckFor);
};
struct APrimalStructureSeating : public APrimalStructureItemContainer
{
	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class ShooterGame.PrimalStructureSeating");
		return ptr;
	}
};
struct FWeaponData
{
	PAD(0x14);
	float                                              TimeBetweenShots;                                          // 0x0014(0x0004)
	float                                              ReloadDurationPerAmmoCount;                                // 0x0020(0x0004)
};
struct FInstantWeaponData
{
	float                                              WeaponSpread;                                              // 0x0000(0x0004)
	float                                              TargetingSpreadMod;                                        // 0x0004(0x0004)
	float                                              FinalWeaponSpreadMultiplier;                               // 0x0008(0x0004)
	float                                              FiringSpreadIncrement;                                     // 0x000C(0x0004)
	float                                              FiringSpreadMax;                                           // 0x0010(0x0004)
	float                                              WeaponRange;                                               // 0x0014(0x0004)
};
struct FItemNetInfo
{
	PAD(0x28);
	FString											   CustomItemName;                                            // 0x0028(0x0010)
	uint32_t                                           WeaponClipAmmo;                                            // 0x0098(0x0004) (ZeroConstructor, IsPlainOldData, NoDestructor)
	float                                              ItemDurability;                                            // 0x009C(0x0004) (ZeroConstructor, IsPlainOldData, NoDestructor)
	int                                                EggGenderOverride;                                         // 0x016C(0x0004) (ZeroConstructor, IsPlainOldData, RepSkip, NoDestructor)
	int                                                EggRandomMutationsFemale;                                  // 0x01A0(0x0004) (ZeroConstructor, IsPlainOldData, RepSkip, NoDestructor)
	int                                                EggRandomMutationsMale;                                    // 0x01A4(0x0004) (ZeroConstructor, IsPlainOldData, RepSkip, NoDestructor)
};
struct UPrimalCharacterStatusComponent
{
	PAD(0x6CC);
	int                                                BaseCharacterLevel;                                        // 0x06CC(0x0004)
	uint16_t                                           ExtraCharacterLevel;                                       // 0x06D0(0x0002)
	PAD(0x82);																									  // 0x0754 - 0x06D0 = 0x0084 - 2 Bytes(uint16_t) = 0x0082
	float                                              KnockedOutTorpidityRecoveryRateMultiplier;                 // 0x0754(0x0004)
	PAD(0x5C);																									  // 0x07B4 - 0x0754 = 0x0060 - 4 Bytes(float) = 0x005C
	float                                              MinInventoryWeight;                                        // 0x07B4(0x0004)
	PAD(0x3B4);																									  // 0x0B6C - 0x07B4 = 0x03B8 - 4 Bytes(float) = 0x03B4
	float                                              DinoImprintingQuality;                                     // 0x0B6C(0x0004)

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class ShooterGame.PrimalCharacterStatusComponent");
		return ptr;
	}
};
struct FPrimalPlayerCharacterConfigStructReplicated
{
public:
	bool                                                       bIsFemale : 1;                                           // 0x0000(0x0001) BIT_FIELD NoDestructor
	unsigned char                                              UnknownData_410C[0x3];                                   // 0x0001(0x0003) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	struct FLinearColor                                        BodyColors[0x4];                                         // 0x0004(0x0040) ZeroConstructor, IsPlainOldData, NoDestructor
	unsigned char                                              UnknownData_7LRN[0x4];                                   // 0x0044(0x0004) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	class FString                                              PlayerCharacterName;                                     // 0x0048(0x0010) ZeroConstructor
	float                                                      RawBoneModifiers[0x16];                                  // 0x0058(0x0058) ZeroConstructor, IsPlainOldData, NoDestructor
	int32_t                                                    PlayerSpawnRegionIndex;                                  // 0x00B0(0x0004) ZeroConstructor, IsPlainOldData, NoDestructor
	unsigned char                                              UnknownData_QOL9[0x4];                                   // 0x00B4(0x0004) MISSED OFFSET (PADDING)
};
struct APlayerState
{
	PAD(0x488);  // ORI 0x478
	struct FString                                     PlayerName;                                                // 0x0478  // need update here     0x0488);
	PAD(0x20);  // ORI 0x10
	int                                                PlayerId;                                                  // 0x0498  // need update here     0x04A8);

	static UClass* StaticClass()
	{
		static UClass* ptr = nullptr;
		if (!ptr)
			ptr = UObject::FindClass("Class Engine.PlayerState");
		return ptr;
	}
};
struct AShooterPlayerState : public APlayerState
{

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class ShooterGame.ShooterPlayerState");
		return ptr;
	}
	void ServerRequestCreateNewPlayer(const struct FPrimalPlayerCharacterConfigStructReplicated& PlayerCharacterConfig)
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterPlayerState.ServerRequestCreateNewPlayer");
		struct {
			FPrimalPlayerCharacterConfigStructReplicated PlayerCharacterConfig;
		}Params;
		Params.PlayerCharacterConfig = PlayerCharacterConfig;
		ProcessEvent(this, fn, &Params);
	}



	//Class ShooterGame.ShooterPlayerState
};
struct UPrimitiveComponent : public USceneComponent
{
public:
	static UClass* StaticClass()
	{
		static UClass* ptr = nullptr;
		if (!ptr)
			ptr = UObject::FindClass("Class Engine.PrimitiveComponent");
		return ptr;
	}
	void SetMaterial(int32_t ElementIndex, class UMaterialInterface* Material)
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function Engine.PrimitiveComponent.SetMaterial");
		struct {
			int32_t ElementIndex;
			UMaterialInterface* Material;
		}Params;
		Params.ElementIndex = ElementIndex;
		Params.Material = Material;
		ProcessEvent(this, fn, &Params);
	}
};
struct APawn : AActor
{
	PAD(0x28); // ORI 0x18
	UClass* AIControllerClass;                                         // 0x0488      // need update here       0x0498);
	class APlayerState* PlayerState;                                   // 0x0490
	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class Engine.Pawn");
		return ptr;
	}
};
struct UPrimalItem
{
	enum EPrimalItemType : uint8_t
	{
		MiscConsumable = 0,
		Equipment = 1,
		Weapon = 2,
		Ammo = 3,
		Structure = 4,
		Resource = 5,
		Skin = 6,
		WeaponAttachment = 7,
		Artifact = 8,
	};
	enum EPrimalItemStats : uint8_t
	{
		GenericQuality = 0,
		Armor = 1,
		MaxDurability = 2,
		WeaponDamagePercent = 3,
		WeaponClipAmmo = 4,
		Weight = 6,
	};
	enum EPrimalEquipmentType : uint8_t
	{
		Hat = 0,
		Shirt = 1,
		Pants = 2,
		Boots = 3,
		Gloves = 4,
		DinoSaddle = 5,
		Trophy = 6,
		Costume = 7,
		Shield = 8,
	};

	PAD(0x170);  // 0x168
	EPrimalItemType							   MyItemType;                                                // 0x0168(0x0001)   // 170       // need update here     0x0170); 0x0170 
	PAD(0x1);  // 0x1
	EPrimalItemType								MyEquipmentType;                                           // 0x016A(0x0001)  //172         // need update here    0x0172); 0x0172 
	PAD(0x99);  // 0x99
	int                                                SlotIndex;                                                 // 0x0204(0x0004)  // 0x20C     // need update here     0x020C); 0x020C 0x020C
	struct FItemNetID                                  ItemId;                                                    // 0x0208(0x0008)  0x0210
	PAD(0x110)  // 0x110
		struct FString                                     DescriptiveNameBase;                                       // 0x0320(0x0010)           // need update here    0x0328); 0x0328
	PAD(0x244);  // 0x244
	float                                              ItemDurability;                                            // 0x0574(0x0004)     // 0x057C);   // need update here      0x057C); 0x057C
	PAD(0x78);    // 0x78
	int                                                ItemQuantity;                                              // 0x05F0(0x0004)     // 0x05F8     // need update here     0x05F8); 0x05F8
	int                                                MaxItemQuantity;                                           // 0x05F4(0x0004) 0x05FC
	PAD(0x40);    //0x40
	class UClass* DroppedItemTemplateOverride;                                                                   // 0x0638(0x0008)   // 0x0640       // need update here     0x0640);

	bool CanDrop()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalItem.CanDrop");

		struct {
			bool                           ReturnValue;
		}Params;

		//auto flags = fn->FunctionFlags;
		//fn->FunctionFlags |= 0x00000400;

		ProcessEvent(this, fn, &Params);
		//fn->FunctionFlags = flags;


		return Params.ReturnValue;
	}

	float GetDurabilityPercentage()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalItem.GetDurabilityPercentage");

		struct {
			float                          ReturnValue;
		}params;

		//auto flags = fn->FunctionFlags;
		//fn->FunctionFlags |= 0x00000400;

		ProcessEvent(this, fn, &params);
		//fn->FunctionFlags = flags;


		return params.ReturnValue;
	}

	bool CanUse(bool bIgnoreCooldown)
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalItem.CanUse");

		struct {
			bool                           bIgnoreCooldown;
			bool                           ReturnValue;
		}Params;
		Params.bIgnoreCooldown = bIgnoreCooldown;

		//auto flags = fn->FunctionFlags;
		//fn->FunctionFlags |= 0x00000400;

		ProcessEvent(this, fn, &Params);
		//fn->FunctionFlags = flags;


		return Params.ReturnValue;
	}

	bool IsBroken()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalItem.IsBroken");

		struct {
			bool                                               ReturnValue;
		}Params;

		//auto flags = fn->FunctionFlags;
		//fn->FunctionFlags |= 0x00000400;

		ProcessEvent(this, fn, &Params);
		//fn->FunctionFlags = flags;


		return Params.ReturnValue;
	}

	UMaterialInterface* GetHUDIconMaterial()
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalItem.GetHUDIconMaterial");
		struct {
			UMaterialInterface* ReturnValue;
		}Params;

		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class ShooterGame.PrimalItem");
		return ptr;
	}

	//void RepairItem(bool bIgnoreInventoryRequirement, float UseNextRepairPercentage, float RepairSpeedMultiplier);
	//void BPSetWeaponClipAmmo(int NewClipAmmo);
	//float BPGetItemStatModifier(int idx, int ItemStatValue);
	//class UTexture2D* BPGetItemIcon(class AShooterPlayerController* ForPC);
	//float BPGetItemDurabilityPercentage();
	//struct FString BPGetItemDescription(const struct FString& InDescription, bool bGetLongDescription, class AShooterPlayerController* ForPC);
	//void BPDrawItemIcon(class UCanvas* ItemCanvas, const struct FVector2D& ItemCanvasSize, const struct FVector2D& ItemCanvasScale, bool bItemEnabled, const struct FLinearColor& TheTintColor);
	//bool AllowRemoteAddToInventory(class UPrimalInventoryComponent* invComp, class AShooterPlayerController* ByPC, bool bRequestedByPlayer);
	//bool AllowEquipItem(class UPrimalInventoryComponent* toInventory);
	//void AddItemDurability(float durabilityToAdd);
};
struct AShooterWeapon
{
	PAD(0x630);  //  ORI  0x620
	UPrimalItem* AssociatedPrimalItem;                                      // 0x0620(0x0008)         // need update here       0x0630);
	PAD(0x53C);  //  ORI  0x53C
	float                                              TargetingDelayTime;                                        // 0x0B64(0x0004)        // need update here       0x0B74);
	PAD(0xC);  //  ORI  0xC
	float                                               AimDriftYawFrequency;                                      // 0x0B74(0x0004)        // need update here       0x0B84); 0x0B84
	float                                               AimDriftPitchFrequency;                                      // 0x0B78(0x0004)											0xB88
	PAD(0x20);  //  ORI  0x20
	float                                              GlobalFireCameraShakeScale;                                // 0x0B9C(0x0004)        // need update here       0x0BAC);
	PAD(0xC);  //  ORI  0xC
	float                                              GlobalFireCameraShakeScaleTargeting;                       // 0x0BAC(0x0004)        // need update here       0x0BBC); 0x0BBC
	PAD(0x4);  //  ORI  0x4
	float                                              ReloadCameraShakeSpeedScale;                               // 0x0BB4(0x0004)        // need update here       0x0BC4); 0x0BC4
	PAD(0x114);  //  ORI  0x114
	bool                                                       bUseFireCameraShakeScale;                                // 0x0CCC(0x0001) 0x0CDC
	PAD(0x3);  //  ORI  0x3
	struct FInstantWeaponData                          InstantConfig;                                             // 0x0CD0(0x0030)        // need update here       0x0CE0);0x0CE0
	PAD(0x214);
	int32_t                                                    CurrentAmmo;                                             // 0x0AC8(0x0004) BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, Transient, SaveGame, IsPlainOldData, NoDestructor
	int32_t                                                    CurrentAmmoInClip;                                       // 0x0ACC(0x0004) BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, Transient, SaveGame, IsPlainOldData, RepNotify, NoDestructor


	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class ShooterGame.ShooterWeapon");
		return ptr;
	}
	FVector GetMuzzleLocation()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterWeapon.GetMuzzleLocation");
		struct {
			FVector ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	FVector GetMuzzleDirection()
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterWeapon.GetMuzzleDirection");
		struct {
			FVector ReturnValue;
		}Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	//void SetLockedTarget(class AActor* Actor, bool bIsLocked)
	//{
	//	static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterWeapon_Projectile.SetLockedTarget");

	//	AShooterWeapon_Projectile_SetLockedTarget_Params params;
	//	params.Actor = Actor;
	//	params.bIsLocked = bIsLocked;

	//	auto flags = fn->FunctionFlags;
	//	fn->FunctionFlags |= 0x00000400;

	//	UObject::ProcessEvent(fn, &params);
	//	fn->FunctionFlags = flags;

	//}
	//void UseAmmo(int UseAmmoAmountOverride);
	//void SetAmmoInClip(int newAmmo);
	//float GetWeaponDamageMultiplier();
	//struct FVector GetMuzzleDirection();
	//int GetCurrentAmmoInClip();
	//int GetCurrentAmmo();
	//void ConsumeAmmoItem(int Quantity);
	//bool CanFire(bool bForceAllowSubmergedFiring);
	//bool AllowedToFire(bool bForceAllowSubmergedFiring);
};
struct APrimalBuff
{
public:

};
struct UCheatManager : public UObject
{
public:
	static UClass* StaticClass()
	{
		static UClass* ptr = nullptr;
		if (!ptr)
			ptr = UObject::FindClass("Class Engine.CheatManager");
		return ptr;

	}
};
struct UShooterCheatManager : public UCheatManager
{
public:
	void RainMonkeys(int32_t NumberActors, float SpreadAmount, float ZOffset)
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterCheatManager.RainMonkeys");
		struct {
			int32_t NumberActors;
			float SpreadAmount;
			float ZOffset;
		}Params;
		Params.NumberActors = NumberActors;
		Params.SpreadAmount = SpreadAmount;
		Params.ZOffset = ZOffset;
	}
	void ForceCheckInMesh()
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterCheatManager.ForceCheckInMesh");
		struct {
			bool ReturnValue;
		}Params;
		ProcessEvent(this, fn, &Params);

	}
	static UClass* StaticClass()
	{
		static UClass* ptr = nullptr;
		if (!ptr)
			ptr = UObject::FindClass("Class ShooterGame.ShooterCheatManager");
		return ptr;
	}
};
struct AShooterCharacter //: public APrimalCharacter
{
	PAD(0x15F8);  // ORI  0x15C8
	FString                                                    PlatformProfileName;                                       // 0x1578(0x0010)            // need update here        0x15F8); +80 0x15F8
	PAD(0x180);  // ORI  0x180
	AShooterWeapon* CurrentWeapon;                                             // 0x1708(0x0008)            // need update here        0x1788);0x1788
	PAD(0x526);
	bool                                                       bPlaySpawnAnim : 1;                                      // 0x1261(0x0001) BIT_FIELD Net, Transient, NoDestructor
	//bool                                                       bPlayFirstSpawnAnim : 1;                                 // 0x1261(0x0001) BIT_FIELD Net, Transient, NoDestructor

//	PAD(0x1C54);  // ORI  0x1BD4
//	float                                                      ReplicatedWeight;                                        // 0x1BD4(0x0004) Net, ZeroConstructor, Transient, IsPlainOldData, NoDestructor       // need update here        0x1C54);

	//class USkeletalMeshComponent* Mesh;  // 0x0500(0x0008) Edit, BlueprintVisible, ExportObject, BlueprintReadOnly, ZeroConstructor, EditConst, InstancedReference, SubobjectReference, IsPlainOldData, NoDestructor


	//	CHEAT_GEAR_CHECK_OFFSET(CG::AShooterCharacter, ReplicatedWeight, 0x1BD4);


	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class ShooterGame.ShooterCharacter");
		return ptr;
	}
	AShooterWeapon* GetWeapon()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterCharacter.GetWeapon");
		struct {
			AShooterWeapon* ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	APrimalBuff* GetBuff(UClass* BuffClass)
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalCharacter.GetBuff");
		struct {
			UClass* BuffClass;
		}Params;
		Params.BuffClass = BuffClass;
		ProcessEvent(this, fn, &Params);
	}
	void NetSetOverrideHeadHairColor(const struct FLinearColor& HairColor)
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterCharacter.NetSetOverrideHeadHairColor");
		struct {
			FLinearColor HairColor;
		} Params;
		Params.HairColor = HairColor;
		ProcessEvent(this, fn, &Params);
	}
	void RenamePlayer(const class FString& NewName)
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterCharacter.RenamePlayer");
		struct {
			FString NewName;
		}Params;
		Params.NewName = NewName;
		ProcessEvent(this, fn, &Params);
	}

	float GetHealthPercentage();



	//void TryCutEnemyGrapplingCable();
	//void RenamePlayer(const struct FString& NewName);
	//class APrimalDinoCharacter* GetRidingDino();
};
struct ACharacter : public APawn
{
public:

	void ClientCheatGhost()
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function Engine.Character.ClientCheatGhost");
		struct {
		}Params;
		ProcessEvent(this, fn, &Params);
	}
	void ClientCheatFly()
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function Engine.Character.ClientCheatFly");
		struct {
		}Params;
		ProcessEvent(this, fn, &Params);
	}

	static UClass* StaticClass()
	{
		static UClass* ptr = nullptr;
		if (!ptr)
			ptr = UObject::FindClass("Class Engine.Character");
		return ptr;
	}
};
struct APrimalCharacter
{
	PAD(0x7B0);  // ORI  0x7A0
	FString										       TribeName;                                                 // 0x07A0(0x0010)        // need update here     0x07B0); 0x07B0
	PAD(0x1A4);  // ORI  0x18C
	float                                              ReplicatedCurrentHealth;                                   // 0x093C(0x0004)        // need update here     0x0964);  +28 0x0964
	float                                              ReplicatedMaxHealth;                                       // 0x0940(0x0004)				0x968
	PAD(0x78);  // ORI  0x78
	float                                              RunningSpeedModifier;                                      // 0x09BC(0x0004)        // need update here     0x09E4); 0x09E4
	PAD(0x64);  // ORI  0x64
	float                                              FallDamageMultiplier;                                      // 0x0A24(0x0004)        // need update here     0x0A4C); 0x0A4C
	PAD(0x2C0);  // ORI  0x2B8
	UPrimalCharacterStatusComponent* MyCharacterStatusComponent;								  // 0x0CE0(0x0008)        // need update here     0x0D10);  0x0D10
	PAD(0x12C);  // ORI  0x12C
	float                                              OrbitCamMinZoomLevel;                                      // 0x0E14(0x0004)        // need update here     0x0E44); 0x0E44
	float                                              OrbitCamMaxZoomLevel;                                      // 0x0E18(0x0004)													0xe48
	PAD(0x22C);  // ORI  0x21C
	float                                              ExtraMaxSpeedModifier;                                     // 0x1048(0x0004)        // need update here     0x1078);  0x1078
	float                                              ExtraRotationRateModifier;                                 // 0x104C(0x0004)					0x107C
	PAD(0x110);  // ORI  0x110 try to 124 + 4
	float                                              AdditionalMaxUseDistance;                                  // 0x1160(0x0004)        // need update here     0x1190); 0x1190

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class ShooterGame.PrimalCharacter");
		return ptr;
	}

	bool IsConscious()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalCharacter.BPIsConscious");
		struct {
			bool ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	bool IsLocalPlayer()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalCharacter.IsOwningClient");
		struct {
			bool ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	bool IsAlive()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalCharacter.IsAlive");
		struct {
			bool ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
};
struct APrimalDinoCharacter
{
	PAD(0xD10);  //  ORI   0x0CE0
	UPrimalCharacterStatusComponent* MyCharacterStatusComponent;                                // 0x0CE0(0x0008)                                     // need update here         0x0D10);
	PAD(0x630);  //  ORI   0x630
	float                                              ChargeSpeedMultiplier;                                     // 0x1318(0x0004)               // need update here         0x1370);
	PAD(0x4E4);  //  ORI   0x4E4
	float                                              AllowRidingMaxDistance;                                    // 0x1800(0x0004)               // need update here         0x1858);
	PAD(0x1A4);  //  ORI   0x17C
	FName											   DinoNameTag;                                               // 0x1980(0x0008)               // need update here         0x19D8);
	PAD(0x290);  //  ORI   0x290
	float                                              FlyingRunSpeedModifier;                                    // 0x1C18(0x0004)               // need update here         0x1C70);
	PAD(0x17C);  //  ORI   0x17C
	float                                              ExtraRunningSpeedModifier;                                 // 0x1D98(0x0004)               // need update here         0x1DF0);
	float                                              ScaleExtraRunningSpeedModifierMin;                         // 0x1D9C(0x0004)
	float                                              ScaleExtraRunningSpeedModifierMax;                         // 0x1DA0(0x0004) (Edit, BlueprintVisible, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor)
	float                                              ScaleExtraRunningSpeedModifierSpeed;                       // 0x1DB4(0x0004) (Edit, BlueprintVisible, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor)
	PAD(0x14);  //  ORI   0x14
	float                                              RiderMovementSpeedScalingRotationRatePowerMultiplier;      // 0x1DBC(0x0004)               // need update here         0x1E14);
	PAD(0x110);  //  ORI   0x110
	float                                              RiderFlyingRotationRateModifier;                           // 0x1ED0(0x0004)               // need update here         0x1F28);
	PAD(0x8);  //  ORI   0x8
	float                                              WalkingRotationRateModifier;                               // 0x1EDC(0x0004)               // need update here         0x1F34);
	PAD(0xB4);  //  ORI   0xB0
	float                                              RidingSwimmingRunSpeedModifier;                            // 0x1F90(0x0004)               // need update here         0x1FEC);


	//class FString                                              UploadedFromServerName;                                  // 0x1FF8(0x0010) Net, ZeroConstructor, SaveGame, NativeAccessSpecifierPublic
	//class FString                                              LatestUploadedFromServerName;                            // 0x2008(0x0010) ZeroConstructor, SaveGame, NativeAccessSpecifierPublic
	//class FString                                              PreviousUploadedFromServerName;                          // 0x2018(0x0010) ZeroConstructor, SaveGame, NativeAccessSpecifierPublic
	//class FString                                              TamedOnServerName;                                       // 0x2028(0x0010) ZeroConstructor, SaveGame, NativeAccessSpecifierPublic

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class ShooterGame.PrimalDinoCharacter");
		return ptr;
	}
	void SetRider(class AShooterCharacter* aRider)
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function ShooterGame.PrimalDinoCharacter.SetRider");
		struct {
			AShooterCharacter* aRider;
		}Params;
		Params.aRider = aRider;
		ProcessEvent(this, fn, &Params);
	}


	//void SetImprintPlayer(class AShooterCharacter* forChar);
	//void SetBabyAge(float TheAge);
	//void RemovePassenger(class APrimalCharacter* ACharacter, bool bFromCharacter, bool bFromPlayerController);
	//bool IsPassengerSeatAvailable(int PassengerSeatIndex);
	//int GetPassengersSeatIndex(class APrimalCharacter* Passenger);
	//void GetPassengersAndSeatIndexes(TArray<class APrimalCharacter*>* Passengers, TArray<int>* Indexes);
	//TArray<class APrimalCharacter*> GetPassengers();
	//class APrimalCharacter* GetPassengerPerSeat(int SeatIndex);
	//void ForceClearRider();
	//void FireProjectile(const struct FVector& Origin, const struct FVector_NetQuantizeNormal& ShootDir, bool bScaleProjDamageByDinoDamage);
	//void FireMultipleProjectilesEx(class UClass* ProjectileClass, TArray<struct FVector> Locations, TArray<struct FVector> Directions, bool bAddPawnVelocityToProjectile, bool bScaleProjDamageByDinoDamage, class USceneComponent* HomingTarget, const struct FVector& HomingTargetOffset, float OverrideInitialSpeed);
	//void FireMultipleProjectiles(TArray<struct FVector> Locations, TArray<struct FVector> Directions, bool bScaleProjectileDamageByDinoDamage);
	//void DinoShoulderMountedLaunch(const struct FVector& launchDir, class AShooterCharacter* throwingCharacter);
	//void DinoFireProjectileEx(class UClass* ProjectileClass, const struct FVector& Origin, const struct FVector_NetQuantizeNormal& ShootDir, bool bScaleProjDamageByDinoDamage, bool bAddDinoVelocityToProjectile, float OverrideInitialSpeed, float OverrideMaxSpeed, float ExtraDirectDamageMultiplier, float ExtraExplosionDamageMultiplier, bool spawnOnOwningClient);
	//class ADroppedItem* CreateCloneFertilizedEgg(const struct FVector& AtLoc, const struct FRotator& AtRot, class UClass* DroppedItemTemplateOverride, int NumMutationsToAdd);
	//void CopySettingsToOtherDino(class APlayerController* ForPC, class APrimalDinoCharacter* FromDino, class APrimalDinoCharacter* OtherDino, int SettingTypeUseIndex);
	//bool CarryCharacter(class APrimalCharacter* aRider, bool byPassCanCarryCheck);
};
struct APlayerController//  : AController
{
	PAD(0x4A8)  // ORI  0x498  update 0x4A8
		FRotator                                                   ControlRotation;                                           // 0x0498(0x000C)                  // need update here        0x04A8);
	PAD(0x54);  // ORI  0x54  update 64
	struct APlayerCameraManager* PlayerCameraManager;         // 0x4F8 (0x0008)                                                // need update here        0x0508);
	PAD(0x54C);  // ORI  0x534   update 0x54C
	float                                                      MaxUseDistance;                                            // 0x0A30(0x0004)                  // need update here        0x0A48);

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class Engine.PlayerController");
		return ptr;
	}

	bool DeprojectMousePositionToWorld(struct FVector WorldLocation, struct FVector WorldDirection)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.DeprojectMousePositionToWorld");

		//APlayerController_DeprojectMousePositionToWorld_Params params;

		struct {
			struct FVector                 WorldLocation;                  //(Parm, OutParm, ZeroConstructor, IsPlainOldData, NoDestructor)
			struct FVector                 WorldDirection;                 //(Parm, OutParm, ZeroConstructor, IsPlainOldData, NoDestructor)
			bool                           ReturnValue;						//(Parm, OutParm, ZeroConstructor, ReturnParm, IsPlainOldData, NoDestructor)
		}params;
		//auto flags = fn->FunctionFlags;
		//fn->FunctionFlags |= 0x00000400;

		ProcessEvent(this, fn, &params);
		//fn->FunctionFlags = flags;

		//if (WorldLocation != nullptr)
		//	*WorldLocation = params.WorldLocation;
		//if (WorldDirection != nullptr)
		//	*WorldDirection = params.WorldDirection;


		return params.ReturnValue;
	}


	AShooterCharacter* GetPlayerCharacter()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterPlayerController.GetPlayerCharacter");
		struct {
			AShooterWeapon* ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		Params.ReturnValue;
	}

	void EquipPawnItem(const struct FItemNetID& ItemId)
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterPlayerController.ServerEquipPawnItem");

		struct {
			struct FItemNetID                                  ItemId;
		}Params;
		Params.ItemId = ItemId;

		//auto flags = fn->FunctionFlags;
		//fn->FunctionFlags |= 0x00000400;

		ProcessEvent(this, fn, &Params);
		//fn->FunctionFlags = flags;

	}
	void UnlockExplorerNote(int32_t ExplorerNoteIndex, bool forceShowExplorerNoteUI)
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterPlayerController.UnlockExplorerNote");
		struct {
			int ExplorerNoteIndex;
			bool forceShowExplorerNoteUI;
		}Params;
		Params.ExplorerNoteIndex = ExplorerNoteIndex;
		Params.forceShowExplorerNoteUI = forceShowExplorerNoteUI;
		ProcessEvent(this, fn, &Params);
	}
	void ServerSendChatMessage(const class FString& ChatMessage, uint8_t SendMode)
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterPlayerController.ServerSendChatMessage");
		struct {
			FString ChatMessage;
			uint8_t SendMode;
		}Params;
		Params.ChatMessage = ChatMessage;
		Params.SendMode = SendMode;
		ProcessEvent(this, fn, &Params);
	}
	void ClientNotifyDinoDeath(const class FString& DinoName, const class FString& AttackerName, bool bIsVehicle)
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterPlayerController.ClientNotifyDinoDeath");
		struct {
			FString DinoName;
			FString AttackerName;
			bool bIsVehicle;
		}Params;
		Params.DinoName = DinoName;
		Params.AttackerName = AttackerName;
		Params.bIsVehicle = bIsVehicle;
		ProcessEvent(this, fn, &Params);
	}
	void FOV(float NewFOV)
	{
		static UFunction* fn = nullptr;
		if (!fn) fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.FOV");

		struct {
			float NewFOV;
		} Params;
		Params.NewFOV = NewFOV;

		ProcessEvent(this, fn, &Params);
	}
	void UpdateRotation(float DeltaTime)
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterPlayerController.UpdateRotation");
		struct {
			float DeltaTime;
		}Params;
		Params.DeltaTime = DeltaTime;
		ProcessEvent(this, fn, &Params);
	}

	/*
bool STATIC_LineTraceSingle_DEPRECATED(class UObject* WorldContextObject, const struct FVector& Start, const struct FVector& End, ECollisionChannel TraceChannel, bool bTraceComplex, TArray<class AActor*> ActorsToIgnore, ECollisionChannel DrawDebugType, struct FHitResult* OutHit, bool bIgnoreSelf)
{
	static auto fn = UObject::FindObject<UFunction>("Function Engine.KismetSystemLibrary.LineTraceSingle_DEPRECATED");
	struct {
			class UObject*                                     WorldContextObject;                                        // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
			struct FVector                                     Start;                                                     // (ConstParm, Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
			struct FVector                                     End;                                                       // (ConstParm, Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
			ECollisionChannel              TraceChannel;                                              // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
			bool                                               bTraceComplex;                                             // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
			TArray<class AActor*>                              ActorsToIgnore;                                            // (ConstParm, Parm, OutParm, ZeroConstructor, ReferenceParm)
			ECollisionChannel                DrawDebugType;                                             // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
			struct FHitResult                                  OutHit;                                                    // (Parm, OutParm)
			bool                                               bIgnoreSelf;                                               // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
			bool                                               ReturnValue;                                               // (Parm, OutParm, ZeroConstructor, ReturnParm, IsPlainOldData, NoDestructor)
	} params;

	params.WorldContextObject = WorldContextObject;
	params.Start = Start;
	params.End = End;
	params.TraceChannel = TraceChannel;
	params.bTraceComplex = bTraceComplex;
	params.ActorsToIgnore = ActorsToIgnore;
	params.DrawDebugType = DrawDebugType;
	params.bIgnoreSelf = bIgnoreSelf;

	//auto flags = fn->FunctionFlags;
	//fn->FunctionFlags |= 0x00000400;

	ProcessEvent(this, fn, &params);
	//fn->FunctionFlags = flags;

	if (OutHit != nullptr)
		*OutHit = params.OutHit;


	return params.ReturnValue;
}*/

	void IgnoreLookInput(bool bIgnore)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.ClientIgnoreLookInput");
		struct {
			bool bIgnore;
		} Params;
		Params.bIgnore = bIgnore;
		ProcessEvent(this, fn, &Params);
	}
	bool LineTraceComponent(const FVector& TraceStart, const FVector& TraceEnd, bool bTraceComplex, bool bShowTrace, FVector& HitLocation, FVector& HitNormal, FName& BoneName)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PrimitiveComponent.K2_LineTraceComponent");
		struct {
			FVector TraceStart;
			FVector TraceEnd;
			bool bTraceComplex;
			bool bShowTrace;
			FVector HitLocation;
			FVector HitNormal;
			FName BoneName;
			bool ReturnValue;
		} Params;
		Params.TraceStart = TraceStart;
		Params.TraceEnd = TraceEnd;
		Params.bTraceComplex = bTraceComplex;
		Params.bShowTrace = bShowTrace;
		ProcessEvent(this, fn, &Params);
		HitLocation = Params.HitLocation;
		HitNormal = Params.HitNormal;
		BoneName = Params.BoneName;
		return Params.ReturnValue;
	}
	bool LineOfSightTo(AActor* Other, const FVector& ViewPoint, bool bAlternateChecks)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Controller.LineOfSightTo");
		struct {
			AActor* Other;
			FVector ViewPoint;
			bool bAlternateChecks;
			bool ReturnValue;
		} Params;
		Params.Other = Other;
		Params.ViewPoint = ViewPoint;
		Params.bAlternateChecks = bAlternateChecks;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	void GetAllActorsOfClass(UObject* UWorld, UClass* ActorClass, TArray<AActor*>& OutActors)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.GameplayStatics.GetAllActorsOfClass");
		struct {
			UObject* WorldContextObject;
			UClass* ActorClass;
			TArray<AActor*> OutActors;
		} Params;
		Params.WorldContextObject = UWorld;
		Params.ActorClass = ActorClass;
		ProcessEvent(this, fn, &Params);
		OutActors = Params.OutActors;
	}
	void SetControlRotation(const FRotator& NewRotation)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Controller.SetControlRotation");
		struct {
			FRotator NewRotation;
		} Params;
		Params.NewRotation = NewRotation;
		ProcessEvent(this, fn, &Params);
	}
	void SetMousePosition(APlayerController* Controller, float X, float Y)
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.VictoryCore.SetMousePosition");
		struct {
			APlayerController* Controller;
			float X;
			float Y;
		} Params;
		Params.Controller = Controller;
		Params.X = X;
		Params.Y = Y;
		ProcessEvent(this, fn, &Params);
	}
	bool WasInputKeyJustPressed(const FKey& Key)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.WasInputKeyJustPressed");
		if (!fn) return false;
		struct {
			FKey Key;
			bool ReturnValue = false;
		} Params;

		Params.Key = Key;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	bool IsInputKeyDown(const struct FKey& Key)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.IsInputKeyDown");
		struct {
			FKey Key;
			bool ReturnValue;
		} Params;
		Params.Key = Key;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	void AddYawInput(float val)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.AddYawInput");
		struct {
			float val;
		} Params;
		Params.val = val;
		ProcessEvent(this, fn, &Params);
	}
	void AddPitchInput(float val)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.AddPitchInput");
		struct {
			float val;
		} Params;
		Params.val = val;
		ProcessEvent(this, fn, &Params);
	}
	void AddRollInput(float val)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.AddRollInput");
		struct {
			float val;
		} Params;
		Params.val = val;
		ProcessEvent(this, fn, &Params);
	}
	int GetTribeId()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterPlayerState.GetTribeId");
		struct {
			int ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	void TryToForceUploadCharacter()
	{
		static auto fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterPlayerController.TryToForceUploadCharacter");
		struct {
		} Params;
		ProcessEvent(this, fn, &Params);
	}
	void ServerUploadCharaterDataToArk(class UPrimalInventoryComponent* InventoryComp)
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterPlayerController.ServerUploadCharaterDataToArk");
		struct {
			UPrimalInventoryComponent* InventoryComp;
		}Params;
		Params.InventoryComp = InventoryComp;
		ProcessEvent(this, fn, &Params);
	}

	void ClientReceiveTribeLog(const class FString& LogString)
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function ShooterGame.ShooterPlayerController.ClientReceiveTribeLog");
		struct {
			FString LogString;
		}Params;
		Params.LogString = LogString;
		ProcessEvent(this, fn, &Params);
	}

};
struct UPlayer
{
	PAD(0x30);
	APlayerController* PlayerController;											// 0x0030
};
struct ULocalPlayer : public UPlayer
{
	PAD(0x30);
	FVector                                     LastViewLocation;                                          // 0x0080

	inline FRotator FindLookAtRotation(const FVector& Start, const FVector& Target)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.KismetMathLibrary.FindLookAtRotation");
		struct {
			FVector Start;
			FVector Target;
			FRotator ReturnValue;
		} Params;
		Params.Start = Start;
		Params.Target = Target;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	inline FRotator NormalizedDeltaRotator(const FRotator& A, const FRotator& B)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.KismetMathLibrary.NormalizedDeltaRotator");
		struct {
			FRotator A;
			FRotator B;
			FRotator ReturnValue;
		} Params;
		Params.A = A;
		Params.B = B;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	struct UKismetMathLibrary_GetDirectionVector_Params
	{
		struct FVector                                     From;                                                      // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
		struct FVector                                     To;                                                        // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
		struct FVector                                     ReturnValue;                                               // (Parm, OutParm, ZeroConstructor, ReturnParm, IsPlainOldData, NoDestructor)
	};
	struct FVector GetDirectionVector(const struct FVector& From, const struct FVector& To)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.KismetMathLibrary.GetDirectionVector");

		UKismetMathLibrary_GetDirectionVector_Params params;
		params.From = From;
		params.To = To;

		//auto flags = fn->FunctionFlags;
		//fn->FunctionFlags |= 0x00000400;

		ProcessEvent(this, fn, &params);
		//fn->FunctionFlags = flags;


		return params.ReturnValue;
	}
};
struct ULevelBase
{
	PAD(0x88);
	TTransArray<AActor*> Actors;				// 0x0088
};
struct ULevel : public ULevelBase
{

};
struct UGameInstance : public UObject
{
	PAD(0x10);
	TArray<ULocalPlayer*>                        LocalPlayers;						// 0x0038
};
struct UWorld : public UObject
{
	static inline UWorld* GWorld = nullptr;
	PAD(0xD0);
	ULevel* PersistentLevel;          // 0x00F8
	PAD(0x28);
	class AGameState* GameState;				  // 0x0128
	PAD(0x160);
	UGameInstance* OwningGameInstance;       // 0x0290
};
struct AGameState// : public AInfo      // si Ainfo activer wrong info
{
	PAD(0x568); // ORI 554 + 14 correct = +11 0x565
	int                                                NumPlayerConnected;                                        // 0x0554(0x0004)        0x0568        // need update here              0x0568);0x568
	PAD(0x4C);  // ORI = 48 -1  
	int                                                    NumTamedDinos;                                           // 0x05A0(0x0004)            0x05BC    // need update here            0x05B8);0x05B8
	PAD(0x38C); //b8
	class FString                                              ServerSessionName;                                       /// 0x0948(0x0010)t0x0948
	PAD(0x460);
	TArray<class APlayerState*>                                PlayerArray;                                             // 0x04D8

};
struct FMinimalViewInfo
{
	FVector Location;
	FRotator Rotation;
	FRotator AimRotation;
	PAD(0x4);
	float FOV;
};
struct APlayerCameraManager
{
	PAD(0x498);     //UPDATE HERE ORI = 0x488
	float                                              DefaultFOV;


	FRotator GetCameraRotation()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerCameraManager.GetCameraRotation");
		struct {
			FRotator ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
	FVector GetCameraLocation()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerCameraManager.GetCameraLocation");
		struct {
			FVector ReturnValue;
		} Params;
		ProcessEvent(this, fn, &Params);
		return Params.ReturnValue;
	}
};
struct FHitResult
{
	unsigned char                                      bBlockingHit : 1;                                          // 0x0000(0x0001)
	unsigned char                                      bStartPenetrating : 1;                                     // 0x0000(0x0001)
	unsigned char                                      UnknownData_HYSO[0x3];                                     // 0x0001(0x0003)
	float                                              Time;                                                      // 0x0004(0x0004)
	struct FVector									   Location;                                                  // 0x0008(0x000C)
	struct FVector									   Normal;                                                    // 0x0014(0x000C)
	struct FVector									   ImpactPoint;                                               // 0x0020(0x000C)
	struct FVector									   ImpactNormal;                                              // 0x002C(0x000C)
	struct FVector									   TraceStart;                                                // 0x0038(0x000C)
	struct FVector									   TraceEnd;                                                  // 0x0044(0x000C)
	float                                              PenetrationDepth;                                          // 0x0050(0x0004)
	int                                                Item;                                                      // 0x0054(0x0004)
	AActor* Actor;                                                     // 0x0060(0x0008)
	PAD(0x10);
	struct FName                                       BoneName;                                                  // 0x0078(0x0008)
	int                                                FaceIndex;                                                 // 0x0080(0x0004)
	unsigned char                                      UnknownData_RF5T[0x4];                                     // 0x0084(0x0004)

};
struct FCollisionQueryParams
{
	FName TraceTag;
	FName OwnerTag;
	bool bTraceAsyncScene;
	bool bTraceComplex;
	bool bFindInitialOverlaps;
	bool bReturnFaceIndex;
	bool bReturnPhysicalMaterial;
};
struct UCharacterMovementComponent
{
	PAD(0xE4);
	struct FVector									   Velocity;												//0x00E4 (0x000C) 0x00E4(0x000C)
	PAD(0x64);
	float                                              MaxStepHeight;                                             // 0x0154(0x0004) 0x0154
	PAD(0x68);
	float                                              GravityScale;                                              // 0x01C0(0x0004) 0x01C0
	PAD(0x4);
	float                                              MaxWalkSpeed;                                              // 0x01C8(0x0004) 0x01C8
	float                                              MaxWalkSpeedCrouched;                                      // 0x01CC(0x0004)
	float                                              MaxWalkSpeedProne;                                         // 0x01D0(0x0004)
	float                                              MaxCustomMovementSpeed;                                    // 0x01D4(0x0004)
	float                                              MaxSwimSpeed;                                              // 0x01D8(0x0004) 
	float                                              MaxFlySpeed;                                               // 0x01DC(0x0004) 0x01DC
	PAD(0x18);
	float                                              MaxAcceleration;                                           // 0x01F8(0x0004)
	PAD(0x30);
	float                                              Buoyancy;                                                  // 0x022C(0x0004)
	PAD(0x8);
	struct FRotator                                    RotationRate;                                              // 0x0238(0x000C)
	PAD(0x14);
	float                                              MaxOutOfWaterStepHeight;                                   // 0x0250(0x0004)
	PAD(0x20);
	float                                              InitialPushForceFactor;                                    // 0x0274(0x0004)
	float                                              PushForceFactor;                                           // 0x0278(0x0004)
	PAD(0x18);
	float                                              CrouchedSpeedMultiplier;                                   // 0x0294(0x0004)
	PAD(0x1C);
	float                                              BackwardsMaxSpeedMultiplier;                               // 0x02B4(0x0004)
	PAD(0x1C);
	struct FVector                                     Acceleration;                                              // 0x02D4(0x000C)
	PAD(0xD0);
	float                                              RotationAcceleration;                                      // 0x03B0(0x0004)
	PAD(0x8);
	float                                              SwimmingAccelZMultiplier;                                  // 0x03BC(0x0004)
	PAD(0x80);
	struct FRotator                                    CurrentRotationSpeed;                                      // 0x0410(0x000C)
};
struct UOnlineSessionEntryButton
{
public:
	PAD(0x648);
	uint32_t                                           NumPlayers;                                                // 0x0648(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor)
};
struct AShooterWeapon_Rapid
{
	PAD(0x808); // ORI  0x7E8
	FWeaponData                                              WeaponConfig;                                              // 0x0808(0x002C)
	PAD(0x28C); // ORI  0x1DC
	long double                                              LastFireTime;                                              //0x0AC0(0x0008
	PAD(0x38); // ORI  0x38
	long double                                           LastNotifyShotTime;                                          // 0x0B00(0x0008)
};
struct ABuff_TekArmor_C
{
public:
	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("BlueprintGeneratedClass Buff_TekArmor.Buff_TekArmor_C");
		return ptr;
	}

};
enum class E_TekGlovePunchState : uint8_t
{
	NewEnumerator0 = 0,
	NewEnumerator1 = 1,
	NewEnumerator4 = 2,
	NewEnumerator5 = 3,
	NewEnumerator6 = 4,
	E_MAX = 5
};
struct ABuff_TekArmor_Gloves_C : public ABuff_TekArmor_C
{
public:


	void Server_SetPunchChargeState(E_TekGlovePunchState newPunchState)
	{
		static UFunction* fn = nullptr;
		if (!fn)
			fn = UObject::FindObject<UFunction>("Function Buff_TekArmor_Gloves.Buff_TekArmor_Gloves_C.Server_SetPunchChargeState");

		struct {
			E_TekGlovePunchState newPunchState;
		}Params;
		Params.newPunchState = newPunchState;
		ProcessEvent(this, fn, &Params);
	}
	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("BlueprintGeneratedClass Buff_TekArmor_Gloves.Buff_TekArmor_Gloves_C");
		return ptr;
	}
};