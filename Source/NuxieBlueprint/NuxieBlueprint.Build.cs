using UnrealBuildTool;

public class NuxieBlueprint : ModuleRules
{
  public NuxieBlueprint(ReadOnlyTargetRules Target) : base(Target)
  {
    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    CppStandard = CppStandardVersion.Cpp20;

    PublicDependencyModuleNames.AddRange(new string[]
    {
      "Core",
      "CoreUObject",
      "Engine",
      "Nuxie"
    });
  }
}
