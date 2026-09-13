using System;
using System.Collections;
using System.IO;
using UnityEngine;
using Signal47.Chapter;
using Signal47.Core;
using Signal47.Investigation;

namespace Signal47.Station26
{
    /// <summary>
    /// The playable STATION 01 field sequence (P06-P09).  The station is a
    /// small, persistent investigation layer over the main scene; the scene
    /// owner supplies the area root, arrival markers and real FieldCamera.
    /// </summary>
    public sealed partial class StationController : MonoBehaviour
    {
        public Font font;
        public Transform arrival;
        public Transform saroReturn;
        public Transform markerTarget;
        public Transform cableTarget;
        public Light testLamp;
        public GameObject stationRoot;

        public event Action Changed;
        public bool InStation => state.inStation;
        public bool P06Complete => state.p06Complete;
        public bool P07Complete => state.p07Complete;
        public bool P08Complete => state.p08Complete;
        public bool P09Complete => state.p09Complete;
        public bool ModalOpen { get; private set; }
        public bool Traveling { get; private set; }
        public bool LampCovered => state.lampCovered;
        public bool LampObserved => state.lampObserved;
        public bool CableInspected => state.cableInspected;
        public bool ReadE09a => state.readE09a;
        public bool ReadE09b => state.readE09b;
        public bool ReadE10 => state.readE10;
        public bool FutureMotelNotePermitted => P09Complete;
        public string CurrentPage { get; private set; } = "marker";
        public string Feedback { get; private set; } = "";
        public string Objective
        {
            get
            {
                if (!InStation)
                {
                    var camera = Game ? Game.fieldCamera : null;
                    var markerFrame = camera ? camera.GetFrame(FieldCamera.StationMarkerPhotoId) : null;
                    var cableFrame = camera ? camera.GetFrame(FieldCamera.StationCablePhotoId) : null;
                    if (markerFrame != null && !markerFrame.developed) return "SARO PHOTOLAB // DEVELOP FRAME 03 / FIELD RECORD RETAINED";
                    if (cableFrame != null && !cableFrame.developed) return "SARO PHOTOLAB // DEVELOP FRAME 04 / CABLE RECORD RETAINED";
                    return RouteReady ? "FIELD FOLIO // STATION 01 ACCESS PREPARED / USE THE TRAVEL FOLIO" : "SARO ARCHIVE // PREPARE THE STATION 01 FIELD DESTINATION";
                }
                if (!P06Complete && !P07Complete) return "STATION 01 // DOCUMENT THE MOVED MARKER OR RUN A LOCAL LAMP NULL TEST";
                if (!P06Complete) return "STATION 01 // EXPOSE FRAME 03 AND RECORD WHICH FIXED MARKER MOVED";
                if (!P07Complete) return "STATION 01 // COVER THE LOCAL LAMP AND OBSERVE A NORMAL NULL TEST";
                if (!P08Complete) return "STATION 01 // INSPECT THE CABLE BREAK AND EXPOSE FRAME 04";
                if (!P09Complete) return "STATION 01 // MATCH THE 1947 AND 1986 TIMING RECORDS";
                return "STATION 01 // FIELD RECORD COMPLETE / RETURN TO SARO";
            }
        }

        GameSession Game => GameSession.Instance;
        bool SessionReady => Game && Game.hud && Game.hud.Started && !Game.hud.Paused && !Traveling &&
            !Game.hud.TitleVisible && !Game.Transitioning && !ChapterSave.IsRestoring && !ChapterSave.QuitPending && !OtherModalOpen;
        bool OtherModalOpen => Game && Game.hud && Game.hud.ModalOpen && !ModalOpen;
        bool RouteReady
        {
            get
            {
                var worldCase = FindFirstObjectByType<Signal47.WorldCase22.WorldCaseController>();
                return worldCase && worldCase.P05Complete && Game && Game.chapter && Game.chapter.Complete;
            }
        }

        [Serializable]
        sealed class State
        {
            public int version = 1;
            public bool inStation, p06Complete, p07Complete, p08Complete, p09Complete;
            public bool readE09a, readE09b, readE10, lampCovered, lampObserved, cableInspected;
            public string markerAnswer = "", cableAnswer = "", timingAnswer = "";
        }

        State state = new State();
        Vector2 scroll;

        void OnEnable() { Changed += ChapterSave.RequestCheckpoint; }
        void OnDisable() { Changed -= ChapterSave.RequestCheckpoint; }
        void Start() { ApplyAreaState(); }

        public static bool HasProgress(string json)
        {
            if (string.IsNullOrEmpty(json)) return false;
            if (!ValidateState(json, out _)) return true;
            var value = JsonUtility.FromJson<State>(json);
            return value.inStation || value.readE09a || value.readE09b || value.readE10 || value.lampCovered || value.lampObserved || value.cableInspected ||
                value.p06Complete || value.p07Complete || value.p08Complete || value.p09Complete;
        }

        public bool Open(string page)
        {
            if (!CanUseUI() || string.IsNullOrEmpty(page)) return false;
            page = NormalizePage(page);
            if (page.Length == 0) return false;
            if (!InStation && page != "travel") return false;
            if (ModalOpen && CurrentPage == page) return true;
            if (Game && Game.hud && Game.hud.ModalOpen && !ModalOpen) return false;
            ModalOpen = true; CurrentPage = page; Feedback = ""; scroll = Vector2.zero;
            if (Game && Game.hud) Game.hud.SetCursor(false);
            return true;
        }

        public void Close()
        {
            ModalOpen = false; Feedback = ""; scroll = Vector2.zero;
            if (Game && Game.hud) Game.hud.SetCursor(Game.hud.Started && !Game.hud.Paused && !Game.hud.TitleVisible);
        }

        public bool ReadSource(string id)
        {
            if (!InStation || !SessionReady || !ModalOpen || string.IsNullOrEmpty(id)) return false;
            id = id.Trim().ToLowerInvariant();
            bool changed = false;
            switch (id)
            {
                case "e09a": case "marker": case "transit":
                    if (CurrentPage != "marker") return false;
                    changed = !state.readE09a; state.readE09a = true;
                    Feedback = "E09A READ / The outlined triangle with a short bar is the fixed point A. Compare it with the visible transit marks.";
                    break;
                case "e09b": case "cable":
                    if (CurrentPage != "cable" || !PhotoReady(FieldCamera.StationCablePhotoId))
                    { Feedback = "FRAME 04 IS NOT AN EXPORTED, GENUINE FIELD RECORD YET."; return false; }
                    changed = !state.readE09b; state.readE09b = true;
                    Feedback = "E09B READ / The near frame shows a clean, opposed cut at the cable break. The break is evidence; it is not a generated historical photograph.";
                    break;
                case "e10": case "timing":
                    if (CurrentPage != "timing") return false;
                    changed = !state.readE10; state.readE10 = true;
                    Feedback = "E10 READ / 02:17:00 carrier ceased. At 02:17:47 the relay impact and reference motion were observed.";
                    break;
                default: return false;
            }
            if (changed) Changed?.Invoke();
            return true;
        }

        /// <summary>Called by the physical field control or its interaction proxy.</summary>
        public void SetLampCovered(bool covered)
        {
            if (!InStation || !SessionReady || !NearLamp()) return;
            state.lampCovered = covered;
            if (testLamp) testLamp.enabled = !covered;
            Changed?.Invoke();
            if (Game && Game.hud) Game.hud.Toast(covered ? "LOCAL LAMP COVERED // NULL TEST READY" : "LOCAL LAMP EXPOSED", 3);
        }

        public bool ObserveLamp()
        {
            if (!InStation || !SessionReady || !NearLamp() || !state.lampCovered)
            {
                Feedback = !NearLamp() ? "MOVE TO THE LOCAL LAMP BEFORE OBSERVING ITS NULL TEST." : "COVER THE LOCAL LAMP BEFORE OBSERVING A NULL TEST.";
                return false;
            }
            state.lampObserved = true;
            Feedback = "NULL TEST RECORDED / The local source is screened. This control result does not complete the cable reconstruction.";
            if (!state.p07Complete) { state.p07Complete = true; Changed?.Invoke(); }
            return true;
        }

        /// <summary>Records the physical action of following the cable to its cut.</summary>
        public bool InspectCable()
        {
            if (!InStation || !SessionReady || !NearCable())
            {
                Feedback = "MOVE TO THE CABLE CUT BEFORE INSPECTING ITS FACES.";
                return false;
            }
            state.cableInspected = true;
            Feedback = "CABLE INSPECTED / Follow the sheath to the opposed cut faces and take the near frame.";
            Changed?.Invoke();
            return true;
        }

        public bool SubmitMarker(string answer)
        {
            if (!InStation || !SessionReady || !ModalOpen || CurrentPage != "marker") return false;
            if (!state.readE09a) { Feedback = "READ E09A / TRANSIT RECORD BEFORE RECORDING THE MARKER FINDING."; return false; }
            if (!PhotoReady(FieldCamera.StationMarkerPhotoId)) { Feedback = "FRAME 03 MUST BE A GENUINE ARCHIVED STATION EXPOSURE."; return false; }
            if (!IsMarkerAnswer(answer))
            {
                Feedback = "The lamp and the cable do not identify the moved fixed point. Match the outlined triangle and short bar, then record A.";
                return false;
            }
            if (state.p06Complete) return true;
            state.p06Complete = true; state.markerAnswer = "fixed-point-a";
            Feedback = "P06 SUPPORTED / The fixed survey marker A has moved relative to the transit record. The original footing and fixed B marker establish the comparison; this record does not identify who moved A.";
            Changed?.Invoke(); return true;
        }

        public bool SubmitCable(string answer)
        {
            if (!InStation || !SessionReady || !ModalOpen || CurrentPage != "cable") return false;
            if (!P06Complete || !P07Complete) { Feedback = "P08 REQUIRES BOTH THE MARKER FINDING AND THE LOCAL LAMP NULL TEST."; return false; }
            if (!PhotoReady(FieldCamera.StationCablePhotoId)) { Feedback = "FRAME 04 MUST BE A GENUINE ARCHIVED CABLE EXPOSURE."; return false; }
            if (!IsCableAnswer(answer))
            {
                Feedback = "The cut faces are opposed and clean. A lamp fault or ordinary drift does not account for that physical break.";
                return false;
            }
            if (state.p08Complete) return true;
            state.p08Complete = true; state.cableAnswer = "deliberate-cut"; state.readE09b = true;
            Feedback = "P08 SUPPORTED / The cable was deliberately cut. The null test separates the local lamp from the retained reference.";
            Changed?.Invoke(); return true;
        }

        public bool SubmitTiming(string answer)
        {
            if (!InStation || !SessionReady || !ModalOpen || CurrentPage != "timing") return false;
            if (!P08Complete) { Feedback = "RECORD THE MARKER AND CABLE FINDINGS BEFORE MATCHING THE TIMING LOG."; return false; }
            if (!state.readE10) { Feedback = "READ E10 / HISTORICAL TIMING RECORD BEFORE SUBMITTING A TIME."; return false; }
            if (!IsTimingAnswer(answer))
            {
                Feedback = "The matched reading arrives at the 47-second relay impact: 02:17:47. Do not infer a new event from the clock alone.";
                return false;
            }
            if (state.p09Complete) return true;
            state.p09Complete = true; state.timingAnswer = "02:17:47";
            Feedback = "P09 SUPPORTED / The historic relay observation followed carrier loss by 47 seconds. The 1986 fragment repeats T. Vega's same correction and adds no final observation.";
            Changed?.Invoke(); return true;
        }

        public bool CanExpose(string photoId)
        {
            if (!InStation || ModalOpen || Traveling) return false;
            if (string.Equals(photoId, FieldCamera.StationMarkerPhotoId, StringComparison.Ordinal)) return state.readE09a && !P06Complete;
            if (string.Equals(photoId, FieldCamera.StationCablePhotoId, StringComparison.Ordinal)) return P06Complete && P07Complete && state.cableInspected && !P08Complete;
            return false;
        }

        public Transform FrameTarget(string photoId)
        {
            if (string.Equals(photoId, FieldCamera.StationMarkerPhotoId, StringComparison.Ordinal)) return markerTarget;
            if (string.Equals(photoId, FieldCamera.StationCablePhotoId, StringComparison.Ordinal)) return cableTarget;
            return null;
        }

        public string ActionPrompt(string action, string fallback)
        {
            switch ((action ?? "").Trim().ToLowerInvariant())
            {
                case "source-e09a": case "marker": case "transit": return ReadE09a ? "E09A / TRANSIT RECORD READ" : "READ E09A / TRANSIT RECORD";
                case "cover-lamp": return LampCovered ? "UNCOVER LOCAL LAMP" : "COVER LOCAL LAMP / NULL TEST";
                case "observe-lamp": return LampObserved ? "REVIEW NULL TEST" : "OBSERVE LOCAL LAMP NULL TEST";
                case "inspect-cable": case "cable": return CableInspected ? "CABLE CUT / INSPECTED" : "INSPECT CABLE CUT FACES";
                case "source-e10": case "timing": return ReadE10 ? "E10 / TIMING RECORD READ" : "READ E10 / TIMING RECORD";
                case "travel": return InStation ? "RETURN TO SARO" : "TRAVEL TO STATION 01";
                case "return": case "return-saro": return InStation ? "RETURN TO SARO" : "SARO / TRAVEL TO STATION 01";
                default: return string.IsNullOrEmpty(fallback) ? "USE STATION 01 FIELD RECORD" : fallback;
            }
        }

        public void Interact(string action)
        {
            string key = (action ?? "").Trim().ToLowerInvariant();
            switch (key)
            {
                case "source-e09a": case "transit":
                    if (Open("marker")) ReadSource("E09a");
                    break;
                case "marker":
                    if (Open("marker")) ReadSource("E09a");
                    break;
                case "lamp": Open("lamp"); break;
                case "cover-lamp": SetLampCovered(!LampCovered); break;
                case "observe-lamp": ObserveLamp(); break;
                case "inspect-cable": InspectCable(); break;
                case "cable": InspectCable(); Open("cable"); break;
                case "source-e10": case "timing":
                    if (Open("timing")) ReadSource("E10");
                    break;
                case "travel": case "return": case "return-saro": case "station": case "travel-station": Open("travel"); break;
                default: Open(key); break;
            }
        }

        public bool Travel(bool toStation)
        {
            if (Traveling || !CanTravel()) return false;
            if (toStation && !RouteReady)
            {
                Feedback = "FIELD ACCESS IS NOT PREPARED. COMPLETE THE ARCHIVE DESTINATION RECORD FIRST.";
                return false;
            }
            if (toStation == InStation) return true;
            if (ModalOpen) Close();
            StartCoroutine(TravelRoutine(toStation));
            return true;
        }

        public bool RetryPhotoExport()
        {
            if (!SessionReady || !InStation || !Game.fieldCamera || !Game.fieldCamera.ExportRetryAvailable) return false;
            Game.fieldCamera.RetryExport();
            Feedback = "RETRYING PHOTO SAVE / KEEP THIS SESSION OPEN UNTIL THE FILM IS SAVED.";
            return true;
        }

        IEnumerator TravelRoutine(bool toStation)
        {
            Traveling = true;
            bool oldLocation = InStation;
            Transform target = toStation ? arrival : saroReturn;
            Vector3 oldPosition = Game.player.transform.position;
            float oldYaw = Game.player.transform.eulerAngles.y, oldPitch = Game.player.ViewPitch;
            if (!target)
            {
                Feedback = "TRAVEL BLOCKED / THE DESTINATION MARKER IS NOT BUILT.";
                Traveling = false; if(Game && Game.hud)Game.hud.Toast(Feedback,8); yield break;
            }
            if (!BeginCheckpoint()) { Traveling = false; if(Game && Game.hud)Game.hud.Toast(Feedback,8); yield break; }
            bool checkpointReady = CheckpointSucceeded();
            float elapsed = 0;
            while (!checkpointReady && elapsed < 8f)
            {
                elapsed += Time.unscaledDeltaTime;
                checkpointReady = CheckpointSucceeded();
                if (!checkpointReady && !ChapterSave.Saving && !CheckpointPending()) break;
                yield return null;
            }
            if (!checkpointReady)
            {
                Feedback = "TRAVEL BLOCKED / THE CURRENT CASE COULD NOT BE CHECKPOINTED. YOUR LOCATION IS UNCHANGED.";
                Traveling = false; if(Game && Game.hud)Game.hud.Toast(Feedback,8); yield break;
            }

            state.inStation = toStation; ApplyAreaState();
            Game.player.RestorePose(target.position, target.eulerAngles.y, 0);
            Changed?.Invoke(); ChapterSave.RequestCheckpoint();
            elapsed = 0; checkpointReady = false;
            while (elapsed < 8f)
            {
                elapsed += Time.unscaledDeltaTime;
                checkpointReady = CheckpointSucceeded();
                if (checkpointReady) break;
                if (!ChapterSave.Saving && !CheckpointPending()) break;
                yield return null;
            }
            if (!checkpointReady)
            {
                state.inStation = oldLocation; ApplyAreaState();
                Game.player.RestorePose(oldPosition, oldYaw, oldPitch);
                Changed?.Invoke();
                Feedback = "TRAVEL FAILED / THE new location was not saved. The prior location and case state were restored.";
            }
            Traveling = false;
        }

        bool BeginCheckpoint()
        {
            if (!Game || !Game.player || !Game.fieldCamera || !Game.fieldCamera.SaveReady || !ChapterSave.CanSave)
            {
                Feedback = ChapterSave.SaveAvailability;
                return false;
            }
            ChapterSave.RequestCheckpoint();
            return true;
        }

        bool CheckpointSucceeded() => !ChapterSave.Saving && ChapterSave.Status.StartsWith("Case saved.", StringComparison.Ordinal);
        bool CheckpointPending() => ChapterSave.Saving || ChapterSave.Status == "Checkpoint queued." || ChapterSave.Status.StartsWith("Saving case", StringComparison.Ordinal);

        bool CanTravel() => SessionReady && Game.player && Game.fieldCamera && Game.fieldCamera.Acquired && Game.fieldCamera.SaveReady;
        bool CanUseUI() => SessionReady;

        void ApplyAreaState()
        {
            if (stationRoot && stationRoot != gameObject) stationRoot.SetActive(state.inStation);
            if (testLamp) testLamp.enabled = !state.lampCovered;
        }

        bool PhotoReady(string id)
        {
            var camera = Game ? Game.fieldCamera : null;
            if (!camera) return false;
            var frame = camera.GetFrame(id);
            return frame != null && frame.texture && frame.exported && frame.method == "field-observation";
        }

        bool NearLamp() => testLamp && Game && Game.player && Vector3.Distance(Game.player.transform.position, testLamp.transform.position) <= 3f;
        bool NearCable() => cableTarget && Game && Game.player && Vector3.Distance(Game.player.transform.position, cableTarget.position) <= 3f;

        /// <summary>Clears only deductions whose required station exposure is missing after recovery.</summary>
        public void ReconcileRestoredPhotos()
        {
            bool changed = false;
            if (P06Complete && !PhotoReady(FieldCamera.StationMarkerPhotoId))
            {
                state.p06Complete = false; state.markerAnswer = ""; changed = true;
                state.p08Complete = false; state.cableAnswer = "";
                state.p09Complete = false; state.timingAnswer = "";
            }
            else if (P08Complete && !PhotoReady(FieldCamera.StationCablePhotoId))
            {
                state.p08Complete = false; state.cableAnswer = ""; changed = true;
                state.p09Complete = false; state.timingAnswer = "";
            }
            if (changed) Changed?.Invoke();
        }

        /// <summary>Used by save recovery to leave a missing field photo in the known SARO area.</summary>
        public bool ForceReturnForPhotoRecovery()
        {
            if (!InStation) return true;
            state.inStation = false; ApplyAreaState(); Changed?.Invoke(); return true;
        }

        public string CaptureState() => JsonUtility.ToJson(state);
        public static bool ValidatePhotoState(string json,string camera,out string reason)
        {
            reason="";if(string.IsNullOrEmpty(json))return true;
            if(!ValidateState(json,out reason))return false;
            var value=JsonUtility.FromJson<State>(json);
            if(value.p06Complete&&!FieldCamera.StateHasFrame(camera,FieldCamera.StationMarkerPhotoId) || value.p08Complete&&!FieldCamera.StateHasFrame(camera,FieldCamera.StationCablePhotoId))
            {reason="Field findings are missing their original exposure records.";return false;}
            return true;
        }
        public static bool ValidateState(string json, out string reason)
        {
            reason = "";
            if (string.IsNullOrEmpty(json)) return true;
            try
            {
                if (json.Length > 8192) { reason = "The station progress record is too large."; return false; }
                var value = JsonUtility.FromJson<State>(json);
                if (value == null || value.version != 1) { reason = "Unsupported station progress record."; return false; }
                if ((!value.p06Complete && !string.IsNullOrEmpty(value.markerAnswer)) || value.p06Complete != IsMarkerAnswer(value.markerAnswer) || value.p06Complete && !value.readE09a)
                { reason = "P06 is missing its transit source or supported marker answer."; return false; }
                if (value.p07Complete != value.lampObserved) { reason = "P07 is missing its null-test observation."; return false; }
                if ((!value.p08Complete && !string.IsNullOrEmpty(value.cableAnswer)) || value.p08Complete != IsCableAnswer(value.cableAnswer) || value.p08Complete && (!value.p06Complete || !value.p07Complete || !value.cableInspected || !value.readE09b))
                { reason = "P08 is missing its prerequisite field findings."; return false; }
                if ((!value.p09Complete && !string.IsNullOrEmpty(value.timingAnswer)) || value.p09Complete != IsTimingAnswer(value.timingAnswer) || value.p09Complete && (!value.p08Complete || !value.readE10))
                { reason = "P09 is missing its supported timing record."; return false; }
                return true;
            }
            catch (Exception ex) { reason = "Unreadable station progress: " + ex.Message; return false; }
        }

        public void RestoreState(string json)
        {
            if (!ValidateState(json, out string reason)) throw new InvalidDataException(reason);
            state = string.IsNullOrEmpty(json) ? new State() : JsonUtility.FromJson<State>(json);
            ModalOpen = false; Traveling = false; CurrentPage = "marker"; Feedback = ""; scroll = Vector2.zero;
            ApplyAreaState();
        }

        static string NormalizePage(string value)
        {
            value = value.Trim().ToLowerInvariant();
            if (value == "e09a" || value == "transit" || value == "source") return "marker";
            if (value == "e09b") return "cable";
            if (value == "e10") return "timing";
            if (value == "travel" || value == "route") return "travel";
            return value == "marker" || value == "lamp" || value == "cable" || value == "timing" ? value : "";
        }

        static bool IsMarkerAnswer(string value)
        {
            value = (value ?? "").Trim().ToLowerInvariant();
            return value == "fixed-point-a" || value == "fixed-a" || value == "a-moved" || value == "marker-a" || value == "triangle-bar" || value == "moved-a" || value == "a";
        }
        static bool IsCableAnswer(string value)
        {
            value = (value ?? "").Trim().ToLowerInvariant();
            return value == "deliberate-cut" || value == "deliberate" || value == "intentional-cut" || value == "intentional-break" || value == "cut-at-junction" || value == "cut";
        }
        static bool IsTimingAnswer(string value)
        {
            value = (value ?? "").Trim().ToLowerInvariant();
            return value == "02:17:47" || value == "2:17:47" || value == "47-second-mark" || value == "relay-impact";
        }
    }
}
