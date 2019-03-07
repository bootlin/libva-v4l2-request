#!/bin/sh

autoreconf -vi

if [ -z "$NOCONFIGURE" ]
then
	./configure "$@"
fi
