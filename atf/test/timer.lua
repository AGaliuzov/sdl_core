print("Hello")
t = timers.Timer()
d = qt.dynamic()
local i = 1
function d.run()
  print("Bye")
  i = i + 1
  if i > 5 then
    quit()
  end
end
qt.connect(t, "timeout()", d, "run()")
t:setInterval(1000)
t:start()
