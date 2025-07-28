#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SDockTab;
class FBlueprintEditor;
class UBlueprint;
class UEdGraph;
class UToolMenu;
class UChatBridge;

class FGPTAssistentPluginModule : public IModuleInterface
{
public:
    /** Inicializa o módulo */
    virtual void StartupModule() override;

    /** Finaliza o módulo */
    virtual void ShutdownModule() override;

private:
    /** Cria a aba do plugin */
    TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);

    /** Registra o menu principal no editor */
    void RegisterMenus();

    /** Abre a aba de chat GPT */
    void OpenChatTab();

    /** Registra o menu de contexto do Blueprint */
    void ExtendBlueprintContextMenu(FBlueprintEditor& Editor, UBlueprint* InBP, UEdGraph* InGraph, const TArray<UEdGraphNode*>& InSelectedNodes, UToolMenu* Menu);
    void ExtendContextMenuGeneric(FMenuBuilder& MenuBuilder);

    TSharedPtr<FExtender> MenuExtender;

};
