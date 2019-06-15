#!/usr/bin/env python
# coding: utf-8

import sys, yaml, os

yaml_path = sys.argv[1]
md_dir = sys.argv[2]
header_template = '''---
layout: page
title: {}
nav_order: 3
---

# {}
{{: .no_toc}}
'''

with open(yaml_path) as yf:
  data = yaml.load(yf)
  for d in data:
    topics = data[d]['topics']
    for t in topics:
      text = ""
      header = header_template.format(t['name'], t['name'])
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
          text += "  {}\n".format(i['doc'])

      md_path = os.path.join(md_dir, t['name'].lower().replace(' ','-') + ".md")
      with open(md_path, 'w') as f:
        f.write(text)
