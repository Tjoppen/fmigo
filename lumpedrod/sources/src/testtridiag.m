n = 10; 
diag1 =  n *  ones( n, 1 ) + 2 * rand(n , 1); #(1:n)';   # ones(n, 1);
sub1= rand(n-1, 1); 
DD = spdiags( diag1, 0, sparse(n,n) );
L = spdiags( [ [sub1; 0 ] , ones(n,1) ], [-1,0], sparse(n,n));
M = L * DD * L' ; 
diag2  = full( diag(M,  0) ) ; 
sub2   = full( diag(M, -1) ) ; 
#x = ones(n, 1);  
x =  10 * ( 1 - 2 * rand(n, 1) );  
X = M *  x; 
[y, D, S]  = tridiag(diag2, sub2, X);
err = norm(x - y);


