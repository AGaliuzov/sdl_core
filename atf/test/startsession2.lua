mobile = require ("mobile_connection")
events = require("events")

c = mobile.MobileConnection("localhost", 12345)

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
local registerApp =
{
  version = 3,
  encryption  = false,
  frameType   = 1,
  serviceType = 7,
  frameInfo   = 0,
  sessionId   = 0,
  messageId   = 0
}
c:OnConnected(function()
  print("connected")
  c:Send(startSession)
end)

c:OnInputData(function(self, data)
  print("incoming data:")
  if data.frameInfo == 2 then
    print("StartServiceACK")
    local sessionId = data.sessionId
    local messageId = data.messageId
    request = registerApp
    request.sessionId = sessionId
    request.messageId = messageId + 1
    request.rpcType = 1
    request.rpcFunctionId = 1
    request.rpcCorrelationId = 34
    request.payload = {
      syncMsgVersion = {
        majorVersion = 1,
        minorVersion = 2
      },
      appName = "Test Application",
      isMediaApplication = true,
      languageDesired = 'EN-US',
      hmiDisplayLanguageDesired = 'EN-US',
      appHMIType = { "DEFAULT" },
      appID = "2938479"
    }
    c:Send(request)
  end
end)

print("connecting...")
c:Connect()
