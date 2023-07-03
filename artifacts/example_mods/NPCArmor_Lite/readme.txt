NPC Armor Appearance mod (Lite version) for sfall 3.8.x
-------------------------------------------------------

A cut-down version of the mod included in sfall 4.x modders pack. Used to replace the scripted part of B-Team mod.
Appropriate graphics are required for this mod to work, or you can set party members to use player's armor appearance instead.
Can be adopted to any mod by adjusting armor PIDs, allowed weapon anim codes, NPC PIDs and NPC FIDs in INI file.


Requires sfall 3.8.29 or higher.

To use, copy gl_npcarmor_lite.int to your scripts folder and copy npcarmor.ini to the same directory as sfall.

The default npcarmor.ini is set up for Restoration Project 2.3.3.
Check the included "npcarmor - vanilla.ini" for an example that does not require any new graphics.

Note that due to the lack of newer game hooks in sfall 3.8.x, there is a minor glitch in the lite version:
- in combat, NPCs can pick up and try to equip weapons with anim codes that are not allowed in INI file but supported by their appearance (fixed in 3.8.40+).
