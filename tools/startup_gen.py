#!/usr/bin/python

import sys
import subprocess

root = sys.argv[1]
output = sys.argv[2]
final = "#include <Main.h>\n"
target = open( output, 'wb' )
fcts = []
final_fcts = []

p = subprocess.Popen( "grep -r \"flight_register.\" " + root, stdout = subprocess.PIPE, shell = True )
(output, err) = p.communicate()
output = output.split('\n')

for f in output:
	if f.find( ".cpp:" ) > 0 and f.find( "Main.cpp" ) < 0:
		fcts.append( f[f.find(":")+1:] )

for f in fcts:
	if len( f ) > 1:
		if f.find( "::" ) > 0:
			classname = f[:f.find( "::" )]
			classname = classname[classname.rfind(" "):]
			classname = classname.replace( " ", "" )
			final += "#include <" + classname + ".h>\n";

final += "\n"

for f in fcts:
	if len( f ) > 1:
		if f.find( "::" ) < 0:
			final += f + ";\n";

final += "\nint Main::flight_register()\n{\n\tint ret = 0;\n"

for f in fcts:
	if len( f ) > 1:
		call = ""
		if f.find( "::" ) > 0:
			call = f[:f.find( "::" )]
			call = call[call.rfind(" "):]
			call = call.replace( " ", "" ) + "::"
		f = f[f.find("flight_register"):]
		f = f[:f.find("(")]
		final_fcts.append( call + f )
final_fcts = set( final_fcts )

for f in final_fcts:
		final += "\tif ( ( ret = " + f + "( this ) ) < 0 ) {\n\t\treturn ret;\n\t}\n";

final += "\treturn ret;\n}\n";
target.write( final )
target.close()
