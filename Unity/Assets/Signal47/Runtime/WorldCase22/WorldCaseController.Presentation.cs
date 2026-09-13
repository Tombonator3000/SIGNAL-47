using UnityEngine;

namespace Signal47.WorldCase22
{
    public sealed partial class WorldCaseController
    {
        Vector2 scroll;
        int selectedDestination = -1, selectedSurvey = -1, selectedMarker = -1;
        GUIStyle heading, text, small, paperText, paperHeading, button;
        static readonly Color Background = new Color(.035f, .055f, .05f), Paper = new Color(.88f, .87f, .77f), Ink = new Color(.09f, .14f, .12f);
        const string Original = "FIELD SURVEY RECORD / STATION 01 / 1947\n\nThe reference remained visible in the plate after the lamp circuit was opened. A second plate was exposed with the local lamp isolated. The retained mark did not follow the new position of the physical vane.\n\nThe arrangement used three references: A, the fixed survey point; B, the optical comparison vane; and C, the closing sight line. The run was interrupted before the final observation was entered. Preserve the original plates and the complete arrangement drawing.\n\nN. VEGA / FIELD TECHNICIAN";
        const string Amended = "FIELD SURVEY RECORD / STATION 01 / 1947 — AMENDED COPY\n\nThe additional mark is attributed to a fault in plate development. The lamp and comparison vane are sufficient to describe the test. The incomplete closing sight line has been omitted from the service copy.\n\nNo repeat observation is required. Retain the amended copy with the routine reference records.\n\nN. VEGA / FIELD TECHNICIAN";
        const string Maintenance = "SARO / REFERENCE LINEAGE — B-12\n\nThe B-12 comparison vane retains the fixed-point survey identified as STATION 01. Fixed-point register mark: outlined triangle with a short horizontal bar beneath it. The archive sleeve contains the dated plate and fixed-point sheet used for this index. Match the survey identifier and fixed-point mark before selecting the field destination.\n\nSITE REGISTER: STATION 01 / OLD SURVEY STATION. The matching archive sleeve contains the field access sheet.";
        const string Index = "PLATE / 1947 / STATION 01\n\nFixed-point mark: outlined triangle with a short horizontal bar beneath it.\nArrangement: A — B — C.\nService index: B-12.\nField destination: OLD SURVEY STATION.\n\nRETURN CHECK: Match both STATION 01 and the fixed-point mark. A similar triangle without the bar is a general elevation symbol and is not a matching survey reference.";
        void Styles()
        {
            if (text != null) return;
            text = new GUIStyle(GUI.skin.label) { font = font, fontSize = 22, wordWrap = true, normal = { textColor = new Color(.87f, .9f, .81f) } };
            small = new GUIStyle(text) { fontSize = 18 };
            heading = new GUIStyle(text) { fontSize = 31 };
            paperText = new GUIStyle(text) { fontSize = 24, normal = { textColor = Ink } };
            paperHeading = new GUIStyle(paperText) { fontSize = 27 };
            button = new GUIStyle(GUI.skin.button) { font = font, fontSize = 20, wordWrap = true };
        }
        static void Panel(Rect rect, Color color)
        { var previous = GUI.color; GUI.color = color; GUI.DrawTexture(rect, Texture2D.whiteTexture); GUI.color = previous; }
        void OnGUI()
        {
            if (!ModalOpen || !SessionReady) return;
            Styles(); var previous = GUI.matrix; int depth = GUI.depth;
            GUI.depth = -30;
            GUI.matrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(Screen.width / 1280f, Screen.height / 800f, 1));
            Panel(new Rect(0, 0, 1280, 800), Background);
            GUI.Label(new Rect(30, 18, 1040, 42), "SARO ARCHIVE / THE AMENDED RECORD", heading);
            if (GUI.Button(new Rect(1095, 18, 155, 43), "CLOSE / ESC", button)) Close();
            GUI.Label(new Rect(30, 67, 1220, 31), P05Complete ? "P04 RECORDED  /  P05 RECORDED  /  Sources remain available for review" : P04Complete ? "P04 RECORDED  /  P05: Trace the reference from B-12 to a field destination" : "P04: Establish what changed  /  P05: Identify a supported field destination", small);
            string[] pages = { "original", "amended", "maintenance", "index", "compare", "route" };
            string[] labels = { "E07 / ORIGINAL", "E06 / AMENDED", "B-12 / LINEAGE", "E08 / INDEX", "P04 / COMPARE", "P05 / DESTINATION" };
            for (int i = 0; i < pages.Length; i++)
            {
                GUI.backgroundColor = CurrentPage == pages[i] ? new Color(.55f, .7f, .58f) : Color.white;
                if (GUI.Button(new Rect(30 + i * 204, 111, 194, 50), labels[i], button)) SelectPage(pages[i]);
            }
            GUI.backgroundColor = Color.white;
            switch (CurrentPage)
            {
                case "compare": DrawComparison(); break;
                case "route": DrawRoute(); break;
                case "index": DrawIndex(); break;
                default: DrawDocument(); break;
            }
            GUI.matrix = previous; GUI.depth = depth;
        }
        void DrawDocument()
        {
            string body = CurrentPage == "original" ? Original : CurrentPage == "amended" ? Amended : Maintenance;
            string title = CurrentPage == "original" ? "ORIGINAL FIELD PROTOCOL / E07" : CurrentPage == "amended" ? "AMENDED SERVICE COPY / E06" : "MODERN MAINTENANCE CARD / B-12";
            Panel(new Rect(80, 183, 1120, 539), Paper);
            GUI.Label(new Rect(108, 197, 1060, 45), title, paperHeading);
            float height = Mathf.Max(448, paperText.CalcHeight(new GUIContent(body), 1020) + 24);
            scroll = GUI.BeginScrollView(new Rect(105, 251, 1070, 444), scroll, new Rect(0, 0, 1030, height));
            GUI.Label(new Rect(6, 0, 1016, height), body, paperText); GUI.EndScrollView();
            GUI.Label(new Rect(80, 740, 1110, 43), "Read in any order. Scroll to review the full source; use COMPARE and DESTINATION to record findings.", small);
        }
        void DrawComparison()
        {
            Panel(new Rect(30, 183, 598, 301), Paper); Panel(new Rect(646, 183, 604, 301), Paper);
            GUI.Label(new Rect(51, 195, 555, 37), "ORIGINAL / E07", paperHeading);
            GUI.Label(new Rect(669, 195, 554, 37), "AMENDED / E06", paperHeading);
            GUI.Label(new Rect(51, 243, 550, 224), "Three references: A, the fixed survey point; B, the optical comparison vane; C, the closing sight line.\n\nThe mark remained in the plate with the lamp circuit opened.", paperText);
            GUI.Label(new Rect(669, 243, 550, 224), "The incomplete closing sight line has been omitted.\n\nThe additional mark is attributed to a fault in plate development. No repeat observation is required.", paperText);
            GUI.Label(new Rect(30, 498, 1220, 33), ReadOriginal && ReadAmended ? "Which finding is supported by the source records?" : "Read E07 and E06 before recording a finding. The source tabs stay available above.", text);
            bool enabled = GUI.enabled; GUI.enabled = ReadOriginal && ReadAmended;
            if (GUI.Button(new Rect(30, 541, 1220, 42), "The signature proves that the author caused the anomaly.", button)) SubmitComparison("author-guilt");
            if (GUI.Button(new Rect(30, 591, 1220, 42), "The amended copy removes C and replaces the retained-mark observation with a development explanation.", button)) SubmitComparison("omitted-c");
            if (GUI.Button(new Rect(30, 641, 1220, 42), "The development explanation accounts for both versions without an omitted reference.", button)) SubmitComparison("development-only");
            GUI.enabled = enabled;
            GUI.Label(new Rect(31, 698, 1218, 95), Feedback, small);
        }
        void DrawIndex()
        {
            Panel(new Rect(30, 183, 798, 539), Paper); Panel(new Rect(847, 183, 403, 539), Paper);
            GUI.Label(new Rect(51, 195, 742, 45), "ARCHIVE SLEEVE / E08 INDEX", paperHeading);
            float height = Mathf.Max(420, paperText.CalcHeight(new GUIContent(Index), 724) + 20);
            scroll = GUI.BeginScrollView(new Rect(48, 249, 759, 448), scroll, new Rect(0, 0, 735, height));
            GUI.Label(new Rect(4, 0, 724, height), Index, paperText); GUI.EndScrollView();
            GUI.Label(new Rect(869, 201, 363, 68), "FIXED-POINT SHEET\nINDEX DIAGRAM", paperHeading);
            DrawMarker(new Vector2(1048, 377), 100, true, Ink);
            GUI.Label(new Rect(873, 456, 354, 75), "A  —  B  —  C\nSTATION 01 / B-12", paperHeading);
            GUI.Label(new Rect(873, 560, 354, 144), "REFERENCE INDEX / Compare this mark with the fixed-point register on the B-12 lineage card.", paperText);
            GUI.Label(new Rect(31, 740, 1218, 45), "Use the source identity and the marked symbol together. An unmarked triangle is not a match.", small);
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
            // Compose in the dossier's logical coordinates before applying screen scaling.
            GUI.matrix = matrix * Matrix4x4.TRS(new Vector3(from.x, from.y, 0),
                Quaternion.Euler(0, 0, Mathf.Atan2(delta.y, delta.x) * Mathf.Rad2Deg), Vector3.one);
            Panel(new Rect(0, -width / 2, delta.magnitude, width), color); GUI.matrix = matrix;
        }
        void DrawRoute()
        {
            if (P05Complete)
            {
                Panel(new Rect(65, 195, 1150, 498), Paper);
                GUI.Label(new Rect(93, 220, 1090, 47), "FIELD DESTINATION SUPPORTED / OLD SURVEY STATION", paperHeading);
                GUI.Label(new Rect(94, 292, 1030, 217), "B-12 retains STATION 01. The matching archive sleeve provides the dated 1947 index and the outlined triangle with its short bar.\n\nThe amended record omitted C. This establishes a changed report and a continued reference, not a motive or a proven time displacement.", paperText);
                GUI.Label(new Rect(94, 544, 1040, 112), "FIELD ACCESS PREPARED\nUse the FIELD TRAVEL folio beside this dossier to prepare departure. Your findings and original photographs travel with the case.", paperHeading);
                GUI.Label(new Rect(65, 740, 1135, 44), "SAVE STATUS / " + Signal47.Chapter.ChapterSave.Status, small);
                return;
            }
            GUI.Label(new Rect(31, 182, 1218, 61), !P04Complete ? "First record the comparison in P04. You can inspect all four source cards now." : !ReadMaintenance || !ReadIndex ? "Read the B-12 lineage card and E08 index before preparing the destination." : "Match both source identifiers. Select a destination, survey ID and fixed-point mark.", text);
            GUI.Label(new Rect(31, 251, 370, 36), "FIELD DESTINATION", small);
            GUI.Label(new Rect(442, 251, 370, 36), "SURVEY IDENTIFIER", small);
            GUI.Label(new Rect(853, 251, 370, 36), "FIXED-POINT MARK", small);
            string[] destinations = { "OLD SURVEY STATION", "SARO ARRAY APRON", "UNLISTED FIELD SITE" };
            string[] surveys = { "STATION 01", "S-03", "-39 LY AS A YEAR CODE" };
            string[] markers = { "OUTLINED TRIANGLE + BAR", "TRIANGLE / NO BAR", "THREE HORIZONTAL BARS" };
            for (int i = 0; i < 3; i++)
            {
                if (Choice(new Rect(30, 298 + i * 68, 392, 58), destinations[i], selectedDestination == i)) selectedDestination = i;
                if (Choice(new Rect(441, 298 + i * 68, 392, 58), surveys[i], selectedSurvey == i)) selectedSurvey = i;
                if (Choice(new Rect(852, 298 + i * 68, 398, 58), markers[i], selectedMarker == i)) selectedMarker = i;
            }
            GUI.Label(new Rect(31, 511, 1218, 71), "SOURCE CHECK / The maintenance card must connect today's B-12 to the same survey and fixed-point reference in the sleeve. A place name alone is insufficient.", text);
            bool enabled = GUI.enabled; GUI.enabled = P04Complete && ReadMaintenance && ReadIndex && selectedDestination >= 0 && selectedSurvey >= 0 && selectedMarker >= 0;
            if (GUI.Button(new Rect(30, 598, 1220, 54), "RECORD SUPPORTED FIELD DESTINATION", button))
                SubmitDestination(new[] { "old-survey-station", "saro-apron", "unlisted" }[selectedDestination], surveys[selectedSurvey], new[] { "triangle-bar", "triangle", "three-bars" }[selectedMarker]);
            GUI.enabled = enabled;
            GUI.Label(new Rect(31, 676, 1218, 115), Feedback, small);
        }
        bool Choice(Rect rect, string label, bool selected)
        {
            GUI.backgroundColor = selected ? new Color(.55f, .7f, .58f) : Color.white;
            bool pressed = GUI.Button(rect, (selected ? "[X]  " : "[ ]  ") + label, button); GUI.backgroundColor = Color.white; return pressed;
        }
    }
}
