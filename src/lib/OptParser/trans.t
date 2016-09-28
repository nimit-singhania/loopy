L1 = merge(L1, L2); (L1, L2) = isplit(L1, "[i, j, k]: i < j"); L1 = affine(L1, "[i, j, k] -> [i + 1, k, j]")
