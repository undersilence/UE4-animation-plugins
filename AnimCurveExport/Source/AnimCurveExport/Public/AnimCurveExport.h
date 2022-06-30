// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Animation/AnimSequence.h"
#include "Types/SlateEnums.h"

class FToolBarBuilder;
class FMenuBuilder;
class SWidget;
class STextBlock;
class UCurveVector;

class FAnimCurveExportModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();

private:
	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	// enum
	// {
	// 	AXIS_X_BIT = 0x1,
	// 	AXIS_Y_BIT = 0x2,
	// 	AXIS_Z_BIT = 0x4
	// };
	//
	// enum
	// {
	// 	TYPE_TRANSLATION_BIT = 0x1,
	// 	TYPE_ROTATION_BIT = 0x2,
	// 	TYPE_SCALE_BIT = 0x4
	// };
	//
	// struct FSaveConfig
	// {
	// 	uint8_t AxisFlags{AXIS_X_BIT | AXIS_Y_BIT | AXIS_Z_BIT}; 
	// 	uint8_t TypeFlags = {TYPE_TRANSLATION_BIT | TYPE_ROTATION_BIT}; 
	// };


	TSharedRef<SWidget> OnCreateAnimPicker();
	static UCurveVector* CreateCurveVectorAsset(const FString& PackagePath, const FString& CurveName);
	static bool SaveBonesCurves(const UAnimSequence* AnimSequence,
	                            TArray<FName> BoneNames,
	                            const FString& SavePath);
	TSharedRef<SWidget> MakeCurveExtractor();

	// void OnPickAnimation(FAssetData const& AssetData);
	// FReply AddFromContentBrowser();
	// FReply OnExportCurve();
	// bool SaveToAsset(UObject* ObjectToSave);
	// TSharedRef<SWidget> MakeBonePicker();
	// TSharedRef<SWidget> MakeExportCheckbox();

private:
	UAnimSequence* TargetAnimSequence{nullptr};
	FText TargetBoneText;
	FString ExportPath = "/Game";
	// AnimationCurve AnimCurveCache;

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
