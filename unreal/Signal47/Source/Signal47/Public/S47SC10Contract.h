#pragma once

class US47Main;

// Explicit -S47SpineContract=sc10 only. Called by the spine, including after a full Flow restore.
void S47StepSC10Contract(US47Main* Main, double Dt);
