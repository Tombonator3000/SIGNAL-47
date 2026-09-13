using UnityEngine;

namespace Signal47.Station26
{
    public sealed partial class StationController
    {
        GUIStyle heading, text, small, paperText, paperHeading, button;
        static readonly Color Background = new Color(.035f, .055f, .05f);
        static readonly Color Paper = new Color(.88f, .87f, .77f);
        static readonly Color Ink = new Color(.09f, .14f, .12f);
        const string Transit = "FIELD TRANSIT / STATION 01 / E09A\n\n1947 / A: triangle with bar; mounted in the concrete footing on the transit sight line. B: fixed comparison marker to the west. C: closing sight line, shown in the original arrangement.\n\nCompare the empty footing, present markers and this diagram. Photograph both the footing and the marked post before recording which position changed.";
        const string Timing = "FIELD TIMING RECORD / STATION 01 / E10\n\n02:17:00 / Carrier ceased. Recorded voice: ‘Reference west. No—east. Hold the last reading.’\n02:17:47 / Local relay impact and reference motion observed.\nFinal observation / Not entered.\n\nNEW RECEIVER OBSERVATION / 1986\nThe received fragment repeats the same correction, ‘west’ to ‘east,’ in the same place. The original field record identifies the speaker as T. Vega.";

        void Styles()
        {
            if (text != null) return;
            text = new GUIStyle(GUI.skin.label) { font = font, fontSize = 22, wordWrap = true, normal = { textColor = new Color(.87f, .9f, .81f) } };
            small = new GUIStyle(text) { fontSize = 18 };
            heading = new GUIStyle(text) { fontSize = 31 };
            paperText = new GUIStyle(text) { fontSize = 23, normal = { textColor = Ink } };
            paperHeading = new GUIStyle(paperText) { fontSize = 27 };
            button = new GUIStyle(GUI.skin.button) { font = font, fontSize = 20, wordWrap = true };
        }
        static void Panel(Rect rect, Color color)
        { var previous = GUI.color; GUI.color = color; GUI.DrawTexture(rect, Texture2D.whiteTexture); GUI.color = previous; }

        /// <summary>Called by HUDController so the station owns its full-screen modal.</summary>
        public void DrawPanel()
        {
            if (!ModalOpen || !SessionReady) return;
            Styles(); var previous = GUI.matrix; int depth = GUI.depth; GUI.depth = -30;
            GUI.matrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(Screen.width / 1280f, Screen.height / 800f, 1));
            Panel(new Rect(0, 0, 1280, 800), Background);
            GUI.Label(new Rect(30, 18, 1000, 42), "STATION 01 / FIELD RECORD", heading);
            if (GUI.Button(new Rect(1095, 18, 155, 43), "CLOSE / ESC", button)) Close();
            GUI.Label(new Rect(30, 67, 1215, 31), P09Complete ? "P06–P09 RECORDED / Field timing linked / Return to SARO" : Objective, small);
            string[] pages = { "marker", "lamp", "cable", "timing" };
            string[] labels = { "P06 / TRANSIT", "P07 / NULL TEST", "P08 / CABLE", "P09 / TIMING" };
            for (int i = 0; i < pages.Length; i++)
            {
                GUI.backgroundColor = CurrentPage == pages[i] ? new Color(.55f, .7f, .58f) : Color.white;
                if (GUI.Button(new Rect(30 + i * 306, 111, 294, 50), labels[i], button)) Open(pages[i]);
            }
            GUI.backgroundColor = Color.white;
            switch (CurrentPage)
            {
                case "marker": DrawMarkerPage(); break;
                case "lamp": DrawLampPage(); break;
                case "cable": DrawCablePage(); break;
                case "timing": DrawTimingPage(); break;
                case "travel": DrawTravelPage(); break;
            }
            GUI.matrix = previous; GUI.depth = depth;
        }

        void DrawMarkerPage()
        {
            Panel(new Rect(30, 183, 715, 515), Paper); Panel(new Rect(764, 183, 486, 515), Paper);
            GUI.Label(new Rect(54, 201, 660, 39), "E09A / TRANSIT RECORD", paperHeading);
            float height = Mathf.Max(410, paperText.CalcHeight(new GUIContent(Transit), 625) + 20);
            scroll = GUI.BeginScrollView(new Rect(50, 250, 665, 410), scroll, new Rect(0, 0, 635, height));
            GUI.Label(new Rect(5, 0, 625, height), Transit, paperText); GUI.EndScrollView();
            GUI.Label(new Rect(790, 205, 430, 34), "HISTORICAL ARRANGEMENT / DIAGRAM", paperHeading);
            DrawArrangement(new Vector2(1005, 390), Ink);
            GUI.Label(new Rect(795, 555, 420, 95), "A  FIXED POINT\nB  COMPARISON VANE\nC  CLOSING SIGHT LINE", paperText);
            bool ready = ReadE09a;
            if (GUI.Button(new Rect(54, 664, 640, 35), ready ? "E09A READ / FRAME 03 IS A REAL FIELD EXPOSURE" : "READ E09A SOURCE", button) && !ready) ReadSource("E09a");
            GUI.Label(new Rect(54, 709, 1160, 70), Feedback, small);
            if (!P06Complete)
            {
                if (GUI.Button(new Rect(790, 665, 205, 40), "A / MOVED", button)) SubmitMarker("triangle-bar");
                if (GUI.Button(new Rect(1005, 665, 205, 40), "B / MOVED", button)) SubmitMarker("b-moved");
                if (GUI.Button(new Rect(790, 712, 420, 40), "UNCERTAIN / LOCAL LAMP", button)) SubmitMarker("lamp");
            }
        }

        void DrawLampPage()
        {
            Panel(new Rect(65, 183, 1150, 515), Paper);
            GUI.Label(new Rect(94, 207, 1070, 42), "P07 / LOCAL LAMP NULL TEST", paperHeading);
            GUI.Label(new Rect(96, 277, 1040, 120), "Screen the practical lamp and observe the ordinary local null condition. This isolates the source. It does not move the physical reference, and it does not finish the cable finding.", paperText);
            GUI.Label(new Rect(96, 425, 1060, 42), LampCovered ? "LAMP STATUS / COVERED" : "LAMP STATUS / EXPOSED", paperHeading);
            if (GUI.Button(new Rect(96, 493, 330, 46), LampCovered ? "UNCOVER LAMP" : "COVER LAMP", button)) SetLampCovered(!LampCovered);
            if (GUI.Button(new Rect(445, 493, 330, 46), "OBSERVE NULL TEST", button)) ObserveLamp();
            GUI.Label(new Rect(96, 585, 1045, 95), Feedback, paperText);
        }

        void DrawCablePage()
        {
            Panel(new Rect(65, 183, 1150, 515), Paper);
            GUI.Label(new Rect(94, 207, 1070, 42), "P08 / CABLE BREAK", paperHeading);
            GUI.Label(new Rect(96, 277, 1040, 95), "Follow the cable from the local relay. Inspect the opposed cut faces, then expose and inspect the genuine near frame. The retained reference and a deliberate cut are separate findings.", paperText);
            if (GUI.Button(new Rect(96, 410, 330, 46), CableInspected ? "CABLE INSPECTED" : "INSPECT CUT FACES", button)) InspectCable();
            if (GUI.Button(new Rect(445, 410, 330, 46), "RECORD DELIBERATE CUT", button)) SubmitCable("deliberate-cut");
            if (GUI.Button(new Rect(795, 410, 330, 46), "WEATHERING / ACCIDENT", button)) SubmitCable("weathering");
            if (GUI.Button(new Rect(795, 463, 330, 46), "AUTHOR / MOTIVE", button)) SubmitCable("author-guilt");
            GUI.Label(new Rect(96, 518, 1045, 52), ReadE09b ? "FRAME 04 / E09B READ" : "FRAME 04 / CAPTURE REAL FIELD EXPOSURE / PROCESS AFTER RETURN", paperHeading);
            GUI.Label(new Rect(96, 585, 1045, 95), Feedback, paperText);
        }

        void DrawTimingPage()
        {
            Panel(new Rect(65, 183, 1150, 515), Paper);
            GUI.Label(new Rect(94, 207, 1070, 42), "E10 / TIMING RECORD", paperHeading);
            float height = Mathf.Max(295, paperText.CalcHeight(new GUIContent(Timing), 1040) + 20);
            scroll = GUI.BeginScrollView(new Rect(96, 267, 1065, 275), scroll, new Rect(0, 0, 1030, height));
            GUI.Label(new Rect(5, 0, 1020, height), Timing, paperText); GUI.EndScrollView();
            if (GUI.Button(new Rect(96, 566, 500, 44), ReadE10 ? "E10 READ / SUBMIT 02:17:47" : "READ E10 TIMING SOURCE", button) && !ReadE10) ReadSource("E10");
            if (GUI.Button(new Rect(620, 566, 540, 44), "RECORD MATCHED READING / 02:17:47", button)) SubmitTiming("02:17:47");
            if (GUI.Button(new Rect(96, 618, 340, 38), "CARRIER / 02:17:00", button)) SubmitTiming("02:17:00");
            if (GUI.Button(new Rect(450, 618, 340, 38), "47 MINUTES LATER", button)) SubmitTiming("47-minutes");
            GUI.Label(new Rect(96, 715, 1045, 70), Feedback, small);
        }

        void DrawTravelPage()
        {
            Panel(new Rect(95, 205, 1090, 430), Paper);
            GUI.Label(new Rect(128, 232, 1030, 46), InStation ? "RETURN TO SARO / CHECKPOINTED FIELD RECORD" : "FIELD ACCESS / STATION 01", paperHeading);
            string copy = InStation
                ? "Your findings and exposed film travel with you. Process the sealed film at the SARO wet bench. If the case cannot be saved, the transfer is canceled."
                : RouteReadyText();
            GUI.Label(new Rect(130, 310, 1000, 118), copy, paperText);
            var camera = Game.fieldCamera;
            bool retry = InStation && camera && camera.ExportRetryAvailable;
            string kit = retry ? "PHOTO NOT SAVED / Free disk space or restore access to the photo folder, then retry here. Keep this session open until the film is saved."
                : camera && !camera.SaveReady ? camera.SaveStatus
                : "FIELD KIT\nBring the camera. Keep the local lamp isolated and leave the broken cable open. Return here whenever you need the photolab.";
            GUI.Label(new Rect(130, 450, 1000, 95), kit, paperText);
            string caption = retry ? "RETRY PHOTO EXPORT" : InStation ? "CONFIRM RETURN TO SARO" : "CONFIRM TRAVEL TO STATION 01";
            bool enabled = GUI.enabled; GUI.enabled = enabled && (retry || CanTravel());
            if (GUI.Button(new Rect(130, 554, 455, 48), caption, button)) { if (retry) RetryPhotoExport(); else Travel(!InStation); }
            GUI.enabled = enabled;
            if (GUI.Button(new Rect(615, 554, 455, 48), "CANCEL / KEEP CURRENT AREA", button)) Close();
            GUI.Label(new Rect(130, 650, 1015, 70), Feedback, small);
        }

        string RouteReadyText()
        {
            if (!RouteReady) return "The archive destination is still locked. Complete P04 and P05 at SARO before attempting field access.";
            return "The archive identifies OLD SURVEY STATION and the matching STATION 01 fixed point. Travel is a short area transfer; the motel and Nora contact remain locked until the field timing record is complete.";
        }

        static void DrawArrangement(Vector2 center, Color color)
        {
            DrawLine(center + new Vector2(-160, 0), center + new Vector2(160, 0), color, 4);
            DrawLine(center + new Vector2(-160, 0), center + new Vector2(-160, -70), color, 4);
            DrawLine(center + new Vector2(160, 0), center + new Vector2(160, 70), color, 4);
            DrawMarker(center + new Vector2(-160, -95), 55, true, color);
            DrawMarker(center + new Vector2(0, 0), 55, false, color);
            GUI.Label(new Rect(center.x - 190, center.y - 145, 55, 30), "A", new GUIStyle(GUI.skin.label) { fontSize = 24, normal = { textColor = color } });
            GUI.Label(new Rect(center.x - 12, center.y + 28, 55, 30), "B", new GUIStyle(GUI.skin.label) { fontSize = 24, normal = { textColor = color } });
            GUI.Label(new Rect(center.x + 168, center.y + 73, 55, 30), "C", new GUIStyle(GUI.skin.label) { fontSize = 24, normal = { textColor = color } });
        }
        static void DrawMarker(Vector2 center, float size, bool bar, Color color)
        {
            Vector2 top = center + Vector2.up * -size * .58f, left = center + new Vector2(-size * .55f, size * .40f), right = center + new Vector2(size * .55f, size * .40f);
            DrawLine(top, left, color, 4); DrawLine(left, right, color, 4); DrawLine(right, top, color, 4);
            if (bar) DrawLine(center + new Vector2(-size * .40f, size * .61f), center + new Vector2(size * .40f, size * .61f), color, 4);
        }
        static void DrawLine(Vector2 from, Vector2 to, Color color, float width)
        {
            Matrix4x4 matrix = GUI.matrix; var delta = to - from;
            GUI.matrix = matrix * Matrix4x4.TRS(new Vector3(from.x, from.y, 0), Quaternion.Euler(0, 0, Mathf.Atan2(delta.y, delta.x) * Mathf.Rad2Deg), Vector3.one);
            Panel(new Rect(0, -width / 2, delta.magnitude, width), color); GUI.matrix = matrix;
        }
    }
}
