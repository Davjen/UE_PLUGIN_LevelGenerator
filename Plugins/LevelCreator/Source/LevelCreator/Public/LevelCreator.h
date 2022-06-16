// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FLevelCreatorModule : public IModuleInterface,FSelfRegisteringExec
{
public:
	TSharedPtr<SDockTab> MainWindow;
	typedef AActor* (FLevelCreatorModule::* CreateObj)(const int32 X, const int32 Y, UWorld* InWorld);
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	UWorld* CreateWorld();
	void LevelInitFromTxT(const FString& Path);
	void LevelInitFromPng(const FString& Path);
	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	AActor* CreateWall(const int32 X, const int32 Y, UWorld* InWorld);
	AActor* CreateDamageableWall(const int32 X, const int32 Y, UWorld* InWorld);
	void OpenFileDialog(const FString& DialogTitle, const FString& DefaultPath, const FString& FileTypes, TArray<FString>& OutFileNames);
	void DirectoryChanged(const FString& NewDir);
	//void DestroyAll();
	FReply OnUpdateClick();
	FReply OnSelectPath();
	FReply OnGenerateClick();
	TSharedRef<SDockTab> CreateMapGeneratorWindow(const FSpawnTabArgs& TabArgs);
	TMap<FColor, CreateObj> MapCreatorFunctions;

	TSharedPtr<FAssetThumbnailPool> MyThumbnailPool;
	FString MapPath;
	FAssetData WallPath;
	FAssetData DestructibleWallPath;
	FAssetData FloorPath;
};
