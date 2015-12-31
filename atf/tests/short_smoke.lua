Test = require('connecttest')
require('cardinalities')
local hmi_connection = require('hmi_connection')
local websocket      = require('websocket_connection')
local module         = require('testbase')
local events = require('events')
local mobile_session = require('mobile_session')
local mobile  = require('mobile_connection')
local tcp = require('tcp_connection')
local file_connection  = require('file_connection')
local config = require('config')
local json = require('json')
--//////////////////////////////////////////////////////////////////////////////////--
--Script checks 
---- main expectation function of ATF: EXPECT_RESPONSE, EXPECT_NOTIFICATION, EXPECT_ANY,
-- EXPECT_ANY_SESSION_NOTIFICATION, EXPECT_HMIRESPONSE, EXPECT_HMINOTIFICATION;
---- sending, receiving requests, responces, notifications from HMI and mobile side;
---- processing RPC throught different interfaces;
---- creation new sessions, connestions, openning, closing services;
---- RPC with bulk data, with float data, empty;
---- sending invalid JSON from HMI, mobile side;

function DelayedExp()
  local event = events.Event()
  event.matches = function(self, e) return self == e end
  EXPECT_EVENT(event, "Delayed event")
  RUN_AFTER(function()
              RAISE_EVENT(event, event)
            end, 2000)
end

local function SendOnSystemContext(self, ctx)
  self.hmiConnection:SendNotification("UI.OnSystemContext",{ appID = self.applications["Test Application"], systemContext = ctx })
end


--//////////////////////////////////////////////////////////////////////////////////--
--1.Check the processing OnHashChange in case of appearance it during script execution
function Test:OnHashChangePin()
  EXPECT_NOTIFICATION("OnHashChange"):Pin()
  :Do(function(_,data)
    if data.payload.hashID ~= nil then
      print (" \27[33m OnHashChange notification came \27[0m ")
    else print (" \27[33m OnHashChangePin function was called but OnHashChange occurrence is 0 \27[0m ")
    end
  end)
  :Times(AtMost(1))

DelayedExp()
end

function Test:ActivationApp()

    --hmi side: sending SDL.ActivateApp request
      local RequestId = self.hmiConnection:SendRequest("SDL.ActivateApp", { appID = self.applications["Test Application"]})

      --hmi side: expect SDL.ActivateApp response
    EXPECT_HMIRESPONSE(RequestId)
      :Do(function(_,data)
        --In case when app is not allowed, it is needed to allow app
          if
              data.result.isSDLAllowed ~= true then

                --hmi side: sending SDL.GetUserFriendlyMessage request
                  local RequestId = self.hmiConnection:SendRequest("SDL.GetUserFriendlyMessage", 
                          {language = "EN-US", messageCodes = {"DataConsent"}})

                  --hmi side: expect SDL.GetUserFriendlyMessage response
                EXPECT_HMIRESPONSE(RequestId)
                      :Do(function(_,data)

                    --hmi side: send request SDL.OnAllowSDLFunctionality
                    self.hmiConnection:SendNotification("SDL.OnAllowSDLFunctionality", 
                      {allowed = true, source = "GUI", device = {id = config.deviceMAC, name = "127.0.0.1"}})

                    --hmi side: expect BasicCommunication.ActivateApp request
                      EXPECT_HMICALL("BasicCommunication.ActivateApp")
                        :Do(function(_,data)

                          --hmi side: sending BasicCommunication.ActivateApp response
                          self.hmiConnection:SendResponse(data.id,"BasicCommunication.ActivateApp", "SUCCESS", json.EMPTY_OBJECT)

                      end)
                      :Times(2)

                      end)

        end
          end)

    --mobile side: expect OnHMIStatus notification
      EXPECT_NOTIFICATION("OnHMIStatus", {hmiLevel = "FULL"}) 

  end


--//////////////////////////////////////////////////////////////////////////////////--
--Precondition for Case_PerformInteractionTest execution
 function Test:Precondition_ForPITesrCreateInteractionChoiceSet()
	local CorIdChoice = self.mobileSession:SendRPC("CreateInteractionChoiceSet",
  {
    interactionChoiceSetID = 1,
    choiceSet = 
    {
    	{
			choiceID = 1,
			menuName = "Choice 1",
			vrCommands = {"Choice1"}

    	}
	 }
  })


  EXPECT_HMICALL("VR.AddCommand", 
  {
    cmdID = 1,
    vrCommands ={"Choice1"},
    type = "Choice"
  })
  :Do(function(_,data)
    self.hmiConnection:SendResponse(data.id, "VR.AddCommand", "SUCCESS", json.EMPTY_OBJECT)
      end)

  EXPECT_RESPONSE("CreateInteractionChoiceSet", { success = true, resultCode = "SUCCESS" })
  :Timeout(2000)

end

--//////////////////////////////////////////////////////////////////////////////////--
-- 2.Check processing messages on UI interface
function Test:Case_ShowTest()
	local CorIdShow = self.mobileSession:SendRPC("Show",
  {
    mainField1 = "Show main Field 1",
    mainField2 = "Show main Field 2",
    mainField3 = "Show main Field 3",
    mediaClock = "12:04"
  })

  EXPECT_HMICALL("UI.Show", 
  {
    showStrings = 
    {
    	{ fieldName = "mainField1",  fieldText = "Show main Field 1"},
    	{ fieldName = "mainField2",  fieldText = "Show main Field 2"},
    	{ fieldName = "mainField3",  fieldText = "Show main Field 3"},
    	{ fieldName = "mediaClock",  fieldText = "12:04"}
	},

  })
  :Do(function(_,data)
    self.hmiConnection:SendResponse(data.id,"UI.Show", "SUCCESS", json.EMPTY_OBJECT)
      end)

  EXPECT_RESPONSE(CorIdShow, { success = true, resultCode = "SUCCESS", info = nil })
  :Timeout(2000)

end

--//////////////////////////////////////////////////////////////////////////////////--
-- 3.Check processing messages on TTS interface, EXPECT_ANY function
function Test:Case_SpeakTest()

  local TTSSpeakRequestId
  EXPECT_HMICALL("TTS.Speak",
    {
      speakType = "SPEAK",
      ttsChunks = { { text = "ttsChunks", type = "TEXT" } }
    })
    :DoOnce(function(_, data)
          TTSSpeakRequestId = data.id
          self.hmiConnection:SendNotification("TTS.Started",json.EMPTY_OBJECT)
        end)

  EXPECT_NOTIFICATION("OnHMIStatus",
    { systemContext = "MAIN", hmiLevel = "FULL", audioStreamingState = "ATTENUATED" },
    { systemContext = "MAIN",  hmiLevel = "FULL", audioStreamingState = "AUDIBLE"    })
    :Times(2)
    :Do(function(exp, data)
          if exp.occurences == 1 then
            self.hmiConnection:SendResponse(TTSSpeakRequestId,"TTS.Speak", "SUCCESS", json.EMPTY_OBJECT)
            self.hmiConnection:SendNotification("TTS.Stopped",json.EMPTY_OBJECT)
          end
        end)

  local SpeakCId = self.mobileSession:SendRPC("Speak",
  {
    ttsChunks = { { text = "ttsChunks", type = "TEXT"} }
  })

  EXPECT_ANY()
  :ValidIf(function(_, data)
       if data.payload.success == true and
        data.payload.resultCode == "SUCCESS" then
        print (" \27[32m  Message with expected data came \27[0m")
        return true
      else
         print (" \27[36m Some wrong message came"..tostring(data.rpcFunctionId)..", expected 12 \27[0m ")
         return false
      end
    end)

end

--//////////////////////////////////////////////////////////////////////////////////--
-- 4.Check processing messages on TTS, UI interfaces
function Test:Case_AlertTest()
  local AlertRequestId
  EXPECT_HMICALL("UI.Alert", 
  {
    softButtons = 
    {
      {
        text = "Button",
        isHighlighted = false,
        softButtonID = 1122,
        systemAction = "DEFAULT_ACTION"
      }
    }
  })
  :Do(function(_,data)
        AlertRequestId = data.id
        SendOnSystemContext(self, "ALERT")
      end)

  local TTSSpeakRequestId
  EXPECT_HMICALL("TTS.Speak",
    {
      speakType = "ALERT",
      ttsChunks = { { text = "ttsChunks", type = "TEXT" } }
    })
    :Do(function(_, data)
          TTSSpeakRequestId = data.id
        end)

  EXPECT_NOTIFICATION("OnHMIStatus",
    { systemContext = "ALERT", hmiLevel = "FULL", audioStreamingState = "AUDIBLE"    },
    { systemContext = "ALERT", hmiLevel = "FULL", audioStreamingState = "ATTENUATED" },
    { systemContext = "ALERT", hmiLevel = "FULL", audioStreamingState = "AUDIBLE"    },
    { systemContext = "MAIN",  hmiLevel = "FULL", audioStreamingState = "AUDIBLE"    })
    :Times(4)
    :Do(function(exp, data)
          if exp.occurences == 1 then
            self.hmiConnection:SendNotification("TTS.Started",json.EMPTY_OBJECT)
          elseif exp.occurences == 2 then
            self.hmiConnection:SendResponse(TTSSpeakRequestId,"TTS.Speak", "SUCCESS", json.EMPTY_OBJECT)
            self.hmiConnection:SendNotification("TTS.Stopped",json.EMPTY_OBJECT)
          elseif exp.occurences == 3 then
            self.hmiConnection:SendResponse(AlertRequestId,"UI.Alert", "SUCCESS", json.EMPTY_OBJECT)
          end
        end)
  local cid = self.mobileSession:SendRPC("Alert",
  {
    ttsChunks = { { text = "ttsChunks", type = "TEXT"} },
    softButtons =
    {
      {
         type = "TEXT",
         text = "Button",
         isHighlighted = false,
         softButtonID = 1122,
         systemAction = "DEFAULT_ACTION"
      }
    }
  })
  EXPECT_RESPONSE(cid, { success = true, resultCode = "SUCCESS" })
    :Do(function()
          SendOnSystemContext(self, "MAIN")
        end)
end


--//////////////////////////////////////////////////////////////////////////////////--
-- 5.Check processing messages on VR, UI interfaces
function Test:Case_PerformInteractionTest()

	local CorIdPI = self.mobileSession:SendRPC("PerformInteraction",
  {
    initialText = "initialText",
    interactionMode = "BOTH",
    interactionChoiceSetIDList = {1},
  })

local VRPIid
  EXPECT_HMICALL("VR.PerformInteraction", 
  {
  })
  :Do(function(_,data)
  	VRPIid = data.id
    self.hmiConnection:SendNotification("VR.Started",json.EMPTY_OBJECT)
      end)

local UIPIid
  EXPECT_HMICALL("UI.PerformInteraction", 
  {
  })
  :Do(function(_,data)
  	UIPIid = data.id
      end)


	EXPECT_NOTIFICATION("OnHMIStatus",
      { systemContext = "MAIN", hmiLevel = "FULL", audioStreamingState = "NOT_AUDIBLE" },
      { systemContext = "MAIN", hmiLevel = "FULL", audioStreamingState = "AUDIBLE"},
	    { systemContext = "HMI_OBSCURED", hmiLevel = "FULL", audioStreamingState = "AUDIBLE"},
	    { systemContext = "MAIN", hmiLevel = "FULL", audioStreamingState = "AUDIBLE"    })
	    :Times(4)
	    :Do(function(exp, data)
	          if exp.occurences == 1 then
	            self.hmiConnection:SendError(VRPIid, "VR.PerformInteraction", "ABORTED", "VR.PerformInteraction is aborted by user")
	            self.hmiConnection:SendNotification("VR.Stopped",json.EMPTY_OBJECT)

	            SendOnSystemContext(self, "HMI_OBSCURED")
	          elseif exp.occurences == 3 then
	            self.hmiConnection:SendResponse(UIPIid,"UI.PerformInteraction", "SUCCESS", {choiceID = 1})
	          end
	        end)


  EXPECT_RESPONSE(CorIdPI, { success = true, resultCode = "SUCCESS", choiceID = 1, triggerSource = "MENU" })
  :Do(function(_,data)
  	SendOnSystemContext(self, "MAIN")
  		end)

end

--//////////////////////////////////////////////////////////////////////////////////--
-- 6.Check creation of the new session
function Test:Case_SecondSession()
  -- Connected expectation
  self.mobileSession1 = mobile_session.MobileSession(
    self,
    self.mobileConnection)
end

--//////////////////////////////////////////////////////////////////////////////////--
-- 7.Check starting RPC service and registration app througt second created session
function Test:Case_AppRegistrationInSecondSession()
    self.mobileSession1:StartService(7)
    :Do(function()
            local CorIdRegister = self.mobileSession1:SendRPC("RegisterAppInterface",
            {
              syncMsgVersion =
              {
                majorVersion = 3,
                minorVersion = 0
              },
              appName = "Test2 Application",
              isMediaApplication = true,
              languageDesired = 'EN-US',
              hmiDisplayLanguageDesired = 'EN-US',
              appHMIType = { "NAVIGATION" },
              appID = "8675309"
            })

            EXPECT_HMINOTIFICATION("BasicCommunication.OnAppRegistered", 
            {
              application = 
              {
                appName = "Test2 Application"
              }
            })
            :Do(function(_,data)
              self.applications["Test2 Application"] = data.params.application.appID
                end)

            self.mobileSession1:ExpectResponse(CorIdRegister, { success = true, resultCode = "SUCCESS" })
            :Timeout(2000)

            self.mobileSession1:ExpectNotification("OnHMIStatus", 
            { systemContext = "MAIN", hmiLevel = "NONE", audioStreamingState = "NOT_AUDIBLE"})
            :Timeout(2000)
            :Times(1)

            DelayedExp()
        end)
end

--//////////////////////////////////////////////////////////////////////////////////--
-- 8.Check receiving messages on mobile side according to mobile session
function Test:ActivateSecondApp()
  self.mobileSession1:ExpectNotification("OnHMIStatus",
    { systemContext = "MAIN", hmiLevel = "FULL", audioStreamingState = "AUDIBLE"}
    )

    self.mobileSession:ExpectNotification("OnHMIStatus", 
    { systemContext = "MAIN", hmiLevel = "BACKGROUND", audioStreamingState = "NOT_AUDIBLE"}
    )

    local RequestId = self.hmiConnection:SendRequest("SDL.ActivateApp", { appID = self.applications["Test2 Application"]})

    --hmi side: expect SDL.ActivateApp response
    EXPECT_HMIRESPONSE(RequestId)
      :Do(function(_,data)
        --In case when app is not allowed, it is needed to allow app
          if
              data.result.isSDLAllowed ~= true then

                --hmi side: sending SDL.GetUserFriendlyMessage request
                  local RequestId = self.hmiConnection:SendRequest("SDL.GetUserFriendlyMessage", 
                          {language = "EN-US", messageCodes = {"DataConsent"}})

                  --hmi side: expect SDL.GetUserFriendlyMessage response
                EXPECT_HMIRESPONSE(RequestId)
                      :Do(function(_,data)

                    --hmi side: send request SDL.OnAllowSDLFunctionality
                    self.hmiConnection:SendNotification("SDL.OnAllowSDLFunctionality", 
                      {allowed = true, source = "GUI", device = {id = config.deviceMAC, name = "127.0.0.1"}})

                    --hmi side: expect BasicCommunication.ActivateApp request
                      EXPECT_HMICALL("BasicCommunication.ActivateApp")
                        :Do(function(_,data)

                          --hmi side: sending BasicCommunication.ActivateApp response
                          self.hmiConnection:SendResponse(data.id,"BasicCommunication.ActivateApp", "SUCCESS", json.EMPTY_OBJECT)

                      end)
                      :Times(2)

                      end)

        end
          end)

end

-- --//////////////////////////////////////////////////////////////////////////////////--
-- -- 9.Check starting services of video, audio streamings
-- function Test:Case_OpenServices()
--     self.mobileSession1:StartService(11)
--     self.mobileSession1:StartService(10)
-- end

--//////////////////////////////////////////////////////////////////////////////////--
-- Precondition: activation of first app
function Test:WaitActivation()
  EXPECT_NOTIFICATION("OnHMIStatus",
    { systemContext = "MAIN", hmiLevel = "FULL", audioStreamingState = "AUDIBLE" })
  local RequestId = self.hmiConnection:SendRequest("SDL.ActivateApp", { appID = self.applications["Test Application"]})
  EXPECT_HMIRESPONSE(RequestId)
end

--//////////////////////////////////////////////////////////////////////////////////--
-- 10.Check RUN_AFTER function execution
function Test:Case_PerformAudioPassThruTest()
	local CorIdPAPT = self.mobileSession:SendRPC("PerformAudioPassThru",
  {
  	audioPassThruDisplayText1 = "audioPassThruDisplayText1",
  	samplingRate = "16KHZ",
  	maxDuration = 10000,
  	bitsPerSample = "16_BIT",
  	audioType = "PCM"
  })

	local UIPAPTid
  EXPECT_HMICALL("UI.PerformAudioPassThru", 
  {
  })
  :Do(function(_,data)
  	UIPAPTid = data.id
    local function to_be_run()
      self.hmiConnection:SendResponse(UIPAPTid,"UI.PerformAudioPassThru", "SUCCESS", json.EMPTY_OBJECT)
    end 
    RUN_AFTER(to_be_run,7000)
      end)

  EXPECT_NOTIFICATION("OnAudioPassThru")
	    -- :Times(AnyNumber())
      :Times(AtLeast(1))
	EXPECT_RESPONSE(CorIdPAPT, { success = true, resultCode = "SUCCESS" })
	:Timeout(15000)

end

--//////////////////////////////////////////////////////////////////////////////////--
-- Precondition: activation of second app
function Test:ActivateSecondApp()
  EXPECT_ANY_SESSION_NOTIFICATION("OnHMIStatus",
  { systemContext = "MAIN", hmiLevel = "FULL", audioStreamingState = "AUDIBLE"},
  { systemContext = "MAIN", hmiLevel = "BACKGROUND", audioStreamingState = "NOT_AUDIBLE"})
  :Times(2)

  local RequestId = self.hmiConnection:SendRequest("SDL.ActivateApp", { appID = self.applications["Test2 Application"]})

    --hmi side: expect SDL.ActivateApp response
    EXPECT_HMIRESPONSE(RequestId)
      :Do(function(_,data)
        --In case when app is not allowed, it is needed to allow app
          if
              data.result.isSDLAllowed ~= true then

                --hmi side: sending SDL.GetUserFriendlyMessage request
                  local RequestId = self.hmiConnection:SendRequest("SDL.GetUserFriendlyMessage", 
                          {language = "EN-US", messageCodes = {"DataConsent"}})

                  --hmi side: expect SDL.GetUserFriendlyMessage response
                EXPECT_HMIRESPONSE(RequestId)
                      :Do(function(_,data)

                    --hmi side: send request SDL.OnAllowSDLFunctionality
                    self.hmiConnection:SendNotification("SDL.OnAllowSDLFunctionality", 
                      {allowed = true, source = "GUI", device = {id = config.deviceMAC, name = "127.0.0.1"}})

                    --hmi side: expect BasicCommunication.ActivateApp request
                      EXPECT_HMICALL("BasicCommunication.ActivateApp")
                        :Do(function(_,data)

                          --hmi side: sending BasicCommunication.ActivateApp response
                          self.hmiConnection:SendResponse(data.id,"BasicCommunication.ActivateApp", "SUCCESS", json.EMPTY_OBJECT)

                      end)
                      :Times(2)

                      end)

        end
          end)
end

--//////////////////////////////////////////////////////////////////////////////////--
-- 11.Check sending empty request
function Test:Case_ListFilesTest()
  local CorIdList = self.mobileSession1:SendRPC("ListFiles",json.EMPTY_OBJECT)

  self.mobileSession1:ExpectResponse(CorIdList, { success = true, resultCode = "SUCCESS"})
  :Timeout(2000)

end

--//////////////////////////////////////////////////////////////////////////////////--
-- 12.Check processing messages on VehicleInfo interface
function Test:Case_SubscribeVehicleDataTest()
  local CorIdSubscribeVD= self.mobileSession1:SendRPC("SubscribeVehicleData",
  {
    gps = true,
    speed = true
  })

  EXPECT_HMICALL("VehicleInfo.SubscribeVehicleData", 
  {
    gps = true,
    speed = true
  })
  :Do(function(_,data)
    self.hmiConnection:SendResponse(data.id, "VehicleInfo.SubscribeVehicleData", "SUCCESS", {gps = {dataType = "VEHICLEDATA_GPS", resultCode = "SUCCESS"},speed = {dataType = "VEHICLEDATA_SPEED", resultCode = "SUCCESS"}})
      end)

  self.mobileSession1:ExpectResponse(CorIdSubscribeVD, { success = true, resultCode = "SUCCESS", gps = {dataType = "VEHICLEDATA_GPS", resultCode = "SUCCESS"}, speed = {dataType = "VEHICLEDATA_SPEED", resultCode = "SUCCESS"} })
  :Timeout(2000)
end

--//////////////////////////////////////////////////////////////////////////////////--
-- 13.Check processing messages on Navigation interface
function Test:Case_ShowConstantTBTTest()
  local CorIdShowConstantTBT = self.mobileSession1:SendRPC("ShowConstantTBT",
  {
    navigationText1 = "Navigation Text 1",
    navigationText2 = "Navigation Text 2"
  })

  EXPECT_HMICALL("Navigation.ShowConstantTBT", 
  {
    navigationTexts =
    {
      {fieldName = "navigationText1", fieldText = "Navigation Text 1"},
      {fieldName = "navigationText2", fieldText = "Navigation Text 2"}
    }
  })
  :Do(function(_,data)
    self.hmiConnection:SendResponse(data.id, "Navigation.ShowConstantTBT", "SUCCESS", json.EMPTY_OBJECT)
      end)

  self.mobileSession1:ExpectResponse(CorIdShowConstantTBT, { success = true, resultCode = "SUCCESS" })
  :Timeout(2000)
end

--//////////////////////////////////////////////////////////////////////////////////--
-- 14.Check processing RPC with bulk data
function Test:Case_PutFileTest()
  local CorIdPutFile = self.mobileSession1:SendRPC("PutFile",
  {
    syncFileName = "filename",
    fileType = "GRAPHIC_PNG"
  },"files/action.png")

  self.mobileSession1:ExpectResponse(CorIdPutFile, { success = true, resultCode = "SUCCESS", info = "File was downloaded" })
  :Timeout(2000)

end

--//////////////////////////////////////////////////////////////////////////////////--
-- 15. Check processing messages with float data
function Test:Case_GetVehicleDataTest()
  local CorIdSubscribeVD= self.mobileSession1:SendRPC("GetVehicleData",
  {
    gps = true,
    speed = true
  })

  EXPECT_HMICALL("VehicleInfo.GetVehicleData", 
  {
    gps = true,
    speed = true
  })
  :Do(function(_,data)
    self.hmiConnection:SendResponse(data.id, "VehicleInfo.GetVehicleData", "SUCCESS",{gps = {longitudeDegrees = 20.1, latitudeDegrees = -11.9, dimension = "2D"}, speed = 120.10})
      end)

  self.mobileSession1:ExpectResponse(CorIdSubscribeVD, { success = true, resultCode = "SUCCESS",gps = {longitudeDegrees = 20.1, latitudeDegrees = -11.9, dimension = "2D"}, speed = 120.1})
  :Timeout(5000)
end

--//////////////////////////////////////////////////////////////////////////////////--
-- 16. Check sending incorrect JSON from mobile side
function Test:Case_IncorrectJSONFromMobile()
  self.mobileSession1.correlationId = self.mobileSession1.correlationId + 1

  local msg = 
  {
    serviceType      = 7,
    frameInfo        = 0,
    rpcType          = 0,
    rpcFunctionId    = 12,
    rpcCorrelationId = self.mobileSession1.correlationId,
    -- rpcCorrelationId = 10000,
    payload          = "{A = json.EMPTY_OBJECT" 
  }
  self.mobileSession1:Send(msg)
  self.mobileSession1:ExpectResponse(self.mobileSession1.correlationId, { success = false, resultCode = "INVALID_DATA" })
  -- self.mobileSession1:ExpectResponse(10000, { success = false, resultCode = "INVALID_DATA" })
end


--//////////////////////////////////////////////////////////////////////////////////--
-- 18. Check stopping RPC service in second session
function Test:Case_EndService()
  self.mobileSession1:StopService(7)  
end

--//////////////////////////////////////////////////////////////////////////////////--
-- 19. Check processing RPC in first session after stopping RPC service in second session
function Test:Case_PutFileToFirstSesionAfterEndServiceTest()
  local CorIdPutFile = self.mobileSession:SendRPC("PutFile",
  {
    syncFileName = "filename",
    fileType = "GRAPHIC_PNG"
  },"files/action.png")

  self.mobileSession:ExpectResponse(CorIdPutFile, { success = true, resultCode = "SUCCESS" })
  :Timeout(2000)

end

--//////////////////////////////////////////////////////////////////////////////////--
-- 20. Check APPLICATION_NOT_REGISTERED resultCode in response to RPC in case when request
-- is sent to session with closed RPC service 
function Test:Case_PutFileToEndedServiceTest()
  local CorIdPutFile = self.mobileSession1:SendRPC("PutFile",
  {
    syncFileName = "filename",
    fileType = "GRAPHIC_PNG"
  },"files/action.png")

  self.mobileSession1:ExpectResponse(CorIdPutFile, { success = false, resultCode = "APPLICATION_NOT_REGISTERED" })
  :Timeout(2000)

end

--//////////////////////////////////////////////////////////////////////////////////--
-- 21. Check starting session after stopping RPC service
function Test:Case_StartSecondSessionAfterEndService()
  -- Connected expectation
  self.mobileSession1 = mobile_session.MobileSession(
    self,
    self.mobileConnection)
end

--//////////////////////////////////////////////////////////////////////////////////--
-- 22. Check start RPC service and registration app in created session after stopping RPC service
function Test:Case_AppRegistrationInSecondSessionAfterEndService()
    self.mobileSession1:StartService(7)

    :Do(function()
            local CorIdRegister = self.mobileSession1:SendRPC("RegisterAppInterface",
            {
              syncMsgVersion =
              {
                majorVersion = 3,
                minorVersion = 0
              },
              appName = "Test2 Application",
              isMediaApplication = true,
              languageDesired = 'EN-US',
              hmiDisplayLanguageDesired = 'EN-US',
              appHMIType = { "NAVIGATION" },
              appID = "8675309"
            })

            EXPECT_HMINOTIFICATION("BasicCommunication.OnAppRegistered", 
            {
              application = 
              {
                appName = "Test2 Application"
              }
            })
            :Do(function(_,data)
              self.applications["Test2 Application"] = data.params.application.appID
                end)

            self.mobileSession1:ExpectResponse(CorIdRegister, { success = true, resultCode = "SUCCESS" })
            :Timeout(2000)

            self.mobileSession1:ExpectNotification("OnHMIStatus",
            { systemContext = "MAIN", hmiLevel = "NONE", audioStreamingState = "NOT_AUDIBLE"})
        end)
end

--//////////////////////////////////////////////////////////////////////////////////--
-- Precondition: App activation
function Test:Case_ActivateSecondApp()
  self.mobileSession1:ExpectNotification("OnHMIStatus",
    { systemContext = "MAIN", hmiLevel = "FULL", audioStreamingState = "AUDIBLE"}
    )

  local RequestId = self.hmiConnection:SendRequest("SDL.ActivateApp", { appID = self.applications["Test2 Application"]})

  EXPECT_HMIRESPONSE(RequestId)


end

--//////////////////////////////////////////////////////////////////////////////////--
-- 23. Check creation on new connection
function Test:Case_SecondConnection()
  local tcpConnection = tcp.Connection(config.mobileHost, config.mobilePort)
  local fileConnection = file_connection.FileConnection("mobile2.out", tcpConnection)
  self.mobileConnection2 = mobile.MobileConnection(fileConnection)
  self.mobileSession2 = mobile_session.MobileSession(
  self,
  self.mobileConnection2,
  config.application2.registerAppInterfaceParams)
  event_dispatcher:AddConnection(self.mobileConnection2)
  self.mobileSession2:ExpectEvent(events.connectedEvent, "Connection started")
  self.mobileConnection2:Connect()
end

--//////////////////////////////////////////////////////////////////////////////////--
-- 24. Check start RPC service and app registration through second connection
function Test:Case_AppRegistrationInSecondConnection()
    self.mobileSession2:StartService(7)
    :Do(function()
            local CorIdRegister = self.mobileSession2:SendRPC("RegisterAppInterface",
            {
              syncMsgVersion =
              {
                majorVersion = 3,
                minorVersion = 0
              },
              appName = "Test3 Application",
              isMediaApplication = true,
              languageDesired = 'EN-US',
              hmiDisplayLanguageDesired = 'EN-US',
              appHMIType = { "NAVIGATION" },
              appID = "8675310"
            })

            EXPECT_HMINOTIFICATION("BasicCommunication.OnAppRegistered", 
            {
              application = 
              {
                appName = "Test3 Application"
              }
            })
            :Do(function(_,data)
              self.applications["Test3 Application"] = data.params.application.appID
                end)

            self.mobileSession2:ExpectResponse(CorIdRegister, { success = true, resultCode = "SUCCESS" })
            :Timeout(2000)

            self.mobileSession2:ExpectNotification("OnHMIStatus",
            { systemContext = "MAIN", hmiLevel = "NONE", audioStreamingState = "NOT_AUDIBLE"})

        end)
end

--//////////////////////////////////////////////////////////////////////////////////--
-- Precondition: App activation
function Test:Case_ActivateThirdApp()
  self.mobileSession2:ExpectNotification("OnHMIStatus",
    { systemContext = "MAIN", hmiLevel = "FULL", audioStreamingState = "AUDIBLE"}
    )

  local RequestId = self.hmiConnection:SendRequest("SDL.ActivateApp", { appID = self.applications["Test3 Application"]})
  EXPECT_HMIRESPONSE(RequestId)
end

--//////////////////////////////////////////////////////////////////////////////////--
-- 25. Check processing RPC in second connection
function Test:Case_ShowTest()
  local CorIdShow = self.mobileSession2:SendRPC("Show",
  {
    mainField1 = "Show main Field 1",
    mainField2 = "Show main Field 2",
    mainField3 = "Show main Field 3",
    mediaClock = "12:04"
  })

  EXPECT_HMICALL("UI.Show", 
  {
    showStrings = 
    {
      { fieldName = "mainField1",  fieldText = "Show main Field 1"},
      { fieldName = "mainField2",  fieldText = "Show main Field 2"},
      { fieldName = "mainField3",  fieldText = "Show main Field 3"},
      { fieldName = "mediaClock",  fieldText = "12:04"}
  },

  })
  :Do(function(_,data)
    self.hmiConnection:SendResponse(data.id,"UI.Show", "SUCCESS", json.EMPTY_OBJECT)
      end)

   self.mobileSession2:ExpectResponse(CorIdShow, { success = true, resultCode = "SUCCESS" })
  :Timeout(2000)

end

--//////////////////////////////////////////////////////////////////////////////////--
-- Precondition: stop waiting for OnHashChange notification using Unpin
function Test:OnHashChangeUnpin()
  EXPECT_NOTIFICATION("OnHashChange"):Unpin()
  :Do(function(_,data)
    if data.payload.hashID ~= nil then
      print ("OnHashChange notification came")
    else print ("OnHashChangePin function was called but OnHashChange occurrence is 0")
    end
  end)
  :Times(AnyNumber())

end

--//////////////////////////////////////////////////////////////////////////////////--
-- 26. Check ignoring of OnHashChange notification after Unpin function
function Test:Case_AddCommandTest()
  local CorAddCommand = self.mobileSession2:SendRPC("AddCommand",
  {
    cmdID = 11,
    menuParams = {menuName = "Command"}
  })

  EXPECT_HMICALL("UI.AddCommand", 
  {
    cmdID = 11,
    menuParams = {menuName = "Command"}
  })
  :Do(function(_,data)
    self.hmiConnection:SendResponse(data.id,"UI.AddCommand", "SUCCESS", json.EMPTY_OBJECT)
      end)

   self.mobileSession2:ExpectResponse(CorAddCommand, { success = true, resultCode = "SUCCESS" })
  :Timeout(2000)

end

--//////////////////////////////////////////////////////////////////////////////////--
local readDIDRequest = 6
local readDIDRejectedCount = 0
local readDIDSuccessCount = 0
function Test:Case_ReadDIDSeveralRespomses() 
  for i=1, readDIDRequest do
    --mobile side: sending ReadDID request
    local cid = self.mobileSession:SendRPC("ReadDID",{                                      
                              ecuName = 2000,                                   
                              didLocation = 
                              { 
                                25535,
                              }
                            })
  end
  
  --hmi side: expect ReadDID request
  EXPECT_HMICALL("VehicleInfo.ReadDID",{                                      
                      ecuName = 2000,                                   
                      didLocation = 
                      { 
                        25535,
                      }
                    })
  :Do(function(_,data)
    --hmi side: sending VehicleInfo.ReadDID response
    self.hmiConnection:SendResponse(data.id, data.method,"SUCCESS", {
                                      didResult = {{        
                                        resultCode = "SUCCESS", 
                                        didLocation = 25535,
                                        data = "123"
                                      }}
                                    })
  end)
  :Times(5)
  

  EXPECT_RESPONSE("ReadDID",
    {resultCode = "REJECTED"},
    {resultCode = "SUCCESS"},
    {resultCode = "SUCCESS"},
    {resultCode = "SUCCESS"},{resultCode = "SUCCESS"},{resultCode = "SUCCESS"})
  :Times(6)
  
  DelayedExp(1000)
end

--//////////////////////////////////////////////////////////////////////////////////--
-- 27. Check closing HMI and mobile connection 
function Test:CloseConnections()

  self.mobileConnection2:Close()

  DelayedExp()

end

--//////////////////////////////////////////////////////////////////////////////////--
-- 28. Check opening hmi connection
function Test:ConnectHMI()

 self.hmiConnection:Connect()
 DelayedExp()

end
