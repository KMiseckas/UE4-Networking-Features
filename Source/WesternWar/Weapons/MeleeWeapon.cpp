// Copyright C++ Code by Klaudijus Miseckas for WesternWar project

#include "WesternWar.h"
#include "MeleeWeapon.h"


// Sets default values
AMeleeWeapon::AMeleeWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMeleeWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMeleeWeapon::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

