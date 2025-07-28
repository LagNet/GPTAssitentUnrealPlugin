#include "GPTBlueprintContextMenu.h"
#include "GPTToolExecutor.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FGPTBlueprintContextMenu"

TSharedPtr<FGPTBlueprintContextMenu> FGPTBlueprintContextMenu::Instance = nullptr;

void FGPTBlueprintContextMenu::Register()
{
	if (!Instance.IsValid())
	{
		Instance = MakeShareable(new FGPTBlueprintContextMenu());
		FEdGraphUtilities::RegisterVisualNodeFactory(Instance.ToSharedRef());

		UE_LOG(LogTemp, Log, TEXT("[GPTContextMenu] Registrado com sucesso via NodeFactory."));
	}
}

void FGPTBlueprintContextMenu::Unregister()
{
	if (Instance.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(Instance.ToSharedRef());
		Instance.Reset();
	}
}

void FGPTBlueprintContextMenu::CreateContextMenuActions(
	UToolMenu* Menu,
	UEdGraphNode* Node,
	UGraphNodeContextMenuContext* Context
)
 
{
	if (!Menu || !Node)
		return;

	FToolMenuSection& Section = Menu->AddSection("GPTContextMenu", LOCTEXT("GPTSection", "GPT Assistant"));
	Section.AddMenuEntry(
		"GptAnalyzeNode",
		LOCTEXT("GPTAnalyzeNow", "Enviar para GPT"),
		LOCTEXT("GPTAnalyzeTooltip", "Envia os nós selecionados para análise."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FGPTBlueprintContextMenu::HandleAnalyzeNow))
	);
}

void FGPTBlueprintContextMenu::HandleAnalyzeNow()
{
	UE_LOG(LogTemp, Warning, TEXT("[GPTContextMenu] 🔍 Análise GPT solicitada pelos nós selecionados!"));
	
}
