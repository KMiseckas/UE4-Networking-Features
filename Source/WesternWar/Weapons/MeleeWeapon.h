// Copyright C++ Code by Klaudijus Miseckas for WesternWar project

#pragma once

#include "GameFramework/Actor.h"
#include "MeleeWeapon.generated.h"

UCLASS()
class WESTERNWAR_API AMeleeWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMeleeWeapon();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	
	
};
