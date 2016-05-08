## usage: M = rebuildband (bandmatrix)
##
##
function [M, B, active] = rebuildband (bandmatrix)
  m = bandmatrix.matrix;
  N = size(m, 1);
  bw = size(m, 2);
  B = [];
  for i = 1:bw-1
    B = [B, m(:, bw - i +1 ) ];
  endfor
  B = [B, m(:, 1)];
  for i = 2:bw
    z = zeros(i-1, 1);
    B = [ B, [ z; m( 1:end-i+1, i ) ] ];
  endfor
  M = spdiags(B, [ -(bw-1):(bw-1) ], sparse(N, N) );
  active = bandmatrix.active;
  M = M * spdiags( bandmatrix.signs, 0, sparse(N,N) );
	      
endfunction
