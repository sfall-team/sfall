#!/bin/bash

set -xeu -o pipefail

site_dir="docs"
pushd .
cd "$site_dir"
mkdir -p pages
./yaml_to_md.py ../artifacts/scripting/doc.yml pages
bundle install
bundle exec jekyll build
popd
