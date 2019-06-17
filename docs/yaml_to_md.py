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
title: '{name}' # quote just in case
nav_order: 3
has_children: true
# parent - could be empty
{parent}
permalink: {permalink}
---

# {name}
{{: .no_toc}}
'''

function_header_template_noname = '''---
layout: page
title: '{name}' # quote just in case
nav_order: 3
has_children: true
# parent - could be empty
{parent}
permalink: {permalink}
---
'''


function_header_name = '''
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

    # used in filename and permalink
    slug = cat['name'].lower().replace(' ','-').replace(':','-').replace('/','-').replace('--','-')

    # if parent is present, this is a subcategory
    if 'parent' in cat:
      parent = "parent: " + cat['parent']
    if 'items' in cat: # parent pages with no immediate functions don't need name displayed
      header = function_header_template.format(name=cat['name'], parent=parent, permalink="/{}/".format(slug))
    else:
      header = function_header_template_noname.format(name=cat['name'], parent=parent, permalink="/{}/".format(slug))
    text += header

    # common doc for category
    if 'doc' in cat:
      text = text + '\n' + cat['doc'] + '\n'
    text = text + "\n * TOC\n{:toc}\n"

    if 'items' in cat: # allow parent pages with no immediate items
      # individual functions
      items = cat['items']
      items = sorted(items, key=lambda k: k['name'])

      for i in items:
        # header
        text += "\n### {}\n".format(i['name'])
        # usage
        text += '''```c++
{}
```
'''.format(i['detail'])
        # doc, if present
        if 'doc' in i:
          text += i['doc']
          text += '\n\n'

    md_path = os.path.join(md_dir, slug + ".md")
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
