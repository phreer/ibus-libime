#!/bin/bash

VERSION=$(cat VERSION)
git archive --format=tar.gz --prefix=ibus-libime-${VERSION}/ -o ~/rpmbuild/SOURCES/ibus-libime-${VERSION}.tar.gz HEAD
