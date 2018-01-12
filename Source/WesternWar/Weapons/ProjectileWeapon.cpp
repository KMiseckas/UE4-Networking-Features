// Copyright C++ Code by Klaudijus Miseckas for WesternWar project

#include "WesternWar.h"
#include "ProjectileWeapon.h"


// Sets default values
AProjectileWeapon::AProjectileWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon Skeletal Mesh"));

}

// Called when the game starts or when spawned
void AProjectileWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AProjectileWeapon::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

void AProjectileWeapon::Fire()
{

}

void AProjectileWeapon::Reload()
{

}

void AProjectileWeapon::AimDownSightsToggle()
{

}

void AProjectileWeapon::AimDownSightsHold()
{

}

bool AProjectileWeapon::IsClipEmpty()
{
	return false;
}

bool AProjectileWeapon::IsCarryingAmmo()
{
	return false;
}

bool AProjectileWeapon::CanFire()
{
	return false;
}

//INTERFACE FUNCTIONS

bool AProjectileWeapon::UseItem_Implementation()
{
	return true;
}

bool AProjectileWeapon::PickUpItem_Implementation()
{
	return true;
}

bool AProjectileWeapon::ThrowItem_Implementation()
{
	return true;
}


//NETWORKING FUNCTIONS

bool AProjectileWeapon::Server_SendGunFire_Validate(FGunFireData ClientFireData)
{
	if (ClipAmmo <= 0)
	{
		return false;
	}

	return true;
}

void AProjectileWeapon::Server_SendGunFire_Implementation(FGunFireData ClientFireData)
{

}

bool AProjectileWeapon::MultiCastClient_ReplicateGunFireToClients_Validate()
{
	return true;
}

void AProjectileWeapon::MultiCastClient_ReplicateGunFireToClients_Implementation()
{

}

bool AProjectileWeapon::Client_CheckPlayerGunStats_Validate(FServerGunData ServerGunData)
{
	return true;
}

void AProjectileWeapon::Client_CheckPlayerGunStats_Implementation(FServerGunData ServerGunData)
{

}