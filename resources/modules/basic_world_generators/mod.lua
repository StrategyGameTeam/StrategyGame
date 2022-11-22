defaultSeed = 2137

local dot_product = {
  [0x0] = function(x, y, z) return x + y end,
  [0x1] = function(x, y, z) return -x + y end,
  [0x2] = function(x, y, z) return x - y end,
  [0x3] = function(x, y, z) return -x - y end,
  [0x4] = function(x, y, z) return x + z end,
  [0x5] = function(x, y, z) return -x + z end,
  [0x6] = function(x, y, z) return x - z end,
  [0x7] = function(x, y, z) return -x - z end,
  [0x8] = function(x, y, z) return y + z end,
  [0x9] = function(x, y, z) return -y + z end,
  [0xA] = function(x, y, z) return y - z end,
  [0xB] = function(x, y, z) return -y - z end,
  [0xC] = function(x, y, z) return y + x end,
  [0xD] = function(x, y, z) return -y + z end,
  [0xE] = function(x, y, z) return y - x end,
  [0xF] = function(x, y, z) return -y - z end
}

local grad = function(hash, x, y, z)
  return dot_product[(hash % 0x10)](x, y, z)
end

local fade = function(t)
  return t * t * t * (t * (t * 6 - 15) + 10)
end

local lerp = function(t, a, b)
  return a + t * (b - a)
end

local generatePermutation = function(seed)
  math.randomseed(seed)

  local permutation = { 0 }

  for i = 1, 255 do
    table.insert(permutation, math.random(1, #permutation + 1), i)
  end

  local p = {}

  for i = 0, 255 do
    p[i] = permutation[i + 1]
    p[i + 256] = permutation[i + 1]
  end

  return p
end

perlin = {}
perlin.__index = perlin

perlin.noise = function(self, x, y, z)
  y = y or 0
  z = z or 0

  -- log(string.format("X: %d, Y: %d, Z: %d", x, y, z))

  local xi = math.floor(x) % 256
  local yi = math.floor(y) % 256
  local zi = math.floor(z) % 256

  x = x - math.floor(x)
  y = y - math.floor(y)
  z = z - math.floor(z)

  local u = fade(x)
  local v = fade(y)
  local w = fade(z)

  local A, AA, AB, AAA, ABA, AAB, ABB, B, BA, BB, BAA, BBA, BAB, BBB
  A   = self.p[xi] + yi
  AA  = self.p[A] + zi
  AB  = self.p[A + 1] + zi
  AAA = self.p[AA]
  ABA = self.p[AB]
  AAB = self.p[AA + 1]
  ABB = self.p[AB + 1]

  B   = self.p[xi + 1] + yi
  BA  = self.p[B] + zi
  BB  = self.p[B + 1] + zi
  BAA = self.p[BA]
  BBA = self.p[BB]
  BAB = self.p[BA + 1]
  BBB = self.p[BB + 1]

  return lerp(w,
    lerp(v,
      lerp(u,
        grad(AAA, x, y, z),
        grad(BAA, x - 1, y, z)
      ),
      lerp(u,
        grad(ABA, x, y - 1, z),
        grad(BBA, x - 1, y - 1, z)
      )
    ),
    lerp(v,
      lerp(u,
        grad(AAB, x, y, z - 1), grad(BAB, x - 1, y, z - 1)
      ),
      lerp(u,
        grad(ABB, x, y - 1, z - 1), grad(BBB, x - 1, y - 1, z - 1)
      )
    )
  )
end

setmetatable(perlin, {
  __call = function(self, seed)
    seed = seed or defaultSeed

    return setmetatable({
      seed = seed,
      p = generatePermutation(seed),
    }, self)
  end
})


Tau = 2 * math.pi

function CalcOctave(frequency, x, y, width, height, p1)
  x = (x / width) * Tau * frequency
  return p1:noise(math.cos(x) / Tau, math.sin(x) / Tau, (y / height) * frequency)
end

return {
  name = "basic_world_generators",
  declarations = {
    world_generators = {
      {
        name = "default",
        generator = function(Map, Options)
          math.randomseed(os.time())
          
          local width = 128
          local height = 128

          Map.setSize(width, height)
          Map.setTileCoords(Map.OFFSET)

          local grass = Defs.getHex("Grass")
          local stone = Defs.getHex("Stone")
          local water = Defs.getHex("Water")
          local sand = Defs.getHex("Sand")
          local sandrocks = Defs.getHex("SandRocks")
          local dirt = Defs.getHex("Dirt")
          local mountain = Defs.getHex("Mountain")

          local biomeHeight = height / 7
          local frequency = 1
          local octave = 8
          local amplitude = 128
          local maxvalue = 0
          local persistance = 0.5
          local total = 0
          local e = 0
          local p1 = perlin(13)
          mt = {} -- create the matrix
          for i = 1, width do
            mt[i] = {} -- create a new row
            for j = 1, height do
              for o = 1, octave do
                e = CalcOctave(frequency, i, j, width, height, p1)
                e = e * amplitude
                total = total + e
                maxvalue = maxvalue + amplitude
                amplitude = amplitude * persistance
                frequency = frequency * 2
              end
              mt[i][j] = total / maxvalue
              frequency = 1
              total = 0
              maxvalue = 0
              amplitude = 128

              if mt[i][j] > 0.15 then
                Map.setTileAt(i, j, mountain);
              elseif mt[i][j] > -0.05 then
                if j < biomeHeight or j > 6 * biomeHeight then
                  Map.setTileAt(i, j, stone)
                elseif (j < biomeHeight * 3 - math.sin(i / 35 + math.cos(i / 15))*7 and j > biomeHeight * 2 + math.sin(i / 40 + math.cos(i / 20))*10) or (j < biomeHeight * 5 - math.sin(i / 50 + math.cos(i / 30))*5 and j > biomeHeight * 4 + math.sin(i / 30 + math.cos(i / 80))*12) then
                  Map.setTileAt(i, j, sand)
                else
                  Map.setTileAt(i, j, grass)
                end
              else
                Map.setTileAt(i, j, water);
              end
            end
          end
        end
      }
    }
  }
}
