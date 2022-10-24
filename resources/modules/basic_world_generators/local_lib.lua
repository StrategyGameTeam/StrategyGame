function repeating(repeats, fun)
    for i = 1, repeats, 1 do
        fun()
    end
end


return repeating