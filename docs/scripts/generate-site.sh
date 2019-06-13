#!/bin/bash

set -xeu -o pipefail

site_dir="./docs/website"
pushd .
cd "$site_dir"
bundle install
jekyll build
popd
