#pragma once

class AS47Web;

// Original additive fittings and the exact, visible gravel shoulder walk surface.
namespace S47SubstationDetail
{
 bool Build(AS47Web* World);
 bool FloorAt(double WorldX, double WorldZ, double& WorldY);
}
