#!/bin/bash

set -xeu -o pipefail

site_dir="./docs/website"
cd "$site_dir"
jekyll build
