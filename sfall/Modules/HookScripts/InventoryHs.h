#pragma once

// Inventory-Related hook scripts

namespace sfall
{

void InitInventoryHookScripts();

void Inject_RemoveInvenObjHook();
void Inject_MoveCostHook();
void Inject_SwitchHandHook();
void Inject_InventoryMoveHook();
void Inject_InvenWieldHook();

}
