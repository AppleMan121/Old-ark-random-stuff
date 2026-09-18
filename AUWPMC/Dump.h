#pragma once
#include <Windows.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include "SDK/SDK.h"
#include "Logger.h"

//*																			*\\
//*																			*\\
//              (UWP) Ark Offset Dumper | Discord: Debug.It#6837
//*																			*\\
//*																			*\\

//#define DUMP_DEBUG // Debug Define - REMOVE IF DUMPER IS FUNCTIONAL
#define DLL_NAME							"ArkOffsetDumper.dll" // DLL Name - DLL Build Name

static HMODULE DLLHandle = nullptr;
static int PAD_NUMBER = 1;

struct PropertyInfo
{
	UProperty* PropertyPointer{};
	std::string Name{}, Type{}, OffsetString{};
	int PropertyOffset{}, ClassSize{};
	PropertyInfo(UProperty* Property, std::string PropertyName, std::string PropertyType, std::string OffsetStr, int PropertySize, int Offset) { PropertyPointer = Property; Name = PropertyName; Type = PropertyType; OffsetString = OffsetStr; ClassSize = PropertySize; PropertyOffset = Offset; }
};

struct OffsetDumper
{
	//*	Initialize - Initialize SDK, Logger, & More For Class Dumping *//
	static bool Init();

	//*	Eject - Eject ArkOffsetDumper.dll (Not Yet Functional) *//
	static bool Eject();

	//*	PADString - Generate String For Structure/Class Padding *//
	static std::string PADString();

	//*	Find Class Property Offset - Returns A Property Offset Based On Given Class & Property (Return -1 Means Fail) *//
	static int FindClassPropertyOffset(const char* Class, const char* Property);

	//*	Find Function Offset - Returns An Offset For The Provided Function Name (Return -1 Means Fail) *//
	static int FindFunctionOffset(const char* FunctionName);

	//*	Dump Class Property - Print & Pad Class Property Individually *//
	static void DumpClassProperty(PropertyInfo Property, PropertyInfo PreviousProperty, bool FirstProperty);

	static void DumpClass(UClass* ClassPointer);

	//*	Dump Partial Class - Print Specific Members Of A Class Into A Text File *//
	static void DumpPartialClass(const char* Class, std::vector<std::string> ClassProperties, std::vector<std::string> ClassPropertyTypes);

	//*	Dump Requested Offsets - Call Dump Functions Within Function Code Inside "Dump.cpp" To Print Offsets *//
	static void DumpRequestedOffsets();
};