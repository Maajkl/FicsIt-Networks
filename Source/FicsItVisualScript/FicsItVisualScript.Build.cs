using UnrealBuildTool;
using System.IO;
using System;
using EpicGames.Core;

public class FicsItVisualScript : ModuleRules
{
    public FicsItVisualScript(ReadOnlyTargetRules target) : base(target)
    {
	    CppStandard = CppStandardVersion.Cpp17;
	    bUseRTTI = true;
        //bLegacyPublicIncludePaths = false;
	    PCHUsage = PCHUsageMode.Default;

	    OptimizeCode = CodeOptimization.Never;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"Json",
			"JsonUtilities",
			"UMG",
			"SlateCore",
			"Slate",
			"ApplicationCore",
			"InputCore",
			"FactoryGame",
			"FicsItLogLibrary",
            "FicsItNetworks",
            "FicsItNetworksComputer",
            "FicsItNetworksCircuit",
            "FicsItReflection",
            "Tracy"
		});

        PublicIncludePaths.Add("Public");
    }
}
