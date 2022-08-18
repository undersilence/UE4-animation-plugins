// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "AnimBPToolStyle.h"

class FAnimBPToolCommands : public TCommands<FAnimBPToolCommands>
{
public:

	FAnimBPToolCommands()
		: TCommands<FAnimBPToolCommands>(TEXT("AnimBPTool"), NSLOCTEXT("Contexts", "AnimBPTool", "AnimBPTool Plugin"), NAME_None, FAnimBPToolStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	// TSharedPtr< FUICommandInfo > PluginAction;
	TSharedPtr< FUICommandInfo > HidePins;
};
