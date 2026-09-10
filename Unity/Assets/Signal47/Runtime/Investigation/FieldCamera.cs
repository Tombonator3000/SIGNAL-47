using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using UnityEngine.Experimental.Rendering;
using Signal47.Core;
using Signal47.Chapter;

namespace Signal47.Investigation
{
    [Serializable]
    public sealed class ExposureRecord
    {
        public string id, path, utc, localTime, build, method, subject, sha256;
        public Vector3 position, forward;
        public Vector2 referenceViewport, echoViewport;
        public int width, height;
        public bool developed, inspected, exported;
        [NonSerialized] public Texture2D texture;
        [NonSerialized] public byte[] pendingPixels;
    }

    /// <summary>Immutable scene exposures. Developing changes availability, never their pixels.</summary>
    public sealed class FieldCamera : MonoBehaviour
    {
        public const string PhotoId = "s03-field-photograph";
        public const string SecondPhotoId = "b12-control-photograph";
        public static readonly Vector3 Subject = new Vector3(8, 4.8f, -34);
        public bool Acquired { get; private set; }
        public bool Raised { get; private set; }
        public bool Capturing { get; private set; }
        public bool HasPhoto => GetFrame(PhotoId) != null;
        public bool Compared { get; private set; }
        public int RejectedFrames { get; private set; }
        public int FrameCount => frames.Count;
        public int DevelopedCount { get { int count = 0; foreach (var frame in frames) if (frame.developed) count++; return count; } }
        public bool SaveReady => !busy && saveJob == null && frames.TrueForAll(f => f.exported);
        public string SaveStatus { get; private set; } = "";
        public string PhotoPath => GetFrame(PhotoId)?.path ?? "";
        public Texture2D photograph;
        public AudioClip shutter;
        public IReadOnlyList<ExposureRecord> Frames => frames;
        public string Objective => Chapter ? Chapter.Objective : !Acquired ? "FIELD KIT // COLLECT THE CAMERA BESIDE THE EAST DOOR" : "S-03 // C: RAISE CAMERA / SPACE: EXPOSE";
        readonly List<ExposureRecord> frames = new List<ExposureRecord>();
        AudioSource sound;
        ChapterInvestigation chapter;
        ChapterInvestigation Chapter { get { if (!chapter) chapter = FindFirstObjectByType<ChapterInvestigation>(); return chapter; } }
        bool busy;
        Task<string> saveJob;
        ExposureRecord savingFrame;
        string buildId = "";
        RenderTexture exposureTarget;
        [Serializable] sealed class CameraState { public int version = 1; public bool acquired, compared; public int rejected; public List<ExposureRecord> frames; }

        void Awake()
        {
            string path = Path.Combine(Application.dataPath, "../build-id.txt");
            buildId = File.Exists(path) ? File.ReadAllText(path).Trim() : Application.version;
            sound = gameObject.AddComponent<AudioSource>(); sound.playOnAwake = false; sound.spatialBlend = 0;
        }
        public ExposureRecord GetFrame(string id) => frames.Find(f => f.id == id);
        public ExposureRecord PendingFilm => frames.Find(f => !f.developed);
        public void TakeCamera()
        {
            if (Acquired) return;
            Acquired = true; EnsureTarget();
            var g = GameSession.Instance;
            g.notebook.Add("Collected the field camera. Expose the antenna profile from S-03, then process the film in the north photolab.");
            g.hud.Toast("FIELD CAMERA // C TO RAISE / SPACE TO EXPOSE", 4);
            ChapterSave.RequestCheckpoint();
        }
        void EnsureTarget()
        {
            int h = Mathf.RoundToInt(960f * Screen.height / Mathf.Max(1, Screen.width));
            if (exposureTarget && exposureTarget.height == h) return;
            if (exposureTarget) { exposureTarget.Release(); Destroy(exposureTarget); }
            exposureTarget = new RenderTexture(960, h, 24, RenderTextureFormat.ARGB32) { name = "FieldCameraImmutableExposure", antiAliasing = 1 };
            exposureTarget.Create();
        }
        void Update()
        {
            var g = GameSession.Instance; if (!g) return;
            if (saveJob != null && saveJob.IsCompleted)
            {
                bool ok = !saveJob.IsFaulted && !saveJob.IsCanceled && File.Exists(savingFrame.path) && File.Exists(Path.ChangeExtension(savingFrame.path, ".json"));
                savingFrame.exported = ok;
                SaveStatus = ok ? "SAVED TO LOCAL PHOTO ARCHIVE" : "EXPORT FAILED // RETRY AT THE PHOTOLAB";
                if (!ok) { Debug.LogWarning("FIELD_PHOTO_SAVE_FAILED " + saveJob.Exception?.GetBaseException().Message); g.hud.Toast(SaveStatus, 6); }
                else { savingFrame.sha256 = saveJob.Result; savingFrame.pendingPixels = null; Debug.Log("FIELD_PHOTO_EXPORTED " + savingFrame.id); }
                saveJob = null; savingFrame = null;
                if (ok) ChapterSave.RequestCheckpoint();
            }
            if (!g.CanControl) { if (!Capturing) Raised = false; return; }
            var keys = Keyboard.current; if (keys == null || !Acquired) return;
            if (keys.cKey.wasPressedThisFrame && !busy) Raised = !Raised;
            if (Raised && keys.spaceKey.wasPressedThisFrame && !busy)
            {
                string problem = FrameProblem();
                if (problem.Length > 0) { RejectedFrames++; g.hud.Toast(problem, 3); return; }
                StartCoroutine(Expose());
            }
        }
        public string FrameProblem()
        {
            var g = GameSession.Instance; if (!g || !g.player) return "CAMERA UNAVAILABLE";
            var c = g.player.viewCamera;
            if (!g.yard || !g.yard.Completed) return "READ THE S-03 MOTOR LOG FIRST";
            if (saveJob != null) return "FILM ADVANCE // FINISHING THE EXPOSURE";
            if (HasPhoto)
            {
                if (!Chapter || !Chapter.ExperimentReady) return PendingFilm != null ? "UNPROCESSED FILM // NORTH PHOTOLAB" : "REVIEW FRAME 01 AND THE B-12 REFERENCE SHEET";
                if (GetFrame(SecondPhotoId) != null) return "CONTROL FRAME EXPOSED // RETURN TO THE PHOTOLAB";
                if (Vector2.Distance(new Vector2(c.transform.position.x, c.transform.position.z), new Vector2(10.75f, -18.9f)) > 3.5f) return "MOVE TO THE B-12 CONTROL APRON";
                Vector3 v = c.WorldToViewportPoint(Chapter.ReferencePosition);
                if (v.z < 0 || v.x < .22f || v.x > .78f || v.y < .23f || v.y > .78f) return "FRAME THE B-12 REFERENCE VANE AND ITS FIXED BRACKET";
                float range = Vector3.Distance(c.transform.position, Chapter.ReferencePosition);
                if (Physics.Linecast(c.transform.position, Chapter.ReferencePosition, out var obstruction, Physics.DefaultRaycastLayers, QueryTriggerInteraction.Ignore) && obstruction.distance < range - .85f) return "REFERENCE OBSTRUCTED // FIND A CLEAR LINE OF SIGHT";
                if (Chapter.ExperimentMoving) return "WAIT FOR THE REFERENCE VANE TO SETTLE";
                return "";
            }
            if (c.transform.position.x < 9.5f || c.transform.position.z > -9 || c.transform.position.z < -17) return "MOVE TO THE S-03 SERVICE APRON";
            Vector3 point = c.WorldToViewportPoint(Subject); float distance = Vector3.Distance(c.transform.position, Subject);
            if (point.z < 0 || point.x < .36f || point.x > .64f || point.y < .30f || point.y > .65f || distance > 32) return "ALIGN THE CENTRAL ANTENNA IN THE FRAME";
            if (Physics.Linecast(c.transform.position, Subject, out var hit, Physics.DefaultRaycastLayers, QueryTriggerInteraction.Ignore) && hit.distance < distance - 4) return "VIEW OBSTRUCTED // FIND A CLEAR LINE OF SIGHT";
            return "";
        }
        IEnumerator Expose()
        {
            busy = true; Capturing = true; EnsureTarget();
            var g = GameSession.Instance; var camera = g.player.viewCamera;
            bool second = HasPhoto;
            var record = new ExposureRecord
            {
                id = second ? SecondPhotoId : PhotoId, subject = second ? "B-12 / controlled reference" : "S-03 / array profile",
                method = second && Chapter ? Chapter.ExperimentMethod : "baseline", utc = DateTime.UtcNow.ToString("O"), build = buildId,
                localTime = TimeSpan.FromSeconds(g.notebook.LocalClockSeconds).ToString(@"hh\:mm\:ss"),
                position = camera.transform.position, forward = camera.transform.forward, width = exposureTarget.width, height = exposureTarget.height
            };
            if (Chapter)
            {
                Vector3 a = camera.WorldToViewportPoint(Chapter.ReferencePosition), b = camera.WorldToViewportPoint(Chapter.EchoPosition);
                record.referenceViewport = new Vector2(a.x, a.y); record.echoViewport = new Vector2(b.x, b.y);
            }
            // URP renders the real player camera once into a dedicated target. The fictional
            // film response exists only for this render, never as a later replacement image.
            bool rendered = false;
            try
            {
                if (Chapter) Chapter.SetFilmResponse(true);
                var request = new UniversalRenderPipeline.SingleCameraRequest { destination = exposureTarget };
                if (RenderPipeline.SupportsRenderRequest(camera, request)) { RenderPipeline.SubmitRenderRequest(camera, request); rendered = true; }
                else Debug.LogWarning("FIELD_PHOTO_RENDER_UNSUPPORTED");
            }
            catch (Exception ex) { Debug.LogWarning("FIELD_PHOTO_RENDER_FAILED " + ex.Message); }
            finally { if (Chapter) Chapter.SetFilmResponse(false); }
            Capturing = false; Raised = false;
            if (!rendered) { busy = false; g.hud.Toast("EXPOSURE FAILED // NO FILM CONSUMED", 4); yield break; }
            if (shutter) sound.PlayOneShot(shutter, .28f);
            byte[] pixels = null;
            if (SystemInfo.supportsAsyncGPUReadback)
            {
                var request = AsyncGPUReadback.Request(exposureTarget, 0, TextureFormat.RGBA32);
                while (!request.done) yield return null;
                if (!request.hasError) pixels = request.GetData<byte>().ToArray();
            }
            else
            {
                var previous = RenderTexture.active; RenderTexture.active = exposureTarget;
                var copy = new Texture2D(record.width, record.height, TextureFormat.RGBA32, false);
                copy.ReadPixels(new Rect(0, 0, record.width, record.height), 0, 0); copy.Apply(); RenderTexture.active = previous;
                pixels = copy.GetRawTextureData<byte>().ToArray(); Destroy(copy);
            }
            if (pixels == null) { busy = false; g.hud.Toast("FILM READ FAILED // PLEASE EXPOSE AGAIN", 4); yield break; }
            FinishPhoto(pixels, record); busy = false;
        }
        void FinishPhoto(byte[] pixels, ExposureRecord record)
        {
            record.texture = new Texture2D(record.width, record.height, TextureFormat.RGBA32, false) { name = record.id };
            record.texture.LoadRawTextureData(pixels); record.texture.Apply(); record.pendingPixels = pixels;
            string dir = ChapterSave.PhotoArchiveDirectory;
            record.path = string.IsNullOrEmpty(dir) ? "" : Path.Combine(dir, (record.id == PhotoId ? "S03-" : "B12-") + DateTime.UtcNow.ToString("yyyyMMdd-HHmmss") + "-" + Guid.NewGuid().ToString("N").Substring(0, 8) + ".jpg");
            frames.Add(record); if (record.id == PhotoId) photograph = record.texture;
            BeginExport(record);
            var g = GameSession.Instance;
            g.notebook.Add((record.id == PhotoId ? "Frame 01" : "Frame 02") + " exposed. Sealed film awaits processing in the north photolab.");
            g.hud.Toast((record.id == PhotoId ? "FRAME 01" : "FRAME 02") + " EXPOSED // PROCESS FILM IN THE NORTH PHOTOLAB", 5);
            Debug.Log("FIELD_PHOTO_CAPTURED " + record.id + " " + record.width + "x" + record.height + " method=" + record.method);
            if (Chapter) Chapter.ExposureRecorded(record.id);
        }
        void BeginExport(ExposureRecord record)
        {
            if (saveJob != null || record.pendingPixels == null) return;
            if (string.IsNullOrEmpty(record.path) || !Path.IsPathRooted(record.path)) { SaveStatus = "EXPORT FAILED // PHOTO ARCHIVE LOCATION UNAVAILABLE"; GameSession.Instance.hud.Toast(SaveStatus, 6); return; }
            savingFrame = record; SaveStatus = "ARCHIVING EXPOSURE";
            string path = record.path; byte[] pixels = record.pendingPixels;
            // Only this detached snapshot is touched on the worker. The live evidence can
            // be developed/inspected while exporting; its immutable capture data cannot change.
            var archive = new ExposureRecord
            {
                id = record.id, path = record.path, utc = record.utc, localTime = record.localTime,
                build = record.build, method = record.method, subject = record.subject,
                position = record.position, forward = record.forward, referenceViewport = record.referenceViewport,
                echoViewport = record.echoViewport, width = record.width, height = record.height,
                developed = record.developed, inspected = record.inspected
            };
            uint width = (uint)record.width, height = (uint)record.height;
            saveJob = Task.Run(() =>
            {
                byte[] jpg = ImageConversion.EncodeArrayToJPG(pixels, GraphicsFormat.R8G8B8A8_UNorm, width, height, 0, 94);
                using (var digest = SHA256.Create()) archive.sha256 = BitConverter.ToString(digest.ComputeHash(jpg)).Replace("-", "").ToLowerInvariant();
                string dir = Path.GetDirectoryName(path); Directory.CreateDirectory(dir);
                if (File.Exists(path))
                {
                    // A retry may follow a completed JPEG but failed sidecar write. Preserve
                    // the original bytes and refuse to overwrite an unrelated archive file.
                    using (var digest = SHA256.Create())
                        if (BitConverter.ToString(digest.ComputeHash(File.ReadAllBytes(path))).Replace("-", "").ToLowerInvariant() != archive.sha256)
                            throw new IOException("An existing photo differs from this exposure; the archive was left unchanged.");
                }
                else { string tmp = path + ".tmp"; File.WriteAllBytes(tmp, jpg); File.Move(tmp, path); }
                archive.exported = true;
                string metadataPath = Path.ChangeExtension(path, ".json"), metadataTmp = metadataPath + ".tmp";
                // Unity documents ToJson as thread-safe for a plain object with no concurrent mutations.
                File.WriteAllText(metadataTmp, JsonUtility.ToJson(archive, true));
                if (File.Exists(metadataPath)) File.Replace(metadataTmp, metadataPath, null); else File.Move(metadataTmp, metadataPath);
                return archive.sha256;
            });
        }
        public void RetryExport()
        {
            if (saveJob != null) return;
            var frame = frames.Find(f => !f.exported && f.pendingPixels != null);
            if (frame != null) BeginExport(frame);
        }
        public bool Develop(string id)
        {
            var record = GetFrame(id); if (record == null || record.developed || !record.texture) return false;
            record.developed = true;
            GameSession.Instance.notebook.Collect(id, id == PhotoId ? "PHOTO 01 / S-03 contact print" : "PHOTO 02 / B-12 control print", "Processed contact print. Select to inspect the original exposure and its visible reference marks.");
            ChapterSave.RequestCheckpoint(); return true;
        }
        public void Inspect(string id) { var record = GetFrame(id); if (record != null && record.developed) record.inspected = true; }
        public void Compare() { if (HasPhoto && GetFrame(PhotoId).developed) { Compared = true; ChapterSave.RequestCheckpoint(); } }
        public bool DrawPhotograph(GUIStyle button, GUIStyle text) { if (Chapter) { Chapter.OpenEvidence(PhotoId); Chapter.DrawPanel(); } return false; }
        public string CaptureState() => JsonUtility.ToJson(new CameraState { acquired = Acquired, compared = Compared, rejected = RejectedFrames, frames = frames });
        public static bool ValidateState(string json, out string reason)
        {
            reason = "";
            try
            {
                var state = JsonUtility.FromJson<CameraState>(json);
                if (state == null || state.version != 1 || state.frames == null || state.frames.Count > 2) { reason = "Unsupported camera record."; return false; }
                var ids = new HashSet<string>();
                foreach (var frame in state.frames)
                    if (frame == null || (frame.id != PhotoId && frame.id != SecondPhotoId) || !ids.Add(frame.id) || frame.width < 1 || frame.height < 1 || frame.width > 4096 || frame.height > 4096 || string.IsNullOrEmpty(frame.path) || (frame.inspected && !frame.developed) || (frame.id == SecondPhotoId && frame.method != "passive" && frame.method != "active")) { reason = "Invalid exposure metadata."; return false; }
                if ((state.frames.Count > 0 && !state.acquired) || (ids.Contains(SecondPhotoId) && !ids.Contains(PhotoId))) { reason = "Exposure sequence is incomplete."; return false; }
                return true;
            }
            catch (Exception ex) { reason = "Unreadable camera record: " + ex.Message; return false; }
        }
        public void RestoreState(string json)
        {
            if (!ValidateState(json, out string reason)) throw new InvalidDataException(reason);
            var state = JsonUtility.FromJson<CameraState>(json);
            foreach (var old in frames) if (old.texture) Destroy(old.texture);
            frames.Clear(); photograph = null; Acquired = state.acquired; Compared = state.compared; RejectedFrames = state.rejected;
            bool missing = false;
            foreach (var frame in state.frames)
            {
                try
                {
                    if (!File.Exists(frame.path) || new FileInfo(frame.path).Length > 30 * 1024 * 1024) throw new IOException("Photo is missing or invalid.");
                    byte[] bytes = File.ReadAllBytes(frame.path);
                    if (!string.IsNullOrEmpty(frame.sha256))
                    {
                        using (var digest = SHA256.Create()) if (BitConverter.ToString(digest.ComputeHash(bytes)).Replace("-", "").ToLowerInvariant() != frame.sha256) throw new IOException("Photo checksum does not match its original exposure.");
                    }
                    frame.texture = new Texture2D(2, 2, TextureFormat.RGBA32, false) { name = frame.id };
                    if (!ImageConversion.LoadImage(frame.texture, bytes, false) || frame.texture.width != frame.width || frame.texture.height != frame.height) throw new IOException("Photo cannot be decoded at its recorded dimensions.");
                    frame.exported = true; frames.Add(frame); if (frame.id == PhotoId) photograph = frame.texture;
                }
                catch (Exception ex) { missing = true; if (frame.texture) Destroy(frame.texture); GameSession.Instance.notebook.RemoveEvidence(frame.id); Debug.LogWarning("FIELD_PHOTO_RECOVERABLE " + frame.id + " " + ex.Message); }
            }
            if (!HasPhoto && GetFrame(SecondPhotoId) != null)
            {
                var frame = GetFrame(SecondPhotoId); if (frame.texture) Destroy(frame.texture); frames.Remove(frame); GameSession.Instance.notebook.RemoveEvidence(SecondPhotoId);
            }
            if (missing) { Compared = false; SaveStatus = "PHOTO FILE MISSING // RETAKE THE AFFECTED FRAME"; GameSession.Instance.hud.Toast(SaveStatus, 8); }
            else SaveStatus = frames.Count > 0 ? "RESTORED FROM LOCAL PHOTO ARCHIVE" : "";
            Raised = false; Capturing = false; busy = false; if (Acquired) EnsureTarget();
        }
        void OnGUI()
        {
            var g = GameSession.Instance;
            if (!g || !Acquired || Capturing || g.hud.ModalOpen || !g.hud.Started || !Raised) return;
            var ivory = new Color(.92f, .89f, .72f); float w = Screen.width, h = Screen.height;
            void Line(float x, float y, float width, float height) { GUI.color = ivory; GUI.DrawTexture(new Rect(x, y, width, height), Texture2D.whiteTexture); GUI.color = Color.white; }
            for (int right = 0; right < 2; right++) for (int bottom = 0; bottom < 2; bottom++) { float x = w * (right == 0 ? .065f : .935f), y = h * (bottom == 0 ? .10f : .83f); Line(x - (right == 1 ? 45 : 0), y, 45, 2); Line(x, y - (bottom == 1 ? 45 : 0), 2, 45); }
            Line(w / 2 - 16, h / 2, 32, 1); Line(w / 2, h / 2 - 16, 1, 32);
            var style = new GUIStyle(GUI.skin.label) { font = g.hud.terminalFont, fontSize = Mathf.RoundToInt(h * .028f), normal = { textColor = ivory }, wordWrap = true };
            GUI.Label(new Rect(w * .03f, 18, w * .8f, 38), "S A R O / F I E L D  C A M E R A", style);
            GUI.color = new Color(.018f, .023f, .020f, .96f); GUI.DrawTexture(new Rect(0, h * .9f, w, h * .1f), Texture2D.whiteTexture); GUI.color = Color.white;
            GUI.Label(new Rect(w * .03f, h * .925f, w * .14f, 40), HasPhoto ? "FRAME 02" : "FRAME 01", style);
            string problem = FrameProblem(); GUI.Label(new Rect(w * .18f, h * .925f, w * .51f, 55), problem.Length == 0 ? "REFERENCE IN FRAME // READY" : problem, style);
            GUI.Label(new Rect(w * .73f, h * .925f, w * .26f, 45), "C LOWER   SPACE SHUTTER", style);
        }
        void OnDestroy()
        {
            if (chapter) chapter.SetFilmResponse(false);
            foreach (var frame in frames) if (frame.texture) Destroy(frame.texture);
            if (exposureTarget) { exposureTarget.Release(); Destroy(exposureTarget); }
        }
    }
}
