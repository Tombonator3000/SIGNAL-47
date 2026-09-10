using System;
using System.Collections;
using System.IO;
using UnityEngine;
using Signal47.Core;

namespace Signal47.Debugging
{
    /// <summary>
    /// Visual-evidence harness only. It positions the real runtime camera at fixed review
    /// viewpoints and captures unretouched frames. It does not prove player traversal.
    /// </summary>
    public sealed class WorldAreaCapture : MonoBehaviour
    {
        string root;

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
            Directory.CreateDirectory(root);
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
            yield return new WaitForSecondsRealtime(2.25f); // allow the start toast to disappear

            var camera = session.player.viewCamera;
            camera.fieldOfView = 60f;

            yield return Capture(camera, "01-control-room-array.png",
                new Vector3(0f, 1.65f, 4.55f),
                new Vector3(0f, 1.8f, -22f));

            yield return Capture(camera, "02-service-yard-array.png",
                new Vector3(-17f, 1.62f, -11.5f),
                new Vector3(7f, 3.0f, -37f));

            var motel = GameObject.Find("RoadsideMotel_Blockout");
            if (motel)
            {
                yield return Capture(camera, "03-sierra-motor-court-blockout.png",
                    motel.transform.TransformPoint(new Vector3(-34f, 1.65f, -18f)),
                    motel.transform.TransformPoint(new Vector3(-1f, 1.8f, 2f)));
            }
            else Debug.LogError("WORLD_CAPTURE_FAIL RoadsideMotel_Blockout missing");

            var manifest = "{\n" +
                $"  \"unity\": \"{Application.unityVersion}\",\n" +
                $"  \"resolution\": \"{Screen.width}x{Screen.height}\",\n" +
                $"  \"quality\": \"{QualitySettings.names[QualitySettings.GetQualityLevel()]}\",\n" +
                $"  \"gpu\": \"{Escape(SystemInfo.graphicsDeviceName)}\",\n" +
                $"  \"renderer\": \"{SystemInfo.graphicsDeviceType}\",\n" +
                "  \"method\": \"fixed runtime camera review viewpoints; not player traversal\"\n" +
                "}\n";
            File.WriteAllText(Path.Combine(root, "capture-manifest.json"), manifest);
            Debug.Log("WORLD_CAPTURE_PASS " + root);
            yield return new WaitForSecondsRealtime(.5f);
            Application.Quit(0);
        }

        IEnumerator Capture(Camera camera, string file, Vector3 position, Vector3 target)
        {
            camera.transform.position = position;
            camera.transform.rotation = Quaternion.LookRotation((target - position).normalized, Vector3.up);
            yield return null;
            yield return new WaitForEndOfFrame();
            ScreenCapture.CaptureScreenshot(Path.Combine(root, file));
            yield return new WaitForSecondsRealtime(.7f);
            Debug.Log("WORLD_CAPTURE_FRAME " + file);
        }

        static string Escape(string value) => (value ?? "").Replace("\\", "\\\\").Replace("\"", "\\\"");
    }
}
