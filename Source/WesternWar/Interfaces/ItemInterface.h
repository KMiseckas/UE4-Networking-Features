// Copyright C++ Code by Klaudijus Miseckas for WesternWar project

#pragma once

#include "ItemInterface.generated.h"

UINTERFACE(BlueprintType)
class UItemInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class IItemInterface
{

	GENERATED_IINTERFACE_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item Action")
		bool UseItem();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item Pick Up")
		bool PickUpItem();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item Throw")
		bool ThrowItem();
};
