using UnrealBuildTool;

public class GPTAssistentPlugin : ModuleRules
{
    public GPTAssistentPlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
                "Editor/BlueprintGraph/Private",
                "Editor/BlueprintGraph/Public"
            }
        );

        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
            }
        );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UMG",
                "HTTP",
                "Json",
                "JsonUtilities",
                "WebBrowser",
                "WebBrowserWidget",
                "AssetRegistry",
                "BlueprintGraph",   
                "UnrealEd", // necessário para manipulação de Blueprints no Editor
                "KismetCompiler", // (opcional, mas útil para uso futuro
                "WorkspaceMenuStructure",
                "Kismet",
                "ToolMenus",
                "GraphEditor",
                "EditorStyle",
                "ApplicationCore"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "ToolMenus",
                "EditorStyle",
                "UnrealEd",
                "Projects", 
                "WorkspaceMenuStructure", 
                "WorkspaceMenuStructure",
                "WebBrowser",
                "WebBrowserWidget"
            }
        );

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
        );
    }
}