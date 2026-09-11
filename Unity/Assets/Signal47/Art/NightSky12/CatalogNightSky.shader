Shader "Signal47/Catalog Night Sky"
{
    Properties
    {
        _MainTex ("Catalogue cubemap (sRGB)", Cube) = "black" {}
        _Exposure ("Starlight", Range(0, 3)) = 0.65
        _Rotation ("Art direction yaw", Range(0, 360)) = 115
        _Tilt ("Art direction tilt", Range(-90, 90)) = 33
        _Zenith ("Zenith", Color) = (0.002, 0.004, 0.009, 1)
        _Horizon ("Horizon glow", Color) = (0.013, 0.016, 0.024, 1)
    }
    SubShader
    {
        Tags { "Queue"="Background" "RenderType"="Background" "PreviewType"="Skybox" }
        Cull Off ZWrite Off
        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #pragma target 3.0
            #include "UnityCG.cginc"
            samplerCUBE _MainTex;
            float _Exposure, _Rotation, _Tilt;
            float4 _Zenith, _Horizon;
            struct v2f { float4 position : SV_POSITION; float3 direction : TEXCOORD0; };
            v2f vert(float4 vertex : POSITION)
            {
                v2f o;
                o.position = UnityObjectToClipPos(vertex);
                o.direction = vertex.xyz;
                return o;
            }
            float4 frag(v2f i) : SV_Target
            {
                float3 view = normalize(i.direction);
                float s, c;
                sincos(radians(_Rotation), s, c);
                float3 d = float3(c * view.x - s * view.z, view.y, s * view.x + c * view.z);
                sincos(radians(_Tilt), s, c);
                d = float3(d.x, c * d.y - s * d.z, s * d.y + c * d.z);
                // Unity reprojects the equirectangular source once at import.
                // Direction-space filtering avoids a singular polar footprint
                // and longitude derivative discontinuity in the runtime shader.
                float3 stars = texCUBE(_MainTex, d).rgb;
                float atmosphere = smoothstep(-0.015, 0.24, view.y);
                float horizon = pow(saturate(1 - max(0, view.y)), 8);
                float3 sky = lerp(_Zenith.rgb, _Horizon.rgb, horizon);
                return float4(sky + stars * _Exposure * atmosphere, 1);
            }
            ENDCG
        }
    }
}
