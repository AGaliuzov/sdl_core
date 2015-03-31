local protocol_handler = require('protocol_handler')

local startSession =
{
  version     = 3,
  encryption  = false,
  frameType   = 0,
  serviceType = 0,
  frameInfo   = 1,
  sessionId   = 0,
  messageId   = 1
}

local bin = protocol_handler.Compose(startSession)

local msg = protocol_handler.Parse(bin)

for k, v in pairs(msg) do
  print(k, v)
end
