BASEDIR=$(dirname "$0")
cd "$BASEDIR"

../vendor/bin/premake/Linux/premake5 --file=../Build-Walnut-ExampleProject.lua gmake2
