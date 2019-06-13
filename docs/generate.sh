#!/bin/bash

set -xeu -o pipefail

site_dir="docs"
pushd .
cd "$site_dir"
bundle exec jekyll build
popd
