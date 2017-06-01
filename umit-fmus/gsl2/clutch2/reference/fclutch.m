%% usage: torque = clutchcurve (dphi, domega, damping)
%%
%%
function tc = fclutch (dphi, domega, damping)
  
  b = [ -0.087266462599716474; -0.052359877559829883; 0.0; 0.09599310885968812; 0.17453292519943295 ];
  c = [ -1000; -30; 0; 50; 3500 ];
  
  N = length( b );
  
  %%look up internal torque based on dphi
  %%    if too low (< -b( 0 )) then c( 0 )
  %%    if too high (> b( 4 )) then c( 4 )
  %%    else lerp between two values in c
  %%

  tc = c( 1 ); 

  if ( dphi <= b( 1 ) ) 
    tc = (dphi - b( 1 )) / ( b( 2 ) - b( 1 ) ) *  ( c( 2 ) - c( 1 ) ) + c( 1 );
  elseif ( dphi >= b( end ) ) 
    tc = ( dphi - b( end ) ) / ( b( end ) - b( end - 1 ) ) * ( c( end ) - c( end - 1 ) ) + c( end );
  else 
    for i = 1:N-1
      if ( dphi >= b( i ) && dphi <= b( i+1 ) ) 
	k = (dphi - b( i ) ) / (b( i+1 ) - b( i ));
	tc = (1-k) * c( i ) + k * c( i+1 );
	break;
       end
    end
    if (i >= N ) 
      %too high (shouldn't happen)
      tc = c( end );
    end
  end
  %%add damping. 
  tc = tc + damping * domega;

end
