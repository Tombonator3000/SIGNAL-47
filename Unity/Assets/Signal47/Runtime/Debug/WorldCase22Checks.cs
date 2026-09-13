using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using UnityEngine;
using Signal47.Chapter;
using Signal47.Core;
using Signal47.WorldCase22;

namespace Signal47.Debugging
{
    // Opt-in disposable-profile checks. This exercises gameplay APIs, not native controls or reading comprehension.
    public sealed class WorldCase22Checks : MonoBehaviour
    {
        [Serializable] sealed class Report
        {
            public string result, method = "API-driven integrated P04/P05 with real checkpoint writes and scene reloads; no native input, blind reading or performance claim", unity, renderer;
            public List<string> checks = new();
            public List<string> errors = new();
        }
        [Serializable] sealed class Envelope { public string format, payload, sha256; public int version; }
        [Serializable] sealed class StringValue { public string value; }
        [Serializable] sealed class SavedFields { public string worldCase, chapter, camera; public bool worldCasePhotoRecovery; }
        [Serializable] sealed class CameraFields { public List<PhotoFields> frames; }
        [Serializable] sealed class PhotoFields { public string id, path; }
        readonly Report report = new();
        readonly Dictionary<string, byte[]> originalPhotos = new();
        string root, originalChapter;
        float deadline;
        int writes;
        bool finished;
        WorldCaseController Case => GameSession.Instance.worldCase;

        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        static void Init()
        {
            if (Array.IndexOf(System.Environment.GetCommandLineArgs(), "--signal47-worldcase22-checks") >= 0 && !FindFirstObjectByType<WorldCase22Checks>())
                new GameObject("WorldCase22Checks").AddComponent<WorldCase22Checks>();
        }
        void Awake()
        {
            DontDestroyOnLoad(gameObject); Application.runInBackground = true;
            deadline = Time.realtimeSinceStartup + 180;
            Application.logMessageReceived += Log;
        }
        void OnDestroy() { Application.logMessageReceived -= Log; }
        void Log(string message, string stack, LogType type)
        {
            if (message.StartsWith("CHAPTER_SAVE_OK ", StringComparison.Ordinal)) writes++;
            if ((type == LogType.Error || type == LogType.Exception || type == LogType.Assert) && report.errors.Count < 8)
                report.errors.Add(message);
        }
        void Update()
        {
            if (finished) return;
            if (report.errors.Count > 0) Finish(false, "Runtime error: " + report.errors[0]);
            else if (Time.realtimeSinceStartup > deadline) Finish(false, "Timeout");
        }
        void Check(bool pass, string name)
        {
            if (!pass) throw new InvalidOperationException(name);
            report.checks.Add(name); Debug.Log("WORLDCASE22_PASS " + name);
        }
        void Finish(bool pass, string reason)
        {
            if (finished) return;
            finished = true; StopAllCoroutines();
            report.result = pass ? "PASS" : "FAIL: " + reason;
            if (root != null) File.WriteAllText(Path.Combine(root, "result.json"), JsonUtility.ToJson(report, true));
            Debug.Log("WORLDCASE22_" + report.result); Application.Quit(pass ? 0 : 2);
        }
        IEnumerator Start()
        {
            var args = System.Environment.GetCommandLineArgs();
            if (Array.IndexOf(args, "--signal47-save-dir") < 0 || !Path.GetFileName(ChapterSave.StorageDirectory).StartsWith("worldcase22-test-", StringComparison.Ordinal)
                || !File.Exists(Path.Combine(ChapterSave.StorageDirectory, "ALLOW_WORLDCASE22_TEST")))
            { Finish(false, "Dedicated worldcase22-test-* profile and ALLOW_WORLDCASE22_TEST marker required"); yield break; }
            root = Path.Combine(ChapterSave.StorageDirectory, "Evidence"); Directory.CreateDirectory(root);
            report.unity = Application.unityVersion; report.renderer = SystemInfo.graphicsDeviceType + " / " + SystemInfo.graphicsDeviceName;
            // Flatten nested routines so exceptions in screenshot/save/reload helpers become an explicit failed report too.
            var stack = new Stack<IEnumerator>(); stack.Push(Run());
            while (stack.Count > 0 && !finished)
            {
                object next = null; bool advanced = false; Exception failure = null;
                try { advanced = stack.Peek().MoveNext(); if (advanced) next = stack.Peek().Current; }
                catch (Exception error) { failure = error; }
                if (failure != null) { Finish(false, failure.ToString()); yield break; }
                if (!advanced) { stack.Pop(); continue; }
                if (next is IEnumerator nested) stack.Push(nested); else yield return next;
            }
            Finish(report.errors.Count == 0, report.errors.Count > 0 ? report.errors[0] : "");
        }
        IEnumerator Shot(string name)
        {
            if (SystemInfo.graphicsDeviceType == UnityEngine.Rendering.GraphicsDeviceType.Null) yield break;
            yield return new WaitForSecondsRealtime(.2f); yield return new WaitForEndOfFrame();
            var image = ScreenCapture.CaptureScreenshotAsTexture();
            File.WriteAllBytes(Path.Combine(root, name + ".png"), image.EncodeToPNG()); Destroy(image);
        }
        IEnumerator PhysicalShot(string name, Vector3 position, Vector3 target, float fieldOfView)
        {
            var g = GameSession.Instance; var camera = g.player.viewCamera;
            var oldPosition = camera.transform.position; var oldRotation = camera.transform.rotation; float oldFov = camera.fieldOfView;
            bool playerEnabled = g.player.enabled, hudEnabled = g.hud.enabled;
            try
            {
                g.player.enabled = false; g.hud.enabled = false;
                camera.transform.SetPositionAndRotation(position, Quaternion.LookRotation(target - position)); camera.fieldOfView = fieldOfView;
                yield return Shot(name);
            }
            finally
            {
                camera.transform.SetPositionAndRotation(oldPosition, oldRotation); camera.fieldOfView = oldFov;
                g.player.enabled = playerEnabled; g.hud.enabled = hudEnabled;
            }
        }
        static string JsonString(string value)
        {
            string wrapper = JsonUtility.ToJson(new StringValue { value = value });
            return wrapper.Substring(wrapper.IndexOf(':') + 1).TrimEnd('}');
        }
        void CrossStateChecks()
        {
            Check(!ChapterSave.Saving, "Cross-state fixture begins with no in-flight checkpoint writer");
            string primary = ChapterSave.SavePath, backup = Path.Combine(ChapterSave.StorageDirectory, "case.backup.json");
            byte[] primaryBytes = File.ReadAllBytes(primary), backupBytes = File.Exists(backup) ? File.ReadAllBytes(backup) : null;
            try
            {
                var envelope = JsonUtility.FromJson<Envelope>(Encoding.UTF8.GetString(primaryBytes));
                var fields = JsonUtility.FromJson<SavedFields>(envelope.payload);
                Check(WorldCaseController.HasProgress(fields.worldCase) && fields.chapter.Contains("\"complete\":true"), "Cross-state fixture starts with valid progress and a filed report");
                string incomplete = fields.chapter.Replace("\"complete\":true", "\"complete\":false");
                Check(ChapterInvestigation.ValidateState(incomplete, out _) && !ChapterInvestigation.IsCompletedState(incomplete), "Modified chapter remains structurally valid but its report is unfiled");
                string changed = envelope.payload.Replace("\"chapter\":" + JsonString(fields.chapter), "\"chapter\":" + JsonString(incomplete));
                Check(changed != envelope.payload, "Only the serialized chapter field is replaced for the invariant fixture");
                envelope.payload = changed;
                using (var digest = SHA256.Create()) envelope.sha256 = BitConverter.ToString(digest.ComputeHash(Encoding.UTF8.GetBytes(changed))).Replace("-", "");
                string encoded = JsonUtility.ToJson(envelope, true);
                File.WriteAllText(primary, encoded); if (File.Exists(backup)) File.Delete(backup);
                Check(!ChapterSave.InspectSave(out string reason) && reason.Contains("Archive progress requires a filed two-exposure report"), "A checksum-valid checkpoint rejects archive progress with an unfiled original report");
                Check(File.ReadAllText(primary) == encoded, "Rejected cross-state inspection preserves the exact invalid checkpoint bytes");
            }
            finally
            {
                File.WriteAllBytes(primary, primaryBytes);
                if (backupBytes != null) File.WriteAllBytes(backup, backupBytes); else if (File.Exists(backup)) File.Delete(backup);
            }
            Check(Convert.ToBase64String(File.ReadAllBytes(primary)) == Convert.ToBase64String(primaryBytes)
                && (backupBytes == null ? !File.Exists(backup) : Convert.ToBase64String(File.ReadAllBytes(backup)) == Convert.ToBase64String(backupBytes)),
                "Cross-state test restores both valid checkpoint files byte for byte");
            Check(ChapterSave.InspectSave(out _), "Restored original checkpoint remains valid after negative inspection");
        }
        IEnumerator PreviousCaseRoundTrip()
        {
            var g = GameSession.Instance; string expected = Case.CaptureState();
            byte[] originalCheckpoint = File.ReadAllBytes(ChapterSave.SavePath);
            g.hud.SetPaused(true); g.hud.RequestNewShift();
            Check(g.hud.NewShiftConfirmation && g.hud.ConfirmNewShift(), "Confirmed New retires the completed WorldCase22 checkpoint through the real menu API");
            float until = Time.realtimeSinceStartup + 25;
            while (GameSession.Instance == g && Time.realtimeSinceStartup < until) yield return null;
            yield return new WaitForSecondsRealtime(.2f); g = GameSession.Instance;
            Check(!g.hud.Started && !ChapterSave.HasSave && !Case.P04Complete && !Case.P05Complete, "New case returns to a fresh menu without inheriting archived deductions");
            string selected = null;
            foreach (string id in ChapterSave.PreviousCaseIds(out _))
            {
                string candidate = Path.Combine(ChapterSave.StorageDirectory, "PreviousCases", id, "case.json");
                if (File.Exists(candidate) && Convert.ToBase64String(File.ReadAllBytes(candidate)) == Convert.ToBase64String(originalCheckpoint)) { selected = id; break; }
            }
            Check(selected != null && ChapterSave.InspectPreviousCase(selected).Available, "PreviousCases contains an available exact copy of the completed archive checkpoint");
            g.hud.OpenPreviousShifts(); g.hud.SelectPreviousShift(selected);
            Check(g.hud.ConfirmPreviousShift(), "Previous Shifts accepts the reviewed WorldCase22 checkpoint");
            until = Time.realtimeSinceStartup + 25;
            while ((GameSession.Instance == g || ChapterSave.IsRestoring) && Time.realtimeSinceStartup < until) yield return null;
            yield return null;
            Check(GameSession.Instance != g && !ChapterSave.IsRestoring && GameSession.Instance.hud.Started, "Previous Shifts completes an actual scene restoration");
            Check(Case.CaptureState() == expected && Case.P04Complete && Case.P05Complete, "PreviousCases transports every archive field without resetting either deduction");
            Check(Convert.ToBase64String(File.ReadAllBytes(ChapterSave.SavePath)) == Convert.ToBase64String(originalCheckpoint), "Previous-case continuation preserves exact checkpoint envelope bytes");
            Check(GameSession.Instance.chapter.CaptureState() == originalChapter && GameSession.Instance.fieldCamera.Frames.Count == 2, "Archived WorldCase22 restoration also preserves the original case and both photos");
        }
        IEnumerator MissingPhotoRecovery()
        {
            string primary = ChapterSave.SavePath, backup = Path.Combine(ChapterSave.StorageDirectory, "case.backup.json");
            byte[] primaryBytes = File.ReadAllBytes(primary), backupBytes = File.Exists(backup) ? File.ReadAllBytes(backup) : null;
            var envelope = JsonUtility.FromJson<Envelope>(Encoding.UTF8.GetString(primaryBytes));
            var fields = JsonUtility.FromJson<SavedFields>(envelope.payload);
            var camera = JsonUtility.FromJson<CameraFields>(fields.camera);
            string photo = null;
            foreach (var frame in camera.frames) if (frame.id == Signal47.Investigation.FieldCamera.SecondPhotoId) photo = frame.path;
            Check(photo != null && originalPhotos.ContainsKey(photo) && File.Exists(photo), "Photo-recovery fixture targets only the disposable copied second exposure");
            string preserved = photo + ".worldcase22-test-kept";
            Check(!File.Exists(preserved) && !ChapterSave.Saving, "Photo-recovery preservation path is fresh and checkpoint writer is idle");
            string expectedArchive = Case.CaptureState();
            File.Move(photo, preserved);
            try
            {
                yield return Reload("Missing second photo");
                Check(GameSession.Instance.fieldCamera.Frames.Count == 1 && GameSession.Instance.fieldCamera.SaveStatus.StartsWith("PHOTO FILE MISSING", StringComparison.Ordinal)
                    && !GameSession.Instance.chapter.Complete, "Missing copied photo enters the existing retake recovery flow");
                Check(Case.CaptureState() == expectedArchive && Case.P04Complete && Case.P05Complete && !Case.Available && !Case.Open(), "Photo recovery retains archive findings while blocking access until the local report is repaired");
                yield return SaveAndReload("Photo recovery checkpoint", false);
                var saved = JsonUtility.FromJson<SavedFields>(JsonUtility.FromJson<Envelope>(File.ReadAllText(primary)).payload);
                Check(saved.worldCasePhotoRecovery, "Checkpoint explicitly records the archive photo-recovery exception");
                Check(!GameSession.Instance.chapter.Complete && !Case.Available && Case.CaptureState() == expectedArchive && ChapterSave.CanSave,
                    "Recovery checkpoint can Continue and save again without losing archive findings");
            }
            finally
            {
                if (File.Exists(preserved)) File.Move(preserved, photo);
                File.WriteAllBytes(primary, primaryBytes);
                if (backupBytes != null) File.WriteAllBytes(backup, backupBytes); else if (File.Exists(backup)) File.Delete(backup);
            }
            Check(Convert.ToBase64String(File.ReadAllBytes(photo)) == Convert.ToBase64String(originalPhotos[photo]), "Photo-recovery fixture restores exact copied exposure bytes");
            yield return Reload("Original recovery fixture restored");
            Check(GameSession.Instance.chapter.Complete && GameSession.Instance.fieldCamera.Frames.Count == 2 && Case.Available && Case.CaptureState() == expectedArchive,
                "Restoring fixture files reopens the complete original case and archived deductions");
        }
        IEnumerator Reload(string label)
        {
            var previous = GameSession.Instance;
            Check(ChapterSave.TryContinue(), label + ": real Continue accepts checkpoint");
            float until = Time.realtimeSinceStartup + 25;
            while ((GameSession.Instance == previous || ChapterSave.IsRestoring) && Time.realtimeSinceStartup < until) yield return null;
            yield return null;
            Check(GameSession.Instance != previous && !ChapterSave.IsRestoring && GameSession.Instance.hud.Started, label + ": a new scene session has resumed");
            Check(Case && !Case.ModalOpen, label + ": archive is restored with its modal closed");
        }
        IEnumerator SaveAndReload(string label, bool originalCaseMustMatch = true)
        {
            string expected = Case.CaptureState(); Case.Close();
            Check(ChapterSave.CanSave, label + ": normal checkpoint is available");
            int previousWrites = writes; ChapterSave.RequestCheckpoint();
            float until = Time.realtimeSinceStartup + 15;
            while ((writes == previousWrites || ChapterSave.Saving) && Time.realtimeSinceStartup < until) yield return null;
            Check(writes > previousWrites && !ChapterSave.Saving, label + ": normal writer completed");
            var envelope = JsonUtility.FromJson<Envelope>(File.ReadAllText(ChapterSave.SavePath));
            var fields = JsonUtility.FromJson<SavedFields>(envelope.payload);
            Check(fields.worldCase == expected, label + ": checkpoint contains the exact archive state");
            yield return Reload(label);
            Check(Case.CaptureState() == expected, label + ": archive progress survives disk and scene restoration");
            if (originalCaseMustMatch) Check(GameSession.Instance.chapter.CaptureState() == originalChapter, label + ": original two-exposure investigation is unchanged");
        }
        IEnumerator Run()
        {
            yield return new WaitForSecondsRealtime(2);
            var g = GameSession.Instance;
            Check(g && g.worldCase && g.chapter && g.fieldCamera, "Main scene contains the archive controller and original investigation components");
            Check(!ChapterSave.HasSave && !g.hud.Started, "Disposable profile starts at a fresh menu");
            var worldRoot = GameObject.Find("WorldCase22"); var folio = GameObject.Find("WorldCase22.Folio"); var hitObject = GameObject.Find("WorldCase22.FolioHit");
            Check(worldRoot && folio && hitObject && worldRoot.GetComponent<WorldCaseController>() == Case, "Main scene has the physical folio and its assigned archive controller");
            var colliders = FindObjectsByType<Collider>(FindObjectsSortMode.None); int additions = 0;
            foreach (var collider in colliders) if (collider.transform.IsChildOf(worldRoot.transform)) additions++;
            Check(additions == 1 && colliders.Length - additions - Array.FindAll(colliders,c=>GameSession.Instance.station && c.transform.IsChildOf(GameSession.Instance.station.transform)).Length == 154 && LightmapSettings.lightmaps.Length == 2, "Archive adds one interaction collider while preserving the 154 original colliders and two lightmaps");
            var renderers = folio.GetComponentsInChildren<Renderer>(); Check(renderers.Length > 0, "Folio has visible model geometry");
            var bounds = renderers[0].bounds; foreach (var renderer in renderers) bounds.Encapsulate(renderer.bounds);
            var bench = GameObject.Find("CH09.ArchiveBench.FormicaTop").GetComponent<Renderer>().bounds;
            float supportGap = bounds.min.y - bench.max.y;
            Check(supportGap >= -.001f && supportGap <= .003f, "Physical folio rests within 3 mm of the actual archive bench surface");
            Check(bounds.min.x >= bench.min.x && bounds.max.x <= bench.max.x && bounds.min.z >= bench.min.z && bounds.max.z <= bench.max.z,
                "The visible folio footprint stays on the archive bench");
            var eye = new Vector3(1.15f, 1.65f, 10.4f);
            Check(Physics.Raycast(new Ray(eye, bounds.center - eye), out var hit, 2.65f) && hit.collider.gameObject == hitObject,
                "A player-height ray aimed at the visible folio reaches its interaction collider");
            Check(hitObject.GetComponent<WorldCaseAction>().controller == Case, "Physical folio action targets the integrated archive");
            Check(!Case.Available && !Case.Open() && !Case.SubmitComparison("omitted-c"), "Incomplete original case cannot open or solve P04");
            Check(!Case.SelectPage("original") && !Case.ReadOriginal, "Closed archive cannot mark documents read");
            Check(WorldCaseController.ValidateState(null, out _) && WorldCaseController.ValidateState("", out _), "Missing optional archive state preserves historical checkpoint compatibility");
            Check(!WorldCaseController.ValidateState("{\"version\":99}", out _), "Unknown archive version is rejected");
            Check(!WorldCaseController.ValidateState("{\"version\":1,\"p04Complete\":true,\"finding\":\"omitted-c\"}", out _), "A comparison without its read source records is rejected");
            Check(!WorldCaseController.ValidateState("{\"version\":1,\"p05Complete\":true,\"readMaintenance\":true,\"readIndex\":true,\"destination\":\"old-survey-station\",\"surveyId\":\"STATION 01\",\"marker\":\"triangle-bar\"}", out _), "A route without its prerequisite comparison is rejected");
            string seed = Path.Combine(ChapterSave.StorageDirectory, "Seed", "case.json");
            Check(File.Exists(seed), "Historical completed case fixture exists");
            foreach (string path in Directory.GetFiles(ChapterSave.PhotoArchiveDirectory, "*.jpg")) originalPhotos.Add(path, File.ReadAllBytes(path));
            Check(originalPhotos.Count == 2, "Fixture contains both original photographs");
            File.Copy(seed, ChapterSave.SavePath, false); yield return Reload("Historical v1"); g = GameSession.Instance;
            Check(g.chapter.Complete && g.fieldCamera.Frames.Count == 2 && g.fieldCamera.SaveStatus == "RESTORED FROM LOCAL PHOTO ARCHIVE", "Historical completed investigation and two actual photo textures restore");
            originalChapter = g.chapter.CaptureState();
            yield return PhysicalShot("00a-physical-folio", new Vector3(1.30f, 1.62f, 10.30f), new Vector3(2.65f, 1.0f, 10.40f), 48);
            yield return PhysicalShot("00b-archive-area", new Vector3(.30f, 1.78f, 9.10f), new Vector3(2.7f, 1.15f, 11.10f), 65);
            Check(Case.Available && !Case.P04Complete && !Case.P05Complete && !Case.ReadOriginal && !Case.ReadAmended, "Historical v1 case unlocks an unread unsolved archive");
            g.hud.SetPaused(true); Check(!Case.Open(), "Paused session cannot open the archive behind the pause panel"); g.hud.SetPaused(false);
            GameObject.Find("WorldCase22.FolioHit").GetComponent<WorldCaseAction>().Interact();
            Check(Case.ModalOpen && Case.ReadOriginal && !Case.ReadAmended && !g.CanControl, "Physical folio action opens only the first source and locks player control");
            yield return Shot("01-original");
            Check(Case.SelectPage("compare") && !Case.SubmitComparison("omitted-c") && !Case.P04Complete, "P04 cannot be solved after only one source");
            string before = Case.CaptureState(); string page = Case.CurrentPage;
            Check(!Case.SelectPage("invalid") && Case.CurrentPage == page && Case.CaptureState() == before, "Unknown page preserves both document and progress");
            Case.Close(); Check(!Case.ModalOpen && !Case.P04Complete && g.CanControl, "Cancel restores player control without solving P04");
            yield return SaveAndReload("Partial reading");
            Check(Case.ReadOriginal && !Case.ReadAmended && !Case.P04Complete, "Partial reading remains partial after Continue");
            Check(Case.Open() && Case.SelectPage("amended") && Case.ReadAmended, "Amended source remains reachable after partial restoration");
            yield return Shot("02-amended");
            Check(Case.SelectPage("maintenance") && Case.SelectPage("index") && Case.SelectPage("route"), "P05 sources can be investigated before solving P04");
            Check(!Case.SubmitDestination("old-survey-station", "STATION 01", "triangle-bar") && !Case.P05Complete, "Correct destination references cannot bypass the unresolved P04 prerequisite");
            Check(Case.SelectPage("compare"), "Both P04 sources can be compared after visiting P05"); yield return Shot("03-comparison");
            Check(!Case.SubmitComparison("author-guilt") && !Case.P04Complete && Case.Feedback.Length > 0, "Unsupported guilt claim supplies feedback without progression"); yield return Shot("04-unsupported-motive");
            Check(!Case.SubmitComparison("development-only") && !Case.P04Complete, "Repeating the amended explanation is not a documented comparison");
            Check(!Case.SubmitComparison(null) && !Case.P04Complete, "Missing comparison choice is retryable");
            Check(Case.SubmitComparison("omitted-c") && Case.P04Complete && !Case.P05Complete && Case.Feedback.Contains("motive remains unknown"), "P04 records the omitted reference and explicitly leaves motive unknown");
            yield return Shot("05-p04-supported");
            yield return SaveAndReload("P04 complete");
            Check(Case.P04Complete && !Case.P05Complete, "Solved P04 does not silently solve P05 on load");
            // A second valid checkpoint fixture isolates both-document gating from earlier out-of-order reading.
            string p04Only = "{\"version\":1,\"readOriginal\":true,\"readAmended\":true,\"p04Complete\":true,\"finding\":\"omitted-c\"}";
            Case.RestoreState(p04Only);
            Check(Case.Open() && Case.ReadMaintenance && !Case.ReadIndex && Case.SelectPage("route"), "P05 starts with only the maintenance source read");
            Check(!Case.SubmitDestination("old-survey-station", "STATION 01", "triangle-bar") && !Case.P05Complete, "P05 cannot be solved without the archive index even with correct guesses");
            Check(Case.SelectPage("maintenance"), "Maintenance card is available for review"); yield return Shot("06-maintenance");
            Check(Case.SelectPage("index") && Case.ReadIndex, "Archive index supplies the second source"); yield return Shot("07-index");
            Check(Case.SelectPage("route"), "Route evidence form can be opened"); yield return Shot("08-route-unsolved");
            Check(!Case.SubmitDestination("old-survey-station", "", "") && !Case.P05Complete, "Place name alone does not establish the route");
            Check(!Case.SubmitDestination("old-survey-station", "-39 LY", "triangle-bar") && !Case.P05Complete, "Signal offset cannot substitute for the documented survey ID");
            Check(!Case.SubmitDestination("old-survey-station", "STATION 01", "triangle") && !Case.P05Complete, "Elevation triangle without the bar is not accepted as the fixed-point mark"); yield return Shot("09-wrong-marker");
            Check(!Case.SubmitDestination("motel", "STATION 01", "triangle-bar") && !Case.P05Complete, "Correct references cannot justify an unrelated motel destination");
            Case.Close(); Check(!Case.P05Complete && Case.ReadIndex && Case.ReadMaintenance, "Cancel after wrong routes retains sources without committing a destination");
            Check(!Case.SubmitDestination("old-survey-station", "STATION 01", "triangle-bar"), "Closed route form cannot complete P05");
            Check(Case.Open() && Case.SelectPage("route") && Case.SubmitDestination("old-survey-station", "STATION 01", "triangle-bar") && Case.P05Complete, "Matched destination, survey ID and fixed-point mark complete P05");
            Check(!GameSession.Instance.Transitioning && (GameSession.Instance.station ? !GameSession.Instance.station.InStation : Case.Objective.Contains("NOT YET PLAYABLE")), "Preparing field access retains SARO until a separate travel action"); yield return Shot("10-p05-supported");
            before = Case.CaptureState();
            Check(Case.SubmitDestination("old-survey-station", "STATION 01", "triangle-bar") && Case.CaptureState() == before, "Resubmitting supported destination is idempotent");
            yield return SaveAndReload("P05 complete");
            Check(Case.P04Complete && Case.P05Complete && Case.ReadMaintenance && Case.ReadIndex, "Both deductions and all source visits survive checkpoint continuation");
            Check(Case.Open() && Case.CurrentPage == "route", "Reopened completed archive shows the prepared field access"); yield return Shot("11-restored-route"); Case.Close();
            CrossStateChecks(); yield return PreviousCaseRoundTrip(); yield return MissingPhotoRecovery();
            Check(GameSession.Instance.chapter.Complete && GameSession.Instance.fieldCamera.Frames.Count == 2, "Original local case remains complete with exactly two exposures");
            foreach (var pair in originalPhotos)
                Check(Convert.ToBase64String(File.ReadAllBytes(pair.Key)) == Convert.ToBase64String(pair.Value), "Original photo bytes preserved: " + Path.GetFileName(pair.Key));
            Check(report.errors.Count == 0, "No runtime errors were recorded during the integrated journey");
        }
    }
}
