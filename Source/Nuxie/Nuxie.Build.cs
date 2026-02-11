using System.IO;
using UnrealBuildTool;

public class Nuxie : ModuleRules
{
  public Nuxie(ReadOnlyTargetRules Target) : base(Target)
  {
    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    CppStandard = CppStandardVersion.Cpp20;

    PublicDependencyModuleNames.AddRange(new string[]
    {
      "Core",
      "CoreUObject",
      "Engine",
      "Projects",
      "HTTP",
      "Json",
      "JsonUtilities"
    });

    PrivateDependencyModuleNames.AddRange(new string[]
    {
      "ApplicationCore",
      "Projects"
    });

    if (Target.Platform == UnrealTargetPlatform.Android)
    {
      PrivateDependencyModuleNames.Add("Launch");
      string PluginPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../"));
      string AplPath = Path.Combine(PluginPath, "ThirdParty/Android/Nuxie_APL.xml");
      AdditionalPropertiesForReceipt.Add("AndroidPlugin", AplPath);
    }

    if (Target.Platform == UnrealTargetPlatform.IOS)
    {
      PublicFrameworks.AddRange(new string[] { "StoreKit", "WebKit" });
      PublicWeakFrameworks.AddRange(new string[] { "AdSupport" });
    }
  }
}
