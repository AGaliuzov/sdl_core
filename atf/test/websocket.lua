ws = network.WebSocket()

dyn = qt.dynamic()

function dyn.onData(s, b)
  print("Server says: " .. s)
end

function dyn.onText(s, b)
  print("Server says: " .. s)
  ws:close()
  exit()
end

function dyn.connected()
  print("WebSocket connected!")
  ws:write("Arv")
end

res = qt.connect(ws, "binaryFrameReceived(QByteArray,bool)", dyn, "onData(QByteArray,bool)")
res = qt.connect(ws, "textFrameReceived(QString,bool)", dyn, "onText(QString,bool)")
res = qt.connect(ws, "connected()", dyn, "connected()")
ws:open("ws://localhost", 8766)

print(res)
