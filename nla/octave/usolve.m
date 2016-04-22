## usage: x = usolve (L, b)
##
##
function x = usolve (L, b)

  x = b;

  n = length(b);

  for i = n-1:-1:1
    x( i ) -=  L( i+1:end,  i)' * x( i+1:end);
  endfor

endfunction
