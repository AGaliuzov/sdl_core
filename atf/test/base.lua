local Connections = {}

function EXPECT_CALL(connection, method, arguments) --{{{
  -- Test passes if method 'method' of table 'object' was called
  -- (generally used with proxy object Mobile and its methods)
  local ex = Expectation()
  ex.connection = connection
  if type(method) == "string" then
    ex.method = Function[method]
  else
    ex.method = method
  end
  ex.arguments = arguments
  ex.times = 0
  ex.cardinality = Exactly(1)
  ex.pass = true
  return ex
end --}}}
local function Match(connection, message, expectation)
end

function ProcessMobileMessage(connection, message)
  for i, ex in ipairs(ExpectationsList) do
    if Match(connection, message, ex) then
    end
  end
end

function ASSERT_THAT(condition)
  -- Fails test if 'condition' is false
end

local exp = {}
exp.__index = exp
function exp:Times(number)
  -- Test passes if method was called 'number' times.
  -- 'number' may be a number or a comparison object
  self.times = number
  return self
end
function Expectation()
  res = {}
  res.sequences = {}
  res.times = 1
  setmetatable(res, exp)
  return res
end
function exp:InSequence(seq)
  if self.sequences[seq] == nil then
    self.sequences[seq] = true
    seq:add(self)
  end
  return self
end
function exp:After(other)
  if self.sequences[seq] == nil then
    self.sequences[seq] = true
    seq:insertAfter(self, other)
  end
  return self
end
function exp:Before(other)
  if self.sequences[seq] == nil then
    self.sequences[seq] = true
    seq:insertBefore(self, other)
  end
  return self
end
local function Cardinality(lower, upper)
  local c = { }
  c.lower = lower
  c.upper = upper
  return c
end

function AnyNumber(num)
  return Cardinality(0, nil)
end

function AtLeast(num)
  if num <= 0 then
    error("AtLeast: number must be greater than 0")
  end
  return Cardinality(num, nil)
end

function AtMost(num)
  if num <= 0 then
    error("AtMost: number must be greater than 0")
  end
  return Cardinality(0, num)
end

function Between(a, b)
  if (a > b) then
    error("Between: `from' must be less than `to'")
  end
  return Cardinality(a, b)
end
function Exactly(num)
  return Cardinality(num, num)
end
