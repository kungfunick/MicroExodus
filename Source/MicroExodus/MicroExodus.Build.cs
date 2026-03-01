using UnrealBuildTool;

public class MicroExodus : ModuleRules
{
    public MicroExodus(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Json", 
            "JsonUtilities",
            "UMG",
            "Slate",
            "SlateCore"
        });
    }
}
