using UnityEngine;
using Signal47.Chapter;

namespace Signal47.UI
{
    public sealed partial class HUDController
    {
        const int PreviousPageSize=6;
        string[] previousIds=System.Array.Empty<string>();
        ChapterSave.PreviousCase[] previousPage=System.Array.Empty<ChapterSave.PreviousCase>();
        ChapterSave.PreviousCase selectedPrevious;
        int previousPageNumber;
        string previousMessage="";
        public bool PreviousShiftsOpen { get; private set; }
        public string SelectedPreviousShift=>selectedPrevious?.Id??"";
        public int PreviousShiftCount=>previousIds.Length;
        public int PreviousShiftPage=>previousPageNumber;
        public int VisiblePreviousShiftCount=>previousPage.Length;

        public void MovePreviousPage(int direction)
        {
            if(!PreviousShiftsOpen||selectedPrevious!=null||(direction!=-1&&direction!=1))return;
            previousPageNumber=Mathf.Clamp(previousPageNumber+direction,0,Mathf.Max(0,(previousIds.Length-1)/PreviousPageSize));
            ReadPreviousPage();
        }

        public void OpenPreviousShifts()
        {
            if(Started||NewShiftConfirmation||ChapterSave.Saving||ChapterSave.IsRestoring||ChapterSave.QuitPending)return;
            previousIds=ChapterSave.PreviousCaseIds(out previousMessage);previousPageNumber=0;selectedPrevious=null;
            PreviousShiftsOpen=true;ReadPreviousPage();SetCursor(false);
        }
        void ReadPreviousPage()
        {
            int start=previousPageNumber*PreviousPageSize,count=Mathf.Min(PreviousPageSize,previousIds.Length-start);
            previousPage=new ChapterSave.PreviousCase[Mathf.Max(0,count)];
            for(int i=0;i<previousPage.Length;i++)previousPage[i]=ChapterSave.InspectPreviousCase(previousIds[start+i]);
        }
        public void SelectPreviousShift(string id)
        {
            if(!PreviousShiftsOpen||System.Array.IndexOf(previousIds,id)<0)return;
            var info=ChapterSave.InspectPreviousCase(id);
            if(!info.Available){previousMessage=info.Detail;return;}
            selectedPrevious=info;previousMessage="";
        }
        public void BackFromPreviousShifts()
        {
            if(selectedPrevious!=null)
            {
                selectedPrevious=null;previousIds=ChapterSave.PreviousCaseIds(out previousMessage);
                previousPageNumber=0;ReadPreviousPage();return;
            }
            PreviousShiftsOpen=false;ChapterSave.RefreshMenuStatus();
        }
        public bool ConfirmPreviousShift()
        {
            if(!PreviousShiftsOpen||selectedPrevious==null||!selectedPrevious.Available)return false;
            if(!ChapterSave.TryContinuePrevious(selectedPrevious.Id,selectedPrevious.Fingerprint))
            {previousMessage=ChapterSave.Status;return false;}
            PreviousShiftsOpen=false;selectedPrevious=null;return true;
        }
        void DrawPreviousShifts()
        {
            float width=Mathf.Min(760,Screen.width-40),height=Mathf.Min(640,Screen.height-40);
            float x=(Screen.width-width)/2,y=(Screen.height-height)/2;
            GUI.color=new Color(.018f,.026f,.022f,.98f);GUI.DrawTexture(new Rect(0,0,Screen.width,Screen.height),Texture2D.whiteTexture);
            GUI.color=new Color(.055f,.085f,.065f,1);GUI.DrawTexture(new Rect(x,y,width,height),Texture2D.whiteTexture);GUI.color=Color.white;
            var small=new GUIStyle(mono){fontSize=18};
            if(selectedPrevious!=null)
            {
                GUI.Label(new Rect(x+24,y+30,width-48,48),"OPEN PREVIOUS SHIFT?",new GUIStyle(big){fontSize=32});
                GUI.Label(new Rect(x+35,y+115,width-70,80),selectedPrevious.Label+"\n"+selectedPrevious.Detail,mono);
                GUI.Label(new Rect(x+35,y+220,width-70,120),"The selected checkpoint becomes your current shift. Your current checkpoint and its recovery copy are kept in Previous Shifts.\n\nThe selected archive and original photographs are preserved.",mono);
                if(GUI.Button(new Rect(x+35,y+380,(width-85)/2,48),"BACK TO LIST",button))BackFromPreviousShifts();
                if(GUI.Button(new Rect(x+50+(width-85)/2,y+380,(width-85)/2,48),"CONTINUE THIS SHIFT",button))ConfirmPreviousShift();
                GUI.Label(new Rect(x+35,y+455,width-70,105),previousMessage,small);
                GUI.Label(new Rect(x+35,y+height-43,width-70,28),"ESC / Back to list",small);return;
            }
            GUI.Label(new Rect(x+24,y+18,width-48,48),"PREVIOUS SHIFTS",new GUIStyle(big){fontSize=32});
            GUI.Label(new Rect(x+30,y+70,width-60,45),"Checkpoints kept when starting a new night shift. Dates use your local clock.",small);
            if(previousIds.Length==0)
                GUI.Label(new Rect(x+35,y+150,width-70,95),string.IsNullOrEmpty(previousMessage)?"No previous shifts yet. When you start a new shift, any existing checkpoint is kept here.":previousMessage,mono);
            for(int i=0;i<previousPage.Length;i++)
            {
                var info=previousPage[i];float rowY=y+122+i*65;
                GUI.enabled=info.Available;
                if(GUI.Button(new Rect(x+30,rowY,width-60,37),$"{previousPageNumber*PreviousPageSize+i+1:00} / {info.Label}",button))SelectPreviousShift(info.Id);
                GUI.enabled=true;GUI.Label(new Rect(x+40,rowY+38,width-80,24),info.Detail,small);
            }
            if(previousIds.Length>0)
            {
                GUI.enabled=previousPageNumber>0;
                if(GUI.Button(new Rect(x+30,y+height-106,130,34),"PREVIOUS",button))MovePreviousPage(-1);
                GUI.enabled=(previousPageNumber+1)*PreviousPageSize<previousIds.Length;
                if(GUI.Button(new Rect(x+width-160,y+height-106,130,34),"NEXT",button))MovePreviousPage(1);
                GUI.enabled=true;GUI.Label(new Rect(x+175,y+height-103,width-350,30),$"PAGE {previousPageNumber+1} / {(previousIds.Length+PreviousPageSize-1)/PreviousPageSize}",new GUIStyle(small){alignment=TextAnchor.MiddleCenter});
            }
            if(GUI.Button(new Rect(x+width*.5f-145,y+height-55,290,38),"BACK TO START MENU",button))BackFromPreviousShifts();
        }
    }
}
