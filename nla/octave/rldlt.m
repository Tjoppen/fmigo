## LDLT factorization, right looking, i.e., rank update
## usage: [L, D] = rldlt (A)
##
##
function [L, D] = rldlt (A)
  n = size( A, 1 );
  L = tril(A, -1);
  D = diag( A );

  for j = 1:n-1
    
    d = 1 / D(j);

    v = L( j+1:end, j);

    UPDATE = d * v * v'; 
    L(j+1:end, j+1:end ) -= tril( UPDATE, -1 ) ;

    L( j+1:end, j) *= d;

    D( j + 1 : end  ) -=  d * v .* v; 

    D( j ) = d;

  endfor

  D( end ) = 1 / D( end );

 # L = tril( L, -1 ); 
L = L + speye( n );

endfunction
