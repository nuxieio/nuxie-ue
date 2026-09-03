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
      "Json"
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
      PrivateDependencyModuleNames.Add("Swift");
      string PluginPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../"));
      string FrameworkZip = Path.Combine(
        PluginPath,
        "ThirdParty/IOS/lib/NuxieUnrealBridge.embeddedframework.zip"
      );
      if (!File.Exists(FrameworkZip))
      {
        throw new BuildException(
          "Missing Nuxie iOS bridge. Run ThirdParty/IOS/scripts/build-framework.sh."
        );
      }
      PublicAdditionalFrameworks.Add(
        new Framework("NuxieUnrealBridge", FrameworkZip, null, true)
      );
      PublicFrameworks.AddRange(new string[]
      {
        "CoreGraphics",
        "Foundation",
        "Metal",
        "QuartzCore",
        "Security",
        "StoreKit",
        "WebKit"
      });
      PublicWeakFrameworks.AddRange(new string[] { "AdSupport" });
    }
  }
}
