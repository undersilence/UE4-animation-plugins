// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ARFilter.h"
#include "ContentBrowserDelegates.h"
#include "IAnimationSequenceBrowser.h"
#include "IDetailCustomization.h"
#include "IDetailsView.h"
#include "ISkeletonTreeItem.h"
#include "Modules/ModuleManager.h"
#include "Animation/AnimSequence.h"
#include "Components/VerticalBox.h"
#include "Types/SlateEnums.h"

class FToolBarBuilder;
class FMenuBuilder;
class SWidget;
class STextBlock;
class UCurveVector;


class FAnimSequenceToolkitsModule : public IModuleInterface
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
	FReply OnSequencesAdd();
	FReply OnSequencesClear();

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

	static UCurveVector* CreateCurveVectorAsset(const FString& PackagePath, const FString& CurveName);
	static bool GetBoneKeysByNameHelper(UAnimSequence* Seq,
		FString const& BoneName,
		TArray<FVector>& OutPosKey,
		TArray<FQuat>& OutRotKey, bool bConvertCS = false);
	static bool SetVariableCurveHelper(UAnimSequence* Seq, const FString& CurveName, FFloatCurve& InFloatCurve);
	// static TSharedPtr<FFloatCurve> GetOrCreateVariableCurveHelper(UAnimSequence* Seq, const FString& CurveName, bool bClear = true);

	static void MarkFootstepsFor1PAnimation(UAnimSequence* Seq, FString const& KeyBone = "LeftHand");
	static bool SaveBonesCurves(UAnimSequence* AnimSequence, FString const& BoneName, const FString& SavePath);

	TSharedRef<SWidget> ConstructBonePickerWidget();
	TSharedRef<SWidget> ConstructBoneTreeList();
	TSharedRef<SWidget> ConstructCurveExtractorWidget();
	TSharedRef<SWidget> ConstructMarkFootstepWidget();
	TSharedRef<SWidget> ConstructSequenceBrowser();
	TSharedRef<SWidget> ConstructAnimationPicker();
	TSharedRef<SWidget> ConstructAnimSequenceListWidget();
	TSharedRef<SWidget> ConstructPathPickerWidget();

	void OnAnimSequenceSelected(const FAssetData& SelectedAsset);
	void HandleSelectionChanged(const TArrayView<TSharedPtr<ISkeletonTreeItem>>& InSelectedItems,
		ESelectInfo::Type InSelectInfo);
private:
	// USkeleton* TargetSkeleton{nullptr};
	UAnimSequence* TargetAnimSequence{nullptr};
	FText TargetBoneText = FText::FromString("LeftHand");
	FString ExportPath = "/Game";
	// AnimationCurve AnimCurveCache;	

	TArray<UAnimSequence*> AnimSequences;

	// TSharedPtr<SWidget> SkeletonTreePanel;
	TSharedPtr<SListView<UAnimSequence*>> SequencesListView;
	TSharedPtr<SWidget> SequencePickerWidget;
	TSharedPtr<SWidget> PathPickerWidget;
	TSharedPtr<SWidget> BonePickerWidget;
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
