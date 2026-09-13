using System;
using UnityEngine;
using UnityEngine.Rendering;
namespace Signal47.Environment
{
    /// <summary>Reuse the authored grass meshes; store placements once instead of duplicating every vertex.</summary>
    public sealed class FieldVegetation27 : MonoBehaviour
    {
        [Serializable] public sealed class Batch { public Mesh mesh; public Matrix4x4[] matrices; }
        public Material material;
        public Batch[] batches;
        void LateUpdate()
        {
            if(!material||batches==null)return;
            foreach(var batch in batches)
            {
                if(!batch.mesh||batch.matrices==null||batch.matrices.Length==0)continue;
                if(SystemInfo.supportsInstancing)
                    Graphics.DrawMeshInstanced(batch.mesh,0,material,batch.matrices,batch.matrices.Length,null,ShadowCastingMode.Off,true,gameObject.layer,null,LightProbeUsage.Off);
                else foreach(var matrix in batch.matrices)
                    Graphics.DrawMesh(batch.mesh,matrix,material,gameObject.layer,null,0,null,ShadowCastingMode.Off,true,null,LightProbeUsage.Off);
            }
        }
    }
}
