#!/bin/bash

set -xeu -o pipefail

site_dir="./docs/website"
pushd .
cd "$site_dir"
jekyll build
popd
