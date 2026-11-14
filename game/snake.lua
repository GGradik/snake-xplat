-- snake.lua — вся игровая логика «Змейки».
--
-- Этот файл одинаков для всех платформ. Ядро (C) вызывает:
--   init()       — при запуске;
--   update(key)  — 20 раз в секунду, key = "up"/"down"/"left"/"right"/"enter"/nil;
--   draw()       — после каждого update().
-- Для рисования ядро даёт функции cls(), text() и fill().
--
-- Экран 80x25 символов. Поле 30x20 клеток, каждая клетка — 2 символа
-- в ширину, чтобы клетки выглядели квадратными.

-- Цвета (как в CGA/EGA)
local BLACK, GREEN, RED, LGRAY = 0, 2, 4, 7
local DGRAY, LGREEN, YELLOW, WHITE = 8, 10, 14, 15

W, H = 30, 20               -- размер поля в клетках
local FIELD_X, FIELD_Y = 2, 2 -- где поле начинается на экране
local START_DELAY = 4       -- тактов на один шаг змейки в начале (0.2 с)

-- Направления движения
local DIRS = {
  up = { x = 0, y = -1 }, down = { x = 0, y = 1 },
  left = { x = -1, y = 0 }, right = { x = 1, y = 0 },
}
local OPPOSITE = { up = "down", down = "up", left = "right", right = "left" }

-- Всё состояние игры в одной глобальной таблице G — так его удобно
-- проверять в тестах.
G = { best = 0 }

-- Положить еду в случайную свободную клетку.
function place_food()
  local busy = {}
  for _, s in ipairs(G.snake) do busy[s.y * W + s.x] = true end
  local free = {}
  for y = 0, H - 1 do
    for x = 0, W - 1 do
      if not busy[y * W + x] then free[#free + 1] = { x = x, y = y } end
    end
  end
  if #free == 0 then -- змейка заняла всё поле — победа
    G.food = nil
    G.state = "win"
    return
  end
  G.food = free[math.random(#free)]
end

function new_game()
  -- Змейка из 3 клеток в центре поля, голова — первый элемент.
  local cx, cy = W // 2, H // 2
  G.snake = { { x = cx, y = cy }, { x = cx - 1, y = cy }, { x = cx - 2, y = cy } }
  G.dir, G.next_dir = "right", "right"
  G.score = 0
  G.timer = 0
  G.delay = START_DELAY
  G.state = "play"
  place_food()
end

function game_over()
  G.state = "over"
  if G.score > G.best then G.best = G.score end
end

-- Один шаг змейки на одну клетку.
function step()
  G.dir = G.next_dir
  local d = DIRS[G.dir]
  local head = G.snake[1]
  local nx, ny = head.x + d.x, head.y + d.y

  -- Удар о стену
  if nx < 0 or nx >= W or ny < 0 or ny >= H then return game_over() end

  local eating = G.food ~= nil and nx == G.food.x and ny == G.food.y

  -- Удар о себя. Если не едим, хвост в этот шаг уйдёт, поэтому
  -- в последнюю клетку хвоста заходить можно.
  local last = eating and #G.snake or #G.snake - 1
  for i = 1, last do
    if G.snake[i].x == nx and G.snake[i].y == ny then return game_over() end
  end

  table.insert(G.snake, 1, { x = nx, y = ny }) -- новая голова
  if eating then
    G.score = G.score + 1
    -- каждые 5 очков змейка ускоряется
    if G.score % 5 == 0 and G.delay > 1 then G.delay = G.delay - 1 end
    place_food()
  else
    table.remove(G.snake) -- хвост уходит
  end
end

-- ---------- Функции, которые вызывает ядро ----------

function init()
  new_game()
  G.state = "title"
end

function update(key)
  if G.state ~= "play" then
    -- Заставка, проигрыш или победа: Enter начинает новую игру.
    if key == "enter" then new_game() end
    return
  end

  -- Поворот запоминается и применяется на следующем шаге.
  -- Развернуться на 180 градусов нельзя.
  if DIRS[key] and key ~= OPPOSITE[G.dir] then G.next_dir = key end

  G.timer = G.timer + 1
  if G.timer >= G.delay then
    G.timer = 0
    step()
  end
end

-- Нарисовать клетку поля (2 символа в ширину).
local function cell(x, y, str, fg, bg)
  text(FIELD_X + x * 2, FIELD_Y + y, str, fg, bg)
end

local function center(y, str, fg, bg)
  local x = FIELD_X + W - #str // 2
  text(x, y, str, fg, bg)
end

function draw()
  cls(BLACK)

  -- Рамка вокруг поля
  fill(FIELD_X - 2, FIELD_Y - 1, W * 2 + 4, H + 2, DGRAY, DGRAY)
  fill(FIELD_X, FIELD_Y, W * 2, H, LGRAY, BLACK)

  -- Еда и змейка
  if G.food then cell(G.food.x, G.food.y, "<>", RED, BLACK) end
  for i = #G.snake, 1, -1 do
    local s = G.snake[i]
    if i == 1 then
      cell(s.x, s.y, "  ", BLACK, LGREEN)  -- голова
    else
      cell(s.x, s.y, "  ", BLACK, GREEN)   -- тело
    end
  end

  -- Боковая панель
  local px = FIELD_X + W * 2 + 4
  text(px, 2, "S N A K E", YELLOW)
  text(px, 4, "Score: " .. G.score, WHITE)
  text(px, 5, "Best:  " .. G.best, WHITE)
  text(px, 6, "Speed: " .. (START_DELAY - G.delay + 1), WHITE)
  text(px, 9, "Arrows/WASD", LGRAY)
  text(px, 10, "  - move", LGRAY)
  text(px, 11, "Enter - start", LGRAY)
  text(px, 12, "Esc/Q - quit", LGRAY)

  -- Сообщения поверх поля
  if G.state == "title" then
    center(10, " SNAKE (Blockade, 1976) ", YELLOW, BLACK)
    center(12, " Press ENTER to start ", WHITE, BLACK)
  elseif G.state == "over" then
    center(10, " GAME OVER ", WHITE, RED)
    center(12, " Press ENTER to play again ", WHITE, BLACK)
  elseif G.state == "win" then
    center(10, " YOU WIN! ", BLACK, YELLOW)
    center(12, " Press ENTER to play again ", WHITE, BLACK)
  end
end
