#pragma once
struct FLinearColor
{
public:
	float                                                      R;                                                       // 0x0000(0x0004) Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor
	float                                                      G;                                                       // 0x0004(0x0004) Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor
	float                                                      B;                                                       // 0x0008(0x0004) Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor
	float                                                      A;                                                       // 0x000C(0x0004) Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor

	//added manually
	FLinearColor() : R(0.f), G(0.f), B(0.f), A(0.f) {}
	FLinearColor(float color[4]) : R(color[0]), G(color[1]), B(color[2]), A(color[3]) {}
	FLinearColor(float r, float g, float b, float a) : R(r), G(g), B(b), A(a) {}

};