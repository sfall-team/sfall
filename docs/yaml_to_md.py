#!/usr/bin/env python3
# coding: utf-8

import sys, os
import ruamel.yaml
yaml = ruamel.yaml.YAML(typ="rt")
yaml.width = 4096
yaml.indent(mapping=2, sequence=4, offset=2)

functions_yaml = sys.argv[1]
hooks_yaml = sys.argv[2]
md_dir = sys.argv[3]

# template for functions pages
function_header_template = '''---
layout: page
title: '{name}' # quote just in case
nav_order: 6
{has_children}
# parent - could be empty
{parent}
has_toc: false
permalink: {permalink}
---

# {name}
{{: .no_toc}}
'''

# template for hooks types page - hardcoded
hooks_header = '''---
layout: page
title: Hook types
nav_order: 6
parent: Hooks
permalink: /hook-types/
---

# Hook types
{: .no_toc}

* TOC
{:toc}
---
'''

def get_slug(string):
  return string.lower().replace(' ','-').replace(':','-').replace('/','-').replace('--','-')

# functions pages
with open(functions_yaml) as yf:
  data = yaml.load(yf)
  for cat in data: # list categories
    text = ""

    # check for childre
    has_children = ""
    children = [x for x in data if "parent" in x and x["parent"] == cat["name"]]
    if len(children) > 0:
      has_children = "has_children: true"

    # used in filename and permalink
    slug = get_slug(cat['name'])

    # if parent is present, this is a subcategory
    parent = ""
    if 'parent' in cat:
      parent = "parent: " + cat['parent']
    header = function_header_template.format(name=cat['name'], parent=parent, has_children=has_children, permalink="/{}/".format(slug))
    text += header

    # common doc for category
    if 'doc' in cat:
      text = text + '\n' + cat['doc'] + '\n'

    if len(children) > 0:
      text += "\n## Subcategories\n{: .no_toc}\n\n"
      for c in children:
        text += "- [**{}**](/{}/)\n".format(c["name"], get_slug(c["name"]))
      text += "\n"

    if 'items' in cat: # allow parent pages with no immediate items
      text += "## Functions\n{: .no_toc}\n\n"
      text += "* TOC\n{: toc}\n"
      # individual functions
      items = cat['items']
      items = sorted(items, key=lambda k: k['name'])

      for i in items:
        # header
        text += "\n### **{}**\n".format(i['name'])
        # macro label
        if 'macro' in i:
          text += "{: .d-inline-block }\n" + format(i['macro']) + "\n{: .label .label-green }\n"
        # unsafe label
        if 'unsafe' in i and i['unsafe'] is True:
          text += '{: .d-inline-block }\nUNSAFE\n{: .label .label-red }\n'
        # usage
        text += "```c++\n{}\n```\n".format(i['detail'])
        # doc, if present
        if 'doc' in i:
          text += i['doc'] + '\n'
        # macro note
        if 'macro' in i:
          text += "\nThis is a macro, you need to include `{}` to use it.\n".format(i['macro'])
        # end separator
        # text += '\n---\n'

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
    try:
      hid = h['id']
    except:
      hid = "HOOK_" + name.upper()
    if 'filename' in h: # overriden filename?
      filename = h['filename']
    else:
      filename = "hs_" + name.lower() + ".int"

    text += "\n## {}\n\n".format(name) # header
    if filename != "": # if not skip
      text += "`{}` ({})\n\n".format(hid, filename) # `HOOK_SETLIGHTING` (hs_setlighting.int)
    text += doc # actual documentation

    md_path = os.path.join(md_dir, "hook-types.md")
    with open(md_path, 'w') as f:
      f.write(text)
