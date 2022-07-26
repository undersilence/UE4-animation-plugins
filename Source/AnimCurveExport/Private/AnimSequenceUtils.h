#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimSequence.h"
#include "Curves/CurveVector.h"

// Stateless Util Set
namespace AnimSequenceUtils
{
	
struct FFootstepMarker
{
	float Value; // height
	int Orientation; // -1: left, 1: right
};

bool SetVariableCurveHelper(UAnimSequence* Seq, const FString& CurveName, FFloatCurve& InFloatCurve);

UCurveVector* CreateCurveVectorAsset(const FString& PackagePath, const FString& CurveName);

bool GetBoneKeysByNameHelper(UAnimSequence* Seq, FString const& BoneName, TArray<FVector>& OutPosKey, TArray<FQuat>& OutRotKey, bool bConvertCS = false);

bool SetVariableCurveHelper(UAnimSequence* Seq, const FString& CurveName, FFloatCurve& InFloatCurve);

bool MarkFootstepsFor1PAnimation(UAnimSequence* Seq, FString const& KeyBone = "LeftHand", bool bDebug = false);

bool SaveBonesCurves(UAnimSequence* AnimSequence, FString const& BoneName, const FString& SavePath);

bool LoadAnimSequencesByNames(const TArray<FFilePath>& AnimSequencePaths);

}
