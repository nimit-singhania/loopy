affine(Loop, {[t, i, j] -> [t, i+t, j+i]})
affine(Loop, { [t, i, j] -> [t, i1, j1, i2, j2]: i1 = [i/32] and i2 = i%32 and j1 = [j/32] and j2 = j%32} )
//affine(EY, { [t, i, j] -> [t, i1, j1, i2, j2]: i1 = [i/32] and i2 = i%32 and j1 = [j/32] and j2 = j%32} )
//affine(HZ, { [t, i, j] -> [t, i1, j1, i2, j2]: i1 = [i/32] and i2 = i%32 and j1 = [j/32] and j2 = j%32} )
