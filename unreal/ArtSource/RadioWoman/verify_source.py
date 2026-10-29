#!/usr/bin/env python3
"""Fresh private Blender reader: no garment generation, mutation, save or UE.

Run root-owned under guard, with a new profile and a new output receipt.
"""
from pathlib import Path
import argparse
import json
import os
import sys
sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(__file__).resolve().parent))
import coat_yoke as Y
import build_clothes as B


def verify(source_report,output):
    source_report=Path(source_report).resolve(strict=True)
    output=Path(output)
    Y.need(output.is_absolute() and not output.exists() and output.parent.is_dir()
           and output.is_relative_to(Y.ROOT) and output!=source_report,'Fresh private source-reader receipt required')
    report=dict(schema_version=1,status='STARTED_NOT_ACCEPTED',mode='verify-source',pid=os.getpid(),
                source_report=Y.file_row(source_report),source_mutated=False,asset_save_called=False,
                native_accepted=False,appearance_accepted=False)
    try:
        original=Y.load(source_report)
        Y.need(original['status']=='REST_CLOTHES_EXPORTED_NOT_ACCEPTED' and original['blend_reopened'] is True
               and original['pid']>0 and original['pid']!=os.getpid(),'Separate completed source authoring process required')
        for row in original['producer_files'].values():Y.pin(row)
        cfg=B.config();inputs,values=Y.inputs_checked(cfg)
        Y.need(original['coat_yoke']['policy']==Y.POLICY
               and original['coat_yoke']['inputs']==cfg['coat_yoke']['inputs']
               and original['coat_yoke']['source_files']==inputs['files']
               and original['coat_yoke']['preserved_original_parts']==16
               and original['source_body_payload_preserved'] is True,'Exact new R4 source policy required')
        Y.need(original['saved_reopened_fit']['status']=='REST_YOKE_SAMPLED_NOT_SEATED_OR_NATIVE_ACCEPTED',
               'Actual producer reopen/fit must have passed')
        output_files={p:Y.file_row(p) for p in original['output_files_sha256']}
        Y.need(all(row['sha256']==original['output_files_sha256'][p] for p,row in output_files.items()),'Produced source bytes changed')
        blends=[p for p in output_files if p.endswith('.blend')]
        fbxs=[p for p in output_files if p.endswith('.fbx')]
        Y.need(len(blends)==len(fbxs)==1 and len(output_files)==2,'Only actual blend+FBX source outputs required')
        import bpy
        bpy.ops.wm.open_mainfile(filepath=blends[0])
        clothes=bpy.data.objects[cfg['fbx']['mesh_name']];arm=bpy.data.objects[original['rest_skeleton']['object_name']]
        body=bpy.data.objects[original['source_body_object_name']]
        Y.need(B.signature(B.skeleton(arm))==Y.REF and len(arm.data.bones)==341
               and B.skeleton(arm)==original['rest_skeleton'],'Fresh source actual341 reference changed')
        Y.need(Y.signature(B.clothes_snapshot(clothes))==Y.signature(original['garment_payload_snapshot'])
               and B.weight_audit(clothes,arm)==original['weights'],'Fresh saved garment payload/weights changed')
        Y.need(B.sha(original['input_body_fbx'])==original['input_body_sha256']==cfg['source_body_fbx_sha256'],
               'Original private Body FBX changed')
        report['actual_source_heads']=Y.body_source_checked(body,arm,cfg,values,B.clothes_snapshot,B.skeleton)
        parts=Y.recover_parts(clothes,original)
        report['actual_fit']={}
        Y.fit_actual(parts,values,report['actual_fit'])
        Y.need(report['actual_fit']['required_skin_rays']==original['saved_reopened_fit']['required_skin_rays']
               and report['actual_fit']['upper_facet_samples']==original['saved_reopened_fit']['upper_facet_samples'],
               'Fresh reader did not sample the same measured source footprint')
        # No setters/save/export were used after opening the actual saved source.
        Y.need(output_files=={p:Y.file_row(p) for p in output_files},'Fresh reader altered source outputs')
        report.update(status='FRESH_SOURCE_SAMPLED_PASS_NOT_NATIVE_OR_APPEARANCE',producer_pid=original['pid'],
                      output_files=output_files,source_reference_sha256=B.signature(B.skeleton(arm)),
                      source_files=inputs['files'],garment_payload_snapshot=B.clothes_snapshot(clothes),
                      source_part_identity_sha256=Y.signature(original['source_part_identity']))
    except Exception as error:
        report.update(status='FAIL',error=repr(error))
        Y.write_new(output,report)
        raise
    Y.write_new(output,report)
    print('RADIO_WOMAN_R4_FRESH_SOURCE_MEASURED',str(output))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source_report',type=Path)
    parser.add_argument('output',type=Path)
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else None)
    verify(args.source_report,args.output)


if __name__=='__main__':main()
