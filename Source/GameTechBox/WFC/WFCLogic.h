#pragma once

#include "CoreMinimal.h"

struct FTileRule
{
	int32 TileID;
	int32 EdgesIDs[4]; // 0: +X, 1: +Y, 2: -X, 3: -Y;
};

struct FWFCNode
{
	uint64 Possibilities; // Bitmask; 1: Possible, 0: Pruned
	bool bCollapsed = false;
	int32 FinalTileIndex = -1;

	int32 GetEntropy() const { return FMath::CountBits(Possibilities); }
};