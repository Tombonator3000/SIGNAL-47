using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Security.Cryptography;
using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using Gsplat;

/// <summary>Isolated hybrid-scene trial. Its captures are diagnostic exposures, never SIGNAL47 case evidence.</summary>
public sealed class Hybrid23Runtime : MonoBehaviour
{
    public Camera cam;
    public CharacterController player;
    public GameObject splatsRoot, cyanOccluder;
    public GameObject generatedMeshRoot;
    public GsplatProxyRelighting relighting;
    public Transform door, playerStart, doorTestStart, wallTestStart, photoPose;
    public Collider doorCollider, wall;
    public Light lamp;
    public string dataOrigin = "UNSPECIFIED";
    public int declaredSplatCount;

    [Serializable] public sealed class Check { public string name, detail; public bool passed; }
    [Serializable] public sealed class Phase
    {
        public string name, representation; public bool splats, light; public int frames, splatCount;
        public double seconds, meanMs, p95Ms, p99Ms;
        public int rendererCount; public long assetCount, uploadedCount; public bool assetsReady;
    }
    [Serializable] public sealed class Exposure
    {
        public string name, file, sha256, utc, method, dataOrigin, representation;
        public Vector3 position, forward; public int width, height; public bool splats, light;
        public double meanLuma;
        public bool proxyConfigured, proxyReady, proxyRenderSubmitted, samePoseProjection;
    }
    [Serializable] public sealed class Result
    {
        public string unity, renderer, gpu, dataOrigin, scope, failureReason;
        public int width, height, errors, declaredSplatCount;
        public bool completed, automated, compute, generatedMeshComparison;
        public int splatRegionLightPixels, splatRegionLightChangedPixels; public double splatRegionMeanAbsoluteRgbChange;
        public string splatRegionInterpretation = "Pixels selected by common-scene/hybrid A-B at identical light settings; a light response here can still include translucent underlying meshes and requires visual inspection.";
        public string proxyPhotoScope = "No proxy relighting configured";
        public string nativeInput = "UNVERIFIED", visualOcclusion = "UNVERIFIED: inspect original A/B captures", splatRelighting = "UNVERIFIED: mesh-light response does not prove splat relighting";
        public List<Check> checks = new List<Check>();
        public List<Phase> phases = new List<Phase>();
        public List<Exposure> exposures = new List<Exposure>();
    }
    readonly Result result = new Result();
    readonly List<string> errors = new List<string>();
    string output, status = "WASD / MOVE   MOUSE / LOOK   E / DOOR   F / LIGHT   G / SPLATS   C / DIAGNOSTIC PHOTO   ESC / CURSOR";
    int errorCount, photoNumber;
    bool automated, failed, doorOpen, capturing, initialized;
    double deadline;
    float yaw, pitch, vertical;
    Quaternion doorClosed;
    Camera exposureCamera;
    RenderTexture exposureTarget;
    Color32[] firstLightPixels, meshLightPixels, commonOnlyPixels;
    bool[] splatContributionMask;

    void Awake() { Application.logMessageReceived += OnError; }
    void OnDestroy()
    {
        Application.logMessageReceived -= OnError;
        if (exposureTarget) { exposureTarget.Release(); Destroy(exposureTarget); }
        if (exposureCamera) Destroy(exposureCamera.gameObject);
    }
    void OnError(string message, string stack, LogType type)
    {
        if (type != LogType.Error && type != LogType.Exception && type != LogType.Assert) return;
        errorCount++;
        if (errors.Count < 16) errors.Add(message.Length > 2048 ? message.Substring(0, 2048) : message);
    }
    void Abort(string reason, int code = 1)
    {
        if (failed) return;
        failed = true; result.failureReason = reason; status = reason;
        if (splatsRoot) splatsRoot.SetActive(false);
        Save(); Application.Quit(code);
    }
    bool Healthy()
    {
        if (failed) return false;
        if (errorCount > 0) { Abort("Runtime error; stopped at first observed error."); return false; }
        if (automated && Time.realtimeSinceStartupAsDouble > deadline) { Abort("The 90-second API-test deadline was exceeded.", 3); return false; }
        return true;
    }
    bool Save()
    {
        if (string.IsNullOrEmpty(output)) return false;
        try
        {
            result.errors = errorCount;
            File.WriteAllText(Path.Combine(output, "result.json"), JsonUtility.ToJson(result, true));
            File.WriteAllLines(Path.Combine(output, "errors.txt"), errors);
            return true;
        }
        catch (Exception error) when (error is IOException || error is UnauthorizedAccessException || error is ArgumentException)
        { result.failureReason = "Cannot write diagnostic results: " + error.Message; return false; }
    }
    void Record(string name, bool passed, string detail)
    { result.checks.Add(new Check { name = name, passed = passed, detail = detail }); Save(); }
    IEnumerator Start()
    {
        string[] args = Environment.GetCommandLineArgs(); automated = Array.IndexOf(args, "--hybrid23-checks") >= 0;
        int outIndex = Array.IndexOf(args, "--hybrid23-out");
        deadline = Time.realtimeSinceStartupAsDouble + 90;
        try
        {
            if (outIndex >= 0 && (outIndex + 1 >= args.Length || args[outIndex + 1].StartsWith("--", StringComparison.Ordinal))) throw new ArgumentException("--hybrid23-out needs a separate output directory.");
            output = outIndex >= 0 ? Path.GetFullPath(args[outIndex + 1]) : Path.Combine(Application.persistentDataPath, "Hybrid23Diagnostics", DateTime.UtcNow.ToString("yyyyMMdd-HHmmss") + "-" + Guid.NewGuid().ToString("N").Substring(0, 8));
            Directory.CreateDirectory(output);
            if (File.Exists(Path.Combine(output, "result.json")) || Directory.GetFiles(output, "hybridprobe-exposure-*").Length > 0 || Directory.GetFiles(output, "*-frame-ms.csv").Length > 0) throw new IOException("Refusing to overwrite an existing Hybrid23 result or partial capture set.");
        }
        catch (Exception error) when (error is IOException || error is UnauthorizedAccessException || error is ArgumentException)
        { output = null; Abort("Diagnostic output unavailable: " + error.Message, 2); }
        if (failed) yield break;
        result.unity = Application.unityVersion; result.renderer = SystemInfo.graphicsDeviceType.ToString(); result.gpu = SystemInfo.graphicsDeviceName;
        result.scope = "Isolated STATION 01 hybrid probe; synthetic or imported environment as explicitly identified. API motion is not native input. Short frame intervals are not game performance. JPEG files are separate diagnostic exposures, not canonical field photographs.";
        result.generatedMeshComparison = generatedMeshRoot != null;
        if (generatedMeshRoot) result.scope += " Generated-model comparison: source textured mesh is visible only in mesh phases; surface-derived Gaussians replace that mesh in hybrid phases. These Gaussians are sampled from the model surface, not a trained reconstruction or Marble world. Common collision, door and interior geometry stays active. A common-only exposure independently tests visible splat contribution.";
        else result.scope += " Synthetic comparison: common ordinary meshes stay visible; Gaussian reference content is toggled on/off.";
        if (relighting) result.proxyPhotoScope = "Matched-mesh proxy trial. Separate camera inherits the player camera pose, projection and 1280x800 viewport. Its lighting map is explicitly refreshed and rendered immediately before the photo request. This tests same-pose photos only; independent camera poses and general photographic consistency remain unverified.";
        result.dataOrigin = dataOrigin; result.declaredSplatCount = declaredSplatCount; result.automated = automated; result.compute = SystemInfo.supportsComputeShaders;
        if (!cam || !player || !splatsRoot || !door || !doorCollider || !lamp || !wall || !playerStart || !wallTestStart || !doorTestStart || !photoPose)
        { Abort("Missing required hybrid-scene reference.", 2); yield break; }
        if (SystemInfo.graphicsDeviceType != GraphicsDeviceType.Vulkan || !SystemInfo.supportsComputeShaders)
        { Abort("This bounded trial requires Vulkan and compute support; other backends are unverified.", 2); yield break; }
        doorClosed = door.localRotation;
        SetDoor(false); lamp.enabled = true; SetRepresentation(automated || !generatedMeshRoot);
        Screen.SetResolution(1280, 800, false);
        QualitySettings.vSyncCount = 0; Application.targetFrameRate = automated ? -1 : 60;
        SetPose(playerStart); initialized = true;
        Cursor.lockState = automated ? CursorLockMode.None : CursorLockMode.Locked; Cursor.visible = automated;
        yield return null; yield return null;
        if (!Healthy()) yield break;
        result.width = Screen.width; result.height = Screen.height;
        Record("capture-resolution", Screen.width == 1280 && Screen.height == 800, Screen.width + "x" + Screen.height);
        Record("data-origin-labelled", !string.IsNullOrWhiteSpace(dataOrigin) && dataOrigin != "UNSPECIFIED", dataOrigin);
        Record("splat-count-declared", declaredSplatCount > 0, declaredSplatCount + " splats declared by the scene builder; this is not by itself proof of rendered splats.");
        if (generatedMeshRoot)
        {
            var meshes = generatedMeshRoot.GetComponentsInChildren<MeshFilter>(true);
            bool meshPresent = Array.Exists(meshes, mesh => mesh.sharedMesh && mesh.sharedMesh.vertexCount > 0 && mesh.GetComponent<Renderer>() && mesh.GetComponent<Renderer>().enabled);
            Record("generated-source-mesh-present", meshPresent, meshes.Length + " source MeshFilters; at least one must contain renderable vertices.");
            bool separate = !splatsRoot.transform.IsChildOf(generatedMeshRoot.transform) && !generatedMeshRoot.transform.IsChildOf(splatsRoot.transform) && !door.IsChildOf(generatedMeshRoot.transform) && !wall.transform.IsChildOf(generatedMeshRoot.transform) && !player.transform.IsChildOf(generatedMeshRoot.transform) && !lamp.transform.IsChildOf(generatedMeshRoot.transform) && (!cyanOccluder || !cyanOccluder.transform.IsChildOf(generatedMeshRoot.transform)) && generatedMeshRoot.GetComponentsInChildren<Collider>(true).Length == 0 && splatsRoot.GetComponentsInChildren<Collider>(true).Length == 0;
            Record("visual-comparison-preserves-common-physics", separate, "Generated visual root must be separate from splats, door, wall, player, lamp and opaque comparison object. Both switched visual roots must contain no colliders so A/B preserves common physics.");
        }
        if (!automated) { Save(); yield break; }
        yield return RunChecks();
    }
    void Update()
    {
        if (!Healthy() || !initialized || automated) return;
        var keys = Keyboard.current; var mouse = Mouse.current;
        if (keys != null && keys.escapeKey.wasPressedThisFrame)
        { bool locked = Cursor.lockState == CursorLockMode.Locked; Cursor.lockState = locked ? CursorLockMode.None : CursorLockMode.Locked; Cursor.visible = locked; }
        if (Cursor.lockState != CursorLockMode.Locked || capturing) return;
        if (mouse != null)
        {
            Vector2 delta = mouse.delta.ReadValue(); yaw += delta.x * .1f; pitch = Mathf.Clamp(pitch - delta.y * .1f, -75, 75);
            player.transform.rotation = Quaternion.Euler(0, yaw, 0); cam.transform.localRotation = Quaternion.Euler(pitch, 0, 0);
        }
        Vector3 motion = Vector3.zero;
        if (keys != null)
        {
            float x = (keys.dKey.isPressed ? 1 : 0) - (keys.aKey.isPressed ? 1 : 0), z = (keys.wKey.isPressed ? 1 : 0) - (keys.sKey.isPressed ? 1 : 0);
            motion = (player.transform.right * x + player.transform.forward * z).normalized * 2.8f;
            if (keys.eKey.wasPressedThisFrame) TryDoor();
            if (keys.fKey.wasPressedThisFrame) { lamp.enabled = !lamp.enabled; status = lamp.enabled ? "LOCAL LAMP ON" : "LOCAL LAMP OFF"; }
            if (keys.gKey.wasPressedThisFrame) { SetRepresentation(!splatsRoot.activeSelf); status = splatsRoot.activeSelf ? "SPLAT REPRESENTATION ON" : generatedMeshRoot ? "SOURCE MESH REFERENCE ON" : "SPLAT REFERENCE OFF"; }
            if (keys.cKey.wasPressedThisFrame) StartCoroutine(CaptureExposure("manual-" + (++photoNumber).ToString("D3"), false));
        }
        vertical = player.isGrounded && vertical < 0 ? -2 : Mathf.Max(vertical - 9.81f * Time.deltaTime, -20);
        player.Move((motion + Vector3.up * vertical) * Time.deltaTime);
    }
    public void SetRepresentation(bool showSplats, bool includeGeneratedReference = true)
    {
        if (generatedMeshRoot) generatedMeshRoot.SetActive(!showSplats && includeGeneratedReference);
        if (splatsRoot) splatsRoot.SetActive(showSplats);
    }
    string Representation => generatedMeshRoot ? splatsRoot.activeSelf ? "surface-derived Gaussians + common ordinary scene" : generatedMeshRoot.activeSelf ? "source textured mesh + common ordinary scene" : "common ordinary scene only / visibility control" : splatsRoot.activeSelf ? "synthetic Gaussian reference + ordinary scene" : "ordinary scene only / synthetic reference disabled";
    public bool TryDoor()
    {
        if (!cam || !Physics.Raycast(cam.transform.position, cam.transform.forward, out var hit, 2.8f, Physics.DefaultRaycastLayers, QueryTriggerInteraction.Ignore) || hit.collider != doorCollider) return false;
        SetDoor(!doorOpen); status = doorOpen ? "SURVEY HUT / DOOR OPEN" : "SURVEY HUT / DOOR CLOSED"; return true;
    }
    public void SetDoor(bool open)
    {
        if (!door) return;
        doorOpen = open; door.localRotation = doorClosed * Quaternion.Euler(0, open ? 90 : 0, 0);
        Physics.SyncTransforms();
    }
    void SetPose(Transform pose)
    {
        player.enabled = false; player.transform.SetPositionAndRotation(pose.position, Quaternion.Euler(0, pose.eulerAngles.y, 0)); player.enabled = true;
        yaw = pose.eulerAngles.y; pitch = 0; vertical = 0; cam.transform.localRotation = Quaternion.identity; Physics.SyncTransforms();
    }
    IEnumerator TestMove(string name, Transform start, bool blocked)
    {
        SetPose(start); Vector3 before = player.transform.position; var flags = CollisionFlags.None;
        for (int i = 0; i < 40; i++)
        {
            flags |= player.Move(start.forward * .05f + Vector3.down * .003f);
            yield return null; if (!Healthy()) yield break;
        }
        float distance = Vector3.Dot(player.transform.position - before, start.forward);
        bool pass = blocked ? distance >= 0 && distance < .9f && (flags & CollisionFlags.Sides) != 0 : distance > 1.75f;
        Record(name, pass, "Requested 2m with 40 actual CharacterController.Move calls; forward movement=" + distance.ToString("R", CultureInfo.InvariantCulture) + "m; collision flags=" + flags + "; final=" + player.transform.position.ToString("F3"));
    }
    void SetPhotoView()
    {
        player.enabled = false;
        Quaternion facing = Quaternion.Euler(0, photoPose.eulerAngles.y, 0);
        player.transform.SetPositionAndRotation(photoPose.position - facing * cam.transform.localPosition, facing);
        player.enabled = true;
        cam.transform.SetPositionAndRotation(photoPose.position, photoPose.rotation); Physics.SyncTransforms();
    }
    IEnumerator RunChecks()
    {
        SetDoor(false); SetPose(playerStart);
        if (generatedMeshRoot)
        {
            SetRepresentation(false); yield return null; yield return null; if (!Healthy()) yield break;
            yield return CaptureScreen("00-arrival-mesh"); if (!Healthy()) yield break;
            SetRepresentation(true); yield return null; yield return null; if (!Healthy()) yield break;
        }
        yield return CaptureScreen("00-arrival"); if (!Healthy()) yield break;
        bool wallInPath = Physics.Raycast(wallTestStart.position + Vector3.up * .9f, wallTestStart.forward, out var wallHit, 2f, Physics.DefaultRaycastLayers, QueryTriggerInteraction.Ignore) && wallHit.collider == wall;
        Record("wall-proxy-in-test-path", wallInPath, "The assigned opaque wall collider is the first obstacle along the wall test path.");
        yield return TestMove("ordinary-wall-blocks-controller", wallTestStart, true); if (!Healthy()) yield break;
        SetPose(doorTestStart); cam.transform.LookAt(doorCollider.bounds.center);
        yield return CaptureScreen("01-door-closed-before-check"); if (!Healthy()) yield break;
        yield return TestMove("closed-door-blocks-controller", doorTestStart, true); if (!Healthy()) yield break;
        yield return CaptureScreen("02-door-blocked-after-check"); if (!Healthy()) yield break;
        SetPose(doorTestStart); cam.transform.LookAt(doorCollider.bounds.center);
        bool interacted = TryDoor();
        Record("door-ray-interaction", interacted && doorOpen && doorCollider.enabled, "API calls the same ray-gated door interaction as E; collision remains enabled while the real door rotates.");
        yield return CaptureScreen("03-door-open-before-check"); if (!Healthy()) yield break;
        yield return TestMove("open-door-allows-controller", doorTestStart, false); if (!Healthy()) yield break;
        yield return CaptureScreen("04-door-passed-after-check"); if (!Healthy()) yield break;
        SetDoor(false); SetPhotoView();
        if (generatedMeshRoot)
        {
            SetRepresentation(false, false); lamp.enabled = true;
            yield return null; yield return null; if (!Healthy()) yield break;
            yield return CaptureScreen("common-only-light-on"); if (!Healthy()) yield break;
            yield return CaptureExposure("common-only-light-on", true); if (!Healthy()) yield break;
        }
        foreach (var item in new[] { "mesh-light-on", "hybrid-light-on", "hybrid-light-off", "mesh-light-off" })
        {
            if (!Healthy()) yield break;
            SetRepresentation(item.StartsWith("hybrid", StringComparison.Ordinal)); lamp.enabled = item.EndsWith("-on", StringComparison.Ordinal);
            yield return Measure(item); if (!Healthy()) yield break;
            yield return CaptureScreen(item); if (!Healthy()) yield break;
            yield return CaptureExposure(item, true); if (!Healthy()) yield break;
        }
        Record("separate-camera-exposures", result.exposures.Count == (generatedMeshRoot ? 5 : 4), "Diagnostic JPEGs rendered through a separate disabled camera and URP SingleCameraRequest; no screen-copy substitution.");
        result.completed = true;
        bool allPassed = result.checks.TrueForAll(check => check.passed);
        if (!allPassed) result.failureReason = "One or more diagnostic checks failed; consult checks and original captures.";
        if (!Save()) { Abort(result.failureReason, 2); yield break; }
        Application.Quit(allPassed ? 0 : 1);
    }
    IEnumerator Measure(string name)
    {
        double warm = Time.realtimeSinceStartupAsDouble + 1;
        while (Time.realtimeSinceStartupAsDouble < warm) { yield return null; if (!Healthy()) yield break; }
        int rendererCount = 0; long assetCount = 0, uploadedCount = 0; bool ready = true;
        if (splatsRoot.activeSelf)
        {
            var renderers = splatsRoot.GetComponentsInChildren<GsplatRenderer>(false); rendererCount = renderers.Length;
            foreach (var renderer in renderers)
            {
                if (renderer.GsplatAsset) assetCount += renderer.GsplatAsset.SplatCount;
                uploadedCount += renderer.SplatCount;
                ready &= renderer.isActiveAndEnabled && renderer.Valid && renderer.GsplatAsset && renderer.GsplatResource != null && renderer.SplatCount == renderer.GsplatAsset.SplatCount && renderer.SplatCount > 0;
            }
            ready &= rendererCount > 0 && assetCount == declaredSplatCount;
            Record(name + "-splat-resources", ready, rendererCount + " active Gsplat renderers; actual asset splats=" + assetCount + "; uploaded=" + uploadedCount + "; declared=" + declaredSplatCount + ". Visibility is tested independently in captured pixels.");
        }
        var samples = new List<double>(4096); double start = Time.realtimeSinceStartupAsDouble, previous = start;
        while (Time.realtimeSinceStartupAsDouble - start < 2 && samples.Count < 20000)
        {
            yield return null; if (!Healthy()) yield break;
            double now = Time.realtimeSinceStartupAsDouble; samples.Add((now - previous) * 1000); previous = now;
        }
        try { File.WriteAllLines(Path.Combine(output, name + "-frame-ms.csv"), samples.ConvertAll(v => v.ToString("R", CultureInfo.InvariantCulture))); }
        catch (Exception error) { Abort("Frame evidence write failed: " + error.Message, 2); }
        if (failed) yield break;
        if (samples.Count == 0) { Abort("No frame interval was measured."); yield break; }
        samples.Sort(); double seconds = previous - start;
        result.phases.Add(new Phase { name = name, representation = Representation, rendererCount = rendererCount, assetCount = assetCount, uploadedCount = uploadedCount, assetsReady = ready, splats = splatsRoot.activeSelf, light = lamp.enabled, splatCount = splatsRoot.activeSelf ? declaredSplatCount : 0, frames = samples.Count, seconds = seconds, meanMs = seconds * 1000 / samples.Count, p95Ms = samples[(int)((samples.Count - 1) * .95)], p99Ms = samples[(int)((samples.Count - 1) * .99)] }); Save();
    }
    IEnumerator CaptureScreen(string name)
    {
        yield return new WaitForEndOfFrame(); if (!Healthy()) yield break;
        Texture2D capture = null;
        try { capture = ScreenCapture.CaptureScreenshotAsTexture(); File.WriteAllBytes(Path.Combine(output, name + ".png"), capture.EncodeToPNG()); }
        catch (Exception error) { Abort("Screenshot capture failed: " + error.Message, 2); }
        finally { if (capture) Destroy(capture); }
    }
    IEnumerator CaptureExposure(string name, bool compareLighting)
    {
        if (capturing || !Healthy()) yield break;
        capturing = true;
        if (!exposureCamera)
        {
            exposureCamera = new GameObject("Hybrid23SeparateDiagnosticCamera").AddComponent<Camera>(); exposureCamera.enabled = false;
            exposureTarget = new RenderTexture(1280, 800, 24, RenderTextureFormat.ARGB32) { name = "Hybrid23DiagnosticExposure", antiAliasing = 1 };
            exposureTarget.Create();
        }
        bool proxyRequired = relighting && splatsRoot.activeSelf;
        if (proxyRequired)
        {
            try { relighting.Refresh(); }
            catch (Exception error) { Abort("Proxy configuration refresh failed: " + error.Message); }
            if (!Healthy()) { capturing = false; yield break; }
        }
        exposureCamera.CopyFrom(cam); exposureCamera.enabled = false; exposureCamera.targetTexture = null;
        exposureCamera.transform.SetPositionAndRotation(cam.transform.position, cam.transform.rotation);
        var sourceData = cam.GetUniversalAdditionalCameraData();
        var targetData = exposureCamera.GetUniversalAdditionalCameraData();
        targetData.SetRenderer(0);
        targetData.renderPostProcessing = sourceData.renderPostProcessing; targetData.renderShadows = sourceData.renderShadows;
        targetData.antialiasing = sourceData.antialiasing; targetData.antialiasingQuality = sourceData.antialiasingQuality;
        targetData.stopNaN = sourceData.stopNaN; targetData.dithering = sourceData.dithering;
        bool samePoseProjection = Vector3.Distance(exposureCamera.transform.position, cam.transform.position) < .0001f && Quaternion.Angle(exposureCamera.transform.rotation, cam.transform.rotation) < .001f && exposureCamera.cullingMask == cam.cullingMask && Screen.width == 1280 && Screen.height == 800 && cam.pixelWidth == 1280 && cam.pixelHeight == 800 && cam.rect == new Rect(0, 0, 1, 1);
        for (int row = 0; row < 4; row++)
            for (int column = 0; column < 4; column++)
                samePoseProjection &= Mathf.Abs(exposureCamera.projectionMatrix[row, column] - cam.projectionMatrix[row, column]) < .00001f;
        bool proxyReady = false, proxyRendered = false;
        if (proxyRequired)
        {
            GsplatRelighting activeSource = null;
            bool configuration = relighting.isActiveAndEnabled && relighting.SourceCamera == cam && relighting.SplatRenderer && relighting.SplatRenderer.TryGetActiveRelighting(out activeSource) && activeSource == relighting.RelightingSource && activeSource.SourceCamera == cam && relighting.Blend > 0;
            Record(name + "-proxy-active-source", configuration, "Player camera binding=" + (relighting.SourceCamera == cam) + "; active source resolved=" + configuration + "; proxy layer=" + relighting.gameObject.layer + "; blend=" + relighting.Blend + "; brightness=" + relighting.Brightness + "; background=" + relighting.Background + "; texture scale=" + relighting.TextureScale + ". Independent source-camera substitution is not performed.");
            Record(name + "-proxy-same-pose-projection", samePoseProjection, "Separate camera and proxy source must share pose, projection, culling mask and 1280x800 viewport; this is a same-pose-only test.");
            if (!configuration || !samePoseProjection) { capturing = false; Abort("Proxy photo configuration or same-pose condition failed."); yield break; }
            try
            {
                activeSource.Refresh(); proxyReady = activeSource.IsReady;
                if (proxyReady) proxyRendered = activeSource.RenderNow();
            }
            catch (Exception error) { Abort("Proxy lighting-map render failed: " + error.Message); }
            Record(name + "-proxy-render-request", proxyReady && proxyRendered && Healthy(), "IsReady=" + proxyReady + "; RenderNow returned=" + proxyRendered + ". A true return confirms submitted rendering, not visual lighting correctness.");
            if (!proxyReady || !proxyRendered || !Healthy()) { capturing = false; if (!failed) Abort("Proxy lighting RenderNow is unsupported or not ready; no photo fallback is substituted.", 2); yield break; }
        }
        var request = new UniversalRenderPipeline.SingleCameraRequest { destination = exposureTarget };
        bool rendered = false;
        try
        {
            if (RenderPipeline.SupportsRenderRequest(exposureCamera, request)) { RenderPipeline.SubmitRenderRequest(exposureCamera, request); rendered = true; }
        }
        catch (Exception error) { Abort("Separate camera render request failed: " + error.Message); }
        if (!rendered || !Healthy()) { capturing = false; if (!failed) Abort("URP separate-camera render requests are unavailable.", 2); yield break; }
        yield return null; if (!Healthy()) { capturing = false; yield break; }
        Texture2D pixels = null; RenderTexture previous = RenderTexture.active;
        try
        {
            RenderTexture.active = exposureTarget; pixels = new Texture2D(1280, 800, TextureFormat.RGB24, false);
            pixels.ReadPixels(new Rect(0, 0, 1280, 800), 0, 0); pixels.Apply();
            var colors = pixels.GetPixels32(); double luma = 0;
            foreach (var color in colors) luma += (.2126 * color.r + .7152 * color.g + .0722 * color.b) / 255;
            byte[] bytes = pixels.EncodeToJPG(95); string filename = "hybridprobe-exposure-" + name + ".jpg";
            File.WriteAllBytes(Path.Combine(output, filename), bytes);
            string hash; using (var sha = SHA256.Create()) hash = BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-", "").ToLowerInvariant();
            var exposure = new Exposure { name = name, proxyConfigured = proxyRequired, proxyReady = proxyReady, proxyRenderSubmitted = proxyRendered, samePoseProjection = samePoseProjection, representation = Representation, file = filename, sha256 = hash, utc = DateTime.UtcNow.ToString("O"), method = "Separate camera / URP SingleCameraRequest / RenderTexture / JPEG 95", dataOrigin = dataOrigin, position = exposureCamera.transform.position, forward = exposureCamera.transform.forward, width = 1280, height = 800, splats = splatsRoot.activeSelf, light = lamp.enabled, meanLuma = luma / colors.Length };
            result.exposures.Add(exposure); File.WriteAllText(Path.Combine(output, "hybridprobe-exposure-" + name + ".json"), JsonUtility.ToJson(exposure, true));
            if (compareLighting && name == "common-only-light-on") commonOnlyPixels = colors;
            if (compareLighting && name == "mesh-light-on") meshLightPixels = colors;
            if (compareLighting && name == "hybrid-light-on")
            {
                firstLightPixels = colors;
                int changed = 0, representationChanged = 0; splatContributionMask = new bool[colors.Length];
                var visibilityBaseline = generatedMeshRoot ? commonOnlyPixels : meshLightPixels;
                for (int i = 0; i < colors.Length; i++)
                {
                    if (visibilityBaseline != null && Math.Abs(colors[i].r - visibilityBaseline[i].r) + Math.Abs(colors[i].g - visibilityBaseline[i].g) + Math.Abs(colors[i].b - visibilityBaseline[i].b) >= 12) { changed++; splatContributionMask[i] = true; }
                    if (meshLightPixels != null && Math.Abs(colors[i].r - meshLightPixels[i].r) + Math.Abs(colors[i].g - meshLightPixels[i].g) + Math.Abs(colors[i].b - meshLightPixels[i].b) >= 12) representationChanged++;
                }
                Record("splat-camera-image-response", changed > 100, changed + " separate-camera pixels differ from the common-scene baseline with identical lamp state. This establishes visible splat contribution, not correct occlusion.");
                if (generatedMeshRoot) Record("generated-representation-ab-response", representationChanged > 100, representationChanged + " pixels differ between source mesh and surface-derived Gaussians. Compare original captures for preservation of shape, texture and occlusion; difference alone is not visual quality.");
                meshLightPixels = commonOnlyPixels = null;
            }
            if (compareLighting && name == "hybrid-light-off" && firstLightPixels != null)
            {
                int changed = 0, region = 0, regionChanged = 0; long absolute = 0, regionAbsolute = 0;
                for (int i = 0; i < colors.Length; i++)
                {
                    int difference = Math.Abs(colors[i].r - firstLightPixels[i].r) + Math.Abs(colors[i].g - firstLightPixels[i].g) + Math.Abs(colors[i].b - firstLightPixels[i].b); absolute += difference; if (difference >= 12) changed++;
                    if (splatContributionMask != null && splatContributionMask[i]) { region++; regionAbsolute += difference; if (difference >= 12) regionChanged++; }
                }
                result.splatRegionLightPixels = region; result.splatRegionLightChangedPixels = regionChanged; result.splatRegionMeanAbsoluteRgbChange = region > 0 ? regionAbsolute / (3.0 * region) : 0;
                splatContributionMask = null;
                Record("rendered-light-change", changed > 100, changed + " pixels differ by >=12 summed RGB; mean absolute RGB change=" + (absolute / (3.0 * colors.Length)).ToString("R", CultureInfo.InvariantCulture) + ". Establishes an image response only; inspect mesh and splat regions separately.");
                firstLightPixels = null;
            }
            status = "DIAGNOSTIC JPEG SAVED / " + filename; Save();
        }
        catch (Exception error) { Abort("Diagnostic exposure failed: " + error.Message, 2); }
        finally { RenderTexture.active = previous; if (pixels) Destroy(pixels); capturing = false; }
    }
    void OnGUI()
    {
        if (!initialized || automated || failed) return;
        GUI.Box(new Rect(15, 15, Screen.width - 30, 75), "STATION 01 / ISOLATED HYBRID PROBE\n" + dataOrigin + "\n" + status);
    }
}
