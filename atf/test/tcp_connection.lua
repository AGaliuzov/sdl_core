local module = { mt = { __index = {} } }
function module.Connection(host, port) --{{{
  local res =
  {
    host = host,
    port = port
  }
  res.socket = TcpClient()
  setmetatable(res, module.mt)
  return res
end --}}}
local function checkSelfArg(s)
  if type(s) ~= "table" or
    getmetatable(s) ~= module.mt then
    error("Invalid argument 'self': must be connection (use ':', not '.')")
  end
end
function module.mt.__index:Connect()
  checkSelfArg(self)
  self.socket:connect(self.host, self.port)
end
function module.mt.__index:Send(data)
  checkSelfArg(self)
  return self.socket:write(data)
end
function module.mt.__index:Recv(len)
  checkSelfArg(self)
  return self.socket:read(len)
end
function module.mt.__index:OnDataAvailable(func)
  checkSelfArg(self)
  local d = qt.dynamic()
  local this = self
  function d.readyRead()
    func(this)
  end
  qt.connect(self.socket, "readyRead()", d, "readyRead()")
end
function module.mt.__index:Close()
  checkSelfArg(self)
  self.socket:close();
end
return module
