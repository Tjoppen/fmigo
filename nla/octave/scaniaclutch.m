##
## usage: tc = scaniaclutch ( dphi, domega, clutch_damping )
##
function tc = scaniaclutch ( dphi, domega, clutch_damping )

  
				#Scania's clutch curve
  b = [ -0.087266462599716474, -0.052359877559829883, 0.0, 0.09599310885968812, 0.17453292519943295 ];
  c = [ -1000, -30, 0, 50, 3500 ];
  N = length( c );
  
  ##
  ##    if too low (< -b[ 0 ]) then c[ 0 ]
  ##    if too high (> b[ 4 ]) then c[ 4 ]
  ##    else lerp between two values in c

  tc = c( 1 ); #clutch torque
  
  if (dphi <= b( 1 )) 
    tc = 970.0 * (dphi - b( 1 ) ) / 0.034906585039886591  + c( 1 );
  elseif ( dphi >= b( end ) ) 
    tc = 3450.0 * ( dphi - b( end ) ) / 0.078539816339744828   + c( end );
  else 
    
    for i = 1:length(b)-1
      if (dphi >= b( i ) && dphi <= b( i+1 )) 
	k = (dphi - b( i )) / (b( i+1 ) - b( i ));
	tc = (1-k) * c( i ) + k * c( i+1 );
	break;
      endif
    endfor
    if (i >= length( b )  ) 
				#too high (shouldn't happen)
      tc = c( end );
    endif
  endif
  ## add damping. 
  tc += clutch_damping * domega;
  
endfunction
