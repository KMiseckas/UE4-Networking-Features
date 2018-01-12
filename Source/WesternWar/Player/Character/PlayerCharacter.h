// Copyright C++ Code by Klaudijus Miseckas for WesternWar project

#pragma once

#include "GameFramework/Pawn.h"
#include "CharacterMovementComp.h"
#include "PlayerCharacter.generated.h"

USTRUCT()
struct FClientCharacterData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		float VerticalInput;
	UPROPERTY()
		float HorizontalInput;
	UPROPERTY()
		float UpInput;
	UPROPERTY()
		float VerticalLookInput;
	UPROPERTY()
		float HorizontalLookInput;

	UPROPERTY()
		FVector Location;
	UPROPERTY()
		FRotator Rotation;

	UPROPERTY()
		float DeltaTime;
	UPROPERTY()
		int16 SimulationID;

};

USTRUCT()
struct FServerCharacterData
{
	//Contains server data for the simulated character from local client input

	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FVector Location;
	UPROPERTY()
		FRotator Rotation;
	UPROPERTY()
		float HorizontalCharacterTurnVal = 0;
	UPROPERTY()
		float ServerTime;

	UPROPERTY()
		int16 SimulationID;
};

USTRUCT()
struct FInterpolationData
{
	//Contains server data for the simulated character from local client input thats required for interpolation on clients

	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FVector Location;
	UPROPERTY()
		FRotator Rotation;
	UPROPERTY()
		float ServerTime;
	UPROPERTY()
		bool bIsEmpty = true;

	static FInterpolationData Lerp(FInterpolationData FromData, FInterpolationData ToData, float Step)
	{
		FInterpolationData Result;

		Result.Location = FMath::Lerp(FromData.Location, ToData.Location, Step);
		Result.Rotation = FMath::Lerp(FromData.Rotation, ToData.Rotation, Step);

		return Result;
	}
};

UCLASS()
class WESTERNWAR_API APlayerCharacter : public APawn
{
	GENERATED_BODY()
private:
	void VerticalMovementInput(float val);
	void HorizontalMovementInput(float val);
	void UpMovementInput(float val);
	void VerticalLookInput(float val);
	void HorizontalLookInput(float val);

	//Networking Debugs
	void DisplayClientPredictionsHistory();
	void DisplayServerSimulationsHistory();
	void DisplayFixedPredictionsHistory(FVector Location);
	void DisplayWrongPredictionMoment(FVector Location);

	FVector Debug_LastPredictionLocation;
	FVector Debug_LastSimulatedLocation;
	FVector Debug_LastFixedLocation;

	//Character Data 
	FClientCharacterData Client_CharacterData;
	FClientCharacterData Server_CharacterData;
	FServerCharacterData CharacterSimulatedData;
	int16 SimulationID = 0;

	bool bIsRewinding = false;
	bool bCanInterpolateData = false;
	bool bIsFirstTimeInterpolation = true;

	int InterpolationDataReceived = 0;
	int ServerSimulationSteps = 0;

	TQueue<FClientCharacterData, EQueueMode::Mpsc> Client_CharacterInputDataQueue;
	TQueue<FInterpolationData, EQueueMode::Mpsc> InterpolationDataQueue;

	FInterpolationData TargetInterpolationData;
	FInterpolationData PreviousInterpolationData;
	bool bCanStartNewInterpolationSet = true;
	float Step = 0;

	void MoveCharacter(bool bIsServerSide, FClientCharacterData CharacterData);
	void InterpolateMovementData();
	void AddInterpolationData(FServerCharacterData ServerData);
	void RotateCamera();

	bool IsPlayerGrounded();
	bool CanJump = true;
	float JumpTimer = 0;

	void CompareServerToClientSimulationResults();
	void RewindAndReplay();

	FVector GetMoveDirection(FClientCharacterData CharacterData);
	void SetLookRotation(FClientCharacterData CharacterData);

	FVector MoveDirection = FVector::ZeroVector;

	float HorizontalPlayerTurnVal = 0;	//Stores the value by which the camera is rotated horizontally (around y axis)
	float VerticalCameraTurnVal = 0;	//Stores the value by which the camera is rotated vertically (around x axis)

	//Lag Compensation
	TMap<int16, FServerCharacterData> Server_CharacterDataHistory;
	bool RewindServerCharacterLocation(int16 SimulationID);
	void CheckForProjectileImpact(FVector ProjectileStart, FVector ProjectileDirection);

	FRotator PreviousRotation_LC;
	FVector PreviousLocation_LC;

	//Client Prediction Vars
	float MaxLocationErrorMargin = 0.1f;
	float MaxRotationErrorMargin = 5;

	//Networking functions
	UFUNCTION(Server, Unreliable, WithValidation)
		void Server_SendClientCharacterData(FClientCharacterData Client_CharacterData);
	UFUNCTION(NetMulticast, Unreliable, WithValidation)
		void MultiCastClient_ReplicatePawnToClients(FServerCharacterData Server_CharacterData);


public:
	// Sets default values for this pawn's properties
	APlayerCharacter();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "RootMesh")
		UStaticMeshComponent *RootMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MovementComponent")
		UCharacterMovementComp *MovementComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "CameraComponent")
		UCameraComponent *CharacterCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "UpdatedMoveComponentMesh")
		UStaticMeshComponent *PlayerMainCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control Properties|Movement")
		float VerticalMovementSpeed = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control Properties|Movement")
		float HorizontalMovementSpeed = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control Properties|Movement")
		float UpMovementSpeed = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control Properties|FPS Camera")
		float VerticalLookSensitivity = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control Properties|FPS Camera")
		float HorizontalLookSensitivity = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control Properties|FPS Camera")
		float DefaultCameraFOV = 80;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control Properties|FPS Camera")
		float MaxAngle = 80;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control Properties|FPS Camera")
		float MinAngle = -80;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control Properties|Fake Physics")
		float Gravity = 9.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control Properties|Interpolation")
		bool bEnableEntityInterpolation = true;

	//Debug
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Options")
		bool bEnableDebug = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Options|Networking|Prediction & Recoincilation")
		bool bEnablePredictionHistory = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Options|Networking|Prediction & Recoincilation")
		bool bEnableServerSimulationHistory = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Options|Networking|Prediction & Recoincilation")
		bool bEnableFixedPredictionHistory = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Options|Networking|Entity Interpolation")
		bool bEnableInterpolationTargets = true;

};

static FORCEINLINE bool Raycast(
	UWorld* World,
	AActor* ActorToIgnore,
	const FVector& Start,
	const FVector& End,
	FHitResult& HitOut,
	ECollisionChannel CollisionChannel = ECC_Pawn,
	bool ReturnPhysMat = false
	) 
{
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams TraceParams(FName(TEXT("Hover Trace")), true, ActorToIgnore);
	TraceParams.bTraceComplex = true;
	//TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = ReturnPhysMat;

	//Ignore Actors
	TraceParams.AddIgnoredActor(ActorToIgnore);

	//Re-initialize hit info
	HitOut = FHitResult(ForceInit);

	//Trace!
	World->LineTraceSingleByChannel(
		HitOut,	//result
		Start, //start
		End, //end
		CollisionChannel, //collision channel
		TraceParams
		);

	//Hit any Actor?
	return (HitOut.GetActor() != NULL);
}
