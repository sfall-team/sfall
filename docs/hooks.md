---
title: Hooks
nav_order: 2
has_children: true
---

# Hooks

Hook scripts are specially named scripts that are run by sfall at specific points to allow mods to override normally hardcoded behaviour in a more flexible way than sfall's normal ini configuration.

In addition to the bit of code it overrides, the script will be run once when first loaded and again at each player reload to allow for setup. Hook scripts have access to a set of arguments supplied to sfall, but aren't required to use them all. They also return one or more values, but again they're optional, and you only need to return a value if you want to override the default.

See [hook types](pages/hook-types.html) and hook [functions reference](pages/hook-functions.html) for details.

## Hooks compatibility

To aid in mods compatibility, avoid using `hs_xxx` .int scripts. Instead it is recommended to use a normal global script combined with `register_hook_proc` or `register_hook`.

Example setup for a hook-script based mod:

```c++
procedure tohit_hook_handler begin
   display_msg("Modifying hit_hook " + get_sfall_arg);
   set_hit_chance_max(100);
   set_sfall_return(100);
end

procedure start begin
   if game_loaded then begin
      register_hook_proc(HOOK_TOHIT, tohit_hook_handler);
   end
end
```
