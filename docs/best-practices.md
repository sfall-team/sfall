---
title: Best practices
permalink: /best-practices/
nav_order: 2
---


# Best practices
{: .no_toc}

* TOC
{: toc}

## Mod compatibility

0. If it can be done in a [global script]({{ site.baseurl }}/global-scripts/), do it in a global script. Combined with hooks, they are extremely powerful, possibilities ranging from creating new perks to UI scripting to prototype altering on-the-fly.
  While scripting _does_ take a bit longer to get started, and hacking prototypes directly with GUI programs _might look_ easier at first, consider that:
  * Scripts from different mods modifying the same thing can actually be compatible with each other. Binary files can't.
  * Scripts can be version controlled and thus are much more easier to maintain.

1. If you're using [set_sfall_return]({{ site.baseurl }}/hook-functions/#set_sfall_return), always couple it with [set_sfall_arg]({{ site.baseurl }}/hook-functions/#set_sfall_arg) for the corresponding `arg`(s), unless you have a specific reason not to. Detailed explanation is available [here]({{ site.baseurl }}/hook-functions/#register_hook_proc).

2. Pick yourself a 2-3 character [modding prefix](http://www.nma-fallout.com/threads/a-modding-prefix-for-your-mods.217791/). Use it for:
  * global script names
  * global variable names and saved array names
  * debug messages

  This will ensure (to some degree), that another mod doesn't overwrite your scripts, doesn't mess with your global variables, and that debug messages coming from your scripts can be distinguished easily.

  For example, if you pick prefix "**a_**", your script could be named `gl_a_myscript.int`, and might look like this:
  
  ```js
    #define S_NAME "gl_a_myscript"
    #define ndebug(message) debug_msg(S_NAME + ": " + message + "\n")

    procedure start begin
      if game_loaded then begin
        set_sfall_global("a_myvar", 1000);
        ndebug("initialized");
      end
    end
    ...
  ```

## Performance

1. Do not abuse sfall [global variables]({{ site.baseurl }}/global-variables/) and [saved arrays]({{ site.baseurl }}/arrays/#storing-arrays). This will lead to savegame bloating. But do use them if necessary, they are added for a reason.
2. Do not abuse [set_global_script_repeat]({{ site.baseurl }}/global-scripts/). Whenever possible, register your script as a [hook]({{ site.baseurl }}/hooks/) instead. You can register the same procedure at multiple hook points, if necessary.
  - If you have `set_global_script_repeat(300)` in your script, you're probably doing something wrong. That's an invocation every 3-5 seconds, approximately.
  - If you have `set_global_script_repeat(30)`, you are definitely doing something wrong. Look for suitable hooks harder, think of another way for implementing it, ask fellow modders for help.
