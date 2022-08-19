// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "AnimBPToolCommands.h"

#define LOCTEXT_NAMESPACE "FAnimBPToolModule"

void FAnimBPToolCommands::RegisterCommands()
{
	// UI_COMMAND(PluginAction, "AnimBPTool", "Execute AnimBPTool action", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(HidePins, "HideUnconnectedPins", "Hide All Unconnected Pins for K2Node_BreakStruct", EUserInterfaceActionType::Button, FInputChord())
}

#undef LOCTEXT_NAMESPACE
