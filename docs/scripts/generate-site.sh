#!/bin/bash

set -xeu -o pipefail

site_dir="./docs/website"
cd "$site_dir"
bundle install
jekyll build
