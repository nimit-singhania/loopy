affine(Mult, { [i, k, j] -> [i, j, k] } )
affine(Mult, { [i, j, k] -> [i1, j1, k1, i2, j2, k2]: i1 = [i/32] and i2 = i%32 and j1 = [j/32] and j2 = j%32 and k1 = [k/32] and k2 = k%32 } )
