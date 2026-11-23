function main()
    v3 = Vec3(0)
    v3.xr # ERROR: mixing components from different swizzle sets
    v3.xs # ERROR: mixing components from different swizzle sets
    v3.rs # ERROR: mixing components from different swizzle sets
end
