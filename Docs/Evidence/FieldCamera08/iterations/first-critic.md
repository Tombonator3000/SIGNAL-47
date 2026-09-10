# Independent critique — first field camera candidate

Reviewer: fresh `camera_critic` subagent following the adopted Dream Loop review step. Inspected actual screenshots and targets; no independent playtesting was claimed.

- FAIL camera instruction: journey-15 front text is mirrored. Turn the TextMesh toward the approach.
- FAIL viewfinder corners: journey-25 top brackets point upward; left stroke touches the heading. Replace float-equality direction checks with explicit top/bottom indices.
- PASS photograph layout: journey-28/31 image, cream mat, 026/042/0 fields and controls are legible without clipping. System font and glossy default buttons differ from the target: use the project font and flat colors.
- Controls PASS visually; function UNVERIFIED by this reviewer. Main agent's separate native runs passed all 34 checkpoints.
- No obvious blocking gameplay bug in the reviewed FieldCamera / HUD source. Existing sparse world geometry and variable actual photograph viewpoint remain the agreed baseline.

Main-agent corrections: preserve imported FBX transforms under a metre-scale wrapper (first preview had overwritten the FBX unit transform); fix label orientation; explicit bracket indices; lead the player to the camera before motor inspection; project-font evidence sheet and flat controls; clear return-inside requirement. Final images and performance must be checked again after these changes.
