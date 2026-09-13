using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using UnityEngine;
using UnityEngine.Profiling;
using Signal47.Chapter;
using Signal47.Core;
using Signal47.Environment;
using Signal47.WorldCase22;

namespace Signal47.Debugging
{
    /// <summary>Opt-in, read-only visual/performance probe for the Station26 field.</summary>
    public sealed class Environment27Checks : MonoBehaviour
    {
        [Serializable] sealed class Report
        {
            public string result, method, unity, applicationVersion, buildId, os, cpu, gpu, renderer, graphicsApi, resolution, quality;
            public string profile, seedSource, seedCaseSha256;
            public bool preview, demoSeed, development, focusedThroughout, hudSuppressed, nativeInputClaim;
            public bool frameTimesRetained, gpuTimingAvailable, supportsAsyncGpuReadback, supportsComputeShaders;
            public int width, height, frames, screenshots, stallsOver50ms, uniqueTimingSamples, targetFrameRate, vSyncCount;
            public double warmupSeconds, measuredSeconds, averageFps, p50ms, p95ms, p99ms, maxMs, engineMemoryPeakMiB;
            public List<double> unscaledFrameTimesMs;
            public List<string> checks = new List<string>();
            public List<string> errors = new List<string>();
        }

        struct RoutePose
        {
            public string id, shot; public Vector3 body, target; public float seconds;
            public RoutePose(string id, string shot, Vector3 body, Vector3 target, float seconds)
            { this.id = id; this.shot = shot; this.body = body; this.target = target; this.seconds = seconds; }
        }
        [Serializable] sealed class SeedSelection { public string selected, sourceCaseSha256; }

        readonly Report report = new Report
        { method = "Station26 field camera route; completed WorldCase seed; six pre-run player-eye captures; five eased 24 s transitions (120 s); unscaled Update intervals; all samples retained; no native input claim" };
        readonly List<double> frameTimes = new List<double>(20000);
        readonly List<double> cpuTimes = new List<double>(20000);
        readonly List<double> gpuTimes = new List<double>(20000);
        readonly System.Text.StringBuilder timingCsv = new System.Text.StringBuilder();
        readonly FrameTiming[] latestTiming = new FrameTiming[1];
        string root; bool finished, recording, preview, demoSeed, allFocused = true;
        double lastTime, peakMemory; ulong lastTimingStamp; int screenshotCount;
        GameSession Game => GameSession.Instance;

        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        static void Init()
        {
            var args = System.Environment.GetCommandLineArgs();
            bool enabled = Array.IndexOf(args, "--signal47-environment27-checks") >= 0;
            bool quick = Array.IndexOf(args, "--signal47-environment27-preview") >= 0;
            bool demo = Array.IndexOf(args, "--signal47-environment27-demo-checks") >= 0;
            if ((enabled || quick || demo) && !FindFirstObjectByType<Environment27Checks>())
                new GameObject("Environment27Checks").AddComponent<Environment27Checks>();
        }

        void Awake()
        {
            DontDestroyOnLoad(gameObject); Application.runInBackground = true;
            lastTime = Time.realtimeSinceStartupAsDouble; Application.logMessageReceived += Log;
        }
        void OnDestroy() { Application.logMessageReceived -= Log; }
        void Log(string message, string stack, LogType type)
        {
            if ((type == LogType.Error || type == LogType.Exception || type == LogType.Assert) && report.errors.Count < 8)
                report.errors.Add(message);
        }
        void Update()
        {
            double now = Time.realtimeSinceStartupAsDouble;
            if (recording)
            {
                double interval = (now - lastTime) * 1000.0;
                if (interval >= 0) frameTimes.Add(interval);
                allFocused &= Application.isFocused;
                FrameTimingManager.CaptureFrameTimings();
                if (FrameTimingManager.GetLatestTimings(1, latestTiming) > 0 && latestTiming[0].frameStartTimestamp != lastTimingStamp)
                {
                    FrameTiming frame = latestTiming[0]; lastTimingStamp = frame.frameStartTimestamp;
                    if (frame.cpuFrameTime > 0) cpuTimes.Add(frame.cpuFrameTime);
                    if (frame.gpuFrameTime > 0) gpuTimes.Add(frame.gpuFrameTime);
                    timingCsv.Append(frameTimes.Count - 1).Append(',').Append(frame.frameStartTimestamp).Append(',')
                        .Append(frame.cpuFrameTime.ToString("F6", CultureInfo.InvariantCulture)).Append(',')
                        .Append(frame.gpuFrameTime.ToString("F6", CultureInfo.InvariantCulture)).Append(',')
                        .Append(GC.CollectionCount(0)).Append('\n');
                }
                peakMemory = Math.Max(peakMemory, Profiler.GetTotalAllocatedMemoryLong());
            }
            lastTime = now;
            if (!finished && report.errors.Count > 0) Finish(false, "Runtime error: " + report.errors[0]);
        }
        void Check(bool pass, string name)
        {
            if (!pass) throw new InvalidOperationException(name);
            report.checks.Add(name); Debug.Log("ENVIRONMENT27_PASS " + name);
        }
        IEnumerator WaitFor(Func<bool> condition, float seconds, string failure)
        {
            double until = Time.realtimeSinceStartupAsDouble + seconds;
            while (!condition() && Time.realtimeSinceStartupAsDouble < until) yield return null;
            Check(condition(), failure);
        }

        IEnumerator Start()
        {
            string[] args = System.Environment.GetCommandLineArgs();
            demoSeed = Array.IndexOf(args, "--signal47-environment27-demo-checks") >= 0;
            preview = demoSeed || Array.IndexOf(args, "--signal47-environment27-preview") >= 0;
            int saveIndex = Array.IndexOf(args, "--signal47-save-dir");
            try
            {
                if (saveIndex < 0 || saveIndex + 1 >= args.Length) throw new InvalidOperationException("Dedicated environment27 profile required");
                string storage = ChapterSave.StorageDirectory;
                if (!Path.GetFileName(storage).StartsWith("environment27-test-", StringComparison.Ordinal) || !File.Exists(Path.Combine(storage, "ALLOW_ENVIRONMENT27_TEST")))
                    throw new InvalidOperationException("Dedicated environment27 profile and ALLOW_ENVIRONMENT27_TEST marker required");
                root = Path.Combine(storage, "Evidence"); Directory.CreateDirectory(root);
                report.preview = preview; report.demoSeed = demoSeed; report.profile = storage; report.unity = Application.unityVersion; report.applicationVersion = Application.version;
                report.os = SystemInfo.operatingSystem; report.cpu = SystemInfo.processorType; report.gpu = SystemInfo.graphicsDeviceName;
                report.renderer = SystemInfo.graphicsDeviceType.ToString(); report.graphicsApi = SystemInfo.graphicsDeviceVersion;
                report.width = Screen.width; report.height = Screen.height; report.resolution = Screen.width + "x" + Screen.height;
                report.quality = QualitySettings.names[QualitySettings.GetQualityLevel()]; report.development = Debug.isDebugBuild;
                report.targetFrameRate = Application.targetFrameRate; report.vSyncCount = QualitySettings.vSyncCount;
                report.supportsAsyncGpuReadback = SystemInfo.supportsAsyncGPUReadback; report.supportsComputeShaders = SystemInfo.supportsComputeShaders;
                string buildStamp = Path.Combine(Application.dataPath, "../build-id.txt"); report.buildId = File.Exists(buildStamp) ? File.ReadAllText(buildStamp).Trim() : "unknown";
                string selection = Path.Combine(storage, "Seed/selection.json");
                if (File.Exists(selection))
                {
                    var selected = JsonUtility.FromJson<SeedSelection>(File.ReadAllText(selection));
                    report.seedSource = selected.selected; report.seedCaseSha256 = selected.sourceCaseSha256;
                }
                report.hudSuppressed = true; report.nativeInputClaim = false; report.frameTimesRetained = true;
            }
            catch (Exception error) { Finish(false, error.ToString()); yield break; }

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
            if (!finished) Finish(report.errors.Count == 0, report.errors.Count > 0 ? report.errors[0] : "");
        }

        IEnumerator Run()
        {
            yield return new WaitForSecondsRealtime(2f);
            Check(Game && Game.player && Game.player.viewCamera && Game.hud && Game.station && Game.worldCase, "Main scene contains player, HUD, WorldCase and Station26 components");
            var previousSession = GameSession.Instance;
            if (demoSeed)
            {
                Check(ChapterSave.HasSave && File.Exists(ChapterSave.SavePath), "Initialized demo profile contains its persistent checkpoint");
            }
            else
            {
                string seedPath = Path.Combine(ChapterSave.StorageDirectory, "Seed/case.json");
                Check(File.Exists(seedPath), "Disposable completed-WorldCase seed was staged");
                Check(!ChapterSave.HasSave, "Disposable profile begins without a live checkpoint");
                File.Copy(seedPath, ChapterSave.SavePath, false);
            }
            Check(ChapterSave.TryContinue(), "Completed WorldCase seed opens through the real Continue API");
            yield return WaitFor(() => GameSession.Instance != previousSession && !ChapterSave.IsRestoring, 25, "Seed Continue completes a scene restoration");
            yield return null;
            Check(WorldCaseController.IsDestinationPrepared(Game.worldCase.CaptureState()), "Loaded disposable seed contains a completed WorldCase destination");
            Check(Game.chapter && Game.chapter.Complete, "Loaded seed contains a completed chapter report");
            Check(Game.station.InStation && Game.station.stationRoot && Game.station.stationRoot.activeSelf, "Completed seed opens directly in the active Station26 field area");
            if (demoSeed)
            {
                Check(Game.fieldCamera && Game.fieldCamera.FrameCount == 2, "Demo profile restores exactly two historical photo frames");
                Check(!Game.station.P06Complete && !Game.station.P07Complete && !Game.station.P08Complete && !Game.station.P09Complete, "Demo profile clears all future Station26 deductions");
            }
            ContactSmokeChecks();
            RoutePose[] route = BuildRoute(Game.station);
            Game.hud.enabled = false; Game.player.enabled = false;
            yield return SoundSmokeChecks();
            foreach (RoutePose pose in route) yield return CaptureAtPose(pose);
            if (preview)
            {
                Check(screenshotCount == 6, "Preview captured six player-eye route vistas");
            }
            else
            {
                // Capture overhead is outside the benchmark. Warm up after the
                // six captures, then begin on a clean frame boundary.
                report.warmupSeconds = 12;
                SetCameraPose(route[0].body, route[0].target);
                yield return new WaitForSecondsRealtime(12f);
                timingCsv.Length = 0; timingCsv.Append("observation_frame,frame_timestamp,cpu_frame_ms,gpu_frame_ms,gc0_total\n");
                yield return null;
                lastTime = Time.realtimeSinceStartupAsDouble; recording = true;
                double routeStart = lastTime;
                for (int i = 0; i < route.Length - 1; i++)
                {
                    yield return MoveBetween(route[i], route[i + 1], 24f);
                }
                recording = false; report.measuredSeconds = Time.realtimeSinceStartupAsDouble - routeStart;
                Check(report.measuredSeconds >= 119.0, "Measured route retained at least 119 seconds");
                FinishPerformanceFiles(); Check(frameTimes.Count > 0, "Measured route retained actual unscaled frame intervals");
            }
            Check(report.errors.Count == 0, "No runtime errors during Environment27 probe");
        }

        RoutePose[] BuildRoute(Signal47.Station26.StationController station)
        {
            Vector3 origin = station.stationRoot.transform.position;
            Transform timing = GameObject.Find("Station26.TimingRecord")?.transform;
            Vector3 hutTarget = origin + new Vector3(0, 1.2f, 10.3f);
            return new[]
            {
                new RoutePose("arrival-wide", "01-arrival-wide", station.arrival.position, hutTarget, 24f),
                new RoutePose("marker", "02-marker", Body(station.markerTarget, 3.2f), station.markerTarget.position + Vector3.up * .25f, 24f),
                new RoutePose("lamp", "03-lamp", Body(station.testLamp.transform, 2.2f), station.testLamp.transform.position, 24f),
                new RoutePose("cable", "04-cable", Body(station.cableTarget, 2.2f), station.cableTarget.position, 24f),
                new RoutePose("hut-exterior", "05-hut-exterior", origin + new Vector3(0, .12f, 6.2f), hutTarget, 24f),
                new RoutePose("cabin-interior", "06-cabin-interior", origin + new Vector3(0, .12f, 12.0f), timing ? timing.position + Vector3.up * .2f : origin + new Vector3(0, 1.0f, 13.5f), 24f)
            };
        }
        static Vector3 Body(Transform target, float distance)
        { Vector3 value = target.position - Vector3.forward * distance; value.y = .12f; return value; }

        IEnumerator CaptureAtPose(RoutePose pose)
        {
            var player = Game.player;
            SetCameraPose(pose.body, pose.target);
            yield return null; yield return new WaitForEndOfFrame();
            Texture2D image = ScreenCapture.CaptureScreenshotAsTexture();
            bool captured = image != null;
            if (image)
            {
                File.WriteAllBytes(Path.Combine(root, pose.shot + ".png"), image.EncodeToPNG()); Destroy(image); screenshotCount++;
            }
            Check(captured, "Runtime player-eye vista captured: " + pose.id);
        }

        void SetCameraPose(Vector3 body, Vector3 target)
        {
            var player = Game.player; var camera = player.viewCamera;
            Vector3 eye = body + Vector3.up * camera.transform.localPosition.y;
            Quaternion rotation = Quaternion.LookRotation(target - eye);
            player.transform.SetPositionAndRotation(body, Quaternion.Euler(0, rotation.eulerAngles.y, 0));
            camera.transform.localRotation = Quaternion.Euler(Mathf.DeltaAngle(0, rotation.eulerAngles.x), 0, 0);
        }

        IEnumerator MoveBetween(RoutePose from, RoutePose to, float seconds)
        {
            double start = Time.realtimeSinceStartupAsDouble;
            while (Time.realtimeSinceStartupAsDouble - start < seconds)
            {
                float t = Mathf.Clamp01((float)((Time.realtimeSinceStartupAsDouble - start) / seconds));
                float eased = t * t * (3f - 2f * t);
                SetCameraPose(Vector3.Lerp(from.body, to.body, eased), Vector3.Lerp(from.target, to.target, eased));
                yield return null;
            }
            SetCameraPose(to.body, to.target);
        }

        void ContactSmokeChecks()
        {
            var controller = Game.player.GetComponent<CharacterController>();
            Check(controller && controller.height >= 1.5f && controller.height <= 2.2f && controller.radius >= .2f && controller.radius <= .5f, "Player contact capsule has human-scale height and radius");
            Check(Game.station.arrival && Game.station.markerTarget && Game.station.cableTarget && Game.station.testLamp && Game.station.stationRoot, "Station route anchors are assigned");
            int colliders = Game.station.stationRoot.GetComponentsInChildren<Collider>(true).Length;
            Check(colliders >= 20, "Station field retains a populated collision set");
            foreach (string name in new[] { "Transit", "LampControl", "CableInspection", "TimingRecord" })
            {
                GameObject target = GameObject.Find("Station26." + name); Collider collider = target ? target.GetComponent<Collider>() : null;
                Check(collider && collider.bounds.size.sqrMagnitude > .0025f, "Contact-scale collider exists: " + name);
            }
            Check(Physics.Raycast(Game.station.arrival.position + Vector3.up * .7f, Vector3.down, out RaycastHit hit, 1.5f), "Arrival pose has a physical ground contact");
        }

        IEnumerator SoundSmokeChecks()
        {
            var sound = Game.station.stationRoot.GetComponentInChildren<FieldSound27>(true);
            Check(sound && sound.wind && sound.generator, "FieldSound27 has both wind and generator sources assigned");
            yield return new WaitForSecondsRealtime(.5f);
            Check(sound.wind.isPlaying && sound.generator.isPlaying, "FieldSound27 sources are active after scene warmup");
        }

        void FinishPerformanceFiles()
        {
            report.frames = frameTimes.Count; report.unscaledFrameTimesMs = new List<double>(frameTimes); report.gpuTimingAvailable = gpuTimes.Count > 0; report.uniqueTimingSamples = cpuTimes.Count; report.focusedThroughout = allFocused;
            double total = Sum(frameTimes); report.averageFps = frameTimes.Count / Math.Max(.001, total / 1000.0);
            report.p50ms = Percentile(frameTimes, .50); report.p95ms = Percentile(frameTimes, .95); report.p99ms = Percentile(frameTimes, .99); report.maxMs = Percentile(frameTimes, 1.0);
            int stalls = 0; foreach (double value in frameTimes) if (value > 50) stalls++; report.stallsOver50ms = stalls; report.engineMemoryPeakMiB = peakMemory / 1048576.0;
            var csv = new System.Text.StringBuilder("frame,unscaled_interval_ms\n"); for (int i = 0; i < frameTimes.Count; i++) csv.Append(i).Append(',').Append(frameTimes[i].ToString("F6", CultureInfo.InvariantCulture)).Append('\n');
            File.WriteAllText(Path.Combine(root, "frames.csv"), csv.ToString()); File.WriteAllText(Path.Combine(root, "frame-timing.csv"), timingCsv.ToString());
            report.screenshots = screenshotCount; File.WriteAllText(Path.Combine(root, "performance.json"), JsonUtility.ToJson(report, true));
        }
        static double Sum(List<double> values) { double sum = 0; foreach (double value in values) sum += value; return sum; }
        static double Percentile(List<double> values, double percentile)
        {
            if (values.Count == 0) return 0; var sorted = new List<double>(values); sorted.Sort();
            return sorted[Math.Max(0, (int)Math.Ceiling(sorted.Count * percentile) - 1)];
        }
        void Finish(bool pass, string reason)
        {
            if (finished) return; finished = true; recording = false; StopAllCoroutines(); report.result = pass ? "PASS" : "FAIL: " + reason; report.screenshots = screenshotCount;
            if (root != null)
            {
                if (report.frames > 0 && !File.Exists(Path.Combine(root, "performance.json"))) FinishPerformanceFiles();
                File.WriteAllText(Path.Combine(root, "result.json"), JsonUtility.ToJson(report, true));
            }
            Application.Quit(pass ? 0 : 2);
        }
    }
}
