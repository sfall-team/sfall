#!/usr/bin/env python
# coding: utf-8

import sys, yaml, os
reload(sys)
sys.setdefaultencoding('utf8')

yaml_path = sys.argv[1]
md_dir = sys.argv[2]
header_template = '''---
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

with open(yaml_path) as yf:
  data = yaml.load(yf)
  for cat in data: # list categories
    text = ""
    parent = ""
    # if parent is present, this is a subcategory
    if 'parent' in cat:
      parent = "parent: " + cat['parent']
    header = header_template.format(name=cat['name'], parent=parent)
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
