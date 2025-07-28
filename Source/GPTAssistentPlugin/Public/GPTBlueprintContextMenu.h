#pragma once

#include "CoreMinimal.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenus.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphUtilities.h"

/**
 * Gerencia a criação do menu de contexto para nós Blueprint.
 */
class FGPTBlueprintContextMenu : public FGraphPanelNodeFactory
{
public:
	// Registro no sistema
	static void Register();

	// Remoção, se necessário
	static void Unregister();

	// Adiciona entrada ao menu de contexto
	virtual void CreateContextMenuActions(
		UToolMenu* Menu,
		UEdGraphNode* Node,
		UGraphNodeContextMenuContext* Context
	);

private:
	static TSharedPtr<FGPTBlueprintContextMenu> Instance;

	// Ação executada ao clicar no botão do menu
	static void HandleAnalyzeNow();
};
