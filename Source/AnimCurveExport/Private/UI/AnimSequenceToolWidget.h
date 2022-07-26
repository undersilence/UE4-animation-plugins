#pragma once
#include "BoneProxy.h"
#include "IDetailsView.h"
#include "Animation/AnimSequence.h"

#include "Layout/WidgetPath.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"

#include "AnimSequenceToolWidget.generated.h"

UCLASS()

class UFootstepSettings : public UObject
{
	GENERATED_BODY()

public:
	UFootstepSettings()
	{
	}

	static UFootstepSettings* Get()
	{
		if (!IsInitialized)
		{
			DefaultSetting = DuplicateObject(GetMutableDefault<UFootstepSettings>(), nullptr);
			DefaultSetting->AddToRoot();
			IsInitialized = true;
		}

		return DefaultSetting;
	}

	static void Destroy()
	{
		if (IsInitialized)
		{
			if (UObjectInitialized() && DefaultSetting)
			{
				DefaultSetting->RemoveFromRoot();
				DefaultSetting->MarkPendingKill();
			}

			DefaultSetting = nullptr;
			IsInitialized = false;
		}
	}

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

private:

	static bool IsInitialized;
	static UFootstepSettings* DefaultSetting;

public:
	UPROPERTY(EditAnywhere, Category=FootstepSetting)
	TArray<FString> TrackBoneNames = {"LeftHand", "RightHand"};
	
	UPROPERTY(EditAnywhere, Category=FootstepSetting)
	bool IsEnableDebug = false;
	
	SWidget* m_ParentWidget;
};

UCLASS()
class UAnimCurveSettings : public UObject
{
	GENERATED_BODY()

public:
	UAnimCurveSettings()
	{
	}

	static UAnimCurveSettings* Get()
	{
		if (!IsInitialized)
		{
			DefaultSetting = DuplicateObject(GetMutableDefault<UAnimCurveSettings>(), nullptr);
			DefaultSetting->AddToRoot();
			IsInitialized = true;
		}

		return DefaultSetting;
	}

	static void Destroy()
	{
		if (IsInitialized)
		{
			if (UObjectInitialized() && DefaultSetting)
			{
				DefaultSetting->RemoveFromRoot();
				DefaultSetting->MarkPendingKill();
			}

			DefaultSetting = nullptr;
			IsInitialized = false;
		}
	}

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
private:
	static bool IsInitialized;
	static UAnimCurveSettings* DefaultSetting;

public:

	UPROPERTY(EditAnywhere, Category=CurveSetting)
	FString TargetBoneName = "LeftHand";
	
	UPROPERTY(EditAnywhere, Category=CurveSetting)
	FDirectoryPath ExportDirectoryPath{"/Game/"};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CurveSetting)
	bool IsExtractPositionXYZ = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CurveSetting)
	bool IsExtractRotationXYZ = true;
	
	SWidget* m_ParentWidget;
};

UCLASS()
class UAnimSequenceSelection : public UObject
{
	GENERATED_BODY()

public:
	UAnimSequenceSelection()
	{
	}

	static UAnimSequenceSelection* Get()
	{
		if (!IsInitialized)
		{
			DefaultSetting = DuplicateObject(GetMutableDefault<UAnimSequenceSelection>(), nullptr);
			DefaultSetting->AddToRoot();
			IsInitialized = true;
		}

		return DefaultSetting;
	}

	static void Destroy()
	{
		if (IsInitialized)
		{
			if (UObjectInitialized() && DefaultSetting)
			{
				DefaultSetting->RemoveFromRoot();
				DefaultSetting->MarkPendingKill();
			}

			DefaultSetting = nullptr;
			IsInitialized = false;
		}
	}

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
private:
	static bool IsInitialized;
	static UAnimSequenceSelection* DefaultSetting;

public:
	UPROPERTY(EditAnywhere, Category=SequenceSelection)
	bool IsImportFromJson = false;

	UPROPERTY(EditAnywhere, Category=SequenceSelection, Meta=(EditCondition="IsImportFromJson", EditConditionHides))
	FFilePath Anim1pJsonPath;
	
	UPROPERTY(EditAnywhere, Category=SequenceSelection)
	TArray<UAnimSequence*> AnimationSequences;
	
	SWidget* m_ParentWidget;
};

class SAnimSequenceToolWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimSequenceToolWidget)
		{
		}

	SLATE_END_ARGS()

public:
	SAnimSequenceToolWidget()
	{
	}

	~SAnimSequenceToolWidget()
	{
	}

	void Construct(const FArguments& Args);

	template <class T>
	TSharedPtr<IDetailsView> CreateSettingView(FString Name, T* SettingViewObject);

private:
	static bool IsPropertyVisible(const FPropertyAndParent& PropertyAndParent, bool bInShouldShowNonEditable);

private:
	
	void Setup();

	TSharedRef<SWidget> Content();
	void ProcessAnimSequencesFilter();

	FReply OnSubmitMarkFootsteps();

	FReply OnSubmitExtractCurves();

	bool LoadFromAnim1pJson(const FFilePath& JsonPath);
	
private:
	
	UAnimCurveSettings* AnimCurveSetting;
	TSharedPtr<IDetailsView> AnimCurveSettingView;

	UFootstepSettings* FootstepSetting;
	TSharedPtr<IDetailsView> FootstepSettingView;

	UAnimSequenceSelection* SequenceSelection;
	TSharedPtr<IDetailsView> SequenceSelectionView;
	
public:

};
