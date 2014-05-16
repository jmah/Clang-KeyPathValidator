#!/bin/sh

IFS=

clang $@
STATUS=$?

CUSTOM_CLANG_ROOT=${CUSTOM_CLANG_ROOT:-$(dirname $0)/../../../../../Release}
if test "$STATUS" -eq 0 && test -d $CUSTOM_CLANG_ROOT && ! ( echo $@ | grep -wF -- -filelist ); then
	# Xcode uses --serialize-diagnostics, which writes diagnostics to a file, overwriting the previous contents.
	# Emit our additional diagnostics after the original has run.
	DISABLE_BAD_WARNINGS_IN_CUSTOM_CLANG=-Wno-unused-property-ivar
	$CUSTOM_CLANG_ROOT/bin/clang -Xclang -load -Xclang $CUSTOM_CLANG_ROOT/lib/libKeyPathValidator.dylib -Xclang -plugin -Xclang validate-key-paths -fsyntax-only -Qunused-arguments $@ $DISABLE_BAD_WARNINGS_IN_CUSTOM_CLANG
	STATUS=$?
fi

exit $STATUS
