#!/usr/bin/python

import sys
import subprocess

root = sys.argv[1]
output = sys.argv[2]
final = "#include <Debug.h>\n"
final = "#include <Main.h>\n"
target = open( output, 'w' )
classes = []

p = subprocess.Popen( "find " + root + " \\( -wholename \"*/build*\" -prune \\) -o -name \"*.cpp\" -exec grep \"::flight_register\" {} \; | cut -d ':' -f1 | rev | cut -d ' ' -f1 | rev", stdout = subprocess.PIPE, shell = True )
(output, err) = p.communicate()
output = output.decode("utf-8").split("\n")

for f in output:
	if len( f ) > 1:
		classes.append( f )

for classname in classes:
	final += "#ifdef BUILD_" + classname + "\n"
	final += "#include <" + classname + ".h>\n"
	final += "#endif\n"

final += "\n"

final += "\nint Main::flight_register()\n{\n\tint ret = 0;\n"


for classname in classes:
	final += "#ifdef BUILD_" + classname + "\n"
	final += "\tif ( ( ret = " + classname + "::flight_register( this ) ) < 0 ) {\n\t\tgDebug() << \"Module \\\"" + classname + "\\\" failed to initialize (\" << ret << \")\\n\";\n\t}\n";
	final += "#endif\n"

final += "\treturn ret;\n}\n";
target.write( final )
target.close()
