## usage: x = lsolve (L, b)
##
##
function x = lsolve (L, b)
  n = length(b);
  x = b;

  for i = 1:n-1
    x( i+1 : end ) -= x( i ) * L( i+1: end, i ); 
  endfor

endfunction
