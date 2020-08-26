. scripts/env.sh
debugserver=/Applications/Xcode.app/Contents/SharedFrameworks/LLDB.framework/Resources/debugserver
$debugserver localhost:22222 python3 ../../nodegl-env/bin/ngl-test $1.py $2 refs/$2.ref
