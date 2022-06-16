#include "LevelCreator.h"
#include "AssetRegistryModule.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Factories/WorldFactory.h"
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "SEditorViewport.h"
#include "SLevelViewport.h"
#include "SAssetEditorViewport.h"
#include "Slate/SceneViewport.h"
#include "UObject/Object.h"
#include "Components/StaticMeshComponent.h"
#include "Widgets/Input/SFilePathPicker.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/Input/SDirectoryPicker.h"
#include "EditorDirectories.h"
#include "SAssetSearchBox.h"
#include "ContentBrowserModule.h"
#include "Delegates/DelegateSignatureImpl.inl"
#include "IContentBrowserSingleton.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/SViewport.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Misc/FileHelper.h"
#include "ImageUtils.h"

static const FName MapGeneratorTab("MapGeneratorTab");

#define LOCTEXT_NAMESPACE "FLevelCreatorModule"

void FLevelCreatorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(MapGeneratorTab, FOnSpawnTab::CreateRaw(this, &FLevelCreatorModule::CreateMapGeneratorWindow))
	.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory());;
	MapCreatorFunctions.Add(FColor::FromHex(FString("000000FF")), &FLevelCreatorModule::CreateWall);
	MapCreatorFunctions.Add(FColor::FromHex(FString("FF0000FF")), &FLevelCreatorModule::CreateDamageableWall);
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FLevelCreatorModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MapGeneratorTab);

	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

UWorld* FLevelCreatorModule::CreateWorld()
{
	UWorldFactory* NewWorld = NewObject<UWorldFactory>();
	uint64 SuffixName = FPlatformTime::Cycles64();
	FString AssetName = FString::Printf(TEXT("Generated_Level_%llu"), SuffixName);

	UPackage* Package = CreatePackage(*FString::Printf(TEXT("/Game/Level/%s"), *AssetName));
	UObject* NewWorldObj = NewWorld->FactoryCreateNew(NewWorld->SupportedClass, Package, *AssetName, EObjectFlags::RF_Standalone | EObjectFlags::RF_Public, nullptr, GWarn);
	FAssetRegistryModule::AssetCreated(NewWorldObj);
	UWorld* WorldCasted = Cast<UWorld>(NewWorldObj);
	WorldCasted->SpawnActor<ADirectionalLight>();
	return WorldCasted;
}

void FLevelCreatorModule::LevelInitFromTxT(const FString& Path)
{

	TArray<FString>Datas;
	FFileHelper::LoadFileToStringArray(Datas, *Path);

	UWorld* World = FLevelCreatorModule::CreateWorld();


	World->Modify();
	int32 StartX = 0;
	int32 StartY = 0;
	float DeltaIncrement = 50.f;
	float PlaneDimensionX = 0;
	float PlaneDimensionY = Datas.Num();

	UE_LOG(LogTemp, Warning, TEXT("DataNum : %d"), Datas.Num());

	//CreateWall(StartX, StartY, WorldCasted);

	/*Instantiate Floor*/


	for (int32 Y = 0; Y < Datas.Num(); Y++)
	{
		TArray<FString> Row;
		Datas[Y].ParseIntoArray(Row, TEXT(","));
		for (int32 X = 0; X < Row.Num(); X++)
		{
			if (Row[X].Equals(FString("1")))
			{
				CreateWall(StartX, StartY, World);
			}
			StartX += 100;
		}
		StartX = 0;
		StartY += 100;
	}

	World->PostEditChange();
	World->MarkPackageDirty();
}

bool FLevelCreatorModule::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (FParse::Command(&Cmd, TEXT("InitWorld")))
	{
		FString Path = FParse::Token(Cmd, true);
		UE_LOG(LogTemp, Warning, TEXT("path: %s"), *Path);

		LevelInitFromTxT(Path);
		return true;
	}
	if (FParse::Command(&Cmd, TEXT("InitWorldpng")))
	{
		FString Path = FParse::Token(Cmd, true);
		UE_LOG(LogTemp, Warning, TEXT("path: %s"), *Path);

		LevelInitFromPng(Path);
		return true;
	}
	return false;
}

void FLevelCreatorModule::LevelInitFromPng(const FString& Path)
{

	UWorld* World = FLevelCreatorModule::CreateWorld();

	World->Modify();

	UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(Path);
	int32 PlatformSizeX = Texture->GetSizeX();
	int32 PlatformSizeY = Texture->GetSizeY();
	int32 StartX = 0;
	int32 StartY = 0;

	const FColor* Pixels = static_cast<const FColor*>(Texture->PlatformData->Mips[0].BulkData.LockReadOnly());
	//LOCK DELLA MIPS
	for (int32 IndexY = 0; IndexY < PlatformSizeY; IndexY++)
	{
		for (int32 IndexX = 0; IndexX < PlatformSizeX; IndexX++)
		{
			const FColor CurrentPixel = Pixels[IndexX * PlatformSizeY + IndexY];
			FString Hex = CurrentPixel.ToHex();
			UE_LOG(LogTemp, Warning, TEXT("COLORE:%s"), *Hex);


			if (MapCreatorFunctions.Find(CurrentPixel))
			{
				(this->* * MapCreatorFunctions.Find(CurrentPixel))(StartX, StartY, World);
			}
			StartX += 100;
		}
		StartX = 0;
		StartY += 100;
	}
	Texture->PlatformData->Mips[0].BulkData.Unlock();

	World->PostEditChange();
	World->MarkPackageDirty();
}

AActor* FLevelCreatorModule::CreateWall(const int32 X, const int32 Y, UWorld* InWorld)
{
	UE_LOG(LogTemp, Warning, TEXT("CREATING A WALL"));
	FString WallPath = FString("StaticMesh'/Engine/BasicShapes/Cube.Cube'");
	UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *WallPath);
	if (StaticMesh)
	{
		AStaticMeshActor* Wall = InWorld->SpawnActor<AStaticMeshActor>();
		FVector Position;
		FTransform WallTransform;
		if (Wall->GetRootComponent())
		{
			Position = FVector(X, Y, Wall->GetActorRelativeScale3D().Z * 50);
			WallTransform.AddToTranslation(Position);
			Wall->SetActorTransform(WallTransform);
			Wall->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
			return Wall;
		}
	}
	return nullptr;
}


AActor* FLevelCreatorModule::CreateDamageableWall(const int32 X, const int32 Y, UWorld* InWorld)
{
	UE_LOG(LogTemp, Warning, TEXT("CREATING A DamageableWALL"));

	FString DestruWallPath = FString("Blueprint'/Game/DamageableWall.DamageableWall'");
	UBlueprint* Bp = LoadObject<UBlueprint>(nullptr, *DestruWallPath);
	if (Bp && Bp->GeneratedClass->IsChildOf<AActor>())
	{
		AActor* DestruWall = InWorld->SpawnActor<AActor>(Bp->GeneratedClass);
		FVector Position;
		FTransform WallTransform;
		if (DestruWall->GetRootComponent())
		{

			Position = FVector(X, Y, DestruWall->GetActorRelativeScale3D().Z * 50);
			WallTransform.AddToTranslation(Position);
			DestruWall->SetActorTransform(WallTransform);
			return DestruWall;
		}
	}
	return nullptr;
}

//void FLevelCreatorModule::DestroyAll()
//{
//	TArray<AActor*> FoundActors;
//	UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), FoundActors);
//	for (AActor* act : FoundActors)
//	{
//		act->Destroy();
//	}
//}

///////////////***********SLATE WINDOW REGION***********///////////////

void FLevelCreatorModule::OpenFileDialog(const FString& DialogTitle, const FString& DefaultPath, const FString& FileTypes, TArray<FString>& OutFileNames)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	OutFileNames.Empty();
	DesktopPlatform->OpenFileDialog(nullptr, DialogTitle, DefaultPath, FString(""), FileTypes, 0, OutFileNames);
	FGlobalTabmanager::Get()->TryInvokeTab(MapGeneratorTab);
}

void FLevelCreatorModule::DirectoryChanged(const FString& NewDir)
{
	MapPath = NewDir;
}

FReply FLevelCreatorModule::OnUpdateClick()
{
	//DestroyAll();
	//Procedural.ParseOnWorld(World, WallPath.ObjectPath.ToString(), DestructibleWallPath.ObjectPath.ToString(), FloorPath.ObjectPath.ToString(), FPaths::ConvertRelativePathToFull(Result[0]));
	return FReply::Handled();
}

FReply FLevelCreatorModule::OnSelectPath()
{
	OpenFileDialog(TEXT("Prova"), TEXT(""), TEXT("Image Files|*png"), Result);

	return FReply::Handled();
}

FReply FLevelCreatorModule::OnGenerateClick()
{
	//DestroyAll();
	//UE_LOG(LogTemp, Warning, TEXT("%s"), *Result[0]);
	//UWorld* GenWorld = FLevelCreatorModule::CreateWorld();
	//Procedural.ParseOnWorld(GenWorld, WallPath.ObjectPath.ToString(), DestructibleWallPath.ObjectPath.ToString(), FloorPath.ObjectPath.ToString(), FPaths::ConvertRelativePathToFull(Result[0]));
	return FReply::Handled();
}
TSharedRef<SDockTab> FLevelCreatorModule::CreateMapGeneratorWindow(const FSpawnTabArgs& TabArgs)
{
	/*MainWindow = SNew(SDockTab).TabRole(ETabRole::MajorTab);
	return MainWindow.ToSharedRef();*/
	MainWindow = SNew(SDockTab)
		.TabRole(ETabRole::MajorTab)
		[
			SNew(SSplitter).Orientation(Orient_Horizontal)
			+ SSplitter::Slot().Value(0.25f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
		.HAlign(EHorizontalAlignment::HAlign_Center)
		.VAlign(EVerticalAlignment::VAlign_Center)
		[
			SNew(STextBlock)

			.Text(FText::FromString(TEXT("Default Wall")))
		]
	+ SHorizontalBox::Slot().HAlign(EHorizontalAlignment::HAlign_Right).Padding(0, 0, 10, 0)
		[
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(UBlueprint::StaticClass())
		.ObjectPath_Lambda([this]()->FString {return WallPath.GetAsset()->GetPathName(); })
		.OnObjectChanged_Lambda([this](const FAssetData& Data)->void {WallPath = Data; })
		.ThumbnailPool(MyThumbnailPool)
		.DisplayThumbnail(true)
		]
		]
	+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
		.HAlign(EHorizontalAlignment::HAlign_Center)
		.VAlign(EVerticalAlignment::VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Destructible Wall")))
		]
	+ SHorizontalBox::Slot().HAlign(EHorizontalAlignment::HAlign_Right).Padding(0, 0, 10, 0)
		[
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(UBlueprint::StaticClass())
		.ObjectPath_Lambda([this]()->FString {return DestructibleWallPath.GetAsset()->GetPathName(); })
		.OnObjectChanged_Lambda([this](const FAssetData& Data)->void {DestructibleWallPath = Data; })
		.ThumbnailPool(MyThumbnailPool)
		.DisplayThumbnail(true)
		]

		]
	+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
		.HAlign(EHorizontalAlignment::HAlign_Center)
		.VAlign(EVerticalAlignment::VAlign_Center)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("Floor")))
		]
	+ SHorizontalBox::Slot().HAlign(EHorizontalAlignment::HAlign_Right).Padding(0, 0, 10, 0)
		[
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(UBlueprint::StaticClass())
		.ObjectPath_Lambda([this]()->FString {return FloorPath.GetAsset()->GetPathName(); })
		.OnObjectChanged_Lambda([this](const FAssetData& Data)->void {FloorPath = Data; })
		.ThumbnailPool(MyThumbnailPool)
		.DisplayThumbnail(true)
		]
		]
	+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Select Texture")))
		.OnClicked(FOnClicked::CreateRaw(this, &FLevelCreatorModule::OnSelectPath))
		/*.OnPathPicked(SDirectoryPicker::FOnDirectoryChanged::CreateRaw(this, &FCustomViewportModule::DirectoryChanged))*/
		]
	+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Update Preview")))
		.OnClicked(FOnClicked::CreateRaw(this, &FLevelCreatorModule::OnUpdateClick))
		]
	+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SButton).Text(FText::FromString(TEXT("Generate Level")))
			.OnClicked(FOnClicked::CreateRaw(this, &FLevelCreatorModule::OnGenerateClick))
		]
		]
	+ SSplitter::Slot()
		[
		]
		];
	return MainWindow.ToSharedRef();

}
#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLevelCreatorModule, LevelCreator)