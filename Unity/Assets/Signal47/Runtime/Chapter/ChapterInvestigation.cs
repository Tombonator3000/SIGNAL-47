using System;
using System.Collections;
using System.IO;
using UnityEngine;
using Signal47.Core;
using Signal47.Investigation;

namespace Signal47.Chapter
{
    /// <summary>The chapter's physical actions and deductions. Photo pixels and save transport have separate owners.</summary>
    public sealed class ChapterInvestigation : MonoBehaviour
    {
        public const string ArchiveId = "b12-reference-sheet";
        public const string ControlId = "b12-control-observation";
        public const string ConclusionId = "chapter09-local-conclusion";
        public Transform referenceVane;
        public GameObject photoEcho;
        public Light experimentLight;
        public Renderer developingPrint;
        public bool ModalOpen => !string.IsNullOrEmpty(panel);
        public bool Complete => state.complete;
        public bool CanSave => Camera && Camera.SaveReady;
        public string ExperimentMethod => state.experimentMethod ?? "";
        public bool ExperimentMoving => experimentRemaining > 0;
        public bool ExperimentReady => !string.IsNullOrEmpty(state.experimentMethod) && state.controlObserved && !ExperimentMoving;
        public bool ClueMarked => state.clueMarked;
        public bool ComparisonConfirmed => state.comparisonConfirmed;
        public bool ArchiveMatched => state.archiveMatched;
        public bool SecondClueMarked => state.secondClueMarked;
        public bool ControlObserved => state.controlObserved;
        public string Hypothesis => state.hypothesis ?? "";
        public int WrongHypotheses => state.wrongHypotheses;
        public int WrongConclusions => state.wrongConclusions;
        public int LabStep => state.labStep;
        public float LabRemaining => state.labRemaining;
        public string CurrentPanel => panel;
        public Vector3 ReferencePosition => referenceVane ? referenceVane.position : new Vector3(10, 1.6f, -22);
        public Vector3 EchoPosition => photoEcho ? photoEcho.transform.position : ReferencePosition + Vector3.right * .45f;
        GameSession Game => GameSession.Instance;
        FieldCamera Camera => Game ? Game.fieldCamera : null;
        string panel = "", feedback = "", selectedPhoto = FieldCamera.PhotoId;
        float zoom = 1; Vector2 pan = new Vector2(.5f, .5f);
        int hintLevel;
        Quaternion vaneRest;
        float lampRest = 2, experimentRemaining;
        Material printMaterial;
        AudioSource foley;
        public AudioClip filmHandlingClip, mechanicalClip;
        AudioClip filmSound, transferSound, shutterContact, discoveryTone;
        bool initialized;
        GUIStyle ink, small, heading, button;
        readonly Color paper = new Color(.89f, .885f, .79f), dark = new Color(.08f, .14f, .12f), accent = new Color(.26f, .37f, .27f);

        [Serializable] sealed class State
        {
            public int version = 1;
            public bool archiveRead, archiveMatched, clueMarked, secondClueMarked, controlObserved, comparisonConfirmed, complete;
            public string hypothesis = "", experimentMethod = "", labFrameId = "";
            public int labStep, wrongHypotheses, wrongConclusions;
            public float labRemaining;
        }
        State state = new State();

        public string Stage
        {
            get
            {
                if (state.complete) return "complete";
                if (!Game || !Game.yard || !Game.yard.Active) return "prologue";
                if (!Camera || !Camera.Acquired) return "collect-camera";
                if (!Game.yard.Completed) return "motor-log";
                var first = Camera.GetFrame(FieldCamera.PhotoId); var second = Camera.GetFrame(FieldCamera.SecondPhotoId);
                if (first == null) return "first-exposure";
                if (!first.developed) return "develop-first";
                if (!state.clueMarked || !state.archiveMatched || string.IsNullOrEmpty(state.hypothesis)) return "interpret-first";
                if (string.IsNullOrEmpty(state.experimentMethod)) return "choose-control";
                if (!state.controlObserved) return "observe-control";
                if (second == null) return "second-exposure";
                if (!second.developed) return "develop-second";
                return state.comparisonConfirmed ? "file-report" : "compare-exposures";
            }
        }
        public string Objective
        {
            get
            {
                switch (Stage)
                {
                    case "collect-camera": return "FIELD KIT // COLLECT THE CAMERA BESIDE THE EAST DOOR";
                    case "motor-log": return "S-03 // READ THE MOTOR CABINET IN THE EAST SERVICE YARD";
                    case "first-exposure": return "S-03 APRON // C: CAMERA / SPACE: EXPOSE THE CENTRAL ANTENNA";
                    case "develop-first": case "develop-second": return state.labStep == 0 ? "SEALED FILM // NORTH DOOR: PHOTOLAB / PROCESS AT THE WET BENCH" : state.labRemaining > 0 ? "PHOTOLAB // THE FILM IS IN THE TANK; FINISH THE PROCESS AT THE BENCH" : state.labStep == 1 ? "PHOTOLAB // TRANSFER THE CONTACT PRINT TO THE FIXER" : "PHOTOLAB // COLLECT THE PROCESSED PRINT";
                    case "interpret-first": return !state.clueMarked ? "FRAME 01 // INSPECT THE PRINT AND MARK A COMPARABLE DETAIL" : !state.archiveMatched ? "PHOTOLAB ARCHIVE // MATCH THE MARKED DETAIL TO THE FACILITY REFERENCE SHEET" : "REFERENCE SHEET // CHOOSE A TESTABLE EXPLANATION";
                    case "choose-control": return "B-12 // FOLLOW THE EAST PATH SOUTH TO THE LOCAL REFERENCE CONTROL";
                    case "observe-control": return "B-12 // NOTE THE DIRECT OBSERVATION AFTER THE VANE SETTLES";
                    case "second-exposure": return "B-12 // C: CAMERA / SPACE: RECORD THE CONTROLLED REFERENCE";
                    case "compare-exposures": return "TWO PRINTS // COMPARE THE SAME REFERENCE AND RECORD WHAT THE TEST SUPPORTS";
                    case "file-report": return "PHOTOLAB // FILE THE TWO-EXPOSURE REPORT AT THE RECORDS DESK";
                    case "complete": return "LOCAL CASE FILED // THE SECOND EXPOSURE";
                    default: return "NIGHT SHIFT // FOLLOW THE RECEIVER PROCEDURE";
                }
            }
        }
        void Start() { InitializeWorld(); }
        void InitializeWorld()
        {
            if (initialized) return; initialized = true;
            if (referenceVane) vaneRest = referenceVane.localRotation;
            if (experimentLight) lampRest = experimentLight.intensity;
            if (photoEcho) photoEcho.SetActive(false);
            if (developingPrint) { printMaterial = developingPrint.material; printMaterial.color = paper; }
            foley = gameObject.AddComponent<AudioSource>(); foley.playOnAwake = false; foley.spatialBlend = 0;
            filmSound = filmHandlingClip ? filmHandlingClip : MakeSound("FilmTank", .38f, 145, .15f);
            transferSound = MakeSound("WetPaperTransfer", .62f, 290, .09f);
            shutterContact = mechanicalClip ? mechanicalClip : MakeSound("ReferenceDetent", .15f, 510, .11f);
            discoveryTone = MakeSound("ContactPrintResonance", 1.25f, 94, .09f);
            ApplyExperiment(1); RefreshPrint();
        }
        static AudioClip MakeSound(string name, float length, float frequency, float strength)
        {
            const int rate = 22050; var samples = new float[Mathf.CeilToInt(length * rate)];
            for (int n = 0; n < samples.Length; n++)
            {
                float t = (float)n / rate, fade = Mathf.Sin(Mathf.PI * t / length); fade *= fade;
                float grain = Mathf.Sin(t * 17339.1f) * Mathf.Sin(t * 9913.3f);
                samples[n] = (Mathf.Sin(t * frequency * Mathf.PI * 2) * .65f + grain * .35f) * fade * strength;
            }
            var clip = AudioClip.Create(name, samples.Length, 1, rate, false); clip.SetData(samples, 0); return clip;
        }
        void Sound(AudioClip clip) { if (foley && clip) foley.PlayOneShot(clip, clip == mechanicalClip ? .20f : clip == filmHandlingClip ? .35f : 1); }
        void Update()
        {
            if (!Game || !Game.hud.Started || Game.hud.Paused) return;
            if (state.labRemaining > 0)
            {
                state.labRemaining = Mathf.Max(0, state.labRemaining - Time.deltaTime); RefreshPrint();
                if (state.labRemaining <= 0) { Game.hud.Toast(state.labStep == 1 ? "CONTACT PRINT READY // TRANSFER TO FIXER" : "PRINT FIXED // COLLECT AT THE BENCH", 4); ChapterSave.RequestCheckpoint(); }
            }
            if (experimentRemaining > 0)
            {
                experimentRemaining = Mathf.Max(0, experimentRemaining - Time.deltaTime);
                ApplyExperiment(1 - experimentRemaining / 1.8f);
                if (experimentRemaining <= 0) { Sound(shutterContact); Game.hud.Toast("B-12 SETTLED // NOTE THE DIRECT OBSERVATION AT THE CONTROL", 4); ChapterSave.RequestCheckpoint(); }
            }
        }
        public void SetFilmResponse(bool enabled) { if (photoEcho) photoEcho.SetActive(enabled); }
        public string ActionPrompt(string action, string fallback)
        {
            if (action == "develop")
            {
                if (!Camera || Camera.PendingFilm == null) return Camera && Camera.DevelopedCount > 1 ? "COMPARE CONTACT PRINTS" : Camera && Camera.DevelopedCount > 0 ? "EXAMINE CONTACT PRINT" : "PHOTOLAB / WET BENCH";
                if (state.labRemaining > 0) return "CHECK PROCESSING TANK";
                return state.labStep == 0 ? "PROCESS SEALED FILM" : state.labStep == 1 ? "TRANSFER PRINT TO FIXER" : "COLLECT PROCESSED PRINT";
            }
            if (action == "experiment") return string.IsNullOrEmpty(state.experimentMethod) ? "B-12 / LOCAL REFERENCE CONTROL" : !state.controlObserved ? "NOTE THE DIRECT OBSERVATION" : "B-12 / REVIEW CONTROL SETTINGS";
            if (action == "report") return state.complete ? "READ FILED CHAPTER REPORT" : "FILE LOCAL INVESTIGATION";
            return string.IsNullOrEmpty(fallback) ? "READ FACILITY REFERENCE SHEET" : fallback;
        }
        public void Interact(string action)
        {
            if (!Game || !Camera || Camera.Capturing) return;
            if (!Game.yard || !Game.yard.Active) { Game.hud.Toast("NIGHT SHIFT // COMPLETE THE RECEIVER PROCEDURE FIRST", 3); return; }
            switch (action)
            {
                case "develop":
                    if (!Camera.SaveReady) Camera.RetryExport();
                    OpenPanel(Camera.PendingFilm != null ? "lab" : Camera.DevelopedCount > 1 ? "comparison" : Camera.DevelopedCount == 1 ? "first" : "lab"); break;
                case "archive":
                    state.archiveRead = true;
                    Game.notebook.Collect(ArchiveId, "B-12 / Reference installation sheet", "Local survey references: B-12 is the movable single ivory stripe south of the S-03 apron. S-07 uses three horizontal bars. The warm inspection lamp and local reference drive have independent controls.");
                    OpenPanel("archive"); ChapterSave.RequestCheckpoint(); break;
                case "experiment": OpenPanel("experiment"); break;
                case "report": OpenPanel(state.complete ? "ending" : "report"); break;
            }
        }
        void OpenPanel(string name)
        {
            panel = name; feedback = ""; zoom = 1; pan = new Vector2(.5f, .5f); hintLevel = 0;
            Game.hud.SetCursor(false); Sound(shutterContact);
        }
        public void ClosePanel() { panel = ""; feedback = ""; if (Game && Game.hud) Game.hud.SetCursor(true); }
        public bool OpenEvidence(string id)
        {
            if (id == FieldCamera.PhotoId || id == FieldCamera.SecondPhotoId)
            {
                var frame = Camera ? Camera.GetFrame(id) : null;
                if (frame == null || !frame.developed) { if (Game) Game.hud.Toast("THIS FRAME STILL NEEDS PROCESSING AT THE PHOTOLAB", 3); return true; }
                selectedPhoto = id; Camera.Inspect(id); OpenPanel(id == FieldCamera.PhotoId ? "first" : "comparison"); return true;
            }
            if (id == ArchiveId) { OpenPanel("archive"); return true; }
            if (id == ControlId) { OpenPanel("experiment"); return true; }
            if (id == ConclusionId) { OpenPanel(state.complete ? "ending" : "report"); return true; }
            return false;
        }
        public void ExposureRecorded(string id)
        {
            if (id == FieldCamera.SecondPhotoId) Game.notebook.Add("The control exposure preserves the local test condition: " + (ExperimentMethod == "passive" ? "lamp shielded, motor isolated." : "reference driven to 042 degrees."));
        }
        void BeginDevelopment()
        {
            var film = Camera.PendingFilm; if (film == null || state.labStep != 0) return;
            state.labFrameId = film.id; state.labStep = 1; state.labRemaining = 2.2f;
            Sound(filmSound); feedback = "SEALED TANK LOADED // The exposed film is safe from room light."; RefreshPrint(); ChapterSave.RequestCheckpoint();
        }
        void TransferPrint()
        {
            if (state.labStep != 1 || state.labRemaining > 0) return;
            state.labStep = 2; state.labRemaining = 1.5f; Sound(transferSound); feedback = "PRINT IN FIXER // The image is becoming stable."; RefreshPrint(); ChapterSave.RequestCheckpoint();
        }
        void CollectPrint()
        {
            if (state.labStep != 2 || state.labRemaining > 0 || !Camera.Develop(state.labFrameId)) return;
            bool second = state.labFrameId == FieldCamera.SecondPhotoId;
            state.labStep = 0; state.labFrameId = ""; Sound(transferSound);
            OpenPanel(second ? "comparison" : "first"); Camera.Inspect(second ? FieldCamera.SecondPhotoId : FieldCamera.PhotoId);
            Game.notebook.Add(second ? "Processed the control print. Both original exposures are available for comparison." : "Processed frame 01. A contact print is ready for close inspection; the facility sheet is at the archive desk.");
            Debug.Log("CHAPTER_PRINT_DEVELOPED " + (second ? "02" : "01")); ChapterSave.RequestCheckpoint();
        }
        void RefreshPrint()
        {
            if (!printMaterial || !Camera) return;
            ExposureRecord frame = !string.IsNullOrEmpty(state.labFrameId) ? Camera.GetFrame(state.labFrameId) : Camera.GetFrame(FieldCamera.SecondPhotoId) ?? Camera.GetFrame(FieldCamera.PhotoId);
            bool visible = frame != null && (frame.developed || state.labStep == 2);
            printMaterial.mainTexture = visible ? frame.texture : null;
            printMaterial.color = visible ? Color.Lerp(new Color(.12f, .08f, .055f), Color.white, frame.developed ? 1 : 1 - state.labRemaining / 1.5f) : paper;
        }
        void SelectReference(bool correct)
        {
            if (!state.clueMarked) { feedback = "Mark a detail in the processed photograph before assigning a reference number."; return; }
            if (!correct) { state.wrongHypotheses++; feedback = "S-07 has THREE HORIZONTAL BARS. Compare that silhouette with the vertical feature you marked. The log does not place S-07 on this sightline."; Game.notebook.Add("Rejected S-07 as the photographed reference: its three-bar pattern does not match the marked foreground detail."); }
            else { state.archiveMatched = true; feedback = "B-12 selected. Its installation sheet specifies ONE ivory stripe on a movable vane. Choose a cause you can test."; Game.notebook.Add("Matched the marked foreground reference to B-12 using the installation sheet and the S-03 sightline."); }
            ChapterSave.RequestCheckpoint();
        }
        void SelectHypothesis(string value)
        {
            if (!state.archiveMatched || !state.clueMarked) { feedback = "Identify the photographed reference first."; return; }
            if (value == "motor-command")
            {
                state.wrongHypotheses++; feedback = "The S-03 log records ZERO commands during the first movement. A second command does not explain this record. Test light or the local reference instead.";
                Game.notebook.Add("Hypothesis rejected: a second motor command contradicts the S-03 zero-command record.");
            }
            else
            {
                state.hypothesis = value; feedback = value == "stray-light" ? "Working hypothesis: reflected work light. At B-12, shielding the lamp changes the light while leaving the motor isolated." : "Working hypothesis: reference/encoder drift. At B-12, a commanded move provides a known reference state.";
                Game.notebook.Add("Working hypothesis: " + (value == "stray-light" ? "an extra stripe caused by reflected work light." : "a mismatch caused by the local reference or encoder."));
                Game.hud.Toast("FIELD TEST AVAILABLE // B-12, SOUTH OF THE S-03 APRON", 5);
            }
            Debug.Log("CHAPTER_HYPOTHESIS " + value); ChapterSave.RequestCheckpoint();
        }
        void BeginExperiment(string method)
        {
            if (!state.archiveMatched || !state.clueMarked || string.IsNullOrEmpty(state.hypothesis) || Camera.GetFrame(FieldCamera.SecondPhotoId) != null) return;
            if (!Near(new Vector3(11.65f, 0, -20.25f), 3.75f)) { feedback = "Operate this control at the B-12 field station. The notebook preserves a record; it cannot move the physical reference."; return; }
            state.experimentMethod = method; state.controlObserved = false; experimentRemaining = 1.8f;
            Sound(shutterContact); ClosePanel();
            Game.hud.Toast(method == "passive" ? "LAMP SHIELD ENGAGED // MOTOR ISOLATED" : "REFERENCE DRIVE // COMMANDING 042 DEGREES", 4);
            Debug.Log("CHAPTER_CONTROL " + method); ChapterSave.RequestCheckpoint();
        }
        void ApplyExperiment(float fraction)
        {
            if (string.IsNullOrEmpty(state.experimentMethod)) { if (referenceVane) referenceVane.localRotation = vaneRest; if (experimentLight) experimentLight.intensity = lampRest; return; }
            float angle = state.experimentMethod == "passive" ? -60 : 45;
            if (referenceVane) referenceVane.localRotation = Quaternion.Slerp(vaneRest, vaneRest * Quaternion.Euler(0, angle, 0), Mathf.SmoothStep(0, 1, fraction));
            if (experimentLight) experimentLight.intensity = Mathf.Lerp(lampRest, state.experimentMethod == "passive" ? 0 : lampRest * 1.35f, fraction);
        }
        void ObserveControl()
        {
            if (string.IsNullOrEmpty(state.experimentMethod) || ExperimentMoving) return;
            if (!Near(new Vector3(11.65f, 0, -20.25f), 3.75f)) { feedback = "Return to B-12 to make a direct observation of the physical reference."; return; }
            state.controlObserved = true;
            string observation = state.experimentMethod == "passive" ? "B-12 / PASSIVE CONTROL\n\nLamp: shielded, no direct illumination.\nMotor: isolated, no drive command.\nDirect observation: one ivory stripe on the folded reference vane; one fixed bracket.\n\nMake a control exposure from the apron without changing this setup." : "B-12 / ACTIVE CONTROL\n\nLocal command: reference 042 degrees.\nEncoder: 042 degrees, reference moved and settled.\nDirect observation: one ivory stripe on the turned vane; one fixed bracket.\nWork lamp remains lit.\n\nMake a control exposure without changing this setup.";
            Game.notebook.RemoveEvidence(ControlId); Game.notebook.Collect(ControlId, "B-12 / Direct control observation", observation);
            Game.notebook.Add("Direct control observation: one physical ivory stripe. The " + (state.experimentMethod == "passive" ? "work lamp is shielded and motor remains isolated." : "local reference responds to the 042-degree command."));
            ClosePanel(); Game.hud.Toast("DIRECT OBSERVATION NOTED // C: FRAME THE B-12 VANE / SPACE: EXPOSE", 5); ChapterSave.RequestCheckpoint();
        }
        void Conclude(bool correct, string wrong = "")
        {
            if (!state.secondClueMarked) { feedback = "Mark the reference detail in FRAME 02 before recording a conclusion."; return; }
            if (!correct)
            {
                state.wrongConclusions++;
                feedback = wrong == "light" ? state.experimentMethod == "passive" ? "The lamp was shielded for frame 02. Its extra mark cannot be explained by that lamp remaining on." : "The vane moved to a known angle while the second mark kept its old alignment. A lamp reflection alone does not account for the fixed second reference." : "The first S-03 movement had no command. The control used a different local reference. These records do not support two antenna commands.";
                ChapterSave.RequestCheckpoint(); return;
            }
            if (state.comparisonConfirmed) return;
            state.comparisonConfirmed = true; Camera.Compare();
            Game.notebook.Collect(ConclusionId, "Two exposures / Local finding", ConclusionText());
            feedback = "FINDING RECORDED // The physical reference changed. The film retains a second reference. File both prints at the records desk.";
            Game.notebook.Add("The second exposure reproduces the extra reference stripe under a controlled condition. The local test rules out " + (state.experimentMethod == "passive" ? "the work lamp as its source." : "simple reference/encoder drift as its source."));
            StartCoroutine(SignatureResponse()); Debug.Log("CHAPTER_COMPARISON_CONFIRMED " + state.experimentMethod); ChapterSave.RequestCheckpoint();
        }
        IEnumerator SignatureResponse()
        {
            Sound(discoveryTone);
            for (int group = 0; group < 2; group++)
            {
                int count = group == 0 ? 4 : 7;
                for (int i = 0; i < count; i++) { Sound(shutterContact); yield return new WaitForSeconds(.16f); }
                yield return new WaitForSeconds(.42f);
            }
        }
        string ConclusionText()
        {
            return "SARO / LOCAL INCIDENT S-03 + B-12\n\nS-03: encoder 026; scheduled 042; commands 0.\nFrame 01: foreground reference does not match the one-stripe installation sheet.\n\nCONTROL: " + (state.experimentMethod == "passive" ? "lamp shielded, motor isolated." : "local reference commanded to 042; encoder and visible vane agree.") + "\nDirect observation: one physical stripe.\nFrame 02: the physical vane changes; a second stripe retains its earlier alignment.\n\nSUPPORTED: the discrepancy persists in the photographic record under the chosen control. " + (state.experimentMethod == "passive" ? "The work lamp is not sufficient to explain it." : "Simple reference/encoder drift is not sufficient to explain it.") + "\n\nLIMIT: one local test does not establish the cause of the array movement or the signal. Preserve the negatives and the original receiver printout.";
        }
        void FileReport()
        {
            if (!state.comparisonConfirmed || Camera.DevelopedCount != 2 || !state.controlObserved) return;
            if (!Near(new Vector3(2.7f, 0, 12.45f), 3.5f)) { feedback = "Place the prints in the physical case file at the photolab records desk."; return; }
            state.complete = true;
            Game.notebook.Add("LOCAL CASE CLOSED: S-03 and B-12 records filed together. Preserve negatives. Follow-up: why does the original return report a distance of -39 LY?");
            OpenPanel("ending"); Sound(transferSound); Debug.Log("CHAPTER_COMPLETED method=" + state.experimentMethod); ChapterSave.RequestCheckpoint();
        }
        bool Near(Vector3 point, float range)
        {
            if (!Game || !Game.player) return false;
            Vector3 position = Game.player.transform.position; position.y = point.y;
            return Vector3.Distance(position, point) <= range;
        }

        void Styles()
        {
            if (ink != null) return;
            Font font = Game.hud.terminalFont;
            ink = new GUIStyle(GUI.skin.label) { font = font, fontSize = 25, wordWrap = true, normal = { textColor = dark } };
            small = new GUIStyle(ink) { fontSize = 21 };
            heading = new GUIStyle(ink) { fontSize = 34 };
            button = new GUIStyle(ink) { fontSize = 24, alignment = TextAnchor.MiddleCenter, wordWrap = true, normal = { textColor = new Color(.97f, .95f, .86f) } };
        }
        void Block(Rect rect, Color color) { Color old = GUI.color; GUI.color = color; GUI.DrawTexture(rect, Texture2D.whiteTexture); GUI.color = old; }
        bool Button(Rect rect, string text, bool enabled = true)
        {
            Block(rect, enabled ? accent : new Color(.58f, .62f, .54f));
            GUI.Label(rect, text, button);
            return enabled && GUI.Button(rect, GUIContent.none, GUIStyle.none);
        }
        void Label(float x, float y, float w, float h, string value, bool compact = false) { GUI.Label(new Rect(x, y, w, h), value, compact ? small : ink); }
        public void DrawPanel()
        {
            if (!ModalOpen || !Game || !Camera) return;
            Styles();
            Block(new Rect(0, 0, Screen.width, Screen.height), new Color(.018f, .025f, .021f, .975f));
            float scale = Mathf.Min(Screen.width / 1280f, Screen.height / 800f);
            Matrix4x4 previous = GUI.matrix;
            GUI.matrix = Matrix4x4.TRS(new Vector3((Screen.width - 1280 * scale) / 2, (Screen.height - 800 * scale) / 2, 0), Quaternion.identity, Vector3.one * scale);
            Block(new Rect(24, 24, 1232, 752), paper);
            GUI.Label(new Rect(50, 40, 1030, 45), "S A R O  /  " + PanelTitle(), heading);
            Block(new Rect(50, 91, 1180, 2), accent);
            if (Button(new Rect(1110, 42, 120, 38), "CLOSE")) { ClosePanel(); GUI.matrix = previous; return; }
            switch (panel)
            {
                case "lab": DrawLab(); break;
                case "first": DrawFirst(); break;
                case "archive": DrawArchive(); break;
                case "experiment": DrawExperiment(); break;
                case "comparison": DrawComparison(); break;
                case "report": DrawReport(); break;
                case "ending": DrawEnding(); break;
            }
            if (!string.IsNullOrEmpty(feedback))
            {
                Block(new Rect(48, 662, 1184, 80), new Color(.78f, .81f, .69f));
                Label(64, 670, 1150, 70, feedback, true);
            }
            if (Camera.SaveStatus.StartsWith("EXPORT FAILED"))
            {
                Label(52, 745, 825, 25, "PHOTO EXPORT FAILED / The exposure remains in memory. Retry before quitting.", true);
                if (Button(new Rect(912, 743, 314, 29), "RETRY PHOTO EXPORT")) Camera.RetryExport();
            }
            else Label(52, 748, 1150, 24, "NIGHT SHIFT 1986  /  ESC CLOSE  /  Originals remain in the local field-photo archive", true);
            GUI.matrix = previous;
        }
        string PanelTitle()
        {
            switch (panel) { case "lab": return "W E T  B E N C H"; case "archive": return "R E F E R E N C E  F I L E"; case "experiment": return "B - 1 2  C O N T R O L"; case "comparison": return "T W O  E X P O S U R E S"; case "report": return "L O C A L  R E P O R T"; case "ending": return "T H E  S E C O N D  E X P O S U R E"; default: return "C O N T A C T  P R I N T  0 1"; }
        }
        void DrawLab()
        {
            var film = Camera.PendingFilm;
            if (film == null) { Label(75, 140, 1100, 150, "The tank is empty.\nExpose a frame from the S-03 apron before loading film. The archive desk holds the local reference plan."); return; }
            Label(75, 125, 1070, 55, (film.id == FieldCamera.PhotoId ? "SEALED FRAME 01 / S-03" : "SEALED FRAME 02 / B-12 CONTROL") + "    " + film.localTime);
            string[] steps = { "1 / LOAD LIGHT-TIGHT TANK", "2 / TRANSFER PRINT TO FIXER", "3 / COLLECT STABLE PRINT" };
            for (int i = 0; i < 3; i++)
            {
                Block(new Rect(75 + i * 380, 204, 350, 184), state.labStep == i ? new Color(.70f, .75f, .63f) : new Color(.81f, .82f, .73f));
                Label(91 + i * 380, 221, 316, 70, steps[i]);
                Label(91 + i * 380, 297, 316, 70, i == 0 ? "The exposed film stays protected from room light." : i == 1 ? "Lift the contact print into the adjacent tray." : "The image can now be handled, filed and examined.", true);
            }
            if (state.labRemaining > 0)
            {
                Label(80, 431, 1100, 65, state.labStep == 1 ? "The contact sheet is forming in the tank. The transfer handle releases when ready." : "The fixer stabilizes the image. The print can be collected in a moment.");
                float duration = state.labStep == 1 ? 2.2f : 1.5f; Block(new Rect(80, 510, 1100, 6), new Color(.64f, .69f, .59f)); Block(new Rect(80, 510, 1100 * (1 - state.labRemaining / duration), 6), accent);
            }
            else if (state.labStep == 0 && Button(new Rect(340, 454, 600, 66), "LOAD SEALED FILM INTO TANK")) BeginDevelopment();
            else if (state.labStep == 1 && Button(new Rect(340, 454, 600, 66), "TRANSFER CONTACT PRINT TO FIXER")) TransferPrint();
            else if (state.labStep == 2 && Button(new Rect(340, 454, 600, 66), "COLLECT AND EXAMINE PRINT")) CollectPrint();
            Label(80, 558, 850, 55, Camera.SaveStatus, true);
            if (!Camera.SaveReady && Button(new Rect(950, 554, 230, 48), "RETRY PHOTO EXPORT")) Camera.RetryExport();
        }
        void DrawFirst()
        {
            var frame = Camera.GetFrame(FieldCamera.PhotoId);
            if (frame == null || !frame.developed) { Label(75, 140, 1050, 100, "The first frame must be processed at the wet bench."); return; }
            Camera.Inspect(frame.id);
            Label(55, 107, 800, 35, "FRAME 01 / S-03 ARRAY PROFILE / " + frame.localTime, true);
            DrawPhoto(frame, new Rect(55, 149, 810, 438), false);
            DrawZoom(884, 149, 334);
            Label(884, 258, 334, 160, state.clueMarked ? "REFERENCE DETAIL MARKED\n\nCompare its shape and position with the installation sheet. Keep observations separate from causes." : "CLICK A DETAIL IN THE PRINT\n\nFind something that can also be checked directly at the facility. Zoom and move the crop if necessary.", true);
            if (Button(new Rect(884, 443, 334, 48), "A SMALL HINT")) { hintLevel++; feedback = hintLevel == 1 ? "Look for a manufactured reference feature, not the shape of the antenna itself." : hintLevel == 2 ? "The nearer equipment below the central dish carries a stripe. Its mounting appears in the facility reference sheet." : "The B-12 assembly is in the lower foreground, in front of the dish. Mark its cream stripe and compare the number of stripes with the sheet."; }
            if (Button(new Rect(884, 511, 334, 58), "REFERENCE SHEET", state.archiveRead)) OpenPanel("archive");
            Label(60, 605, 1150, 43, state.clueMarked ? "MARK SAVED / The unaltered exposure remains available in your notebook." : "Click to mark a location. Marking never edits the source photograph.", true);
        }
        void DrawZoom(float x, float y, float width)
        {
            if (Button(new Rect(x, y, 82, 42), "−")) zoom = Mathf.Max(1, zoom - .75f);
            Label(x + 86, y + 5, 155, 36, "LOUPE " + zoom.ToString("0.0") + "x", true);
            if (Button(new Rect(x + width - 82, y, 82, 42), "+")) zoom = Mathf.Min(4, zoom + .75f);
            Label(x, y + 49, 30, 30, "X", true); pan.x = GUI.HorizontalSlider(new Rect(x + 31, y + 57, width - 31, 20), pan.x, 0, 1);
            Label(x, y + 76, 30, 30, "Y", true); pan.y = GUI.HorizontalSlider(new Rect(x + 31, y + 84, width - 31, 20), pan.y, 1, 0);
        }
        void DrawPhoto(ExposureRecord frame, Rect outer, bool second)
        {
            Block(outer, new Color(.965f, .952f, .882f)); if (frame == null || !frame.texture) return;
            float aspect = (float)frame.width / frame.height;
            Rect rect = new Rect(outer.x + 9, outer.y + 9, outer.width - 18, outer.height - 18);
            if (rect.width / rect.height > aspect) { float w = rect.height * aspect; rect.x += (rect.width - w) / 2; rect.width = w; }
            else { float h = rect.width / aspect; rect.y += (rect.height - h) / 2; rect.height = h; }
            float size = 1 / zoom; Vector2 lower = new Vector2((1 - size) * pan.x, (1 - size) * pan.y);
            GUI.DrawTextureWithTexCoords(rect, frame.texture, new Rect(lower.x, lower.y, size, size));
            if (Event.current.type == EventType.MouseDown && Event.current.button == 0 && rect.Contains(Event.current.mousePosition))
            {
                Vector2 local = new Vector2((Event.current.mousePosition.x - rect.x) / rect.width, 1 - (Event.current.mousePosition.y - rect.y) / rect.height);
                Vector2 uv = lower + local * size;
                float distance = Mathf.Min(Vector2.Distance(uv, frame.referenceViewport), Vector2.Distance(uv, frame.echoViewport));
                if (distance < (second ? .115f : .095f))
                {
                    if (second) state.secondClueMarked = true; else state.clueMarked = true;
                    feedback = second ? "CONTROL REFERENCE MARKED // Compare the moved physical vane with the mark that keeps its earlier alignment." : "DETAIL MARKED // Now identify this manufactured reference using the facility sheet.";
                    ChapterSave.RequestCheckpoint();
                }
                else feedback = "No comparable reference feature at that point. Try a different manufactured detail, or request a hint.";
                Event.current.Use();
            }
            if (second ? state.secondClueMarked : state.clueMarked)
            {
                Vector2 point = (frame.echoViewport - lower) / size;
                Vector2 pixel = new Vector2(rect.x + point.x * rect.width, rect.y + (1 - point.y) * rect.height);
                if (rect.Contains(pixel)) { Color color = new Color(.94f, .79f, .34f); Block(new Rect(pixel.x - 13, pixel.y - 13, 26, 2), color); Block(new Rect(pixel.x - 13, pixel.y + 11, 26, 2), color); Block(new Rect(pixel.x - 13, pixel.y - 13, 2, 26), color); Block(new Rect(pixel.x + 11, pixel.y - 13, 2, 26), color); }
            }
        }
        void DrawArchive()
        {
            Label(65, 113, 1100, 45, "LOCAL SURVEY REFERENCES / INSTALLATION SHEET 11-86");
            Block(new Rect(65, 174, 400, 320), new Color(.80f, .82f, .71f));
            Label(90, 190, 350, 45, "SIGHTLINE FROM SERVICE APRON", true);
            Block(new Rect(236, 249, 7, 196), accent);
            Label(110, 251, 125, 56, "S-03\nAPRON", true); Label(270, 308, 162, 60, "B-12\nREFERENCE", true); Label(103, 414, 133, 60, "CENTRAL\nANTENNA", true);
            Block(new Rect(222, 247, 36, 10), dark); Block(new Rect(219, 321, 42, 12), dark); Block(new Rect(211, 438, 58, 10), dark);
            Label(496, 176, 702, 85, "B-12: one ivory stripe on a movable reference vane. The fixed bracket carries the station number. A local lamp and an isolated reference drive allow two different checks.");
            Label(496, 282, 702, 83, "S-07: three horizontal bars, on the west fence. It does not lie between the S-03 service apron and the central antenna.");
            Label(496, 383, 702, 85, "A contact print can preserve an optical error as well as a physical object. Make a direct observation and change one known condition before trusting a cause.");
            if (!state.archiveMatched)
            {
                if (Button(new Rect(69, 524, 547, 55), "MATCH MARKED DETAIL: B-12")) SelectReference(true);
                if (Button(new Rect(637, 524, 547, 55), "MATCH MARKED DETAIL: S-07")) SelectReference(false);
            }
            else
            {
                if (Button(new Rect(65, 510, 362, 65), "HYPOTHESIS: STRAY LIGHT")) SelectHypothesis("stray-light");
                if (Button(new Rect(443, 510, 362, 65), "HYPOTHESIS: ENCODER DRIFT")) SelectHypothesis("encoder-drift");
                if (Button(new Rect(821, 510, 362, 65), "HYPOTHESIS: MOTOR COMMAND")) SelectHypothesis("motor-command");
            }
            if (Button(new Rect(420, 600, 440, 42), "RETURN TO FRAME 01", Camera.GetFrame(FieldCamera.PhotoId)?.developed == true)) OpenPanel("first");
        }
        void DrawExperiment()
        {
            Label(65, 112, 1100, 47, "B-12 / LOCAL OPTICAL REFERENCE / MANUAL CONTROL ONLY");
            if (!state.archiveMatched || string.IsNullOrEmpty(state.hypothesis))
            {
                Label(76, 195, 1100, 150, "The local control is available, but a useful test needs a question.\n\nProcess frame 01, mark its reference detail and choose a testable explanation using the photolab archive sheet."); return;
            }
            bool locked = Camera.GetFrame(FieldCamera.SecondPhotoId) != null;
            Label(75, 174, 1100, 76, "WORKING HYPOTHESIS: " + (state.hypothesis == "stray-light" ? "STRAY LIGHT / REFLECTION" : "LOCAL REFERENCE / ENCODER DRIFT") + "\nChange a known condition, observe directly, then photograph the unchanged setup.");
            Block(new Rect(75, 279, 538, 186), new Color(.79f, .81f, .71f));
            Block(new Rect(638, 279, 538, 186), new Color(.79f, .81f, .71f));
            Label(94, 292, 500, 151, "PASSIVE / SHIELD THE LAMP\n\nFold the vane into its shield stop. The warm lamp is blocked and the reference motor remains isolated. A reflection should depend on its source.");
            Label(657, 292, 500, 151, "ACTIVE / DRIVE TO 042\n\nMove the local reference to a known heading. Compare the visible vane with its encoder. The work lamp remains on.");
            if (!locked)
            {
                if (Button(new Rect(76, 484, 538, 57), "USE PASSIVE SHIELD CONTROL", !ExperimentMoving)) BeginExperiment("passive");
                if (Button(new Rect(638, 484, 538, 57), "USE ACTIVE REFERENCE CONTROL", !ExperimentMoving)) BeginExperiment("active");
            }
            else Label(85, 482, 1090, 65, "CONTROL EXPOSURE SEALED / Its setup is fixed in the photographic record. Return to the photolab to process the film.");
            if (!string.IsNullOrEmpty(state.experimentMethod))
            {
                string note = ExperimentMoving ? "REFERENCE IN MOTION" : state.controlObserved ? "DIRECT OBSERVATION FILED / " + state.experimentMethod.ToUpperInvariant() : "NOTE DIRECT OBSERVATION: ONE PHYSICAL STRIPE";
                if (Button(new Rect(250, 584, 780, 56), note, !ExperimentMoving && !locked)) ObserveControl();
            }
        }
        void DrawComparison()
        {
            var first = Camera.GetFrame(FieldCamera.PhotoId); var second = Camera.GetFrame(FieldCamera.SecondPhotoId);
            if (first == null || second == null || !first.developed || !second.developed) { Label(70, 160, 1100, 130, "Both contact prints must be processed before they can be compared."); return; }
            Camera.Inspect(second.id);
            Label(55, 108, 550, 35, "01 / BASELINE / " + first.localTime, true);
            Label(650, 108, 570, 35, "02 / " + second.method.ToUpperInvariant() + " CONTROL / " + second.localTime, true);
            DrawPhoto(first, new Rect(55, 153, 566, 337), false);
            DrawPhoto(second, new Rect(650, 153, 566, 337), true);
            if (Button(new Rect(57, 508, 110, 41), "−")) zoom = Mathf.Max(1, zoom - .75f);
            if (Button(new Rect(178, 508, 110, 41), "+")) zoom = Mathf.Min(4, zoom + .75f);
            Label(301, 509, 140, 35, "LOUPE " + zoom.ToString("0.0") + "x", true);
            pan.x = GUI.HorizontalSlider(new Rect(455, 519, 240, 20), pan.x, 0, 1); pan.y = GUI.HorizontalSlider(new Rect(720, 519, 240, 20), pan.y, 1, 0);
            if (Button(new Rect(985, 508, 230, 41), "COMPARE A DETAIL")) { hintLevel++; feedback = hintLevel == 1 ? "Mark the same assembly in the right-hand print. What moved when the control changed? What did not?" : "Look at the ivory reference stripe and the second pale mark beside it. The second mark keeps the original alignment while the physical vane turns."; }
            if (Button(new Rect(55, 583, 362, 62), "THE LAMP CREATED THE MARK")) Conclude(false, "light");
            if (Button(new Rect(439, 583, 362, 62), "TWO ANTENNA COMMANDS")) Conclude(false, "commands");
            if (Button(new Rect(823, 583, 394, 62), "FILM RETAINS A SECOND REFERENCE")) Conclude(true);
        }
        void DrawReport()
        {
            if (!state.comparisonConfirmed) { Label(80, 165, 1100, 150, "The local case needs a supported conclusion.\n\nCompare both developed prints, mark the control reference and state what the test actually supports. Preserve uncertainty about the larger signal."); return; }
            Label(75, 119, 1090, 433, ConclusionText());
            if (Button(new Rect(350, 578, 580, 65), "FILE PRINTS AND CLOSE THE LOCAL CASE")) FileReport();
        }
        void DrawEnding()
        {
            Label(83, 126, 1100, 58, "LOCAL INCIDENT S-03 / B-12 — FILED");
            Label(83, 205, 1100, 141, "The array moved without a command.\nYou changed a known condition at B-12 and preserved the result.\nThe eye saw one reference. The film kept two.");
            Label(83, 365, 1100, 111, "Your " + (state.experimentMethod == "passive" ? "shielded-lamp test rules out the work lamp as a sufficient explanation." : "042-degree reference test rules out simple encoder drift as a sufficient explanation.") + "\nBoth negatives, the motor log and your conclusion now belong to the same case.");
            Block(new Rect(82, 506, 1100, 2), accent);
            Label(83, 523, 1100, 75, "One line on the original receiver print remains unexplained:\nDISTANCE: −39 LY. What does the receiver mean by ‘behind’?");
            if (Button(new Rect(343, 616, 594, 46), "RETURN TO THE OBSERVATORY")) ClosePanel();
            Label(83, 685, 1100, 49, "Music: ‘Signal to Noise’ — Scott Buckley / CC BY 4.0 / scottbuckley.com.au\nNo-piano mix, excerpt with fades. License: creativecommons.org/licenses/by/4.0/", true);
        }
        public string CaptureState() => JsonUtility.ToJson(state);
        public static bool ValidateState(string json, out string reason)
        {
            reason = "";
            try
            {
                var value = JsonUtility.FromJson<State>(json);
                if (value == null || value.version != 1 || value.labStep < 0 || value.labStep > 2 || float.IsNaN(value.labRemaining) || value.labRemaining < 0 || value.labRemaining > 3 || (!string.IsNullOrEmpty(value.experimentMethod) && value.experimentMethod != "passive" && value.experimentMethod != "active")) { reason = "Invalid chapter progress record."; return false; }
                if ((!string.IsNullOrEmpty(value.hypothesis) && value.hypothesis != "stray-light" && value.hypothesis != "encoder-drift") || (value.complete && !value.comparisonConfirmed) || (value.labStep > 0 && value.labFrameId != FieldCamera.PhotoId && value.labFrameId != FieldCamera.SecondPhotoId)) { reason = "Inconsistent chapter progress record."; return false; }
                return true;
            }
            catch (Exception ex) { reason = "Unreadable chapter progress: " + ex.Message; return false; }
        }
        public void RestoreState(string json)
        {
            if (!ValidateState(json, out string reason)) throw new InvalidDataException(reason);
            InitializeWorld(); state = JsonUtility.FromJson<State>(json);
            if (!Camera.HasPhoto)
            {
                state.clueMarked = false; state.archiveMatched = false; state.hypothesis = ""; state.experimentMethod = ""; state.controlObserved = false;
                state.secondClueMarked = false; state.comparisonConfirmed = false; state.complete = false;
                Game.notebook.RemoveEvidence(ControlId); Game.notebook.RemoveEvidence(ConclusionId);
            }
            else if (Camera.GetFrame(FieldCamera.SecondPhotoId) == null)
            {
                state.secondClueMarked = false; state.comparisonConfirmed = false; state.complete = false; Game.notebook.RemoveEvidence(ConclusionId);
            }
            if (state.labStep != 0 && (string.IsNullOrEmpty(state.labFrameId) || Camera.GetFrame(state.labFrameId) == null || Camera.GetFrame(state.labFrameId).developed)) { state.labStep = 0; state.labRemaining = 0; state.labFrameId = ""; }
            panel = ""; experimentRemaining = 0; ApplyExperiment(1); RefreshPrint(); SetFilmResponse(false);
        }
        void OnDestroy()
        {
            if (printMaterial) Destroy(printMaterial); if (filmSound && filmSound != filmHandlingClip) Destroy(filmSound); if (transferSound) Destroy(transferSound); if (shutterContact && shutterContact != mechanicalClip) Destroy(shutterContact); if (discoveryTone) Destroy(discoveryTone);
        }
    }
}
