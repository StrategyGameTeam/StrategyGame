
local dot_product = {
    [0x0]=function(x,y,z) return  x + y end,
    [0x1]=function(x,y,z) return -x + y end,
    [0x2]=function(x,y,z) return  x - y end,
    [0x3]=function(x,y,z) return -x - y end,
    [0x4]=function(x,y,z) return  x + z end,
    [0x5]=function(x,y,z) return -x + z end,
    [0x6]=function(x,y,z) return  x - z end,
    [0x7]=function(x,y,z) return -x - z end,
    [0x8]=function(x,y,z) return  y + z end,
    [0x9]=function(x,y,z) return -y + z end,
    [0xA]=function(x,y,z) return  y - z end,
    [0xB]=function(x,y,z) return -y - z end,
    [0xC]=function(x,y,z) return  y + x end,
    [0xD]=function(x,y,z) return -y + z end,
    [0xE]=function(x,y,z) return  y - x end,
    [0xF]=function(x,y,z) return -y - z end
  }
  
  local grad = function(hash, x,y,z)
    return dot_product[(hash % 0x10)](x,y,z)
  end
  
  local fade = function(t)
    return t * t * t * (t * (t * 6 - 15) + 10)
  end
  
  local lerp = function(t,a,b)
    return a + t * (b - a)
  end
  
  local generatePermutation = function(seed)
    math.randomseed(seed)
  
    local permutation = {0}
  
    for i=1,255 do
      table.insert(permutation,math.random(1,#permutation+1),i)
    end
  
    local p = {}
  
    for i=0,255 do
      p[i] = permutation[i+1]
      p[i+256] = permutation[i+1]
    end
  
    return p
  end
  
  perlin = {}
  perlin.__index = perlin
  
  perlin.noise = function(self,x,y,z)
    y = y or 0
    z = z or 0
  
    local xi = math.floor(x) % 0x100
    local yi = math.floor(y) % 0x100
    local zi = math.floor(z) % 0x100
  
    x = x - math.floor(x)
    y = y - math.floor(y)
    z = z - math.floor(z)
  
    local u = fade(x)
    local v = fade(y)
    local w = fade(z)
  
    local A, AA, AB, AAA, ABA, AAB, ABB, B, BA, BB, BAA, BBA, BAB, BBB
    A   = self.p[xi  ] + yi
    AA  = self.p[A   ] + zi
    AB  = self.p[A+1 ] + zi
    AAA = self.p[ AA ]
    ABA = self.p[ AB ]
    AAB = self.p[ AA+1 ]
    ABB = self.p[ AB+1 ]
  
    B   = self.p[xi+1] + yi
    BA  = self.p[B   ] + zi
    BB  = self.p[B+1 ] + zi
    BAA = self.p[ BA ]
    BBA = self.p[ BB ]
    BAB = self.p[ BA+1 ]
    BBB = self.p[ BB+1 ]
  
    return lerp(w,
      lerp(v,
        lerp(u,
          grad(AAA,x,y,z),
          grad(BAA,x-1,y,z)
        ),
        lerp(u,
          grad(ABA,x,y-1,z),
          grad(BBA,x-1,y-1,z)
        )
      ),
      lerp(v,
        lerp(u,
          grad(AAB,x,y,z-1), grad(BAB,x-1,y,z-1)
        ),
        lerp(u,
          grad(ABB,x,y-1,z-1), grad(BBB,x-1,y-1,z-1)
        )
      )
    )
  end
  
  setmetatable(perlin,{
    __call = function(self,seed)
      seed = seed or defaultSeed
  
      return setmetatable({
        seed = seed,
        p = generatePermutation(seed),
      },self)
    end
  })
  function returnMap(seed,width,height)
  log("elo")
  local TAU = 2*3.14159265358979323846
  local nx = 0
   local p1 = perlin(1234)
  local height=100
  local width = 100
     mt = {}          -- create the matrix
      for i=1,width do
        mt[i] = {}     -- create a new row
        for j=1,height do
        nx= TAU * i
          mt[i][j] = (p1:noise(i/width ,j/height ))
          --mt[i][j] = (p1:noise(math.cos(nx)/TAU,j/100 - 0.5))
          if mt[i][j] > 0.1 then mt[i][j] = 2 else mt[i][j] = 4 end
        end
      end
  
      return mt
  end
  
  DEV_MapGenerator(returnMap)
  
  
  
   --local p2 = perlin(1235)
  
    --for i=1,100 do
        --for j=1,100 do
       -- nx= TAU * i
        --    if mt[i][j] == 1 then
        --    mt[i][j] = --(p2:noise(math.cos(nx)/TAU,j/100 - 0.5))
        --    (p2:noise(i/100 - 0.5,j/100 - 0.5))
        --    end
    --end
  --end
  
  