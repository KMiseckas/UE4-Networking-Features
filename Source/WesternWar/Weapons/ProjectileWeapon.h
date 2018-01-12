// Copyright C++ Code by Klaudijus Miseckas for WesternWar project

#pragma once

#include "GameFramework/Actor.h"
#include "Interfaces/ItemInterface.h"
#include "ProjectileWeapon.generated.h"

USTRUCT()
struct FGunFireData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FVector ProjectileStart;
	UPROPERTY()
		FVector ProjectileDirection;
	UPROPERTY()
		bool bIsPlayerHit = false;
	UPROPERTY()
		int16 HitPlayerSimulationID = 0;
	UPROPERTY()
		int32 PlayerNetworkID = 0;
	UPROPERTY()
		int16 SimulationID = 0;
};

USTRUCT()
struct FServerGunData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		int16 ClipAmmo = 0;
	UPROPERTY()
		int16 SimulationID = 0;
};

UENUM(BlueprintType)
namespace EWeaponState
{
	enum Type
	{
		WP_Busy,
		WP_None,
	};
}


UCLASS()
class AProjectileWeapon : public AActor, public IItemInterface
{
	GENERATED_BODY()
	
private:
	void Fire();
	void Reload();
	void AimDownSightsToggle();
	void AimDownSightsHold();

	bool IsClipEmpty();
	bool IsCarryingAmmo();
	bool CanFire();

	int ClipAmmo = 0;

	bool bIsADS = false;
	bool bCanShoot = true;

	//Network related variables

	FGunFireData ClientFireData;

	//Networking Functions
	UFUNCTION(Server, Unreliable, WithValidation)
		void Server_SendGunFire(FGunFireData ClientFireData);
	UFUNCTION(NetMulticast, Unreliable, WithValidation)
		void MultiCastClient_ReplicateGunFireToClients();
	UFUNCTION(Client, Unreliable, WithValidation)
		void Client_CheckPlayerGunStats(FServerGunData ServerGunData);


public:	
	// Sets default values for this actor's properties
	AProjectileWeapon();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "WeaponMesh")
		USkeletalMeshComponent *WeaponMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		TEnumAsByte<EWeaponState::Type> WeaponState;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		FString WeaponName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		FString WeaponDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage")
		float HeadShotDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage")
		float BodyShotDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage")
		float ArmShotDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage")
		float LegShotDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Accurracy")
		float DefaultHipFireInaccurracyAngle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Accurracy")
		float DefaultADSInaccurracyAngle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Accurracy")
		float MovementInaccurracyModifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ammo")
		bool CanReloadSingleBullet = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ammo")
		int MaxClipAmmo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Movement")
		float MovementModifier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Movement")
		bool bCanSprint = true;

	//Interfaces

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Item Action")
		bool UseItem();
		virtual bool UseItem_Implementation() override;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Item Pick Up")
		bool PickUpItem();
		virtual bool PickUpItem_Implementation() override;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Item Throw")
		bool ThrowItem();
		virtual bool ThrowItem_Implementation() override;

};
