Shader "Signal47/Archive World Text"
{
 Properties { _MainTex("Font Atlas",2D)="white"{} }
 SubShader {
  Tags {"Queue"="Transparent" "RenderType"="Transparent" "RenderPipeline"="UniversalPipeline"}
  Pass {
   Blend SrcAlpha OneMinusSrcAlpha
   Cull Off ZWrite Off ZTest LEqual
   HLSLPROGRAM
   #pragma vertex vert
   #pragma fragment frag
   #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Core.hlsl"
   TEXTURE2D(_MainTex); SAMPLER(sampler_MainTex);
   struct Input {float4 positionOS:POSITION;float2 uv:TEXCOORD0;half4 color:COLOR;};
   struct Varying {float4 positionCS:SV_POSITION;float2 uv:TEXCOORD0;half4 color:COLOR;};
   Varying vert(Input v){Varying o;o.positionCS=TransformObjectToHClip(v.positionOS.xyz);o.uv=v.uv;o.color=v.color;return o;}
   half4 frag(Varying i):SV_Target {half4 c=i.color;c.a*=SAMPLE_TEXTURE2D(_MainTex,sampler_MainTex,i.uv).a;return c;}
   ENDHLSL
  }
 }
}
