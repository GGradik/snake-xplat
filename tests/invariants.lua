-- invariants.lua — свойства (инварианты), которые должны выполняться
-- после КАЖДОГО такта игры, при любых нажатиях клавиш.
-- Загружается тестами в то же Lua-состояние, что и snake.lua.
--
-- check_invariants() возвращает true или false; при false
-- в глобальной переменной INV_ERR лежит описание нарушения.

local prev = nil -- состояние на предыдущем такте

local function fail(msg)
  INV_ERR = msg
  return false
end

local function copy_state()
  return {
    state = G.state, score = G.score, best = G.best, len = #G.snake,
    hx = G.snake[1].x, hy = G.snake[1].y,
  }
end

function check_invariants()
  local valid = { title = true, play = true, over = true, win = true }
  if not valid[G.state] then return fail("неизвестное состояние " .. tostring(G.state)) end

  -- 1. Длина змейки = 3 + количество съеденной еды
  if #G.snake ~= 3 + G.score then
    return fail(("длина %d, а очков %d"):format(#G.snake, G.score))
  end

  -- 2. Все клетки змейки внутри поля и не повторяются
  local seen = {}
  for i, s in ipairs(G.snake) do
    if s.x < 0 or s.x >= W or s.y < 0 or s.y >= H then
      return fail(("сегмент %d вне поля: (%d,%d)"):format(i, s.x, s.y))
    end
    local k = s.y * W + s.x
    if seen[k] then return fail(("змейка пересекает себя в (%d,%d)"):format(s.x, s.y)) end
    seen[k] = true
  end

  -- 3. Соседние сегменты стоят вплотную (змейка не «рвётся»)
  for i = 2, #G.snake do
    local a, b = G.snake[i - 1], G.snake[i]
    if math.abs(a.x - b.x) + math.abs(a.y - b.y) ~= 1 then
      return fail(("сегменты %d и %d не соседние"):format(i - 1, i))
    end
  end

  -- 4. Еда есть (если не победа), лежит в поле и не на змейке
  if G.state ~= "win" then
    if not G.food then return fail("нет еды на поле") end
    local f = G.food
    if f.x < 0 or f.x >= W or f.y < 0 or f.y >= H then return fail("еда вне поля") end
    if seen[f.y * W + f.x] then return fail(("еда на змейке в (%d,%d)"):format(f.x, f.y)) end
  end

  -- 5. Скорость соответствует очкам: каждые 5 очков задержка на 1 меньше
  local want = math.max(1, 4 - G.score // 5)
  if G.delay ~= want then return fail(("задержка %d, ожидалась %d"):format(G.delay, want)) end

  -- 6. Рекорд не меньше счёта после проигрыша и никогда не уменьшается
  if G.state == "over" and G.best < G.score then return fail("рекорд меньше счёта") end

  if prev then
    if G.best < prev.best then return fail("рекорд уменьшился") end
    if prev.state == "play" and G.state == "play" then
      -- 7. За один такт голова сдвигается не больше чем на 1 клетку,
      --    а длина растёт не больше чем на 1
      local d = math.abs(G.snake[1].x - prev.hx) + math.abs(G.snake[1].y - prev.hy)
      if d > 1 then return fail("голова прыгнула на " .. d .. " клеток") end
      if #G.snake < prev.len or #G.snake > prev.len + 1 then
        return fail(("длина изменилась с %d на %d"):format(prev.len, #G.snake))
      end
      -- 8. Счёт в течение игры не уменьшается
      if G.score < prev.score then return fail("счёт уменьшился") end
    end
  end

  prev = copy_state()
  return true
end
