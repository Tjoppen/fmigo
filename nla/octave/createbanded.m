##
##
##
## usage: bmatrix = createbanded (N, B, signs)
## Create a symmetric banded matrix, and keep it simple. 
##
## Make a diagonally dominant matrix with off diagonal values: 
## -1 -2 -3 ... -(B-1)  
## and diagonal value B^2
##
##
function [m , banded] = createbanded (n, b, signs, active)

  if( !exist("signs"))
    signs = ones(n, 1);
  endif

  if( !exist("active"))
    active = ones(n, 1);
  endif

  d = 2 * b^2 * ones( n, 1 );
  s = -[1:b-1] .* ones( n, 1 );
  s1 = -[b-1:-1:1] .* ones( n, 1 );
  m = spdiags( [ s, d, s1], [-(b-1):(b-1)], sparse(n, n) ) *  spdiags( signs, 0, sparse(n, n) );
  
  banded = struct("matrix", [d, s1], "signs", signs, "active", active);
  
  
endfunction
