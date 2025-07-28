#include "GPTAssistentPlugin.h"
#include "UChatBridge.h"
#include "UGPTChatController.h"
#include "SGPTChatWindow.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructureModule.h"
#include "WorkspaceMenuStructure.h"
#include "GPTBlueprintContextMenu.h"
#include "BlueprintEditorModule.h"
#include "GraphEditorActions.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxExtender.h"

UChatBridge* GlobalChatBridge = nullptr;

IMPLEMENT_MODULE(FGPTAssistentPluginModule, GPTAssistentPlugin)

void FGPTAssistentPluginModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("[GPT Plugin] Módulo inicializado."));

    GlobalChatBridge = NewObject<UChatBridge>();
    GlobalChatBridge->AddToRoot();

    FString PluginConfigPath = FPaths::Combine(
        IPluginManager::Get().FindPlugin(TEXT("GPTAssistentPlugin"))->GetBaseDir(),
        TEXT("GPTAssistentPluginConfig.ini")
    );

    FString ApiKey;
    GConfig->LoadFile(PluginConfigPath);
    if (!GConfig->GetString(TEXT("GPTSettings"), TEXT("ApiKey"), ApiKey, PluginConfigPath))
    {
        UE_LOG(LogTemp, Error, TEXT("[GPT Plugin] Não foi possível encontrar a API Key no arquivo .ini (%s)"), *PluginConfigPath);
    }
    else
    {
        GlobalChatBridge->Initialize(ApiKey);
        UE_LOG(LogTemp, Log, TEXT("[GPT Plugin] API Key carregada com sucesso."));
    }

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner("GPTChatTab",
        FOnSpawnTab::CreateRaw(this, &FGPTAssistentPluginModule::OnSpawnPluginTab))
        .SetDisplayName(FText::FromString("GPT Chat"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FGPTAssistentPluginModule::RegisterMenus)
    );
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float DeltaTime)
    {
        if (GEditor && FModuleManager::Get().IsModuleLoaded("Kismet"))
        {
            TSharedRef<FGPTBlueprintContextMenu> GPTContextMenu = MakeShareable(new FGPTBlueprintContextMenu());
            GPTContextMenu->Register();
            UE_LOG(LogTemp, Log, TEXT("[GPT Plugin] Menu de contexto registrado com sucesso."));
            return false;
        }
        return true; 
    }), 0.5f);

}

void FGPTAssistentPluginModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("[GPT Plugin] Módulo finalizado."));

    if (GlobalChatBridge)
    {
        GlobalChatBridge->RemoveFromRoot();
        GlobalChatBridge = nullptr;
    }

    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("GPTChatTab");
}

TSharedRef<SDockTab> FGPTAssistentPluginModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
    UGPTChatController* Controller = NewObject<UGPTChatController>();
    Controller->AddToRoot(); 
    Controller->Initialize(GlobalChatBridge);

    TSharedRef<SGPTChatWindow> ChatWindow = SNew(SGPTChatWindow)
        .ChatController(Controller);

    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        .Label(FText::FromString("GPT Chat"))
        [
            ChatWindow
        ];
}

void FGPTAssistentPluginModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");

    FToolMenuSection& Section = Menu->AddSection("GPT", FText::FromString("GPT Assistente"));

    Section.AddMenuEntry(
        "OpenGPTChat",
        FText::FromString("Abrir Chat GPT"),
        FText::FromString("Abre a janela de interação com o GPT."),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateRaw(this, &FGPTAssistentPluginModule::OpenChatTab))
    );
}

void FGPTAssistentPluginModule::OpenChatTab()
{
    FGlobalTabmanager::Get()->TryInvokeTab(FTabId("GPTChatTab"));
}


