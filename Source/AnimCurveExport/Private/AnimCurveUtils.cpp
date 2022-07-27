﻿#include "AnimCurveUtils.h"

#include "AnimationBlueprintLibrary.h"
#include "AssetRegistryModule.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "PackageTools.h"
#include "Kismet2/BlueprintEditorUtils.h"


DEFINE_LOG_CATEGORY_STATIC(LogAnimCurveUtil, Log, All);


bool FAnimCurveUtils::SetVariableCurveHelper(
	UAnimSequence* Seq, const FString& CurveName, FFloatCurve& InFloatCurve)
{
	// Create Variable Curve
	USkeleton* Skeleton = Seq->GetSkeleton();
	FSmartName NewTrackName;
	// Add Skeleton Curve if not exists
	Skeleton->AddSmartNameAndModify(USkeleton::AnimCurveMappingName, *CurveName,
		NewTrackName);

	if (!Seq->RawCurveData.GetCurveData(NewTrackName.UID))
	{
		// Add Footsteps Track for animation sequence if not exists
		Seq->Modify(true);
		Seq->RawCurveData.AddCurveData(NewTrackName);
		Seq->MarkRawDataAsModified();
	}
	auto Curve = static_cast<FFloatCurve*>(Seq->RawCurveData.GetCurveData(NewTrackName.UID));
	Curve->FloatCurve = InFloatCurve.FloatCurve;
	return true;
}


bool FAnimCurveUtils::SaveBonesCurves(UAnimSequence* AnimSequence, FString const& BoneName, const FString& SaveDir)
{
	TArray<UPackage*> Packages;

	const auto AnimName = AnimSequence->GetName();
	const auto RawPackagePath = SaveDir + "/" + AnimName;
	// Initialize Bone Packages
	FString PackagePath, FailReason;
	if(!FPackageName::TryConvertFilenameToLongPackageName(RawPackagePath, PackagePath, &FailReason))
	{
		UE_LOG(LogAnimCurveUtil, Error, TEXT("[%s->%s]: Create packages error, %s."), *AnimName, *BoneName, *FailReason)
		return false;
	}
	const auto CurveNamePrefix = AnimName + "_" + BoneName;
	auto PosCurve = CreateCurveVectorAsset(PackagePath, CurveNamePrefix + "_Translation");
	auto RotCurve = CreateCurveVectorAsset(PackagePath, CurveNamePrefix + "_Rotation");

	Packages.Add(PosCurve->GetOutermost());
	Packages.Add(RotCurve->GetOutermost());

	TArray<FVector> PosKeys;
	TArray<FQuat> RotKeys;
	if (!GetBoneKeysByNameHelper(AnimSequence, BoneName, PosKeys, RotKeys, true))
	{
		UE_LOG(LogAnimCurveUtil, Error, TEXT("[%s->%s]: Bone not Exists!"), *AnimName, *BoneName);
		return false;
	}

	for (int i = 0; i < AnimSequence->GetRawNumberOfFrames(); ++i)
	{
		const auto Time = AnimSequence->GetTimeAtFrame(i);
		const auto Translation = PosKeys[i];
		const auto EulerAngle = RotKeys[i].Euler();

		PosCurve->FloatCurves[0].UpdateOrAddKey(Time, Translation.X);
		PosCurve->FloatCurves[1].UpdateOrAddKey(Time, Translation.Y);
		PosCurve->FloatCurves[2].UpdateOrAddKey(Time, Translation.Z);

		RotCurve->FloatCurves[0].UpdateOrAddKey(Time, EulerAngle.X, true);
		RotCurve->FloatCurves[1].UpdateOrAddKey(Time, EulerAngle.Y, true);
		RotCurve->FloatCurves[2].UpdateOrAddKey(Time, EulerAngle.Z, true);
	}
	// MarkFootstepsFor1PAnimation(AnimSequence);
	// Save Packages
	return UEditorLoadingAndSavingUtils::SavePackages(Packages, true);
}


bool FAnimCurveUtils::LoadAnimSequencesByReference(const TArray<FString>& AnimSequencePaths, TArray<UAnimSequence*>& OutSequences)
{
	for (auto& Path : AnimSequencePaths)
	{
		auto Seq = LoadObject<UAnimSequence>(nullptr, *Path);
		if (Seq)
		{
			OutSequences.AddUnique(Seq);
		}
		else
		{
			UE_LOG(LogAnimCurveUtil, Log, TEXT("Load Failed from %s: %s"), *Seq->GetName(), *Path);
		}
	}
	return true;
}

UCurveVector* FAnimCurveUtils::CreateCurveVectorAsset(const FString& PackagePath,
	const FString& CurveName)
{
	const auto PackageName =
		UPackageTools::SanitizePackageName(PackagePath + "/" + CurveName);
	const auto Package = CreatePackage(nullptr, *PackageName);
	EObjectFlags Flags = RF_Public | RF_Standalone | RF_Transactional;

	const auto NewObj =
		NewObject<UCurveVector>(Package, FName(*CurveName), Flags);
	if (NewObj)
	{
		FAssetRegistryModule::AssetCreated(NewObj);
		Package->MarkPackageDirty();
	}
	return NewObj;
}

bool FAnimCurveUtils::GetBoneKeysByNameHelper(
	UAnimSequence* Seq, FString const& BoneName, TArray<FVector>& OutPosKey,
	TArray<FQuat>& OutRotKey, bool bConvertCS /* = false */)
{
	auto Skeleton = Seq->GetSkeleton();
	auto RefSkeleton = Skeleton->GetReferenceSkeleton();
	auto BoneIndex = RefSkeleton.FindRawBoneIndex(*BoneName);
	if (BoneIndex == INDEX_NONE)
	{
		return false;
	}
	// In BoneSpace, need Convert to ComponentSpace
	auto NbrOfFrames = Seq->GetNumberOfFrames();
	OutPosKey.Init(FVector::ZeroVector, NbrOfFrames);
	OutRotKey.Init(FQuat::Identity, NbrOfFrames);
	auto BoneInfos = RefSkeleton.GetRefBoneInfo();

	TArray<FName> BoneTraces;
	do
	{
		auto _BoneName = RefSkeleton.GetBoneName(BoneIndex);
		BoneTraces.Add(_BoneName);
		BoneIndex = BoneInfos[BoneIndex].ParentIndex;
	} while (bConvertCS && BoneIndex);

	TArray<FTransform> Poses;
	for (int i = 0; i < Seq->GetNumberOfFrames(); ++i)
	{
		UAnimationBlueprintLibrary::GetBonePosesForFrame(Seq, BoneTraces, i, false, Poses);
		FTransform FinalTransform = FTransform::Identity;
		for (int j = 0; j < BoneTraces.Num(); ++j)
		{
			FinalTransform = FinalTransform * Poses[j];
		}

		OutPosKey[i] = FinalTransform.GetLocation();
		OutRotKey[i] = FinalTransform.GetRotation();
	}

	/* NOT USE ANYMORE
	do
	{
		auto _BoneName = RefSkeleton.GetBoneName(BoneIndex);
		UE_LOG(LogAnimCurveUtil, Log, TEXT("Current Bone Hierarchy %d, %s"), BoneIndex, *_BoneName.ToString());

		auto TrackIndex = AnimTrackNames.IndexOfByKey(_BoneName); // Correspond BoneIndex in AnimTracks, Find by Unique Name
		auto Track = Seq->GetRawAnimationTrack(TrackIndex);
		auto CurrPosKey = Track.PosKeys;
		auto CurrRotKey = Track.RotKeys;
		uint32 x = 0, IncrementPos = CurrPosKey.Num() != 1;
		uint32 y = 0, IncrementRot = CurrRotKey.Num() != 1;
		// FAnimationRuntime::GetComponentSpaceTransformRefPose();
		for (int i = 0; i < OutPosKey.Num(); ++i)
		{
			// UE_LOG(LogAnimCurveUtil, Log, TEXT("BoneIndex: %d, Get Relative Z value %f."),
			// BoneIndex, CurrPosKey[x].Z);
			OutPosKey[i] += CurrPosKey[x];
			OutRotKey[i] *= CurrRotKey[y];
			x += IncrementPos;
			y += IncrementRot;
		}
		BoneIndex = BoneInfos[BoneIndex].ParentIndex;
	} while (bConvertCS && BoneIndex);
	*/

	return true;
}

bool FAnimCurveUtils::MarkFootstepsFor1PAnimation(
	UAnimSequence* Seq, FString const& KeyBone /* = "LeftHand" */, bool bDebug /* = false */)
{
	FFloatCurve FootstepsCurve, PosXCurve, PosZCurve, RotYCurve;
	// Get KeyBone translation and rotation keys
	TArray<FVector> PosKeys;
	TArray<FQuat> RotKeys;
	if (!GetBoneKeysByNameHelper(Seq, KeyBone, PosKeys, RotKeys, true))
	{
		UE_LOG(LogAnimCurveUtil, Warning, TEXT("[%s] BoneName (%s) not exist, or Curves can not be extracted"), *Seq->GetName(), *KeyBone);
		return false;
	}
	// Total Nums of the Key
	// Strategy: Capture the local Z minimal point in LeftHand
	auto N = PosKeys.Num();
	auto M = RotKeys.Num();
	auto FrameCount = Seq->GetNumberOfFrames();

	FVector RotAvg(0.0f), PosAvg(0.0f);

	TArray<FVector> PosCache, RotCache;
	uint32 x = 0, PosIncrement = N == 1 ? 0 : 1;
	uint32 y = 0, RotIncrement = M == 1 ? 0 : 1;
	for (int i = 0; i < FrameCount; ++i)
	{
		auto Time = Seq->GetTimeAtFrame(i);

		FVector CurrPos = PosKeys[x];
		FVector CurrRot = RotKeys[y].Euler();

		// Coordinates Transform
		Swap(CurrPos.Y, CurrPos.Z);
		CurrPos.Z = -CurrPos.Z;

		x += PosIncrement;
		y += RotIncrement;

		PosCache.Add(CurrPos);
		RotCache.Add(CurrRot);
		PosAvg += CurrPos;
		RotAvg += CurrRot;

		PosXCurve.FloatCurve.UpdateOrAddKey(Time, CurrPos.X);
		PosZCurve.FloatCurve.UpdateOrAddKey(Time, CurrPos.Z);
		RotYCurve.FloatCurve.UpdateOrAddKey(Time, CurrRot.Y, true);
	}

	if (bDebug)
	{
		RotAvg /= Seq->GetNumberOfFrames();
		PosAvg /= Seq->GetNumberOfFrames();
		UE_LOG(LogAnimCurveUtil, Log, TEXT("key counts Pos: %d, Rot: %d"), N, M);
		UE_LOG(LogAnimCurveUtil, Log, TEXT("Position Average: %f, Rotation Average: %f"),
			PosAvg.X, RotAvg.Y);
	}

	FVector PrevPos, NextPos, CurrPos;
	FVector PrevRot, NextRot, CurrRot;

	TArray<FFootstepMarker> LocalMinims;
	TArray<int32> MinimalKeys;
	TArray<int32> Permutations;

	// Ignore end frame, cause frame_start == frame_end
	PosCache.Pop();
	RotCache.Pop();
	float Eps = 1e-6;
	N = PosCache.Num(), M = RotCache.Num();
	for (int i = 0; i < N; ++i)
	{
		PrevPos = PosCache[(i - 1 + N) % N];
		CurrPos = PosCache[i];
		NextPos = PosCache[(i + 1) % N];

		if (CurrPos.Z + Eps <= PrevPos.Z && CurrPos.Z + Eps <= NextPos.Z)
		{
			Permutations.Add(LocalMinims.Num());
			MinimalKeys.Add(i);
			LocalMinims.Push(FFootstepMarker{CurrPos.Z, (PrevPos.X > CurrPos.X) * 2 - 1});
		}
	}

	Permutations.Sort([&](int i, int j)
	{
		return LocalMinims[i].Value < LocalMinims[j].Value;
	});

	// constexpr int DesiredFootstepsCount = 2;
	// int FootstepCount = LocalMinims.Num();

	if (LocalMinims.Num() == 2)
	{
		// Normal cases;
		auto I = Permutations[0], J = Permutations[1];
		FootstepsCurve.UpdateOrAddKey(-LocalMinims[I].Orientation, Seq->GetTimeAtFrame(MinimalKeys[I]) - 0.001);
		FootstepsCurve.UpdateOrAddKey(LocalMinims[I].Orientation, Seq->GetTimeAtFrame(MinimalKeys[I]));
		FootstepsCurve.UpdateOrAddKey(-LocalMinims[J].Orientation, Seq->GetTimeAtFrame(MinimalKeys[J]) - 0.001);
		FootstepsCurve.UpdateOrAddKey(LocalMinims[J].Orientation, Seq->GetTimeAtFrame(MinimalKeys[J]));
		
		if (LocalMinims[I].Orientation == LocalMinims[J].Orientation)
		{
			UE_LOG(LogAnimCurveUtil, Warning, TEXT("[%s->%s] Footstep Marks on the same side, please check it manually."), *Seq->GetName(), *KeyBone);
			return false;
		}
	}
	else if (LocalMinims.Num() > 2)
	{
		// Check Potential Loops
		bool bLoop = true;
		for (int i = 0; i < LocalMinims.Num(); ++i)
		{
			auto I = Permutations[i], J = Permutations[(i + 2) % LocalMinims.Num()];
			if (abs(LocalMinims[I].Value - LocalMinims[J].Value) > Eps)
			{
				bLoop = false;
				break;
			}
		}
		if (bLoop)
		{
			UE_LOG(LogAnimCurveUtil, Warning, TEXT("[%s] Footstep Marks may contain potential loops."), *Seq->GetName());
			return false;
		}
		else
		{
			UE_LOG(LogAnimCurveUtil, Warning, TEXT("[%s] mark footstep failed when target Curves.Pos.Z (of %s) contains more than 2 local minimas"), *Seq->GetName(), *KeyBone);
			return false;
		}
	}
	else if (LocalMinims.Num() == 1)
	{
		auto I = Permutations[0];
		FootstepsCurve.UpdateOrAddKey(-LocalMinims[I].Orientation, Seq->GetTimeAtFrame(MinimalKeys[I]) - 0.001);
		FootstepsCurve.UpdateOrAddKey(LocalMinims[I].Orientation, Seq->GetTimeAtFrame(MinimalKeys[I]));
		UE_LOG(LogAnimCurveUtil, Warning, TEXT("[%s] has only one footstep mark, please check it manually."), *Seq->GetName());
	}
	else
	{
		UE_LOG(LogAnimCurveUtil, Warning, TEXT("[%s] no footstep detected, skip."), *Seq->GetName());
		return true;
	}

	// for (auto It = FootstepsCurve.FloatCurve.GetKeyHandleIterator(); It; ++It)
	// {
	// 	const FKeyHandle KeyHandle = *It;
	// 	FootstepsCurve.FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Constant);
	// }

	SetVariableCurveHelper(Seq, "Footsteps_Curve", FootstepsCurve);
	if (bDebug)
	{
		SetVariableCurveHelper(Seq, FString::Printf(TEXT("%s_PosX_Curve"), *KeyBone), PosXCurve);
		SetVariableCurveHelper(Seq, FString::Printf(TEXT("%s_PosZ_Curve"), *KeyBone), PosZCurve);
		SetVariableCurveHelper(Seq, FString::Printf(TEXT("%s_RotY_Curve"), *KeyBone), RotYCurve);
	}
	return true;
}