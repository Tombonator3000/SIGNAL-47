using System;
using System.IO;
using UnityEngine;
using Signal47.Chapter;
using Signal47.Core;

namespace Signal47.WorldCase22
{
    /// <summary>Archive deductions following the preserved two-exposure case. The chapter checkpoint owns persistence.</summary>
    public sealed partial class WorldCaseController : MonoBehaviour
    {
        public Font font;
        // Reserved for a player-safe, revealed-knowledge map. Do not assign the full production concept here.
        public Texture2D worldMap;
        public event Action Changed;
        public bool ModalOpen { get; private set; }
        public bool Available => Game && Game.chapter && Game.chapter.Complete;
        public bool P04Complete => state.p04Complete;
        public bool P05Complete => state.p05Complete;
        public bool ReadOriginal => state.readOriginal;
        public bool ReadAmended => state.readAmended;
        public bool ReadMaintenance => state.readMaintenance;
        public bool ReadIndex => state.readIndex;
        public string CurrentPage { get; private set; } = "original";
        public string Feedback { get; private set; } = "";
        public string Objective => !P04Complete ? "PHOTOLAB ARCHIVE // COMPARE THE ORIGINAL AND AMENDED FIELD RECORDS" : !P05Complete ? "PHOTOLAB ARCHIVE // MATCH THE B-12 LINEAGE TO THE ARCHIVE INDEX" : Game && Game.station ? Game.station.Objective : "STATION 01 // FIELD ACCESS PREPARED / NEXT AREA NOT YET PLAYABLE";
        GameSession Game => GameSession.Instance;
        bool SessionReady => Available && Game.hud && Game.hud.Started && !Game.hud.Paused && !Game.hud.TitleVisible && !Game.Transitioning && !ChapterSave.IsRestoring && !ChapterSave.QuitPending;

        [Serializable] sealed class State
        {
            public int version = 1;
            public bool readOriginal, readAmended, readMaintenance, readIndex, p04Complete, p05Complete;
            public string finding = "", destination = "", surveyId = "", marker = "";
        }
        State state = new State();
        void OnEnable() { Changed += ChapterSave.RequestCheckpoint; }
        void OnDisable() { Changed -= ChapterSave.RequestCheckpoint; }
        public static bool HasProgress(string json)
        {
            if (string.IsNullOrEmpty(json)) return false;
            if (!ValidateState(json, out _)) return true;
            var value = JsonUtility.FromJson<State>(json);
            return value.readOriginal || value.readAmended || value.readMaintenance || value.readIndex || value.p04Complete || value.p05Complete;
        }
        public bool Open()
        {
            if (!SessionReady || (Game.hud.ModalOpen && !ModalOpen)) return false;
            if (ModalOpen) return true;
            ModalOpen = true; Feedback = "";
            Game.hud.SetCursor(false);
            SelectPage(P05Complete ? "route" : P04Complete ? "maintenance" : "original");
            return true;
        }
        public void Close()
        {
            ModalOpen = false; Feedback = "";
            if (Game && Game.hud) Game.hud.SetCursor(!Game.hud.Paused && Game.hud.Started && !Game.hud.TitleVisible);
        }
        public bool SelectPage(string page)
        {
            if (!SessionReady || !ModalOpen) return false;
            bool changed = false;
            switch (page)
            {
                case "original": changed = !state.readOriginal; state.readOriginal = true; break;
                case "amended": changed = !state.readAmended; state.readAmended = true; break;
                case "maintenance": changed = !state.readMaintenance; state.readMaintenance = true; break;
                case "index": changed = !state.readIndex; state.readIndex = true; break;
                case "compare": break;
                case "route": break;
                default: return false;
            }
            CurrentPage = page; Feedback = ""; scroll = Vector2.zero;
            if (changed) Changed?.Invoke();
            return true;
        }
        public bool SubmitComparison(string finding)
        {
            if (!SessionReady || !ModalOpen || CurrentPage != "compare") return false;
            if (!ReadOriginal || !ReadAmended) { Feedback = "Read both the original protocol and amended copy before recording a comparison."; return false; }
            if (finding != "omitted-c")
            {
                Feedback = finding == "author-guilt" ? "The shared signature identifies the report. It does not establish the author's motive or responsibility. Which reference changes between the two versions?" : "Compare the arrangement, not just the amended explanation. Which reference appears in the original and is omitted from the service copy?";
                return false;
            }
            Feedback = "SUPPORTED / E07 records A, B and C and a retained mark with the lamp isolated. E06 omits C, the closing sight line, and calls the mark a development fault. The report was changed; motive remains unknown.";
            if (!state.p04Complete) { state.p04Complete = true; state.finding = finding; Changed?.Invoke(); }
            return true;
        }
        public bool SubmitDestination(string destination, string surveyId, string marker)
        {
            if (!SessionReady || !ModalOpen || CurrentPage != "route") return false;
            if (!P04Complete) { Feedback = "Record what changed in the two field records before preparing the field destination."; return false; }
            if (!ReadMaintenance || !ReadIndex) { Feedback = "Read both the B-12 lineage card and archive sleeve index. A place name alone does not establish the connection."; return false; }
            if (surveyId != "STATION 01") { Feedback = "Match the survey ID in both sources. The dated archive supplies 1947; -39 LY is not a calendar code or a survey ID."; return false; }
            if (marker != "triangle-bar") { Feedback = "The matching fixed-point mark has a short bar beneath the outlined triangle. A triangle without the bar is only an elevation symbol."; return false; }
            if (destination != "old-survey-station") { Feedback = "The B-12 service lineage and matching archive sleeve identify OLD SURVEY STATION. The documents do not establish another destination."; return false; }
            Feedback = "SUPPORTED / B-12 retains STATION 01. The archive sleeve matches its ID and triangle-with-bar fixed-point mark. OLD SURVEY STATION is the supported destination. Field access prepared. Consult the travel folio beside the archive dossier for departure.";
            if (!state.p05Complete)
            {
                state.p05Complete = true; state.destination = destination; state.surveyId = surveyId; state.marker = marker; Changed?.Invoke();
            }
            return true;
        }
        public static bool IsDestinationPrepared(string json) => ValidateState(json, out _) && !string.IsNullOrEmpty(json) && JsonUtility.FromJson<State>(json).p05Complete;
        public string CaptureState() => JsonUtility.ToJson(state);
        public static bool ValidateState(string json, out string reason)
        {
            reason = "";
            // Earlier v1 chapter files predate this optional extension and resume with an unread archive.
            if (string.IsNullOrEmpty(json)) return true;
            try
            {
                if (json.Length > 8192) { reason = "The archive progress record is too large."; return false; }
                var value = JsonUtility.FromJson<State>(json);
                if (value == null || value.version != 1) { reason = "Unsupported archive progress record."; return false; }
                if (value.p04Complete != (value.finding == "omitted-c") || (!value.p04Complete && !string.IsNullOrEmpty(value.finding)) || value.p04Complete && !(value.readOriginal && value.readAmended))
                { reason = "The archive comparison is missing its source records."; return false; }
                bool route = value.destination == "old-survey-station" && value.surveyId == "STATION 01" && value.marker == "triangle-bar";
                if (value.p05Complete != route || value.p05Complete && !(value.p04Complete && value.readMaintenance && value.readIndex) || !value.p05Complete && (!string.IsNullOrEmpty(value.destination) || !string.IsNullOrEmpty(value.surveyId) || !string.IsNullOrEmpty(value.marker)))
                { reason = "The archive destination is missing its matched source references."; return false; }
                return true;
            }
            catch (Exception error) when (error is ArgumentException || error is InvalidOperationException)
            { reason = "Unreadable archive progress: " + error.Message; return false; }
        }
        public void RestoreState(string json)
        {
            if (!ValidateState(json, out string reason)) throw new InvalidDataException(reason);
            state = string.IsNullOrEmpty(json) ? new State() : JsonUtility.FromJson<State>(json);
            ModalOpen = false; CurrentPage = "original"; Feedback = ""; scroll = Vector2.zero;
            selectedDestination = selectedSurvey = selectedMarker = -1;
        }
    }
}
