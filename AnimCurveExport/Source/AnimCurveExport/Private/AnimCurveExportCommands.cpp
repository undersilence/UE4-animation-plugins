// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "AnimCurveExportCommands.h"

#define LOCTEXT_NAMESPACE "FAnimCurveExportModule"

void FAnimCurveExportCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "AnimCurveExport", "Bring up AnimCurveExport window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
