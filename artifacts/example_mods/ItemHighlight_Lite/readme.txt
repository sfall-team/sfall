Item Highlight mod (Lite version) for sfall 3.8.x
-------------------------------------------------

A cut-down version of the mod included in sfall 4.x, offers a bit more features than the built-in function.
Features:
- highlighting items, containers (optional) and lootable corpses (optional) on the ground
- configurable hotkey is used to trigger highlight
- only objects in direct line-of-sight of player are highlighted (optional)
- motion scanner is required to enable highlight (optional)
- motion scanner charges are decreased on each use (optional)


Requires sfall 3.8.12 or higher.

To use, copy gl_highlighting_lite.int to your scripts folder, and copy sfall-mods.ini to the same directory as sfall.
Also, you should disable the built-in function in ddraw.ini (ToggleItemHighlightsKey=0).

Note that due to the lack of newer game hooks in sfall 3.8.x, there are some minor visual glitches with the lite version:
- items will be kept highlighted when entering combat while holding the highlight key.
- when you pick up items while holding the highlight key, they will be kept highlighted if you drop them on the ground.
Both glitches can bo solved by pressing and releasing the highlight key again.
