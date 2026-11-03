function main()
    begin end
    
    begin
	x = 1
    end
    y = x # COMMENT: begin/end doesn't introduce a new scope

    z = begin y end # COMMENT: single-Expr begin/end can be "unrolled"
end
