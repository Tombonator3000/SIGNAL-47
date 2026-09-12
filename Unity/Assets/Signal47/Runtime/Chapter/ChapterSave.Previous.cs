using System;
using System.IO;
using System.Globalization;
using System.Text;
using Signal47.Core;
using UnityEngine;

namespace Signal47.Chapter
{
    public sealed partial class ChapterSave
    {
        public sealed class PreviousCase
        {
            public string Id,Label,Detail,Fingerprint;
            public bool Available,UsesBackup;
        }
        static string PreviousRoot=>Path.Combine(StorageDirectory,"PreviousCases");

        // Caller owns FileGate. A failure leaves all active originals untouched.
        static void PreserveActiveCheckpoint()
        {
            string retired=null;
            foreach(string path in new[]{SavePath,BackupPath})
            {
                if(!File.Exists(path))continue;
                if(retired==null)
                {
                    retired=Path.Combine(PreviousRoot,DateTime.UtcNow.ToString("yyyyMMddTHHmmssfff")+"-"+Guid.NewGuid().ToString("N"));
                    Directory.CreateDirectory(retired);
                }
                string copy=Path.Combine(retired,Path.GetFileName(path));
                using(var input=File.OpenRead(path))
                using(var output=new FileStream(copy,FileMode.CreateNew,FileAccess.Write,FileShare.None))
                {input.CopyTo(output);output.Flush(true);}
            }
        }
        static bool RegularPath(string path)=>!File.Exists(path)&&!Directory.Exists(path)
            ||(File.GetAttributes(path)&FileAttributes.ReparsePoint)==0;
        static bool PreviousDirectory(string id,out string directory)
        {
            directory="";
            if(string.IsNullOrEmpty(StorageDirectory)||string.IsNullOrEmpty(id)||id.Length>100)return false;
            foreach(char c in id)if(!(c>='a'&&c<='z')&&!(c>='A'&&c<='Z')&&!(c>='0'&&c<='9')&&c!='-'&&c!='_')return false;
            directory=Path.Combine(PreviousRoot,id);
            return Directory.Exists(directory)&&RegularPath(PreviousRoot)&&RegularPath(directory);
        }
        public static string[] PreviousCaseIds(out string message)
        {
            message="";
            try
            {
                if(string.IsNullOrEmpty(StorageDirectory)){message="The save location is unavailable.";return Array.Empty<string>();}
                if(File.Exists(PreviousRoot)||!RegularPath(PreviousRoot)){message="Previous shifts could not be opened. Existing files have been kept.";return Array.Empty<string>();}
                if(!Directory.Exists(PreviousRoot))return Array.Empty<string>();
                var directories=Directory.GetDirectories(PreviousRoot);
                var ids=new System.Collections.Generic.List<string>();
                foreach(string path in directories)
                {string id=Path.GetFileName(path);if(PreviousDirectory(id,out _))ids.Add(id);}
                ids.Sort((a,b)=>StringComparer.Ordinal.Compare(b,a));
                return ids.ToArray();
            }
            catch(Exception error) when(error is IOException||error is UnauthorizedAccessException||error is ArgumentException)
            {Debug.LogWarning("PREVIOUS_CASE_LIST_FAILED "+error.Message);message="Previous shifts could not be read. Existing files have been kept.";return Array.Empty<string>();}
        }
        static bool ReadPrevious(string id,out Snapshot state,out string encoded,out bool backup)
        {
            state=null;encoded=null;backup=false;
            if(!PreviousDirectory(id,out string directory))return false;
            string primary=Path.Combine(directory,"case.json"),recovery=Path.Combine(directory,"case.backup.json");
            if(!RegularPath(primary)||!RegularPath(recovery))return false;
            if(TryRead(primary,out state,out _,true,out encoded))return true;
            if(TryRead(recovery,out state,out _,true,out encoded)){backup=true;return true;}
            return false;
        }
        public static PreviousCase InspectPreviousCase(string id)
        {
            var info=new PreviousCase{Id=id,Label="Unreadable previous shift",Detail="The saved files have been kept. This entry cannot be continued."};
            try
            {
                if(!ReadPrevious(id,out var state,out string encoded,out bool backup))return info;
                info.Available=true;info.UsesBackup=backup;info.Fingerprint=Digest(encoded);
                info.Label=DateTime.Parse(state.savedUtc,CultureInfo.InvariantCulture,DateTimeStyles.RoundtripKind).ToLocalTime().ToString("dd MMM yyyy / HH:mm:ss",CultureInfo.InvariantCulture);
                info.Detail=backup?"Recovery copy / Chapter one":"Saved checkpoint / Chapter one";
                return info;
            }
            catch(Exception error) when(error is IOException||error is UnauthorizedAccessException||error is ArgumentException||error is FormatException)
            {info.Available=false;Debug.LogWarning("PREVIOUS_CASE_INSPECT_FAILED "+error.Message);return info;}
        }
        public static bool TryContinuePrevious(string id,string expectedFingerprint)
        {
            FinishWrite();var g=GameSession.Instance;
            if(Saving||IsRestoring||quitPending||!g||!g.hud||g.hud.Started||g.Transitioning)
            {Status="Return to the start menu after saving your current shift.";return false;}
            try
            {
                if(string.IsNullOrEmpty(expectedFingerprint)||!ReadPrevious(id,out var state,out string encoded,out bool fromBackup))
                {Status="This previous shift cannot be read. Its saved files have been kept.";return false;}
                if(Digest(encoded)!=expectedFingerprint)
                {Status="This checkpoint changed. Select it again before continuing.";return false;}
                lock(FileGate)
                {
                    // Retire current primary AND backup before switching. The old
                    // active backup must not silently restore a different case later.
                    PreserveActiveCheckpoint();
                    File.Delete(BackupPath);
                    WriteAtomic(SavePath,encoded,null);
                    caseGeneration++;
                }
                pendingRestore=state;loadedBackup=fromBackup;IsRestoring=true;checkpointRequested=false;
                Status="Opening the selected previous shift…";g.Restart();return true;
            }
            catch(Exception error) when(error is IOException||error is UnauthorizedAccessException||error is ArgumentException)
            {
                // The primary swap is atomic. If clearing its old backup succeeded
                // before a failed swap, both older files remain in PreviousCases.
                Status="Could not switch shifts. Your checkpoints remain available in Previous Shifts.";
                Debug.LogWarning("PREVIOUS_CASE_CONTINUE_FAILED "+error.Message);return false;
            }
        }
    }
}
