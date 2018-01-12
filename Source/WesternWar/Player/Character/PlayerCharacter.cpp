// Copyright C++ Code by Klaudijus Miseckas for WesternWar project

#include "WesternWar.h"
#include "PlayerCharacter.h"


// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Initialise a new root component for the character
	RootMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RootMeshComponent"));
	SetRootComponent(Cast<USceneComponent>(RootMesh));

	//Initialise collision mesh for player
	PlayerMainCollision = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshCollisionMain"));
	PlayerMainCollision->AttachToComponent(RootMesh, FAttachmentTransformRules::KeepRelativeTransform);

	//Initialise a movement component for the character
	MovementComponent = CreateDefaultSubobject<UCharacterMovementComp>(TEXT("PawnMovementComp"));
	MovementComponent->UpdatedComponent = RootMesh;

	//Initialise a camera component for the character
	CharacterCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CharacterCamera->AttachToComponent(PlayerMainCollision, FAttachmentTransformRules::KeepRelativeTransform);

}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	//If this is the local client store character data
	if (Role == ROLE_AutonomousProxy)
	{
		Client_CharacterData.Location = GetActorLocation();
		Client_CharacterData.Rotation = GetActorRotation();
		Client_CharacterData.SimulationID = 0;
		Client_CharacterData.DeltaTime = GetWorld()->DeltaTimeSeconds;

		Client_CharacterInputDataQueue.Enqueue(Client_CharacterData);
	}
	//If this is the server, disable ticking on this class
	else if(Role == ROLE_Authority)
	{
		PrimaryActorTick.bCanEverTick = false;
	}

}

// Called every frame
void APlayerCharacter::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	//If local client
	if (Role == ROLE_AutonomousProxy && !bIsRewinding)
	{
		RotateCamera();
		MoveCharacter(false, Client_CharacterData);

		//Store current frame input data locally in a queue
		Client_CharacterInputDataQueue.Enqueue(Client_CharacterData);
		//Send current frame input data to the server
		Server_SendClientCharacterData(Client_CharacterData);

		//GEngine->AddOnScreenDebugMessage(-1, -1, FColor::Green, "Called Tick Local");

		//DEBUG ONLY
		if (bEnableDebug)
		{
			if (bEnablePredictionHistory)
			{
				DisplayClientPredictionsHistory();
			}

			if (bEnableServerSimulationHistory)
			{
				DisplayServerSimulationsHistory();
			}
		}
	}

	//If non-local client & interpolation is enabled
	if (Role == ROLE_SimulatedProxy && bCanInterpolateData)
	{
		InterpolateMovementData();
		//GEngine->AddOnScreenDebugMessage(-1, -1, FColor::Red, "Interpolation Running");
	}

}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);

	InputComponent->BindAxis("VerticalMovement", this, &APlayerCharacter::VerticalMovementInput);
	InputComponent->BindAxis("HorizontalMovement", this, &APlayerCharacter::HorizontalMovementInput);
	InputComponent->BindAxis("UpMovement", this, &APlayerCharacter::UpMovementInput);
	InputComponent->BindAxis("VerticalLook", this, &APlayerCharacter::VerticalLookInput);
	InputComponent->BindAxis("HorizontalLook", this, &APlayerCharacter::HorizontalLookInput);

}

//Forward & Backward movement of the character
void APlayerCharacter::VerticalMovementInput(float val)
{
	Client_CharacterData.VerticalInput = val;
}

//Right & Left movement of the character
void APlayerCharacter::HorizontalMovementInput(float val)
{
	Client_CharacterData.HorizontalInput = val;
}

//Up (Jump) movement of the character
void APlayerCharacter::UpMovementInput(float val)
{
	Client_CharacterData.UpInput = val;
}


//Up & Down camera movement
void APlayerCharacter::VerticalLookInput(float val)
{
	Client_CharacterData.VerticalLookInput = val;
}

//Sideways camera movement
void APlayerCharacter::HorizontalLookInput(float val)
{
	Client_CharacterData.HorizontalLookInput = val;
}

/*
* Move the character - server or client side
* This is a shared function between the server and client
* to make sure the movement simulations are the same on the
* the server as on the client
*/
void APlayerCharacter::MoveCharacter(bool bIsServerSide, FClientCharacterData CharacterData)
{
	//Time passed between the last and current frame
	Client_CharacterData.DeltaTime = GetWorld()->DeltaTimeSeconds;
	float DeltaSeconds = CharacterData.DeltaTime;

	FHitResult Hit;

	//Get the direction of the movement, which is based on the user input
	FVector MoveDirectionVector = GetMoveDirection(CharacterData) * DeltaSeconds;

	//Set rotation of the character before moving
	SetLookRotation(CharacterData);	

	//Move the character
	MovementComponent->SafeMoveUpdatedComponent(MoveDirectionVector,GetActorRotation(),true, Hit, ETeleportType::None);
	MovementComponent->SlideAlongSurface(MoveDirectionVector, 1.0f, Hit.Normal, Hit, false);

	if (!bIsServerSide)
	{
		//Increment the simulation ID, if too large, reset to 0
		if (SimulationID >= 400)
		{
			SimulationID = 0;
		}
		SimulationID++;

		Client_CharacterData.Location = GetActorLocation();
		Client_CharacterData.Rotation = GetActorRotation();
		Client_CharacterData.SimulationID = SimulationID;
	}
	else
	{

		//GEngine->AddOnScreenDebugMessage(-1, -1, FColor::Green, "Ex-Client Data | Sending | Actor Label = " + GetActorLabel());

		//Store all essential data for character replication on other clients
		CharacterSimulatedData.Location = GetActorLocation();
		CharacterSimulatedData.Rotation = GetActorRotation();
		CharacterSimulatedData.SimulationID = Server_CharacterData.SimulationID;
		CharacterSimulatedData.HorizontalCharacterTurnVal = HorizontalPlayerTurnVal;
		CharacterSimulatedData.ServerTime = GetWorld()->RealTimeSeconds;

		//Add the character simulated data to the server history (for Lag Compensation)
		Server_CharacterDataHistory.Add(CharacterSimulatedData.SimulationID, CharacterSimulatedData);

		ServerSimulationSteps++;

		/*
		* Had a bug where server called this function twice in a very short time
		* - May be to do with that I call this function from the client directly at every packet 
		*   the server receives 
		*/
		if (ServerSimulationSteps == FMath::Round(60/NetUpdateFrequency))
		{
			ServerSimulationSteps = 0;
			MultiCastClient_ReplicatePawnToClients(CharacterSimulatedData);
		}
	}

}

/*
* Rotate Camera - Rotates the character attached camera
* Rotates the camera around the x axis
*/
void APlayerCharacter::RotateCamera()
{
	VerticalCameraTurnVal += Client_CharacterData.VerticalLookInput;

	//Clamp the max and min angle for the camera look
	VerticalCameraTurnVal = FMath::Clamp(VerticalCameraTurnVal, MinAngle, MaxAngle);
	CharacterCamera->SetRelativeRotation(FRotator(VerticalCameraTurnVal, 0, 0));
}

//Once a packet is received from the server, data for interpolation is added & queued
void APlayerCharacter::AddInterpolationData(FServerCharacterData ServerData)
{
	FInterpolationData TempInterData;
	TempInterData.Location = ServerData.Location;
	TempInterData.Rotation = ServerData.Rotation;
	TempInterData.ServerTime = ServerData.ServerTime;

	InterpolationDataQueue.Enqueue(TempInterData);

	InterpolationDataReceived++;

	//Used for the first time a player joins, 2 queued data sets are required for interpolation to work
	if (InterpolationDataReceived >= 2)
	{
		bCanInterpolateData = true;
	}

	//GEngine->AddOnScreenDebugMessage(-1, 0.2f, FColor::Red, "Added Data");
}

/*
* Interpolate between received locations and positions from the server simulations
* - Fixes jitter that is caused by low frequency network updates
*/
void APlayerCharacter::InterpolateMovementData()
{
	if (bCanStartNewInterpolationSet)
	{
		if (!bIsFirstTimeInterpolation)
		{
			PreviousInterpolationData = TargetInterpolationData;
		}
		else
		{
			InterpolationDataQueue.Dequeue(PreviousInterpolationData);
			bIsFirstTimeInterpolation = false;
		}

		InterpolationDataQueue.Dequeue(TargetInterpolationData);
		bCanStartNewInterpolationSet = false;
		Step = 0;

		if (bEnableInterpolationTargets && bEnableDebug)
		{
			DrawDebugSphere(GetWorld(), PreviousInterpolationData.Location, 15, 16, FColor().Green, false, 1/NetUpdateFrequency, 2);
			DrawDebugSphere(GetWorld(), TargetInterpolationData.Location, 15, 16, FColor().Red, false, 1/NetUpdateFrequency * 2, 2);
		}
	}

	if (!bCanStartNewInterpolationSet)
	{
		Step += GetWorld()->DeltaTimeSeconds*NetUpdateFrequency;

		if (Step > 1)
		{
			Step = 1;
			bCanStartNewInterpolationSet = true;
		}

		FInterpolationData InterpolatedData = FInterpolationData::Lerp(PreviousInterpolationData, TargetInterpolationData, Step);
		SetActorLocation(InterpolatedData.Location);
		SetActorRotation(InterpolatedData.Rotation);

		//GEngine->AddOnScreenDebugMessage(-1, 0.05f, FColor::Green, "Step - " + FString::SanitizeFloat(Step) + "Delta: " + FString::SanitizeFloat(GetWorld()->DeltaTimeSeconds) + "Time Diff: " + FString::SanitizeFloat(FMath::Abs(PreviousInterpolationData.ServerTime - TargetInterpolationData.ServerTime)));
	}

}

void APlayerCharacter::CompareServerToClientSimulationResults()
{
	FClientCharacterData CharacterData;
	Client_CharacterInputDataQueue.Dequeue(CharacterData);

	while (CharacterData.SimulationID != CharacterSimulatedData.SimulationID)
	{
		Client_CharacterInputDataQueue.Dequeue(CharacterData);
	}

	//Check distance between server and client character location, if difference is too large rewind and replay the simulation on local client
	// - We assume server is always correct 
	if (FVector::Dist(CharacterData.Location, CharacterSimulatedData.Location) > MaxLocationErrorMargin)
	{
		DisplayWrongPredictionMoment(CharacterSimulatedData.Location);
		RewindAndReplay();
		//GEngine->AddOnScreenDebugMessage(-1, 0.2f, FColor::Red, "Location Wrong");
	}

	float TempClientRot = CharacterData.Rotation.Yaw;
	float TempServerRot = CharacterSimulatedData.Rotation.Yaw;

	if (TempClientRot < 0)
	{
		TempClientRot = 360 + TempClientRot;
	}

	if (TempServerRot < 0)
	{
		TempServerRot = 360 + TempServerRot;
	}

	if (FMath::Abs(TempClientRot - TempServerRot) >= MaxRotationErrorMargin)
	{
		DisplayWrongPredictionMoment(CharacterSimulatedData.Location);
		RewindAndReplay();
		//GEngine->AddOnScreenDebugMessage(-1, 0.2f, FColor::Red, "Rotation Wrong");
		//GEngine->AddOnScreenDebugMessage(-1, 0.2f, FColor::Blue, "Rotation Wrong - Client -" + FString::SanitizeFloat(TempClientRot));
		//GEngine->AddOnScreenDebugMessage(-1, 0.2f, FColor::Green, "Rotation Wrong - Server - " + FString::SanitizeFloat(TempServerRot));
	}
}

void APlayerCharacter::RewindAndReplay()
{
	bIsRewinding = true;

	TQueue<FClientCharacterData, EQueueMode::Mpsc> TempClientCharacterDataQueue;
	FClientCharacterData CharacterData;

	FClientCharacterData TempData;
	FVector PreviousLocation = FVector::ZeroVector;
	FRotator PreviousRotation = FRotator::ZeroRotator;

	SetActorLocation(CharacterSimulatedData.Location);
	SetActorRotation(CharacterSimulatedData.Rotation);
	HorizontalPlayerTurnVal = CharacterSimulatedData.HorizontalCharacterTurnVal;

	while (Client_CharacterInputDataQueue.Dequeue(CharacterData))
	{

		TempData = CharacterData;
		if (PreviousLocation == FVector::ZeroVector)
		{
			TempData.Location = CharacterSimulatedData.Location;
			TempData.Rotation = CharacterSimulatedData.Rotation;
		}
		else
		{
			TempData.Location = PreviousLocation;
			TempData.Rotation = PreviousRotation;
		}

		FHitResult Hit;
		FVector MoveVector = GetMoveDirection(TempData) * TempData.DeltaTime;

		SetLookRotation(TempData);
		MovementComponent->SafeMoveUpdatedComponent(MoveVector, GetActorRotation(), true, Hit, ETeleportType::None);
		
		CharacterData.Location = GetActorLocation();
		CharacterData.Rotation = GetActorRotation();

		TempClientCharacterDataQueue.Enqueue(CharacterData);

		PreviousLocation = CharacterData.Location;
		PreviousRotation = CharacterData.Rotation;

		if (bEnableFixedPredictionHistory)
		{
			DisplayFixedPredictionsHistory(CharacterData.Location);
		}
	}

	Debug_LastFixedLocation = FVector::ZeroVector;

	Client_CharacterInputDataQueue.Empty();

	while (TempClientCharacterDataQueue.Dequeue(CharacterData))
	{
		Client_CharacterInputDataQueue.Enqueue(CharacterData);
	}

	bIsRewinding = false;

}

//Get the direction the player should move in
FVector APlayerCharacter::GetMoveDirection(FClientCharacterData CharacterData)
{
	float GravityForce = Gravity;

	if (IsPlayerGrounded())
	{
		if (!CanJump)
		{
			JumpTimer += CharacterData.DeltaTime;
			if (JumpTimer > 0.1f)
			{
				CanJump = true;
				JumpTimer = 0;
			}
		}

		FVector ForwardVector = CharacterData.VerticalInput * GetActorForwardVector() * VerticalMovementSpeed;
		FVector RightVector = CharacterData.HorizontalInput * GetActorRightVector() * HorizontalMovementSpeed;

		MoveDirection = ForwardVector + RightVector;
		MoveDirection.Z = 0;

		if (CharacterData.UpInput == 1 && CanJump)
		{
			//FVector UpVector = GetActorUpVector() * UpMovementSpeed;
			MoveDirection.Z = UpMovementSpeed;
			MoveDirection.X /= 1.5f;
			MoveDirection.Y /= 1.5f;

			CanJump = false;
		}

		GravityForce = 0;
	}
	else
	{
		MoveDirection.Z -= GravityForce * CharacterData.DeltaTime;
	}


	//GEngine->AddOnScreenDebugMessage(-1, -1, FColor::Red,"Gravity Force = " + FString::SanitizeFloat(AcquiredGravity));

	return MoveDirection;
}

//Set the rotation of the player (direction the player faces)
void APlayerCharacter::SetLookRotation(FClientCharacterData CharacterData)
{
	FRotator newRot;
	newRot = GetActorRotation();

	HorizontalPlayerTurnVal += CharacterData.HorizontalLookInput;
	newRot.Yaw = HorizontalPlayerTurnVal;

	SetActorRotation(newRot);
}

bool APlayerCharacter::IsPlayerGrounded()
{
	FVector Start = GetActorLocation();
	FVector End = Start;

	Start.Z += 5;
	End.Z -= 5;

	FHitResult Hit;

	if (Raycast(GetWorld(), this, Start, End, Hit, ECC_Pawn, false))
	{
		return true;
	}

	return false;

}

bool APlayerCharacter::RewindServerCharacterLocation(int16 SimulationID)
{
	if (Server_CharacterDataHistory.Find(SimulationID) != nullptr)
	{
		FServerCharacterData TempData = *Server_CharacterDataHistory.Find(SimulationID);

		PreviousLocation_LC = GetActorLocation();
		PreviousRotation_LC = GetActorRotation();

		SetActorLocation(TempData.Location);
		SetActorRotation(TempData.Rotation);

		return true;
	}
	else
	{
		return false;
	}

}

/*
* -- Network Functions - Server to Client Communication --
*/

bool APlayerCharacter::Server_SendClientCharacterData_Validate(FClientCharacterData Client_CharacterData)
{
	return true;
}

//Send local clients character input data to the server for simulation
void APlayerCharacter::Server_SendClientCharacterData_Implementation(FClientCharacterData Client_CharacterData)
{
	if (Role == ROLE_Authority)
	{
		Server_CharacterData = Client_CharacterData;
		MoveCharacter(true, Server_CharacterData);
	}
}

bool APlayerCharacter::MultiCastClient_ReplicatePawnToClients_Validate(FServerCharacterData Server_CharacterData)
{
	return true;
}

//Send all clients the simulated server character results
void APlayerCharacter::MultiCastClient_ReplicatePawnToClients_Implementation(FServerCharacterData SimulatedCharacterData)
{
	CharacterSimulatedData = SimulatedCharacterData;

	if (Role == ROLE_SimulatedProxy)
	{
		if (bEnableEntityInterpolation)
		{
			AddInterpolationData(SimulatedCharacterData);
		}
		else
		{
			SetActorLocation(CharacterSimulatedData.Location);
			SetActorRotation(CharacterSimulatedData.Rotation);

			//GEngine->AddOnScreenDebugMessage(-1, 0.05f, FColor::Red, "Ex-Client Data | Receiving | Actor Label = " + GetActorLabel());
		}
	}
	else if (Role == ROLE_AutonomousProxy)
	{
		CompareServerToClientSimulationResults();

		//GEngine->AddOnScreenDebugMessage(-1, 0.05f, FColor::Red, "Compare Results| Receiving | | Actor Label = " + GetActorLabel());
	}
}

/*
* Debug Functions
*/

void APlayerCharacter::DisplayClientPredictionsHistory()
{
	FVector dLocation = GetActorLocation();
	dLocation.Z += 100;

	//if (FVector::Dist(Debug_LastPredictionLocation, dLocation) >= 1)
	//{
		DrawDebugPoint(GetWorld(), dLocation, 5, FColor().Green, false, 3);

		if (Debug_LastPredictionLocation != FVector::ZeroVector)
		{
			DrawDebugLine(GetWorld(), Debug_LastPredictionLocation, dLocation, FColor().Green, false, 3, 0, 0.1f);
		}
	//}

	Debug_LastPredictionLocation = dLocation;
}

void APlayerCharacter::DisplayServerSimulationsHistory()
{
	FVector dLocation = CharacterSimulatedData.Location;
	dLocation.Z += 100;

	//if (FVector::Dist(Debug_LastSimulatedLocation, dLocation) >= 1)
	//{
		DrawDebugPoint(GetWorld(), dLocation, 5, FColor().Yellow, false, 3);

		if (Debug_LastSimulatedLocation != FVector::ZeroVector)
		{
			DrawDebugLine(GetWorld(), Debug_LastSimulatedLocation, dLocation, FColor().Yellow, false, 3, 0, 0.1f);
		}
	//}

	Debug_LastSimulatedLocation = dLocation;
}

void APlayerCharacter::DisplayFixedPredictionsHistory(FVector Location)
{
	FVector dLocation = Location;
	dLocation.Z += 100;

	//if (FVector::Dist(Debug_LastFixedLocation, dLocation) >= 1)
	//{
		DrawDebugPoint(GetWorld(), dLocation, 5, FColor().Blue, false, 3);

		if (Debug_LastFixedLocation != FVector::ZeroVector)
		{
			DrawDebugLine(GetWorld(), Debug_LastFixedLocation, dLocation, FColor().Blue, false, 3, 0, 0.1f);
		}
	//}

	Debug_LastFixedLocation = dLocation;
}

void APlayerCharacter::DisplayWrongPredictionMoment(FVector Location)
{
	FVector Temp = Location;
	Temp.Z += 100;

	DrawDebugPoint(GetWorld(), Temp, 30, FColor().Black, false, 3);

	Temp = GetActorLocation();
	Temp.Z += 100;

	DrawDebugPoint(GetWorld(), Temp, 30, FColor().Red, false, 3);
}

