#pragma once
#include "CoreMinimal.h"
#include "AssetRegistryModule.h"
#include "Animation/AnimSequence.h"
#include "Curves/CurveVector.h"


struct FFootstepMarker
{
	float Value;     // height
	int Orientation; // -1: left, 1: right
};

// Stateless Util Set
class FAnimCurveUtils
{
public:
	template <typename T>
	static void GetObjectsOfClass(TArray<T*>& OutArray)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetData;
		AssetRegistryModule.Get().GetAssetsByClass(T::StaticClass()->GetFName(), AssetData);
		for (int i = 0; i < AssetData.Num(); i++)
		{
			T* Object = Cast<T>(AssetData[i].GetAsset());
			OutArray.Add(Object);
		}
	}

	static bool SetVariableCurveHelper(UAnimSequence* Seq, const FString& CurveName, FFloatCurve& InFloatCurve);

	static UCurveVector* CreateCurveVectorAsset(const FString& PackagePath, const FString& CurveName);

	static bool GetBoneKeysByNameHelper(UAnimSequence* Seq, FString const& BoneName, TArray<FVector>& OutPosKey, TArray<FQuat>& OutRotKey, bool bConvertCS = false);

	static bool MarkFootstepsFor1PAnimation(UAnimSequence* Seq, FString const& KeyBone = "LeftHand", bool bDebug = false);

	static bool SaveBonesCurves(UAnimSequence* AnimSequence, FString const& BoneName, const FString& SavePath);

	static bool LoadAnimSequencesByReference(const TArray<FString>& AnimSequencePaths, TArray<UAnimSequence*>& OutSequences);
};
