#!/bin/bash

set -xeu -o pipefail

site_dir="docs"
yml_dir="../artifacts/scripting"
pages_dir="pages"

cd "$site_dir"
mkdir -p "$pages_dir"
./yaml_to_md.py "$yml_dir/functions.yml" "$yml_dir/hooks.yml" "$pages_dir"

bundle update github-pages
bundle install
bundle exec jekyll build
