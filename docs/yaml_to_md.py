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
  for d in data:
    topics = data[d]['topics']
    for t in topics:
      text = ""
      parent = ""
      if 'parent' in t:
        parent = "parent: " + t['parent']
      header = header_template.format(name=t['name'], parent=parent)
      text += header

      if 'doc' in t:
        text = text + '\n' + t['doc'] + '\n'
      text = text + "\n * TOC\n{:toc}\n"
      # functions
      items = t['items']
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

      md_path = os.path.join(md_dir, t['name'].lower().replace(' ','-').replace(':','-').replace('/','-').replace('--','-') + ".md")
      with open(md_path, 'w') as f:
        f.write(text)
