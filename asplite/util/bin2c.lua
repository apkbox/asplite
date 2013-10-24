--[=[
// The MIT License (MIT)
//
// Copyright (c) 2013 Alex Kozlov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
--]=]

--[=[
	Converts input file to a C array.
--]=]

local use_tabs = false;
local indent = -1;
local bytes_per_row = 8;
local max_width = 0;
local variable_name = 'data';
local template_file = nil;

function PrintUsage()
	print(string.format('usage: %s [-t] [-i:N] [-n:N] [-w:N] [-v:<name>] [-c:<file>] input_file [output_file]', arg[0]));
	print('        -t         Use tabs instead of spaces.');
	print('        -i:N       N indent data with N spaces/tabs (default 4 for spaces or 1 for tabs).');
	print('        -n:N       N number of bytes of data per row (default 8).');
	print('        -w:N       Maximum row width.');
	print('                   This option takes precendence over -n option. ');
	print('        -v:<name>  Variable name (default "data").');
	print('        -c:<file>  Template file name.');
	print('                   Use ${data} for data placeholder.');
	print('                   Use ${size} for array size placeholder.');
	print('                   Use ${name} for variable name placeholder.');
end

if #arg < 1 then
	PrintUsage();
	os.exit(0);
end

args = {};

for i = 1, #arg do
	local option, value = string.match(arg[i], '^%-(%l):(%d*)');
	if option and #value > 0 then
		if option == 'i' then
			indent = tonumber(value);
		elseif option == 'n' then
			bytes_per_row = tonumber(value);
		elseif option == 'w' then
			max_width = tonumber(value);
		end
	else
		option, value = string.match(arg[i], '^%-(%l):(.*)');
		if option and #value > 0 then
			if option == 'v' and #value then
				variable_name = value;
			elseif option == 'c' and #value then
				template_file = value;
			end
		else 
			option = string.match(arg[i], '^%-(%l)');
			if option then
				if option == 't' then
					use_tabs = true;
				end
			else
				table.insert(args, arg[i]);
			end
		end
	end
end

if #args < 1 then
	print('error: missing input file name');
	PrintUsage();
	os.exit(1);
end

local template = 'unsigned char ${name}[${size}] = {\n${data}};\n';

if template_file then
	local tf = io.open(template_file, 'r');
	if not tf then
		print('error: cannot open template file "' .. template_file .. '".');
		os.exit(1);
	end

	template = tf:read('*a');
	tf:close();
end

local out = io.stdout;

if #args > 1 then
	out = io.open(args[2], 'w');
	if not out then
		print('error: cannot open output file "' .. args[2] .. '".');
		os.exit(1);
	end
end

local bin = '';

local f = io.open(args[1], 'rb');
if f then
	bin = f:read('*a');
	f:close();
else
	print('error: cannot open input file "' .. args[1] .. '".');
	os.exit(1);
end

local indent_char = ' ';
if use_tabs then
	indent_char = '\t';
	if indent < 0 then
		indent = 1;
	end
else
	if indent < 0 then
		indent = 4;
	end
end

local buffer = '';

local line = string.rep(indent_char, indent);
local width = #line;
for i = 1, #bin do
	local s = string.format('0x%02x', bin:byte(i));
	if i < #bin then
		s = s .. ', ';
	end

	if max_width > 0 and width + #s >= max_width and #line > 0 then
		buffer = buffer .. line .. '\n';
		line = string.rep(indent_char, indent) .. s;
		width = #line;
	else
		line = line .. s;
		width = width + #s;
	
		if max_width == 0 and (i % bytes_per_row) == 0 then
			if #line > 0 then
				buffer = buffer .. line .. '\n';
			end
			line = string.rep(indent_char, indent);
			width = #line;
		end
	end
end

if #line > 0 then
	buffer = buffer .. line .. '\n';
end

local content = string.gsub(template, '${name}', tostring(variable_name));
content = string.gsub(content, '${size}', tostring(#bin));
content = string.gsub(content, '${data}', buffer);
out:write(content);

if out ~= io.stdout then
	out:close();
end
