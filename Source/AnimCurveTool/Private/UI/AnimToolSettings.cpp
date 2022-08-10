
#include "AnimToolSettings.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// UAnimCurveSettings | Anim Curve settings
////////////////////////////////////////////////////////////////////////////////////////////////////

bool UAnimCurveSettings::IsInitialized = false;
UAnimCurveSettings* UAnimCurveSettings::DefaultSetting = nullptr;

void UAnimCurveSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool UFootstepSettings::IsInitialized = false;
UFootstepSettings* UFootstepSettings::DefaultSetting = nullptr;

void UFootstepSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool UAnimSequenceSelection::IsInitialized = false;
UAnimSequenceSelection* UAnimSequenceSelection::DefaultSetting = nullptr;

void UAnimSequenceSelection::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool UAnimJsonSettings::IsInitialized = false;
UAnimJsonSettings* UAnimJsonSettings::DefaultSetting = nullptr;

void UAnimJsonSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool UAnimCheckSettings::IsInitialized = false;
UAnimCheckSettings* UAnimCheckSettings::DefaultSetting = nullptr;

void UAnimCheckSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
}