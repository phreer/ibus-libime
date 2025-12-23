#!/bin/bash

set -e

VERSION=$(cat VERSION)
git archive --format=tar.gz --prefix=ibus-libime-${VERSION}/ -o ~/rpmbuild/SOURCES/ibus-libime-${VERSION}.tar.gz HEAD
echo "ibus-libime-${VERSION}.tar.gz is generated"
