q = require("qttest")

local dyn = qt.dynamic()
qt.connect(obj, "SignalWithoutArgs()", dyn, "TestSlot()")
function dyn.TestSlot()
  print("TestSlot")
end

function test()
  local obj1 = q.Object1()
  connect_object(obj1)
end

test()

--connect_object()
--connect_object()
collectgarbage()
--obj1:raiseSignalWithoutArgs()
