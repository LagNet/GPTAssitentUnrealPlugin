#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Containers/Ticker.h"
#include "UChatBridge.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGPTResponse, const FString&);


class IConsoleObject;

/**
 * Classe que gerencia a comunicação com a API de Assistants da OpenAI.
 */

USTRUCT()
struct FToolCall
{
    GENERATED_BODY()

    UPROPERTY()
    FString ToolCallId;

    UPROPERTY()
    FString RunId;

    UPROPERTY()
    FString ToolName;

    // ⚠️ Não usar UPROPERTY aqui!
    TSharedPtr<FJsonObject> Arguments;

    FToolCall()
        : ToolCallId(TEXT(""))
        , RunId(TEXT(""))
        , ToolName(TEXT(""))
        , Arguments(MakeShared<FJsonObject>())
    {}
};


UCLASS(BlueprintType)
class GPTASSISTENTPLUGIN_API UChatBridge : public UObject
{
    GENERATED_BODY()

public:
    UChatBridge();

    /** Inicializa a API Key e registra os comandos de console */
    UFUNCTION(BlueprintCallable, Category = "GPT Assistant")
    void Initialize(const FString& InApiKey);

    /** Envia um prompt para o Assistant usando a Thread atual */
    UFUNCTION(BlueprintCallable, Category = "GPT Assistant")
    void SendPromptWithAssistant(const FString& Prompt);

    /** Remove comandos de console registrados */
    UFUNCTION(BlueprintCallable, Category = "GPT Assistant")
    void ClearCommands();

    FOnGPTResponse OnGPTResponse;
    
    UFUNCTION(BlueprintCallable, Category = "GPT")
    void SendToGPT(const FString& Message);

    /** Callback para o comando de console gpt.send */
    void HandleSendPrompt(const TArray<FString>& Args);
    
    /** Cria uma nova thread na API OpenAI. Se fornecido, encadeia um prompt */
    void CreateThread(const FString& OptionalPrompt = TEXT(""));
    
    UFUNCTION()
    void ResetThread();

protected:

    /** Envia o prompt para a thread e inicia o assistant */
    void RunAssistant();

    /** Verifica o status do run periodicamente (polling) */
    void PollForRunCompletion();

    /** Obtém a última resposta do assistant da thread */
    void GetFinalResponse();

    /** Registra comandos de console, como gpt.send */
    void RegisterConsoleCommands();

    /** Inicia o ticker de polling assíncrono */
    void StartPolling();

    /** Ticker do polling (chamado ciclicamente pelo FTSTicker) */
    bool TickPollRun(float DeltaTime);

    /** Encerra o polling */
    void StopPolling();
    

private:

    /** Chave da API OpenAI */
    FString ApiKey;

    /** ID da thread criada no servidor OpenAI */
    FString ThreadId;

    /** ID da execução (run) da assistant */
    FString RunId;

    /** ID do Assistant (configurado na OpenAI Platform) */
    FString AssistantId;

    /** Modelo (ex: gpt-4, gpt-3.5-turbo) */
    FString ModelVersion;

    /** Linguagem preferida (ex: pt-BR) */
    FString DefaultLanguage;

    /** Handle do ticker assíncrono para polling */
    FTSTicker::FDelegateHandle PollTickerHandle;

    /** Controle se polling está ativo */
    bool bIsPolling = false;
    
    /** Comando de console registrado (gpt.send) */
    IConsoleObject* SendPromptCommand = nullptr;
    
    void HandleToolCallsFromRun(const TArray<FToolCall>& ToolCalls);
    
    UFUNCTION()
    void SendPromptWithCallback(const FString& Message, const FString& ThreadId);

    void SubmitToolOutputsToRun(const FString& RunId, const FString& ToolCallId, const FString& Output);

    void SubmitToolOutputsArray(const FString& RunId, const TArray<TSharedPtr<FJsonValue>>& ToolOutputsArray);
};
