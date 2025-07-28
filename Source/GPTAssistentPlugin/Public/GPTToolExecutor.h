#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Dom/JsonObject.h"
#include "GPTToolExecutor.generated.h"

/**
 * Estrutura auxiliar para armazenar resultados de busca com cache
 */
struct FFileSearchCache
{
	TArray<FString> ResultPaths;
	double LastUpdateTime = 0;
};

/**
 * Classe utilitária para executar ferramentas acionadas pela IA
 */
UCLASS()
class GPTASSISTENTPLUGIN_API UGPTToolExecutor : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Executa uma ferramenta com base no nome e nos argumentos
	 * @param ToolName Nome da ferramenta (ex: "readfile")
	 * @param Arguments Parâmetros JSON passados pela IA
	 * @return true se a execução for bem-sucedida
	 */
	bool ExecuteTool(const FString& ToolName, const TSharedPtr<FJsonObject>& Arguments, FString& OutResult);

	// Ferramentas registradas
	bool HandleReadFile(const TSharedPtr<FJsonObject>& Args, FString& OutResult);
	bool HandleListFiles(const TSharedPtr<FJsonObject>& Args, FString& OutResult);
	bool HandleExtractUAsset(const TSharedPtr<FJsonObject>& Arguments, FString& OutResult);
	bool ExportBlueprintToT3D(const TSharedPtr<FJsonObject>& AssetPath, FString& OutResult);
	// Utilitários
	FString ConvertPhysicalPathToVirtual(const FString& PhysicalPath);
	bool HandleCreateBlueprint(const TSharedPtr<FJsonObject>& Arguments, FString& OutResult);
	bool AnalyzeSelectedBlueprintNodes(FString& OutResult);
    
private:

	// Cache interno de resultados de arquivos
	TMap<FString, FFileSearchCache> CachedFileResults;
	
};
