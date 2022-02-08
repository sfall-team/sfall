#pragma once

// Hook scripts related to hex blocking checks (during pathfinding, etc.)

namespace sfall
{

void InitHexBlockingHookScripts();

void Inject_HexSightBlockHook();
void Inject_HexShootBlockHook();
void Inject_HexIABlockHook();
void Inject_HexMoveBlockHook();

}