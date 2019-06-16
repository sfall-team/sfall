#!/usr/bin/env python
# coding: utf-8

import sys, yaml, os
reload(sys)
sys.setdefaultencoding('utf8') # ugly buy works

functions_yaml = sys.argv[1]
hooks_yaml = sys.argv[2]
md_dir = sys.argv[3]

# template for functions pages
function_header_template = '''---
layout: page
title: '{name}'
nav_order: 3
has_children: true
# parent - could be empty
{parent}
---

# {name}
{{: .no_toc}}
'''

# template for hooks types page - hardcoded
hooks_header = '''---
layout: page
title: Hook types
nav_order: 3
parent: Hooks
---

# Hook types
{: .no_toc}

* TOC
{:toc}
'''

# functions pages
with open(functions_yaml) as yf:
  data = yaml.load(yf)
  for cat in data: # list categories
    text = ""
    parent = ""
    # if parent is present, this is a subcategory
    if 'parent' in cat:
      parent = "parent: " + cat['parent']
    header = function_header_template.format(name=cat['name'], parent=parent)
    text += header

    # common doc for category
    if 'doc' in cat:
      text = text + '\n' + cat['doc'] + '\n'
    text = text + "\n * TOC\n{:toc}\n"
    # individual functions
    items = cat['items']
    items = sorted(items, key=lambda k: k['name']) 
    for i in items:
      text += "### {}\n".format(i['name'])
      text += '''```c++
{}
```
'''.format(i['detail'])
      if 'doc' in i:
        text += i['doc']
        text += '\n\n'

    md_path = os.path.join(md_dir, cat['name'].lower().replace(' ','-').replace(':','-').replace('/','-').replace('--','-') + ".md")
    with open(md_path, 'w') as f:
      f.write(text)


# hook types page
with open(hooks_yaml) as yf:
  hooks = yaml.load(yf)
  hooks = sorted(hooks, key=lambda k: k['name']) # alphabetical sort
  text = hooks_header

  for h in hooks:
    name = h['name']
    doc = h['doc']
    codename = "HOOK_" + name.upper()
    if 'filename' in h: # overriden filename?
      filename = h['filename']
    else:
      filename = "hs_" + name.lower() + ".int"

    text += "\n### {}\n\n".format(name) # header
    if filename != "": # if not skip
      text += "`{}` ({})\n\n".format(codename, filename) # `HOOK_SETLIGHTING` (hs_setlighting.int)
    text += doc # actual documentation

    md_path = os.path.join(md_dir, "hook-types.md")
    with open(md_path, 'w') as f:
      f.write(text)
