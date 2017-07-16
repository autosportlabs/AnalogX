
function splitWord(value)
  return bit.band(value, 0xFF), bit.rshift(bit.band(value, 0xFF00),8)
end

function setLinearThreshold(id, segLen, threshold, red, green, blue, flash)
  local lowt
  local hight
  lowt, hight = splitWord(threshold)
  txCAN(0, 744389 + 15, 1, {id, segLen, lowt, hight, red, green, blue, flash})
  sleep(10)
end

function setBaseConfig(bright)
  txCAN(0, 744389 + 2, 1, {bright})
  sleep(10)    
end

function setAlert(id, r, g, b, f)
  txCAN(0, 744389 + 11, 1, {id, r, g, b, f})
  sleep(10)
end

function configLinearGraph(lowRange, highRange, renderStyle, linearStyle)
  local lowRangeLow
  local lowRangeHigh
  local highRangeLow
  local highRangeHigh
  lowRangeLow, lowRangeHigh = splitWord(lowRange)
  highRangeLow, highRangeHigh = splitWord(highRange)
  txCAN(0, 744389 + 14, 1, {renderStyle, linearStyle, lowRangeLow, lowRangeHigh, highRangeLow, highRangeHigh})
  sleep(10)
end   
    
function updateLinearGraph(value)
  low, high = splitWord(value)
  txCAN(0, 744389 + 16, 1, {low, high})
end

function shiftx_init()
  setBaseConfig(1)
  setAlert(0, 0, 255, 0, 10)
  setAlert(1, 0, 0, 255, 2)
  configLinearGraph(0, 10000, 0, 0)
  setLinearThreshold(0, 4, 1000, 0, 255, 0, 0)
  setLinearThreshold(1, 2, 3000, 255,50, 0, 0)
  setLinearThreshold(2, 1, 5000, 255,70, 0, 0)
  setLinearThreshold(3, 2, 8000, 255,127, 0, 0)
  setLinearThreshold(4, 4, 9000, 255, 255, 0, 10)
end

function check_can_msg()
  id, ext, data = rxCAN(0,0)
  if id == 744389 then
    shiftx_init()
    println('discovered shiftx, configuring')
  end
end

function onTick()
  local rpm = getTimerRpm(0)
  updateLinearGraph(rpm)
  check_can_msg()
end

setTickRate(30)
initCAN(0, 500000)   
shiftx_init()
