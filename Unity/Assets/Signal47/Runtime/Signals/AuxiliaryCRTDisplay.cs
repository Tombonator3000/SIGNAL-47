using System.Collections.Generic;
using UnityEngine;
using Signal47.Core;

namespace Signal47.Signals
{
    /// <summary>Read-only, low-resolution instrument faces. No puzzle state is changed here.</summary>
    public sealed class AuxiliaryCRTDisplay : MonoBehaviour
    {
        public bool amber;
        public Renderer screen;
        public string StatusLabel { get; private set; } = "STANDBY";
        public int RefreshCount { get; private set; }
        const int Width = 320, Height = 200;
        Texture2D face;
        Material material;
        Color32[] pixels;
        float nextPoll;
        int previousState = -1;
        Color32 ink, faint;
        // Original 5x7 terminal glyphs; UI still uses the existing attributed VT323 font.
        static readonly Dictionary<char,string> Glyphs = new Dictionary<char,string>
        {
            ['A']="01110100011000111111100011000110001", ['B']="11110100011000111110100011000111110",
            ['C']="01111100001000010000100001000001111", ['D']="11110100011000110001100011000111110",
            ['E']="11111100001000011110100001000011111", ['F']="11111100001000011110100001000010000",
            ['G']="01111100001000010111100011000101111", ['H']="10001100011000111111100011000110001",
            ['I']="11111001000010000100001000010011111", ['J']="00111000100001000010000101001001100",
            ['K']="10001100101010011000101001001010001", ['L']="10000100001000010000100001000011111",
            ['M']="10001110111010110101100011000110001", ['N']="10001110011010110011100011000110001",
            ['O']="01110100011000110001100011000101110", ['P']="11110100011000111110100001000010000",
            ['Q']="01110100011000110001101011001001101", ['R']="11110100011000111110101001001010001",
            ['S']="01111100001000001110000010000111110", ['T']="11111001000010000100001000010000100",
            ['U']="10001100011000110001100011000101110", ['V']="10001100011000110001100010101000100",
            ['W']="10001100011000110101101011101110001", ['X']="10001100010101000100010101000110001",
            ['Y']="10001100010101000100001000010000100", ['Z']="11111000010001000100010001000011111",
            ['0']="01110100011001110101110011000101110", ['1']="00100011000010000100001000010001110",
            ['2']="01110100010000100010001000100011111", ['3']="11110000010000101110000010000111110",
            ['4']="00010001100101010010111110001000010", ['5']="11111100001000011110000010000111110",
            ['6']="01110100001000011110100011000101110", ['7']="11111000010001000100010000100001000",
            ['8']="01110100011000101110100011000101110", ['9']="01110100011000101111000010000101110",
            ['-']="00000000000000011111000000000000000", ['/']="00001000100001000100010000100010000",
            [':']="00000001000010000000001000010000000", ['.']="00000000000000000000000000011000110"
        };
        void Start()
        {
            if(!screen) { enabled=false; return; }
            face=new Texture2D(Width,Height,TextureFormat.RGB24,false) { name="SARO auxiliary CRT", filterMode=FilterMode.Bilinear, wrapMode=TextureWrapMode.Clamp };
            pixels=new Color32[Width*Height];
            material=new Material(screen.sharedMaterial); material.SetColor("_BaseColor",Color.white);
            material.SetTexture("_BaseMap",face);screen.sharedMaterial=material;
            ink=amber?new Color32(238,164,63,255):new Color32(83,199,145,255);
            faint=amber?new Color32(55,37,15,255):new Color32(15,48,34,255);
            RefreshFromSession();
        }
        void Update()
        {
            if(Time.unscaledTime<nextPoll)return;
            nextPoll=Time.unscaledTime+.25f;RefreshFromSession();
        }
        public void RefreshFromSession()
        {
            if(!face)return;
            var session=GameSession.Instance;
            var d=session?session.director:null;
            int state=(d&&d.ReceiverPowered?1:0)|(d&&d.PhoneRinging?2:0)|(d&&d.PhoneAnswered?4:0)|(d&&d.LineDead?8:0)|(d&&d.ArrayOverride?16:0);
            if(state==previousState)return;previousState=state;RefreshCount++;
            StatusLabel=(state&1)==0?"STANDBY":amber?((state&8)!=0?"LINE DISCONNECTED":(state&4)!=0?"LINE CONNECTED":(state&2)!=0?"INCOMING CALL":"NO INCOMING CALL"):(state&16)!=0?"ARRAY OVERRIDE":"TRACK SCHEDULED";
            for(int y=0;y<Height;y++)for(int x=0;x<Width;x++)pixels[y*Width+x]=new Color32(3,(byte)(amber?5:10),(byte)(amber?3:7),255);
            Text(14,14,amber?"SARO / INTERNAL LINE":"ARRAY 03 / OPERATIONS",ink);
            Line(14,36,306,36,faint);
            Text(14,46,StatusLabel,(state&1)!=0?ink:faint);
            if(amber)
            {
                Text(14,83,"CIRCUIT  /  DESK 03",faint);
                Text(14,109,(state&2)!=0?"ANSWER AT HANDSET":"LOCAL SWITCHBOARD",ink);
                Text(14,145,"AUDIO IS NOT RECORDED",faint);
                Text(14,176,"SARO  /  NIGHT SHIFT",faint);
            }
            else
            {
                for(int radius=18;radius<=54;radius+=18)
                    for(int i=0;i<180;i++) { float a=i*Mathf.PI/90;Dot(87+Mathf.RoundToInt(Mathf.Cos(a)*radius),127+Mathf.RoundToInt(Mathf.Sin(a)*radius),faint); }
                Line(29,127,145,127,faint);Line(87,69,87,185,faint);
                Line(87,127,(state&16)!=0?122:64,(state&16)!=0?93:85,ink);
                Text(162,87,"SCHEMATIC",faint);Text(162,117,"BANK 03",ink);
                Text(162,147,(state&1)!=0?"ONLINE":"OFFLINE",ink);
                Text(162,176,"RX / 1986",faint);
            }
            // Very subtle raster texture, baked into the tiny face; no flickering lights or screen-space effect.
            for(int y=0;y<Height;y+=2)for(int x=0;x<Width;x++){int i=y*Width+x;Color32 c=pixels[i];pixels[i]=new Color32((byte)(c.r*.91f),(byte)(c.g*.91f),(byte)(c.b*.91f),255);}
            face.SetPixels32(pixels);face.Apply(false,false);
        }
        void Dot(int x,int y,Color32 c){if(x>=0&&x<Width&&y>=0&&y<Height)pixels[(Height-1-y)*Width+x]=c;}
        void Text(int x,int y,string text,Color32 color)
        {
            foreach(char c in text)
            {
                if(Glyphs.TryGetValue(c,out var bits))
                    for(int row=0;row<7;row++)for(int col=0;col<5;col++)if(bits[row*5+col]=='1')for(int a=0;a<2;a++)for(int b=0;b<2;b++)Dot(x+col*2+a,y+row*2+b,color);
                x+=12;
            }
        }
        void Line(int x0,int y0,int x1,int y1,Color32 c)
        {
            int steps=Mathf.Max(Mathf.Abs(x1-x0),Mathf.Abs(y1-y0));
            for(int i=0;i<=steps;i++){float t=steps==0?0:i/(float)steps;Dot(Mathf.RoundToInt(Mathf.Lerp(x0,x1,t)),Mathf.RoundToInt(Mathf.Lerp(y0,y1,t)),c);}
        }
        void OnDestroy(){if(face)Destroy(face);if(material)Destroy(material);}
    }
}
