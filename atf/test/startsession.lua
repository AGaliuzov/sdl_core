tcp = require ("tcp_connection")
protocol_handler = require("protocol_handler")
events = require("events")

c = tcp.Connection("localhost", 12345)

local startSession =
{
  version     = 3,
  encryption  = false,
  frameType   = 0,
  serviceType = 7,
  frameInfo   = 1,
  sessionId   = 0,
  messageId   = 0
}
c:OnConnected(function()
  print("connected")
  c:Send(protocol_handler.Compose(startSession))
end)

c:OnInputData(function(self, data)
  print("incoming data:")
  local t = protocol_handler.Parse(data)
  for k, v in pairs(t) do
    print(k, v)
  end
end)

print("connecting...")
c:Connect()
