#if UNITY_EDITOR
using System;
using System.IO;
using UnityEditor;
using UnityEditor.Build.Reporting;
using UnityEngine;
namespace Signal47.Editor
{
    public static class Automation
    {
        public static void BuildLinux()
        {
            Signal47SceneBuilder.BuildVerticalSlice();
            PlayerSettings.companyName="SARO";PlayerSettings.productName="SIGNAL 47";
            PlayerSettings.defaultScreenWidth=1280;PlayerSettings.defaultScreenHeight=800;
            PlayerSettings.fullScreenMode=FullScreenMode.Windowed;
            string output=Path.GetFullPath("../Artifacts/Linux/Signal47.x86_64");
            Directory.CreateDirectory(Path.GetDirectoryName(output));
            var report=BuildPipeline.BuildPlayer(new BuildPlayerOptions{
                scenes=new[]{"Assets/Signal47/Scenes/Prototype/SARO_Prologue.unity"},
                locationPathName=output,target=BuildTarget.StandaloneLinux64,options=BuildOptions.Development});
            if(report.summary.result!=BuildResult.Succeeded)throw new Exception("Linux build failed: "+report.summary.result);
            File.Copy("../Docs/THIRD_PARTY_NOTICES.md",Path.Combine(Path.GetDirectoryName(output),"THIRD_PARTY_NOTICES.md"),true);
            File.Copy("Assets/Signal47/Art/ThirdParty/VT323/OFL.txt",Path.Combine(Path.GetDirectoryName(output),"VT323-OFL.txt"),true);
            Debug.Log("SIGNAL47_BUILD_OK "+output);
        }
    }
}
#endif
