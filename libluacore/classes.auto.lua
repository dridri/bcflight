#!/usr/bin/lua

function serialize( array, keep_functions )
	if keep_functions == nil then
		keep_functions = array or false
	end

	local seen = {}

	local function _serialize_core( fwrite, name, value, tab, parent )
		if type(value) == "function" and not keep_functions then
			return false
		end
		if type(value) == "table" and name == "self" then
			return false
		end

		for i = 1, tab do
			fwrite( "\t" )
		end

		if name == "lemma" then
			name = "\27[92mlemma\27[0m"
		end

		if type(name) == "string" and name ~= "" then
-- 			print("name : " .. name)
			fwrite( name .. " = " )
		end
		if type(value) == "nil" then
			fwrite( "nil" )
		elseif type(value) == "userdata" then
			local mt = getmetatable(value)
			if mt and mt.__tostring then
				fwrite( "" .. mt.__tostring(value) )
			else
				fwrite( "nil --[[ userdata ]]" )
			end
		elseif type(value) == "lightuserdata" then
			fwrite( "nil --[[ lightuserdata ]]" )
		elseif type(value) == "function" then
			fwrite( "nil --[[ function ]]" )
		elseif type(value) == "string" then
			fwrite( "\"" .. value .. "\"" )
		elseif type(value) == "number" or type(value) == "integer" then
			local str = "" .. value
			str = str:gsub( ",", "." )
			fwrite( str )
		elseif type(value) == "boolean" then
			if value then
				fwrite( "true" )
			else
				fwrite( "false" )
			end
		elseif type(value) == "table" then
			if seen[value] then
				fwrite( "nil --[[ repeat ]]" )
				return
			end
			seen[value] = true
-- 			if tables_refs[value] ~= nil then
-- 				fwrite( tables_refs[value] )
-- 			else
-- 				tables_refs[value] = parent .. name
				fwrite( "{\n" )
				for k, v in pairs(value) do
-- 					if type(k) ~= "string" or ( k:sub(1, 2) ~= "__" and k ~= "class" and k ~= "super" ) then
-- 					if type(k) ~= "string" or k:sub(1, 2) ~= "__" then
						if _serialize_core( fwrite, k, v, tab + 1, name .. "." ) then
							fwrite( ",\n" )
						end
-- 					end
				end
				for i = 1, tab do
					fwrite( "\t" )
				end
				fwrite( "}" )
-- 			end
		else
			fwrite( "nil --[[ ERROR : unknown value type '" .. type(value) .. "' ]]" )
		end
		return true
	end

	local retstr = ""
	local fwrite = nil

	fwrite = function( str ) retstr = retstr .. str end

	_serialize_core( fwrite, "", array, 0, "" )

	return retstr
end


function parse_member(s)
-- 	print( "s : ", s )
	local params = {}
	local sig, b = s:gmatch( "(.-)%((.*)%)" )()
	b = b:match("^%s*(.-)%s*$")
-- 	print( "sig : ", sig )

	local tokens = {}
	local modifiers = {}
	for tok in sig:gmatch("%S+") do
		if tok == "static" or tok == "virtual" or tok:sub(1, 8) == "template" then
			modifiers[tok] = true
		else
			table.insert( tokens, tok )
		end
	end
	local func = tokens[#tokens]
	local func_type = ""
	for i = 1, #tokens - 1 do
		func_type = func_type .. " " .. tokens[i]
	end
-- 	print( "func : ", table.concat(modifiers, ","), func_type, func )

-- 	print( "params : " .. b )
	for param in b:gmatch("[^,]+") do
		print("[" .. param .. "]")
		param = param:match("^%s*(.-)%s*$")
		if param:find( "=" ) then
			local p, d = param:gmatch("(.*)%s*=%s*(.*)")()
			p = p:match("^%s*(.-)%s*$")
			local type, name = p:gmatch("(.*)%s+(.*)")()
			table.insert( params, { type = type, name = name, default = d } )
		else
			local type, name, def = param:gmatch("(.*)%s+(.*)")()
			table.insert( params, { type = type, name = name, default = def } )
		end
	end
-- 	print(func, params, static)
	if ( s:find("onEvent") ~= nil ) then
-- 		os.exit()
	end
	return func, func_type:match("^%s*(.-)%s*$"), params, modifiers
end


local out_file = nil
local init_fn = "init"
local includes = {}
local generated_templates = {}
local final = {}
local interfaces = {}
local classes = {}
local all_classes = {}


function cast( prop )
	if all_classes[prop.type] then
		return "*(" .. prop.type .. "*)"
	end
	if prop.type:match("vector%s*<") or prop.type:match("list%s*<") then
		return ""
	end
	return "(" .. prop.type .. ")"
end


function cast_suffix( prop, v )
	if prop then
		if prop.type:find("string") then
			return ".toString()"
		end
		if prop.type:find("::") then
			v = classes[parent_class(prop.type)]
		end
		if v and v.enums and v.enums[basename(prop.type)] then
			return ".toInteger()"
		end
	end
	return ""
end

local skip = false
for __, filename in ipairs(arg) do
	if filename:sub( 1, 2 ) == "--" then
		local k, v = filename:gmatch("--(%w+)=([%w_%.%-/\\]+)")()
		if v == nil then
			v = arg[__ + 1]
			skip = true
		end
		if k == "init" then
			init_fn = v
		end
		if k == "output" then
			out_file = v
		end
	elseif skip then
		skip = false
	else
		local file = io.open( filename )
		if not file then
			error( "Cannot open file \"" .. filename .. "\"" )
		end


		local content = file:read("*a")
		content = content:gsub( "%(", " (" ):gsub( "%)", " )" ):gsub( ",", " ," )
		local tokens = {}
		local content_lines = {}
		for line in content:gmatch("[^\n]+") do
			if line:find("//") ~= nil then
				table.insert( content_lines, line:match("(.*)//.*") )
			else
				table.insert( content_lines, line )
			end
		end
		for tok in table.concat(content_lines, "\n"):gmatch("%S+") do
			local comma = tok:match(";")
			local other = tok:match("[^;]+")
			if other then
				table.insert( tokens, other )
			end
			if comma then
				table.insert( tokens, comma )
			end
		end

		local level = 0
		local curr_class = ""
		local curr_exported_class = ""
		local constructor_params = {}
		local properties = {}
		local members = {}
		local statics = {}
		local parent_classes = {}
		local has_destructor = false
		local included = false
		local subclasses = {}
		local enums = {}
		local stack = {}
		local local_definitions = {}

		function list_params( tokens, i, names_only )
			local final = {}
			local skip = false
			local lastvalid = 0

			while tokens[i] ~= "(" do
				i = i + 1
			end
			i = i + 1

			while tokens[i]:find( ")" ) == nil do
				if tokens[i]:find( "=" ) then
					while tokens[i]:find( ")" ) == nil and tokens[i]:find( "," ) == nil do
						i = i + 1
					end
				end
				if not tokens[i]:match("^%s*$") and tokens[i]:find( "," ) == nil and tokens[i]:find( ")" ) == nil then
					lastvalid = i
				end
				if tokens[i]:find( ")" ) then
					break
				end
				if not names_only then
					table.insert( final, tokens[i] )
				elseif ( tokens[i]:find( "," ) or tokens[i]:find( ")" ) ) and lastvalid > 0 then
					table.insert( final, tokens[lastvalid] )
				end
				i = i + 1
			end
			if names_only then
				table.insert( final, tokens[lastvalid] )
			end

			return final
		end

		function build_args( params, has_self )
			local iarg = ( has_self and 2 or 1 )
			local final = {}
			local upvalues = {}
			for _, param in ipairs(params) do
				if param.default ~= nil then
					table.insert( final, "Lua::value<" .. param.type .. ">(L, " .. iarg .. ", top, " .. param.default .. ")" )
				elseif param.type then
					if param.type:find("std::function") ~= nil then
						local fct = param.type:match("std::function%s*<%s*(.*)%s*>")
						local args = fct:match(".*%((.*)%)")
						local subtype, _, subargs_array, _ = parse_member(fct)
						local subargs = {}
						local subcallargs = {}
						if subargs_array then
							local isubarg = 0
							for k, v in pairs(subargs_array) do
								table.insert( subargs, "const LuaValue& subargval" .. isubarg )
								table.insert( subcallargs, "subargval" .. isubarg )
								isubarg = isubarg + 1
							end
						end
						table.insert( upvalues, "LuaValue argval" .. iarg .. " = Lua::value(L, " .. iarg .. ");" );
						table.insert( final, "[L,argval" .. iarg .. "](" .. table.concat(subargs, ", ") .. "){ argval" .. iarg .. "(" .. table.concat(subcallargs, ", ") .. "); }" )
					else
						table.insert( final, "(" .. param.type .. ")Lua::value(L, " .. iarg .. ")" .. cast_suffix(param, { enums = enums }) )
					end
				end
				iarg = iarg + 1
			end
			return final, upvalues
		end
		function build_params( tokens, i, has_self )
			local iarg = ( has_self and 2 or 1 )
			local part = 0
			local skip = false
			local final = {}
			local type = nil

			while tokens[i] ~= "(" do
				i = i + 1
			end
			i = i + 1

			while tokens[i]:find( ")" ) == nil do
				if part == 0 then
					type = tokens[i]
					if (tokens[i] == "Lua*" or tokens[i] == "Lua") then
						skip = true
					end
				end
				if part == 1 then
					if skip == false then
-- 						if type ~= nil then
-- 							final[#final + 1] = "static_cast<" .. type .. ">(Lua::value(L, " .. iarg .. "))"
-- 						else
							final[#final + 1] = "Lua::value(L, " .. iarg .. ")"
-- 						end
					end
					iarg = iarg + 1
				end

				part = part + 1
				if tokens[i] == "," then
					part = 0
					skip = false
				end
				i = i + 1
			end

			return table.concat( final, ", " )
		end

		function namespace_string(classname)
			return classname:gsub(":", "_")
		end

		function parent_class(classname)
			local last = classname
			for token in classname:gmatch("[^::]+") do
				last = token
				break
			end
			return last
		end
		function basename(classname)
			local last = classname
			for token in classname:gmatch("[^::]+") do
				last = token
			end
			return last
		end

		local i = 1
		while i <= #tokens do
			local token = tokens[i]

			if token:sub( #token, #token ) == "{" then
-- 				level = level + 1
			elseif token:sub( 1, 1 ) == "}" and tokens[i+1] == ";" and curr_class ~= "" then
				if level >= 1 then
					io.stderr:write("END OF " .. curr_class .. "(level : " .. level .. ")" .. "\n")
					all_classes[curr_class] = true
					local dest = ( level > 1 and stack[#stack].subclasses or classes )
					dest[curr_class] = dest[curr_class] or {
						exposed = ( curr_exported_class ~= nil and #curr_exported_class > 0 ),
						properties = properties,
						members = members,
						statics = statics,
						included = included,
						parent_classes = parent_classes,
						constructor_params = constructor_params,
						has_destructor = has_destructor,
						enums = enums,
						subclasses = subclasses
					}

					if level > 1 then
						curr_class = stack[#stack].curr_class
						curr_exported_class = stack[#stack].curr_exported_class
						properties = stack[#stack].properties
						members = stack[#stack].members
						statics = stack[#stack].statics
						parent_classes = stack[#stack].parent_classes
						has_destructor = stack[#stack].has_destructor
						included = stack[#stack].included
						constructor_params = stack[#stack].constructor_params
						subclasses = stack[#stack].subclasses
						enums = stack[#stack].enums
						table.remove( stack, #stack )
					else
						curr_class = ""
						curr_exported_class = ""
						properties = {}
						members = {}
						statics = {}
						parent_classes = {}
						has_destructor = false
						included = false
						constructor_params = {}
						enums = {}
						subclasses = {}
					end
				end
				level = level - 1
			end

		-- 	print(token)
			if token == "#define" then
				local_definitions[tokens[i + 1]] = tokens[i + 2]
			elseif (token == "LUA_CLASS" and tokens[i + 1] == "class") or (token == "class" and tokens[i + 2] ~= ";") then
				local isExposed = (token == "LUA_CLASS")
				if level > 0 then
					table.insert( stack, {
						level = level,
						curr_class = curr_class,
						curr_exported_class = curr_exported_class,
						constructor_params = constructor_params,
						properties = properties,
						members = members,
						statics = statics,
						parent_classes = parent_classes,
						has_destructor = has_destructor,
						included = included,
						enums = enums,
						subclasses = subclasses or {}
					})
					curr_class = curr_class .. "::" .. tokens[i + (isExposed and 2 or 1)]
					curr_exported_class = (isExposed and curr_class or "" )
					properties = {}
					members = {}
					statics = {}
					parent_classes = {}
					has_destructor = false
					included = false
					constructor_params = {}
					enums = {}
					subclasses = {}
				else
					curr_class = tokens[i + (isExposed and 2 or 1)]
					curr_exported_class = (isExposed and curr_class or "" )
				end
				if not included and isExposed then
					table.insert( includes, "#include \"" .. filename .. "\"" )
					included = true
				end
				level = level + 1

				i = i + 1
				local line = {}
				while i < #tokens and tokens[i] ~= "{" do
					table.insert( line, tokens[i] )
					i = i + 1
				end
				line = table.concat(line, " ")
				if line:find(":") then
					local parents = line:match(":%s*(.*)")
					for parent in parents:gmatch("[a-z]+%s+(%S+)") do
						table.insert( parent_classes, parent )
					end
				end
			elseif token == "class" then
				io.stderr:write("ON NON-LUA CLASS " .. tokens[i+1] .. "\n")
				curr_class = tokens[i + 1]
			elseif token == "typedef" and tokens[i + 1] == "enum" then
				io.stderr:write("ON ENUM  ")
				while i <= #tokens and tokens[i] ~= "}" do
					i = i + 1
				end
				enums[tokens[i + 1]] = true
				io.stderr:write(tokens[i + 1] .. " = true\n")
			elseif token == "LUA_EXPORT" then
				if tokens[i + 1] == "typedef" and tokens[i + 2] == "enum" then
					io.stderr:write("ON ENUM  ")
					local enum_string = {}
					while i <= #tokens and tokens[i] ~= "}" do
						table.insert( enum_string, tokens[i] )
						i = i + 1
					end
					local lines = {}
					for v in table.concat(enum_string, " "):match(".*{(.*)"):gmatch("([^,]+)") do
						table.insert( lines, v )
					end
					local enum = {}
					local prev = 0
					local subG = {}
					for k, v in pairs(local_definitions) do
						local caps = ( type(v) == "string" and "'" or "" )
						table.insert( subG, "_G['" .. k .. "'] = " .. caps .. v .. caps )
					end
					for i, line in ipairs(lines) do
						if not line:find("{") and not line:find("}") then
							io.stderr:write("    " .. line .. "\n")
							local name, value = line:match("^%s*([a-zA-Z0-9_]*)%s*=*%s*(.*)")
							io.stderr:write("    " .. (name or "" ) .. " = \"" .. (value or "" ) .. "\"\n")
							local f, err = load(table.concat(subG, ";") .."; return " .. value)
							value = f()
							io.stderr:write("    → " .. (name or "" ) .. " = " .. (value or "" ) .. "\n")
							if value == nil or ( type(value) == "string" and #value == 0 ) then
								value = prev
							end
							enum[name] = value
							prev = value + 1
						end
					end
					enums[tokens[i + 1]] = enum
					io.stderr:write(tokens[i + 1] .. " = true\n")
-- 					os.exit()
				else
					i = i + 1
					local line = {}
					while i <= #tokens and tokens[i] ~= ";" do
						table.insert( line, tokens[i] )
						i = i + 1
					end
					line = table.concat(line, " ")
					local func, func_type, params, modifiers = parse_member(line)
					if func == basename(curr_class) then
						constructor_params = build_args( params, true )
						if #constructor_params > 0 then
	-- 						constructor_params = ", " .. constructor_params
						end
					elseif func == "~" .. curr_class then
						has_destructor = true
					else
						if modifiers["static"] then
							local params, upvalues = build_args( params, false )
							params = table.concat( params, ", " )
							table.insert( statics, string.format( "\tlua_pushcclosure( L, []( lua_State* L ) {" ) )
							if func_type ~= "void" then
								table.insert( statics, table.concat( upvalues, "\n" ) )
								table.insert( statics, string.format( "\t\tLuaValue ret = %s::%s(%s);", curr_class, func, params ) )
								table.insert( statics, "\t\tret.push( L );" )
								table.insert( statics, "\t\treturn 1;" )
							else
								table.insert( statics, table.concat( upvalues, "\n" ) )
								table.insert( statics, string.format( "\t\t%s::%s(%s);", curr_class, func, params ) )
								table.insert( statics, "\t\treturn 0;" )
							end
							table.insert( statics, string.format( "\t}, 0);\n\tlua_setfield( L, -2, \"%s\" );", func ) )
						else
							local params, upvalues = build_args( params, true )
							params = table.concat( params, ", " )
							table.insert( members, string.format( [[
		lua_pushcclosure( L, []( lua_State* L ) {
			%s* object = static_cast<%s*>( lua_touserdata( L, 1 ) );]], curr_class, curr_class ) )
							if func_type ~= "void" then
								table.insert( members, table.concat( upvalues, "\n" ) )
								table.insert( members, string.format( "\t\tLuaValue ret = object->%s(%s);", func, params ) )
								table.insert( members, "\t\tret.push( L );" )
								table.insert( members, "\t\treturn 1;" )
							else
								table.insert( members, table.concat( upvalues, "\n" ) )
								table.insert( members, string.format( "\t\tobject->%s(%s);", func, params ) )
								table.insert( members, "\t\treturn 0;" )
							end
							table.insert( members, string.format( [[
		}, 0);
		lua_setfield( L, -2, "%s" );]], func ) )
						end
					end
				end
			elseif token == "LUA_PROPERTY" then
				io.stderr:write("ON LUA_PROPERTY with " .. curr_class .. "\n")
				i = i + 1
				local line = {}
				while tokens[i] ~= ";" do
					table.insert( line, tokens[i] )
					i = i + 1
				end
				line = table.concat(line, " ")
				local luaname, type, name = "", "", ""
						io.stderr:write("LLL\n")
						io.stderr:write(line)
						io.stderr:write("\n")
				if line:match("%(%s*\"([a-zA-Z0-9_%.]+)\"%s*%)%s+(void%s*[^%*])") then
					io.stderr:write("\t→ mode 1\n")
					luaname, funcname, params = line:gmatch("%(%s*\"([a-zA-Z0-9_%.]+)\"%s*%)%s+void%s+([a-zA-Z0-9_]+)%s*%((.*)%)")()
					type = params:match("%s*(.-)%s*[a-zA-Z0-9_]+%s*$"):match("^%s*(.-)%s*$"):gsub("&", "")
					local tables = {}
					for t in luaname:gmatch("([^%.]+)") do
						table.insert( tables, t )
					end
					local parent = properties
					for i = 1, #tables-1 do
						parent[tables[i]] = parent[tables[i]] or {}
						parent = parent[tables[i]]
					end
					io.stderr:write("\t→ " .. luaname .. "\n")
					io.stderr:write("\t→ " .. funcname .. "\n")
					io.stderr:write("\t→ " .. type .. "\n")
					table.insert( parent, { __isprop = true, __issetter = true, luaname = tables[#tables], type = type, funcname = funcname, name = name, classname = curr_class } )
				elseif line:match("%(%s*\"([a-zA-Z0-9_%.]+)\"%s*%)%s+[a-zA-Z0-9_%*<>:%s]+%s+[a-zA-Z0-9_]+%s*%(%s*[void]*%)") or line:match("%(%s*%)%s+[a-zA-Z0-9_%*<>:%s]+%s+[a-zA-Z0-9_]+%s*%(%s*[void]*%)") then
					io.stderr:write("\t→ mode 2\n")
					luaname, type, funcname = line:gmatch("%(%s*\"*([a-zA-Z0-9_%.]*)\"*%s*%)%s+([a-zA-Z0-9_%*<>:%s]+)%s+([a-zA-Z0-9_]+)%s*%(%s*[void]*%)")()
					if luaname == nil or #luaname == 0 then
						luaname = funcname
					end
					local tables = {}
					for t in luaname:gmatch("([^%.]+)") do
						table.insert( tables, t )
					end
					local parent = properties
					for i = 1, #tables-1 do
						parent[tables[i]] = parent[tables[i]] or {}
						parent = parent[tables[i]]
					end
					io.stderr:write("\t→ " .. luaname .. "\n")
					table.insert( parent, { __isprop = true, __isgetter = true, luaname = tables[#tables], type = type, funcname = funcname, name = name, classname = curr_class } )
				else
					io.stderr:write("\t→ mode 3\n")
					luaname, type, name = line:gmatch( "%(%s*\"([a-zA-Z0-9_%.]+)\"%s*%)%s*([a-zA-Z0-9_,%*<>%(%):%s]+)%s+([a-zA-Z0-9_]+)%s*$" )()
					io.stderr:write("\t→ " .. (luaname or "") .. "\n")
					io.stderr:write("\t→ " .. (type or "") .. "\n")
					io.stderr:write("\t→ " .. (name or "") .. "\n")
					table.insert( generated_templates, string.format( "struct %s_%s { typedef %s %s::*type; friend type get(%s_%s); };", namespace_string(curr_class), name, type, curr_class, namespace_string(curr_class), name ) )
					table.insert( generated_templates, string.format( "template struct Rob<%s_%s, &%s::%s>;", namespace_string(curr_class), name, curr_class, name ) )
					local tables = {}
					for t in luaname:gmatch("([^%.]+)") do
						table.insert( tables, t )
					end
					local parent = properties
					for i = 1, #tables-1 do
						parent[tables[i]] = parent[tables[i]] or {}
						parent = parent[tables[i]]
					end
					io.stderr:write("\t→ " .. luaname .. "\n")
					table.insert( parent, { __isprop = true, luaname = tables[#tables], type = type, name = name, classname = curr_class } )
				end
			--[=[
			elseif token == "LUA_MEMBER" then
				local has_self = true
				local tok_shift = 0
				if tokens[i + 1] == "static" then
					has_self = false
					tok_shift = 1
				end
				local type = tokens[i + tok_shift + 1]
				local name = tokens[i + tok_shift + 2]
				local params = list_params( tokens, i + tok_shift + 3, true )
				table.insert( interfaces, string.format( "\n\n%s %s::%s( %s ) {", type, curr_class, name, table.concat( list_params( tokens, i + tok_shift + 3 ), " " ) ) )
				table.insert( interfaces, string.format( "\tlua_State* L = mState->state();" ) )
				table.insert( interfaces, string.format( "\tlua_rawgeti( L, LUA_REGISTRYINDEX, mReference );" ) )
				table.insert( interfaces, string.format( "\tlua_getfield( L, -1, \"%s\" );", name ) )
				table.insert( interfaces, string.format( "\tlua_pushvalue( L, -2 );" ) )
				table.insert( interfaces, string.format( "\tlua_remove( L, -3 );" ) )
				for iarg, argname in ipairs(params) do
					table.insert( interfaces, string.format( "\tLuaValue(%s).push( L );", argname ) )
				end
				if type ~= "void" then
					table.insert( interfaces, string.format( "\tlua_call( L, %d, 1 );", 1 + #params ) )
					table.insert( interfaces, string.format( "\tLuaValue ret = mState->value(-1);" ) )
					table.insert( interfaces, string.format( "\tlua_pop( L, -1 );" ) )
					table.insert( interfaces, string.format( "\treturn ret;" ) )
				else
					table.insert( interfaces, string.format( "\tlua_call( L, %d, 0 );", 1 + #params ) )
				end
				table.insert( interfaces, string.format( "}\n" ) )
												--[[
				if type ~= "void" then
					table.insert( statics, string.format( "\t\tLuaValue ret = %s::%s(%s);", curr_class, name, params ) )
					table.insert( statics, "\t\tret.push( L );" )
					table.insert( statics, "\t\treturn 1;" )
				else
					table.insert( statics, string.format( "\t\t%s::%s(%s);", curr_class, name, params ) )
					table.insert( statics, "\t\treturn 0;" )
				end
				table.insert( statics, string.format( "\t}, 0);\n\tlua_setfield( L, -2, \"%s\" );", name ) )
												]]
					]=]
			end
			i = i + 1
		end
	end
end


local tab_level = 0
function out( s )
	table.insert( final, string.rep("\t", tab_level) )
	table.insert( final, s )
	table.insert( final, "\n" )
end


function output_class(classname, v, global)
	io.stderr:write("[" .. (v.exposed and "1" or "0") .. "]" .. classname .. "\n")
	if v.exposed then
		for _, parent in ipairs(v.parent_classes) do
			io.stderr:write("parent '" .. parent .. "'\n")
			if classes[parent] then
				local concat_properties = nil
				local concat_properties2 = function(dest, src)
					for k, v in pairs(src) do
						if v.__isprop then
							io.stderr:write( "  → " .. v.luaname .. "\n" );
							table.insert( dest, v )
						else
							concat_properties( dest[k], v )
						end
					end
				end
				concat_properties = concat_properties2
				concat_properties( v.properties, classes[parent].properties )
				for k, value in pairs(classes[parent].enums) do
					v.enums[k] = value
				end
			end
		end
		out( string.format( [[
	// %s
	lua_createtable( L, 0, 0 );
	lua_pushvalue( L, -1 );
	lua_setmetatable( L, -2 );

	lua_pushcclosure( L, []( lua_State* L ) {
		%s* pObject = reinterpret_cast<%s*>( lua_newuserdata( L, sizeof(%s) ) );
//		lua_pushvalue( L, -1 );
//		int ref = luaL_ref( L, LUA_REGISTRYINDEX );

		lua_createtable( L, 0, 0 );]], classname, classname, classname, classname ))
				table.insert( v.statics, "\textend( L );" )

		out( "\t\t\tlua_createtable( L, 0, 0 );" )
		out( "\t\t\t\tlua_pushstring( L, \"c-class\" );" )
		out( "\t\t\t\tlua_setfield( L, -2, \"type\" );" )
		out( "\t\t\t\tlua_pushstring( L, \"" .. classname .. "\" );" )
		out( "\t\t\t\tlua_setfield( L, -2, \"name\" );" )
		local append_subtable = nil
		local append_subtable2 = function(t, level)
			for t, v in pairs(t) do
				if v.__isprop then
				else
					out( string.rep("\t", level+4) .. "lua_createtable( L, 0, 0 );" )
					out( string.rep("\t", level+4) .. "	lua_createtable( L, 0, 0 );" )
					out( string.rep("\t", level+4) .. "		lua_pushcclosure( L, []( lua_State* L ) {" )
					out( string.rep("\t", level+4) .. "			const char* field = lua_tostring( L, 2 );" )
					out( string.rep("\t", level+4) .. "			if ( __newindex( L, field ) == true ) {" )
					out( string.rep("\t", level+4) .. "				return 0;" )
					out( string.rep("\t", level+4) .. "			}" )
					out( string.rep("\t", level+4) .. "			lua_getmetatable( L, 1 );" )
					out( string.rep("\t", level+4) .. "			lua_getfield( L, -1, \"__instance\" );" )
					out( string.rep("\t", level+4) .. string.format("			%s* object = static_cast<%s*>( lua_touserdata( L, -1 ) );", classname, classname ) )
					local n = 1
					for i, prop in ipairs(v) do
						if prop.__isprop then
							local pad = string.rep("\t", level+5+2)
							local lines = { pad }
							if n > 1 then
								table.insert( lines, "else " )
							end
							table.insert( lines, "if ( !strcmp( field, \"" .. prop.luaname .. "\" ) ) {\n" )
							if prop.__issetter then
								table.insert( lines, string.format( pad .. "\tobject->%s(" .. cast(prop) .. "Lua::value(L, 3)" .. cast_suffix(prop, v) .. ");\n", prop.funcname ) )
							elseif prop.__isgetter then
							else
								table.insert( lines, string.format( pad .. "\tobject->*get(%s_%s()) = " .. cast(prop) .. "Lua::value(L, 3)" .. cast_suffix(prop, v) .. ";\n", namespace_string(prop.classname), prop.name ) )
							end
							table.insert( lines, pad .. "}" )
							out( table.concat( lines, "" ) )
							n = n + 1
						end
					end
					out( string.rep("\t", level+4) .. "			lua_pop( L, 2 );" )
					out( string.rep("\t", level+4) .. "			return 0;" )
					out( string.rep("\t", level+4) .. "		}, 0);" )
					out( string.rep("\t", level+4) .. "		lua_setfield( L, -2, \"__newindex\" );" )
					out( string.rep("\t", level+4) .. "		lua_createtable( L, 0, 0 );" )
					out( string.rep("\t", level+4) .. "		lua_setfield( L, -2, \"__index\" );" )
					out( string.rep("\t", level+4) .. "		lua_pushvalue( L, -" .. ( level + 5 ) .. " );" )
					out( string.rep("\t", level+4) .. "		lua_setfield( L, -2, \"__instance\" );" )
					out( string.rep("\t", level+4) .. "		lua_pushvalue( L, -" .. ( level + 7 ) .. " );" )
					out( string.rep("\t", level+4) .. "		lua_setfield( L, -2, \"__baseclass\" );" )
					out( string.rep("\t", level+4) .. "	lua_setmetatable( L, -2 );" )
					append_subtable( v, level + 1 )
					out( string.rep("\t", level+4) .. "lua_setfield( L, -2, \"" .. t .. "\" );" )
				end
			end
		end
		append_subtable = append_subtable2
		append_subtable( v.properties, 0 )
			out( "\t\t\t\t	lua_createtable( L, 0, 0 );" )
			out( "\t\t\t\t		lua_pushcclosure( L, []( lua_State* L ) {" )
			out( "\t\t\t\t			const char* field = lua_tostring( L, 2 );" )
			out( "\t\t\t\t			lua_getmetatable( L, 1 );" )
			out( "\t\t\t\t			lua_getfield( L, -1, \"__instance\" );" )
			out( string.format("\t\t\t\t			%s* object = static_cast<%s*>( lua_touserdata( L, -1 ) );", classname, classname) )
			out( "\t\t\t\t			lua_pop( L, 1 ); // lua_getfield(\"__instance\")" )
			out( "\t\t\t\t			lua_pop( L, 1 ); // lua_getmetatable" )
			out( "\t\t\t\t			int ret = 0;" )
		local n = 1
		for i, prop in ipairs(v.properties) do
			if prop.__isprop and prop.__isgetter then
				local lines = { "\t\t\t\t\t" }
				if n > 1 then
					table.insert( lines, "else " )
				end
				table.insert( lines, "if ( !strcmp( field, \"" .. prop.luaname .. "\" ) ) {\n" )
				table.insert( lines, string.format( "\t\t\t\t\t\tLuaValue(object->%s()).push(L);\n", prop.funcname ) )
				table.insert( lines, string.format( "\t\t\t\t\t\tret = 1;\n" ) )
				table.insert( lines, "\t\t\t\t\t}" )
				out( table.concat( lines, "" ) )
				n = n + 1
			end
		end
			out( "\t\t\t\t			if ( ret == 0 ) {" )
			out( "\t\t\t\t				lua_getmetatable( L, 1 );" )
			out( "\t\t\t\t				lua_getfield( L, -1, \"__baseclass\" );" )
			out( "\t\t\t\t				lua_pushvalue( L, 2 );" )
			out( "\t\t\t\t				lua_gettable( L, -2 );" )
			out( "\t\t\t\t				lua_remove( L, -2 ); // lua_getfield(\"__baseclass\")" )
			out( "\t\t\t\t				lua_remove( L, -2 ); // lua_getmetatable" )
			out( "\t\t\t\t				ret = 1;" )
			out( "\t\t\t\t			}" )
			out( "\t\t\t\t			return ret;" )
			out( "\t\t\t\t		}, 0);" )
			out( "\t\t\t\t		lua_setfield( L, -2, \"__index\" );" )
			out( "\t\t\t\t		lua_pushvalue( L, -4 );" )
			out( "\t\t\t\t		lua_setfield( L, -2, \"__instance\" );" )
			out( "\t\t\t\t		lua_pushvalue( L, -6 );" )
			out( "\t\t\t\t		lua_setfield( L, -2, \"__baseclass\" );" )
			out( "\t\t\t\t	lua_setmetatable( L, -2 );" )
			out( "\t\t\t\tlua_setfield( L, -2, \"__index\" );" )
		out( "\t\t\t\tlua_pushcclosure( L, []( lua_State* L ) {" )
		out( "\t\t\t\t\tconst char* field = lua_tostring( L, 2 );" )
		out( "\t\t\t\t\tif ( __newindex( L, field ) == true ) {" )
		out( "\t\t\t\t\t	return 0;" )
		out( "\t\t\t\t\t}" )
		out( string.format( "\t\t\t\t\t%s* object = static_cast<%s*>( lua_touserdata( L, 1 ) );", classname, classname ) )
		local n = 1
		for i, prop in ipairs(v.properties) do
			if prop.__isprop then
					local lines = { "\t\t\t\t\t" }
					if n > 1 then
						table.insert( lines, "else " )
					end
					table.insert( lines, "if ( !strcmp( field, \"" .. prop.luaname .. "\" ) ) {\n" )
					if prop.__issetter then
						table.insert( lines, string.format( "\t\t\t\t\t\tobject->%s(" .. cast(prop) .. "Lua::value(L, 3)" .. cast_suffix(prop, v) .. ");\n", prop.funcname ) )
					elseif prop.__isgetter then
					else
						table.insert( lines, string.format( "\t\t\t\t\t\tobject->*get(%s_%s()) = " .. cast(prop) .. "Lua::value(L, 3)" .. cast_suffix(prop, v) .. ";\n", namespace_string(prop.classname), prop.name ) )
					end
					table.insert( lines, "\t\t\t\t\t}" )
					out( table.concat( lines, "" ) )
					n = n + 1
			end
		end
		out( "\t\t\t\t\treturn 0;" )
		out( "\t\t\t\t}, 0);" )
		out( "\t\t\tlua_setfield( L, -2, \"__newindex\" );" )
		out( "\t\tlua_setmetatable( L, -2 );" )
		out( "" )
		out( "\t\tint top = lua_gettop( L );" )
		out( string.format( "\t\tnew (pObject) %s( %s );", classname, table.concat( v.constructor_params, ", " ) ) )
		out( "\t\tlua_getfield( L, 1, \"init\" );" )
		out( "\t\tif ( not lua_isnil( L, -1 ) ) {" )
		out( "\t\t\tlua_pushvalue( L, -2 );" )
		out( "\t\t\tlua_pushvalue( L, 2 );" )
		out( "\t\t\tlua_call( L, 2, 0 );" )
		out( "\t\t} else {" )
		out( "\t\t\tlua_pop( L, 1 );" )
		out( "\t\t}" )
		out( "\t\tif ( top > " .. ( #v.constructor_params + 1 ) .. " and lua_type( L, 2 ) == LUA_TTABLE ) {" )
		out( "\t\t	lua_pushvalue( L, -1 );" )
		out( "\t\t	lua_pushvalue( L, 2 );" )
		out( "\t\t	lua_pushnil( L );" )
		out( "\t\t	while ( lua_next( L, -2 ) != 0 ) {" )
		out( "\t\t		lua_pushvalue( L, -2 ); // key" )
		out( "\t\t		lua_pushvalue( L, -2 ); // value" )
		out( "\t\t		lua_settable( L, -6 );" )
		out( "\t\t		lua_pop( L, 1 );" )
		out( "\t\t	}" )
		out( "\t\t	lua_pop( L, 1 );" )
		out( "\t\t	lua_pop( L, 1 );" )
		out( "\t\t}" )
		out( "\t\treturn 1;" )
		out( "\t}, 0);" )
		out( "\tlua_setfield( L, -2, \"__call\" );" )
		out( "" )
		if v.has_destructor then
			out( string.format( [[
	lua_pushcclosure( L, []( lua_State* L ) {
		%s* object = static_cast<%s*>( lua_touserdata( L, 1 ) );
		object->~%s();
		return 0;
	}, 0);
	lua_setfield( L, -2, "__gc" );
]], classname, classname, classname ))
		end
		if type(v.enums) == "table" then
			for _, enum in pairs( v.enums ) do
				if type(enum) == "table" then
					for k, v in pairs(enum) do
						out( "\t\t\t\tlua_pushinteger( L, " .. v .. " );" )
						out( "\t\t\t\tlua_setfield( L, -2, \"" .. k .. "\" );" )
					end
				end
			end
		end
		out( table.concat( v.statics, "\n" ) )
		out( table.concat( v.members, "\n" ) )
		if v.subclasses then
			tab_level = tab_level + 1
			for a, b in pairs(v.subclasses) do
				output_class(a, b, false)
			end
			tab_level = tab_level - 1
		end
		if global then
			out( string.format( "\tlua_setglobal( L, \"%s\" );", classname ) )
		else
			out( string.format( "\tlua_setfield( L, -2, \"%s\" );", basename(classname) ) )
		end
	end
end


for classname, v in pairs(classes) do
	output_class(classname, v, true)
end


function final_print( s )
	if out_file ~= nil then
		local file = io.open( out_file, "w" )
		file:write( s )
		file:close()
	else
		print( s )
	end
end

final_print( [[
#include <cstdint>
#include <cstring>
#include <Lua.h>
]] .. table.concat( includes, "\n" ) .. [[

template<typename Tag, typename Tag::type M> struct Rob { 
	friend typename Tag::type get(Tag) {
		return M;
	}
};

]] .. table.concat( generated_templates, "\n" ) .. [[


bool __newindex( lua_State* L, const char* field )
{
	if ( lua_type( L, 3 ) == LUA_TTABLE ) {
		lua_getfield( L, 1, field );
		if ( lua_type( L, -1 ) == LUA_TTABLE ) {
			lua_pushvalue( L, 3 );
			lua_pushnil( L );
			while ( lua_next( L, -2 ) != 0 ) {
				lua_pushvalue( L, -2 ); // key
				lua_pushvalue( L, -2 ); // value
				lua_settable( L, -6 );
				lua_pop( L, 1 );
			}
			lua_pop( L, 1 );
			lua_pop( L, 1 );
			return true;
		}
		lua_pop( L, 1 );
	}
	lua_getmetatable( L, 1 );
		lua_getfield( L, -1, "__index" );
		lua_pushvalue( L, 2 );
		lua_pushvalue( L, 3 );
		lua_rawset( L, -3 );
	lua_pop( L, 1 );
	return false;
}


void extend( lua_State* L )
{
	lua_pushcclosure( L, []( lua_State* L ) {
		lua_createtable( L, 0, 0 );
		lua_pushvalue( L, -1 );
		lua_setmetatable( L, -2 );
//		lua_pushvalue( L, 1 );
//		lua_setfield( L, -2, "super" );
		lua_pushvalue( L, 1 );
		lua_setfield( L, -2, "__index" );
		lua_getfield( L, 1, "__call" );
		lua_setfield( L, -2, "__call" );
		return 1;
	}, 0);
	lua_setfield( L, -2, "extend" );
}

extern "C" void ]] .. init_fn .. [[( lua_State* L )
{
]] .. table.concat(final) .. [[
}
]] .. table.concat( interfaces, "\n" ) .. [[
]])

