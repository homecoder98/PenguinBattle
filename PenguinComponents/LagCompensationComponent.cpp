#include "LagCompensationComponent.h"
#include "Components/BoxComponent.h"
#include "PenguinBattle/Character/PenguinCharacter.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();

	FFramePackage Package;
	SaveFramePackage(Package);
	ShowFramePackage(Package, FColor::Orange);
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<APenguinCharacter>(GetOwner()) : Character;
	if (Character)
	{
		Package.Time = GetWorld()->GetTimeSeconds();
		for (auto& BoxPair : Character->HitCollisionBoxes)
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();
			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, FColor Color)
{
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(
			GetWorld(),
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			Color,
			true
		);
	}
}

