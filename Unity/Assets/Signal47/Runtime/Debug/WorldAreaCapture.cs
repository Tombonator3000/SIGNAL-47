using System;
using System.Collections;
using System.IO;
using UnityEngine;
using Signal47.Core;

namespace Signal47.Debugging
{
    /// <summary>
    /// Visual-evidence harness only. It positions the real runtime camera at fixed review
    /// viewpoints and captures unretouched PNG frames. It does not prove player traversal.
    /// Small JPEG copies are generated only for convenient mobile review; PNGs remain the evidence originals.
    /// </summary>
    public sealed class WorldAreaCapture : MonoBehaviour
    {
        string root, mobileRoot;

        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        static void Init()
        {
            if (Array.IndexOf(System.Environment.GetCommandLineArgs(), "--signal47-world-capture") < 0) return;
            if (!FindFirstObjectByType<WorldAreaCapture>()) new GameObject("WorldAreaCapture").AddComponent<WorldAreaCapture>();
        }

        void Awake()
        {
            DontDestroyOnLoad(gameObject);
            Application.runInBackground = true;
            root = Path.GetFullPath(Path.Combine(Application.dataPath, "../../WorldCapture"));
            mobileRoot = Path.GetFullPath(Path.Combine(Directory.GetCurrentDirectory(), "Docs/Evidence/WorldAreaPass05"));
            Directory.CreateDirectory(root);
            Directory.CreateDirectory(mobileRoot);
            StartCoroutine(CaptureRoutine());
        }

        IEnumerator CaptureRoutine()
        {
            GameSession session = null;
            for (var i = 0; i < 240 && !session; i++)
            {
                session = GameSession.Instance;
                yield return null;
            }
            if (!session || !session.player || !session.player.viewCamera)
            {
                Debug.LogError("WORLD_CAPTURE_FAIL missing runtime player/camera");
                Application.Quit(2);
                yield break;
            }

            session.hud.StartShift();
            session.hud.CloseModal();
            session.hud.InteractionPrompt = "";
            session.player.enabled = false;
            yield return new WaitForSecondsRealtime(2.25f);

            var camera = session.player.viewCamera;
            camera.fieldOfView = 60f;

            yield return Capture(camera, "01-control-room-array.png",
                new Vector3(0f, 1.65f, 4.55f),
                new Vector3(0f, 1.8f, -22f));

            yield return Capture(camera, "02-service-yard-array.png",
                new Vector3(-17f, 1.62f, -11.5f),
                new Vector3(7f, 3.0f, -37f));

            var motel = FindSceneObjectIncludingInactive("RoadsideMotel_Blockout");
            if (motel)
            {
                motel.SetActive(true);
                yield return null;
                yield return Capture(camera, "03-sierra-motor-court-blockout.png",
                    motel.transform.TransformPoint(new Vector3(-34f, 1.65f, -18f)),
                    motel.transform.TransformPoint(new Vector3(-1f, 1.8f, 2f)));
                motel.SetActive(false);
            }
            else Debug.LogError("WORLD_CAPTURE_FAIL RoadsideMotel_Blockout missing");

            var manifest = "{\n" +
                $"  \"unity\": \"{Application.unityVersion}\",\n" +
                $"  \"resolution\": \"{Screen.width}x{Screen.height}\",\n" +
                $"  \"quality\": \"{QualitySettings.names[QualitySettings.GetQualityLevel()]}\",\n" +
                $"  \"gpu\": \"{Escape(SystemInfo.graphicsDeviceName)}\",\n" +
                $"  \"renderer\": \"{SystemInfo.graphicsDeviceType}\",\n" +
                "  \"method\": \"fixed runtime camera review viewpoints; not player traversal\",\n" +
                "  \"motelRuntimeState\": \"inactive in normal prologue; temporarily enabled only for motel review capture\",\n" +
                "  \"mobilePreviews\": \"640x400 JPEG quality 55, derived from the same frames; raw PNG files are authoritative\"\n" +
                "}\n";
            File.WriteAllText(Path.Combine(root, "capture-manifest.json"), manifest);
            File.WriteAllText(Path.Combine(mobileRoot, "capture-manifest.json"), manifest);
            Debug.Log("WORLD_CAPTURE_PASS " + root);
            yield return new WaitForSecondsRealtime(.5f);
            Application.Quit(0);
        }

        static GameObject FindSceneObjectIncludingInactive(string name)
        {
            foreach(var t in Resources.FindObjectsOfTypeAll<Transform>())
                if(t.name==name && t.gameObject.scene.IsValid())return t.gameObject;
            return null;
        }

        IEnumerator Capture(Camera camera, string file, Vector3 position, Vector3 target)
        {
            camera.transform.position = position;
            camera.transform.rotation = Quaternion.LookRotation((target - position).normalized, Vector3.up);
            yield return null;
            yield return new WaitForEndOfFrame();

            var full = new Texture2D(Screen.width, Screen.height, TextureFormat.RGB24, false);
            full.ReadPixels(new Rect(0, 0, Screen.width, Screen.height), 0, 0);
            full.Apply(false, false);
            File.WriteAllBytes(Path.Combine(root, file), full.EncodeToPNG());

            var previous = RenderTexture.active;
            var scaledRt = RenderTexture.GetTemporary(640, 400, 0, RenderTextureFormat.ARGB32);
            Graphics.Blit(full, scaledRt);
            RenderTexture.active = scaledRt;
            var small = new Texture2D(640, 400, TextureFormat.RGB24, false);
            small.ReadPixels(new Rect(0, 0, 640, 400), 0, 0);
            small.Apply(false, false);
            var jpgName = "mobile-" + Path.GetFileNameWithoutExtension(file) + ".jpg";
            File.WriteAllBytes(Path.Combine(mobileRoot, jpgName), small.EncodeToJPG(55));
            RenderTexture.active = previous;
            RenderTexture.ReleaseTemporary(scaledRt);
            Destroy(full);
            Destroy(small);

            yield return null;
            Debug.Log("WORLD_CAPTURE_FRAME " + file + " + " + jpgName);
        }

        static string Escape(string value) => (value ?? "").Replace("\\", "\\\\").Replace("\"", "\\\"");
    }
}
