-- bot.lua — простой «бот» для тестов: выбирает направление к еде,
-- избегая стены и своего тела на следующем шаге.
-- bot() возвращает номер клавиши (как KEY_* в engine.h): 1..4 или 0.

local K = { up = 1, down = 2, left = 3, right = 4 }
local D = { up = { 0, -1 }, down = { 0, 1 }, left = { -1, 0 }, right = { 1, 0 } }

local function safe(x, y)
  if x < 0 or x >= W or y < 0 or y >= H then return false end
  for i = 1, #G.snake - 1 do
    if G.snake[i].x == x and G.snake[i].y == y then return false end
  end
  return true
end

function bot()
  local h, f = G.snake[1], G.food
  if not f then return 0 end
  local order = {}
  if f.x > h.x then order[#order + 1] = "right" elseif f.x < h.x then order[#order + 1] = "left" end
  if f.y > h.y then order[#order + 1] = "down" elseif f.y < h.y then order[#order + 1] = "up" end
  for _, d in ipairs({ G.dir, "up", "right", "down", "left" }) do order[#order + 1] = d end
  for _, d in ipairs(order) do
    if safe(h.x + D[d][1], h.y + D[d][2]) then return K[d] end
  end
  return 0
end
