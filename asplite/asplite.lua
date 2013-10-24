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

asplite.HtmlEscapeString = function(text)
	local HtmlEscapeEntities = {
			['&'] = '&amp;', ['"'] = '&quot;',
			['<'] = '&lt;', ['>'] = '&gt;'};

	return string.gsub(text, '[&"<>]', HtmlEscapeEntities);
end


asplite.UrlDecode = function(str)
	str = string.gsub(str, "+", " ");
	str = string.gsub(str, "%%(%x%x)",
		function(h) return string.char(tonumber(h,16)) end);
	str = string.gsub(str, "\r\n", "\n");
	return str;
end


-- RFC 1123 date format
-- t must be a value returned by os.time()
asplite.HttpDate = function(t)
	local weekdays = { 'Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat' };

	local months = {
		"Jan", "Feb",  "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

	local dt = os.date('!*t', t);
	return string.format('%s, %02d %s %04d %02d:%02d:%02d GMT',
		weekdays[dt.wday], dt.day, months[dt.month], dt.year,
		dt.hour, dt.min, dt.sec);
end

asplite.ReadEntityBodyMode = {
	None = 0,
	Buffered = 1,
	Bufferless = 2,
	Classic = 3
};

asplite.CreateRequestObject = function(variables)
	local object = {
		prototype = {
			queryString_ = asplite.ParseQueryString(variables['QUERY_STRING']);
			cookies_ = {};
			form_ = {};
			files_ = {};
			serverVariables_ = variables;
			totalBytes_ = {};

			readEntityBodyMode_ = asplite.ReadEntityBodyMode.None;
		};
	};

	local function ComposeQueryString_(t)
		local s = '';
		local key = nil;

		while true do
			local values;
			local i;

			key, values = next(t, key);
			if not key then
				break;
			end
		
			if #s > 0 then
				s = s .. '&';
			end

			local pairs = '';

			for i = 1, #values do
				pairs = pairs .. key .. '=' .. values[i];
				if i < #values then
					pairs = pairs .. '&';
				end
			end

			s = s .. pairs;
		end

		return s;
	end


	local function SetNewQueryStringValue_(t, k, v)
		if not t[k] then
			rawset(t, k, {});
		end
		table.insert(t[k], v);
	end


	setmetatable(object.prototype.queryString_, {
		__tostring = ComposeQueryString_;
		__newindex = SetNewQueryStringValue_;
	});

	function object.prototype:getCookies_()
		return self.cookies_;
	end

	function object.prototype:getForm_()
		if self.readEntityBodyMode_ ~= asplite.ReadEntityBodyMode.None and
				self.readEntityBodyMode_ ~= asplite.ReadEntityBodyMode.Classic then
			error('entity already read', 2);
		end
		self.readEntityBodyMode_ = asplite.ReadEntityBodyMode.Classic;
		-- asplite.ParseRequestBody();
		return self.form_;
	end

	function object.prototype:getFiles_()
		if self.readEntityBodyMode_ ~= asplite.ReadEntityBodyMode.None and
				self.readEntityBodyMode_ ~= asplite.ReadEntityBodyMode.Classic then
			error('entity already read', 2);
		end
		self.readEntityBodyMode_ = asplite.ReadEntityBodyMode.Classic;
		-- asplite.ParseRequestBody();
		return self.files_;
	end

	function object.prototype:getQueryString_()
		return self.queryString_;
	end

	function object.prototype:getServerVariables_()
		return self.serverVariables_;
	end

	function object.prototype:getTotalBytes_()
		return self.totalBytes_;
	end

	function object.prototype:binaryRead_()
		error('Not implemented');
		return '';
	end

	object.propertyMap__ = {
		['Cookies'] = {
			get = object.prototype.getCookies_;
		};
		['Form'] = { 
			get = object.prototype.getForm_;
		};
		['Files'] = { 
			get = object.prototype.getFiles_;
		};
		['QueryString'] = {
			get = object.prototype.getQueryString_;
		};
		['ServerVariables'] = {
			get = object.prototype.getServerVariables_;
		};
		['TotalBytes'] = { 
			get = object.prototype.getTotalBytes_;
		};
		['BinaryRead'] = object.prototype.binaryRead_;
	};

	local function getprop__(t, k)
		local entry = rawget(t, 'propertyMap__')[k];
		if type(entry) == 'function' then
			return function(...) return entry(t.prototype, ...); end;
		elseif type(entry) == 'table' then
			if entry.get then
				return entry.get(t.prototype);
			else
				error('Property is write-only');
			end
		else
			error('Undefined property ' .. k, 2);
		end
	end

	local function setprop__(t, k, v)
		local entry = rawget(t, 'propertyMap__')[k];
		if type(entry) == 'table' then
			if entry.set then
				entry.set(t.prototype, v);
			else
				error('Property is read-only');
			end
		else
			error('Undefined property ' .. k);
		end
	end

	local metatable__ = {
		__index = getprop__;
		__newindex = setprop__;
	};

	setmetatable(object, metatable__);
	return object;
end


asplite.CreateResponseObject = function(httpWrite, writeToLog)
	local object = {
		prototype = {
			buffer_ = true;
			contentType_ = 'text/html';
			expires_ = nil;
			status_ = 200;

			headers_ = {};
			content_ = '';
			headersSent_ = false;
			httpWrite_ = httpWrite;
			writeToLog_ = writeToLog;

			HTTP_STATUS_CODES_ = {
				[100] = 'Continue';
				[101] = 'Switching Protocols';
				[200] = 'OK';
				[201] = 'Created';
				[202] = 'Accepted';
				[203] = 'Non-Authoritative Information';
				[204] = 'No Content';
				[205] = 'Reset Content';
				[206] = 'Partial Content';
				[300] = 'Multiple Choices';
				[301] = 'Moved Permanently';
				[302] = 'Found';
				[303] = 'See Other';
				[304] = 'Not Modified';
				[305] = 'Use Proxy';
				[307] = 'Temporary Redirect';
				[400] = 'Bad Request';
				[401] = 'Unauthorized';
				[402] = 'Payment Required';
				[403] = 'Forbidden';
				[404] = 'Not Found';
				[405] = 'Method Not Allowed';
				[406] = 'Not Acceptable';
				[407] = 'Proxy Authentication Required';
				[408] = 'Request Time-out';
				[409] = 'Conflict';
				[410] = 'Gone';
				[411] = 'Length Required';
				[412] = 'Precondition Failed';
				[413] = 'Request Entity Too Large';
				[414] = 'Request-URI Too Large';
				[415] = 'Unsupported Media Type';
				[416] = 'Requested range not satisfiable';
				[417] = 'Expectation Failed';
				[500] = 'Internal Server Error';
				[501] = 'Not Implemented';
				[502] = 'Bad Gateway';
				[503] = 'Service Unavailable';
				[504] = 'Gateway Time-out';
				[505] = 'HTTP Version not supported';
			};
		};
	};

	function object.prototype:sendHeaders_()
		local reasonPhrase = self.HTTP_STATUS_CODES_[self.status_];
		if not reasonPhrase then
			reasonPhrase = '';
		end

		self.httpWrite_('HTTP/1.0 ' .. self.status_ .. ' ' .. reasonPhrase .. '\r\n');

		if #self.contentType_ then
			self.httpWrite_('ContentType: ' .. self.contentType_ .. '\r\n');
		end

		self.httpWrite_('Date: ' .. asplite.HttpDate(os.time()) .. '\r\n');

		if self.expires_ ~= nil then
			self.httpWrite_('Expires: ' .. asplite.HttpDate(self.expires_) .. '\r\n');
		end
			
		for i, p in ipairs(self.headers_) do
			self.httpWrite_(p.name .. ': ' .. p.value .. '\r\n');
		end
		self.httpWrite_('\r\n');
		self.headersSent_ = true;
	end

	function object.prototype:flushInternal_()
		if not self.headersSent_ then
			self:sendHeaders_();
		end
		if #self.content_ then
			self.httpWrite_(self.content_);
			self.content_ = '';
		end
	end

	function object.prototype:setBuffer_(value)
		if self.headersSent_ then
			error('Cannot buffer because headers already sent');
		end
		self.buffer_ = value;
	end

	function object.prototype:getBuffer_()
		return self.buffer;
	end

	function object.prototype:setContentType_(value)
		if self.headersSent_ then
			error('Headers already sent');
		end
		self.contentType_ = value;
	end

	function object.prototype:getContentType_()
		return self.contentType_;
	end

	function object.prototype:setExpires_(value)
		if self.headersSent_ then
			error('Headers already sent');
		end
		if type(value) ~= 'number' or value < 0 then
			error('Invalid Expires value');
		end
		self.expires_ = os.time() + value;
	end

	function object.prototype:getExpires_()
		return self.expires_ - os.time();
	end

	function object.prototype:setExpiresAbsolute_(value)
		if self.headersSent_ then
			error('Headers already sent');
		end
		if type(value) ~= 'number' or value < 0 then
			error('Invalid Expires value');
		end
		self.expires_ = value;
	end

	function object.prototype:getExpiresAbsolute_()
		return self.expires_;
	end

	function object.prototype:setStatus_(value)
		if self.headersSent_ then
			error('Headers already sent');
		end
		self.status_ = value;
	end

	function object.prototype:getStatus_()
		return self.status_;
	end

	function object.prototype:addHeader_(name, value)
		if self.headersSent_ then
			error('Headers already sent');
		end
		if not string.match(name, '^[^\r\n:]+$') then
			error('Invalid header name <' .. name .. '>');
		end
		local t = {name = name, value = value};
		table.insert(self.headers_, t);
	end

	function object.prototype:clearHeaders_()
		if self.headersSent_ then
			error('Headers already sent');
		end
		self.headers_ = {};
	end

	function object.prototype:appendToLog_(text)
		self.writeToLog_(text);
	end

	function object.prototype:clear_()
		if not self.buffer_ then
			error('Output is not buffered');
		end
		self.content_ = '';
	end
	
	function object.prototype:end_()
		self:flushInternal_();
		error('__asplite_end_request__', 0);
	end

	function object.prototype:flush_()
		if not self.buffer_ then
			error("Output is not buffered");
		end
		self:flushInternal_();
	end

	function object.prototype:redirect_(url)
		if self.headersSent_ then
			error('Headers already sent');
		end
		self.status_ = 302;   -- 302 Found
		self:clearHeaders_();
		self:addHeader_("Location", url);
		--[=[ 
			TODO: Unless the request method was HEAD,
			the entity of the response SHOULD contain
			a short hypertext note with a hyperlink
			to the new URI(s).
			At the moment we have no idea about what method was used.
		--]=]
		self:clear_();
		self:flushInternal_();
	end

	function object.prototype:write_(text)
		if self.buffer_ then
			self.content_ = self.content_ .. text;
		else
			self:flushInternal_();
			self.httpWrite_(text);
		end
	end

	object.propertyMap__ = {
		['Buffer'] = { 
			get = object.prototype.getBuffer_;
			set = object.prototype.setBuffer_;
		};
		['ContentType'] = { 
			get = object.prototype.getContentType_;
			set = object.prototype.setContentType_;
		};
		['Expires'] = {
			get = object.prototype.getExpires_;
			set = object.prototype.setExpires_;
		};
		['ExpiresAbsolute'] = {
			get = object.prototype.getExpiresAbsolute_;
			set = object.prototype.setExpiresAbsolute_;
		};
		['Status'] = { 
			get = object.prototype.getStatus_;
			set = object.prototype.setStatus_;
		};
		['AddHeader'] = object.prototype.addHeader_;
		['AppendToLog'] = object.prototype.appendToLog_;
		['Clear'] = object.prototype.clear_;
		['End'] = object.prototype.end_;
		['Flush'] = object.prototype.flush_;
		['Redirect'] = object.prototype.redirect_;
		['Write'] = object.prototype.write_;

		-- An internal method that finalizes page execution.
		['RenderPageInternal'] = object.prototype.flushInternal_;
	};

	local function getprop__(t, k)
		local entry = rawget(t, 'propertyMap__')[k];
		if type(entry) == 'function' then
			return function(...) return entry(t.prototype, ...); end;
		elseif type(entry) == 'table' then
			if entry.get then
				return entry.get(t.prototype);
			else
				error('Property is write-only');
			end
		else
			error('Undefined property ' .. k);
		end
	end

	local function setprop__(t, k, v)
		local entry = rawget(t, 'propertyMap__')[k];
		if type(entry) == 'table' then
			if entry.set then
				entry.set(t.prototype, v);
			else
				error('Property is read-only');
			end
		else
			error('Undefined property ' .. k);
		end
	end

	local metatable__ = {
		__index = getprop__;
		__newindex = setprop__;
	};

	setmetatable(object, metatable__);
	return object;
end


asplite.InitAspEnvironment = function(context)
	Response = asplite.CreateResponseObject(context.write_func, context.log_func);
	Request = asplite.CreateRequestObject(context.request);
end

local res, msg = pcall(asplite.InitAspEnvironment, asplite.context);
if not res then
	error('Failed to initialize ASP: ' .. msg);
end

--[=[
	Do not use code-behind files just yet until
	@Code directive is implemented to specify
	code-behind file explicitly.
--]=]


asplite.AspErrorHandler = function(msg)
	return msg;
end


asplite.InvokeAspPage = function()
	if cb then
		cb();
	end
	AspPage__();
end


local res, msg = xpcall(asplite.InvokeAspPage, asplite.AspErrorHandler);
if not res then
	if msg ~= '__asplite_end_request__' then
		Response.Write('ASP page error: ' .. msg);
	end
end
Response.RenderPageInternal();
