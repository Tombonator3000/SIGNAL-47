using System;
using System.Collections;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;
using Signal47.Core;
using Signal47.Investigation;
using Signal47.Signals;

namespace Signal47.Chapter
{
    /// <summary>A durable case checkpoint, separate from the player's exported photographs.</summary>
    public sealed class ChapterSave : MonoBehaviour
    {
        const int FormatVersion=1;
        const int MaximumFileBytes=2*1024*1024;
        const int QuitBudgetMilliseconds=2000;
        const string FormatName="SIGNAL47-CHAPTER09";
        [Serializable] sealed class Envelope
        {
            public string format,payload,sha256;
            public int version;
        }
        [Serializable] sealed class Snapshot
        {
            public int version;
            public string savedUtc,build,notebook,camera,chapter;
            public bool postPrologue,yardCompleted,yardReturned,doorOpen;
            public Vector3 playerPosition,mugPosition;
            public float playerYaw,playerPitch,frequency,gain,bandwidth,azimuth;
        }

        static readonly object FileGate=new();
        static ChapterSave instance;
        static Snapshot pendingRestore;
        static Task writeJob;
        static bool checkpointRequested,suppressCheckpoints,storageResolved,loadedBackup,isolatedStorage;
        static bool quitPending,quitAllowed,writeFailed,quitWithoutSavingAvailable;
        static System.Diagnostics.Stopwatch quitClock;
        static string storageDirectory="",storageError="";
        static int caseGeneration;
        public static string Status{get;private set;}="No checkpoint saved yet.";
        public static bool IsRestoring{get;private set;}
        public static bool Saving=>writeJob!=null&&!writeJob.IsCompleted;
        public static bool QuitPending=>quitPending;
        public static bool CanQuitWithoutSaving=>quitWithoutSavingAvailable&&!quitPending&&!quitAllowed&&!IsRestoring;
        public static bool LastLoadUsedBackup{get;private set;}
        public static string StorageDirectory
        {
            get
            {
                if(!storageResolved)ResolveStorage();
                return storageDirectory;
            }
        }
        public static string SavePath=>string.IsNullOrEmpty(StorageDirectory)?"":Path.Combine(StorageDirectory,"case.json");
        public static string PhotoArchiveDirectory=>string.IsNullOrEmpty(StorageDirectory)?"":Path.Combine(isolatedStorage?StorageDirectory:Application.persistentDataPath,"FieldPhotos");
        static string BackupPath=>string.IsNullOrEmpty(StorageDirectory)?"":Path.Combine(StorageDirectory,"case.backup.json");
        public static bool HasSave=>!string.IsNullOrEmpty(SavePath)&&(File.Exists(SavePath)||File.Exists(BackupPath));
        public static bool CanSave=>SaveBlocker()=="";
        public static string SaveAvailability=>SaveBlocker();

        static void ResolveStorage()
        {
            storageResolved=true;
            try
            {
                string selected=Path.Combine(Application.persistentDataPath,"Chapter09");
                var arguments=System.Environment.GetCommandLineArgs();
                for(int i=1;i<arguments.Length;i++)
                {
                    if(arguments[i]!="--signal47-save-dir")continue;
                    if(i+1>=arguments.Length||string.IsNullOrWhiteSpace(arguments[i+1])||arguments[i+1].StartsWith("--",StringComparison.Ordinal))
                        throw new ArgumentException("--signal47-save-dir requires a separate directory path.");
                    selected=arguments[++i];isolatedStorage=true;
                }
                storageDirectory=Path.GetFullPath(selected);
            }
            catch(Exception error) when(error is ArgumentException||error is NotSupportedException||error is IOException||error is UnauthorizedAccessException)
            {
                storageDirectory="";storageError="Save location unavailable: "+error.Message;Status=storageError;
            }
        }

        void Awake()
        {
            instance=this;suppressCheckpoints=false;quitPending=false;quitAllowed=false;quitWithoutSavingAvailable=false;
            Application.wantsToQuit-=WantsToQuit;Application.wantsToQuit+=WantsToQuit;
            PlayerSettings.Load();PlayerSettings.Apply();
        }
        IEnumerator Start()
        {
            // All scene components need Awake/Start before restoring meshes, cameras and audio.
            yield return null;
            PlayerSettings.Apply();
            if(pendingRestore==null)yield break;
            var state=pendingRestore;pendingRestore=null;
            var g=GameSession.Instance;
            try
            {
                if(!RuntimeReady(g))throw new InvalidDataException("The chapter scene is missing a required component.");
                if(!ValidateSnapshot(state,out string reason))throw new InvalidDataException(reason);
                g.director.RestoreCompleted(state.mugPosition);
                var receiver=FindFirstObjectByType<Signal47.Interaction.ReceiverBank>();
                if(receiver)receiver.ApplyPoweredVisuals();
                var console=FindFirstObjectByType<SignalConsole>();
                if(!console)throw new InvalidDataException("The saved receiver has no matching console in this scene.");
                console.RestoreCompleted(state.frequency,state.gain,state.bandwidth,state.azimuth);
                g.yard.RestoreState(state.yardCompleted,state.yardReturned);g.yard.door.RestoreState(state.doorOpen);
                g.notebook.RestoreState(state.notebook);
                g.fieldCamera.RestoreState(state.camera);
                g.chapter.RestoreState(state.chapter);
                g.player.RestorePose(state.playerPosition,state.playerYaw,state.playerPitch);
                Time.timeScale=1;AudioListener.pause=false;PlayerSettings.Apply();
                LastLoadUsedBackup=loadedBackup;
                Status=loadedBackup?"Case restored from the recovery copy. The damaged checkpoint was preserved.":"Case continued from the saved checkpoint.";
                IsRestoring=false;g.hud.ResumeFromSave();
                if(g.fieldCamera.SaveStatus.StartsWith("PHOTO FILE MISSING",StringComparison.Ordinal))g.hud.Toast(g.fieldCamera.SaveStatus,8);
                Debug.Log("CHAPTER_CONTINUE_OK backup="+loadedBackup+" saved="+state.savedUtc);
            }
            catch(Exception error)
            {
                // Never leave a partly restored world playable. Keep both files for recovery.
                Status="Continue failed; the saved case is unchanged. "+error.Message;
                Debug.LogWarning("CHAPTER_CONTINUE_FAILED "+error.Message);
                IsRestoring=false;checkpointRequested=false;
                if(g)g.Restart();
            }
        }
        void Update()
        {
            FinishWrite();
            if(quitPending&&writeFailed){CancelQuit("Saving failed. The game remains open; retry Save or Quit after correcting the save location.");return;}
            if(checkpointRequested&&!Saving&&CanSave)BeginWrite();
            if(quitPending)AdvanceQuit();
        }
        void OnDestroy()
        {
            if(instance!=this)return;
            instance=null;Application.wantsToQuit-=WantsToQuit;quitPending=false;quitClock=null;
        }
        static bool WantsToQuit()
        {
            if(quitAllowed)return true;
            if(quitPending)return false;
            var g=GameSession.Instance;
            if(IsRestoring||suppressCheckpoints||!g||!g.hud||!g.hud.Started||!g.yard||!g.yard.Active)return true;
            FinishWrite();writeFailed=false;
            // An older write may still own FileGate. Keep the live world until the
            // newer snapshot and every required exposure have reached durable storage.
            checkpointRequested=true;quitPending=true;quitWithoutSavingAvailable=false;quitClock=System.Diagnostics.Stopwatch.StartNew();
            g.hud.SetPaused(true);Status="Saving the latest case before exit…";
            return false;
        }
        static void AdvanceQuit()
        {
            if(writeFailed){CancelQuit("Saving failed. The game remains open; the current case is still available.");return;}
            if(!checkpointRequested&&writeJob==null)
            {
                quitPending=false;quitAllowed=true;quitClock=null;
                Debug.Log("CHAPTER_QUIT_DRAIN_OK");Application.Quit();return;
            }
            if(quitClock.ElapsedMilliseconds>=QuitBudgetMilliseconds)
                CancelQuit("Exit canceled: saving did not finish within two seconds. The game remains open; wait for the archive, then retry Quit.");
        }
        static void CancelQuit(string message)
        {
            quitPending=false;quitAllowed=false;quitWithoutSavingAvailable=true;quitClock=null;Status=message;
            var g=GameSession.Instance;if(g&&g.hud){g.hud.SetPaused(true);g.hud.Toast(message,8);}
            Debug.LogWarning("CHAPTER_QUIT_CANCELED "+message);
        }
        public static void QuitWithoutSaving()
        {
            if(!CanQuitWithoutSaving)return;
            // Explicit recovery choice after failed Quit only. Do not cancel a running
            // write, delete its temporary file, or touch the last durable checkpoint.
            // Atomic replacement ensures an interrupted writer leaves a valid old file.
            checkpointRequested=false;quitPending=false;quitAllowed=true;quitWithoutSavingAvailable=false;quitClock=null;
            Status="Exiting without another checkpoint. The last saved case and photo exports are preserved.";
            Debug.LogWarning("CHAPTER_QUIT_WITHOUT_SAVING "+Status);Application.Quit();
        }
        void OnApplicationQuit()
        {
            if(quitAllowed)return; // The cancellable quit path has already drained everything.
            // Best effort for shutdowns that bypass wantsToQuit. One shared deadline
            // covers both an older write and a newer checkpoint queued behind it.
            var deadline=System.Diagnostics.Stopwatch.StartNew();writeFailed=false;
            if(CanSave)checkpointRequested=true;
            while(deadline.ElapsedMilliseconds<QuitBudgetMilliseconds)
            {
                FinishWrite();if(writeFailed)break;
                if(checkpointRequested&&!Saving&&CanSave)BeginWrite();
                if(writeFailed||writeJob==null)break;
                int remaining=QuitBudgetMilliseconds-(int)deadline.ElapsedMilliseconds;if(remaining<=0)break;
                try{writeJob.Wait(remaining);}catch(AggregateException){ }
            }
            FinishWrite();
            if(checkpointRequested||Saving||writeFailed)Debug.LogWarning("CHAPTER_FORCED_QUIT_INCOMPLETE Last durable checkpoint preserved; forced shutdown interrupted the latest save.");
        }

        static bool RuntimeReady(GameSession g)=>g&&g.player&&g.hud&&g.notebook&&g.fieldCamera&&g.chapter&&g.yard&&g.yard.door&&g.director&&g.director.mug&&g.director.printer&&g.director.dishes;
        static string SaveBlocker()
        {
            if(string.IsNullOrEmpty(StorageDirectory))return storageError;
            if(IsRestoring||suppressCheckpoints)return "Wait for the case to finish loading.";
            var g=GameSession.Instance;
            if(!instance||!RuntimeReady(g))return "The chapter is not ready to save.";
            if(!g.yard.Active||!g.director.SignalAcquired)return "Saving becomes available after the telephone event and array alignment.";
            if(g.fieldCamera.Capturing||!g.fieldCamera.SaveReady)return "Waiting for the exposed film to reach the local archive.";
            if(!g.chapter.CanSave)return "Finish the current physical action before saving.";
            return "";
        }
        public static void RequestCheckpoint()
        {
            if(IsRestoring||suppressCheckpoints)return;
            var g=GameSession.Instance;
            if(!g||!g.yard||!g.yard.Active)
            {
                Status=SaveBlocker();return;
            }
            checkpointRequested=true;
            string reason=SaveBlocker();Status=reason==""?"Checkpoint queued.":reason;
        }
        static void BeginWrite()
        {
            checkpointRequested=false;writeFailed=false;
            try
            {
                var g=GameSession.Instance;var console=FindFirstObjectByType<SignalConsole>();
                if(!console)throw new InvalidDataException("The receiver is missing from this scene.");
                var state=new Snapshot
                {
                    version=FormatVersion,savedUtc=DateTime.UtcNow.ToString("O"),build=Application.version,
                    postPrologue=g.director.SignalAcquired,yardCompleted=g.yard.Completed,yardReturned=g.yard.Returned,doorOpen=g.yard.door.Open,
                    playerPosition=g.player.transform.position,playerYaw=g.player.transform.eulerAngles.y,playerPitch=g.player.ViewPitch,
                    mugPosition=g.director.mug.transform.position,
                    frequency=console.frequency,gain=console.gain,bandwidth=console.bandwidth,azimuth=console.azimuth,
                    notebook=g.notebook.CaptureState(),camera=g.fieldCamera.CaptureState(),chapter=g.chapter.CaptureState()
                };
                // The camera's SaveReady contract guarantees archived exposures here. File
                // decoding is reserved for Continue so checkpoints cannot stall rendering.
                if(!ValidateSnapshot(state,out string reason,false))throw new InvalidDataException(reason);
                string payload=JsonUtility.ToJson(state);
                string encoded=JsonUtility.ToJson(new Envelope{format=FormatName,version=FormatVersion,payload=payload,sha256=Digest(payload)},true);
                string destination=SavePath,backup=BackupPath;int generation=caseGeneration;
                bool previousValid=TryRead(destination,out _,out _,false);
                Status="Saving case…";
                writeJob=Task.Run(()=>
                {
                    lock(FileGate)
                    {
                        if(generation!=caseGeneration)return;
                        Directory.CreateDirectory(Path.GetDirectoryName(destination));
                        if(File.Exists(destination)&&!previousValid)
                            File.Copy(destination,destination+".damaged-"+DateTime.UtcNow.ToString("yyyyMMddHHmmssfff"),false);
                        WriteAtomic(destination,encoded,previousValid?backup:null);
                    }
                });
            }
            catch(Exception error)
            {
                writeFailed=true;
                Status="Save failed; the previous checkpoint is unchanged. "+error.Message;
                Debug.LogWarning("CHAPTER_SAVE_FAILED "+error.Message);
            }
        }
        static void FinishWrite()
        {
            if(writeJob==null||!writeJob.IsCompleted)return;
            if(writeJob.IsFaulted)
            {
                writeFailed=true;
                Status="Save failed; the previous checkpoint is unchanged. "+writeJob.Exception.GetBaseException().Message;
                Debug.LogWarning("CHAPTER_SAVE_FAILED "+writeJob.Exception.GetBaseException().Message);
            }
            else
            {
                writeFailed=false;
                Status="Case saved. Photographs remain in the local archive.";
                Debug.Log("CHAPTER_SAVE_OK "+SavePath);
            }
            writeJob=null;
        }

        public static bool TryContinue()
        {
            FinishWrite();
            if(Saving){Status="The checkpoint is still being written. Try Continue again in a moment.";return false;}
            if(IsRestoring)return false;
            if(!TryReadBest(out var state,out string reason,out bool backup))
            {
                Status="Cannot continue: "+reason;Debug.LogWarning("CHAPTER_SAVE_REJECTED "+reason);return false;
            }
            var g=GameSession.Instance;if(!g){Status="Open the chapter scene before continuing.";return false;}
            pendingRestore=state;loadedBackup=backup;IsRestoring=true;checkpointRequested=false;
            Status=backup?"Loading the recovery copy…":"Loading saved case…";
            g.Restart();return true;
        }
        public static bool InspectSave(out string reason)=>TryReadBest(out _,out reason,out _);
        static bool TryReadBest(out Snapshot state,out string reason,out bool fromBackup)
        {
            state=null;fromBackup=false;
            if(string.IsNullOrEmpty(SavePath)){reason=storageError;return false;}
            if(TryRead(SavePath,out state,out string primaryReason,true)){reason="";return true;}
            if(TryRead(BackupPath,out state,out _,true)){reason="Recovery copy available.";fromBackup=true;return true;}
            reason=primaryReason;return false;
        }
        static bool TryRead(string path,out Snapshot state,out string reason,bool verifyReferences)
        {
            state=null;reason="No saved case is available.";
            try
            {
                if(!File.Exists(path))return false;
                if(new FileInfo(path).Length>MaximumFileBytes){reason="The checkpoint exceeds the supported size.";return false;}
                string json=File.ReadAllText(path,Encoding.UTF8);
                var file=JsonUtility.FromJson<Envelope>(json);
                if(file==null||file.format!=FormatName||file.version!=FormatVersion)
                {reason="This checkpoint uses an unsupported format. It has been preserved.";return false;}
                if(string.IsNullOrEmpty(file.payload)||string.IsNullOrEmpty(file.sha256)||!string.Equals(file.sha256,Digest(file.payload),StringComparison.Ordinal))
                {reason="The checkpoint is incomplete or damaged. It has been preserved.";return false;}
                state=JsonUtility.FromJson<Snapshot>(file.payload);
                return ValidateSnapshot(state,out reason,verifyReferences);
            }
            catch(Exception error) when(error is ArgumentException||error is IOException||error is UnauthorizedAccessException||error is NotSupportedException)
            {state=null;reason="The checkpoint could not be read: "+error.Message;return false;}
        }
        static bool ValidateSnapshot(Snapshot state,out string reason,bool verifyReferences=true)
        {
            reason="The saved case has inconsistent chapter data.";
            if(state==null||state.version!=FormatVersion||!state.postPrologue||state.yardReturned&&!state.yardCompleted||!DateTime.TryParse(state.savedUtc,out _))return false;
            if(!Finite(state.playerPosition)||!Finite(state.mugPosition)||!Finite(state.playerYaw)||!Finite(state.playerPitch)||state.playerPitch<-70||state.playerPitch>70)return false;
            if(!InRange(state.frequency,1419.5f,1420.7f)||!InRange(state.gain,0,100)||!InRange(state.bandwidth,4,100)||!InRange(state.azimuth,0,180))return false;
            if(!Notebook.ValidateState(state.notebook,out reason))return false;
            if(verifyReferences)
            {
                if(!FieldCamera.ValidateState(state.camera,out reason))return false;
                if(!ChapterInvestigation.ValidateState(state.chapter,out reason))return false;
            }
            else if(string.IsNullOrEmpty(state.camera)||string.IsNullOrEmpty(state.chapter))return false;
            reason="";return true;
        }
        static bool Finite(float value)=>!float.IsNaN(value)&&!float.IsInfinity(value)&&Mathf.Abs(value)<100000;
        static bool Finite(Vector3 value)=>Finite(value.x)&&Finite(value.y)&&Finite(value.z);
        static bool InRange(float value,float min,float max)=>Finite(value)&&value>=min&&value<=max;
        static string Digest(string value)
        {
            using var hash=SHA256.Create();return BitConverter.ToString(hash.ComputeHash(Encoding.UTF8.GetBytes(value))).Replace("-","");
        }
        internal static void WriteAtomic(string destination,string encoded,string backup)
        {
            string temporary=destination+".tmp";
            using(var stream=new FileStream(temporary,FileMode.Create,FileAccess.Write,FileShare.None))
            {
                byte[] bytes=new UTF8Encoding(false).GetBytes(encoded);stream.Write(bytes,0,bytes.Length);stream.Flush(true);
            }
            if(File.Exists(destination))File.Replace(temporary,destination,backup);
            else File.Move(temporary,destination);
        }
        public static void StartNew()
        {
            // Never remove FieldPhotos, settings, or earlier exported evidence.
            suppressCheckpoints=true;checkpointRequested=false;pendingRestore=null;IsRestoring=false;quitPending=false;quitAllowed=false;quitWithoutSavingAvailable=false;quitClock=null;
            try
            {
                lock(FileGate)
                {
                    caseGeneration++;
                    if(!string.IsNullOrEmpty(SavePath))
                    {
                        File.Delete(SavePath);File.Delete(BackupPath);File.Delete(SavePath+".tmp");
                    }
                }
                writeJob=null;Status="New case. Previous photograph exports were preserved.";LastLoadUsedBackup=false;
                var g=GameSession.Instance;
                // Initial Start enters this already-fresh scene; in-game New reloads it.
                if(g&&g.hud&&!g.hud.Started)suppressCheckpoints=false;
            }
            catch(Exception error) when(error is IOException||error is UnauthorizedAccessException)
            {Status="The old checkpoint could not be cleared: "+error.Message;Debug.LogWarning("CHAPTER_NEW_CASE_FAILED "+error.Message);}
        }
    }
}
