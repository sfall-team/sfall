---
layout: page
title: Hooks
nav_order: 3
---

# Hooks
* ```c++
mixed get_sfall_arg()
```
  Gets the next argument from sfall. Each time it's called it returns the next argument, or otherwise it returns 0 if there are no more arguments left.
* ```c++
int get_sfall_args()
```
  Returns all hook arguments as a new temp array.
* ```c++
int init_hook()
```
  The hook script equivilent of game_loaded; it returns 2 when the script is first loaded, 1 when the player reloads and 0 otherwise.
* ```c++
void register_hook(int hooktype)
```
  Used from a normal global script if you want to run it at the same point a full hook script would normally run.
* ```c++
void register_hook_proc(int hooktype, proc procedure)
```
  The same as register_hook, except that you specifically define which procedure in the current script should be called as a hook (instead of "start").
* ```c++
void set_sfall_arg(int argnum, int value)
```
  Changes argument value. This is usefull if you have several hook scripts attached to one hook point (see below).
* ```c++
void set_sfall_return(int value)
```
  Used to return the new values from the script. Each time it's called it sets the next value, or if you've already set all return values it does nothing.
