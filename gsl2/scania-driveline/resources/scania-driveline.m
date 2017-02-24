function [w_inShaftDer,w_wheelDer,tq_clutch,v_vehicle,w_out,w_inShaft,tq_outTransmission,v_driveWheel,r_slip] = ...
      fcn(w_inShaftNeutral, w_wheel, w_inShaftOld, tq_retarder, tq_fricLoss, tq_env, gear_ratio, tq_clutchMax, ... 
      tq_losses, r_tire, m_vehicle, final_gear_ratio, w_eng, tq_eng, J_eng, J_neutral, tq_brake, ts, r_slipFilt )
  
w_inShaft = w_inShaftOld;

tq_retWheel = tq_retarder * final_gear_ratio;

% This is the sum of external forces acting on the vehicle:
% wind + roll + m*g*sin(slope) + brakes, translated to torque at wheel shaft 
% and added friction loss in final gear
% in other words the torque required at prop shaft to maintain current vehicle speed
tq_loadWheelShaft = tq_brake + tq_env + tq_retWheel + tq_fricLoss;

% external load translated to prop shaft torque
tq_loadPropShaft = tq_loadWheelShaft / final_gear_ratio;

% the external load is translated to a torque at the input shaft and
% the mass of the vehicle is translated to an equivalent rotational inertia 
% at transmission input shaft

if gear_ratio ~= 0
    tq_loadAtInShaft = tq_loadPropShaft / gear_ratio;
    J_atInShaft = m_vehicle * ( r_tire / (final_gear_ratio*gear_ratio) )^2; 
else
    % when in neutral the transmission input shaft is disconnected and the
    % speed is then integrated and the shaft inertia is set to J_neutral
    % w_inShaftNeutral is the integration result during  outside
    w_inShaft = w_inShaftNeutral;
    tq_loadAtInShaft = 0;
    J_atInShaft = J_neutral;
end

% Clutch balance speed
% if simplifying the engine and the vehicle as two spinning flywheels attached
% to each plate of the clutch and then closing the clutch, the resulting 
% rotational speed of the clutch w_bal would be the weighted average 
w_bal = (J_eng*w_eng + J_atInShaft*w_inShaft )/(J_eng+J_atInShaft);

% calculate the torque required to accelerate the engine to w_bal in two
% timesteps
tq_bal =  (w_eng-w_bal) * J_eng / (2*ts);


tq_loadBal = (tq_eng * J_atInShaft + tq_loadAtInShaft * J_eng) ...
                      / (J_eng + J_atInShaft);

tq_clutchUnLim = tq_bal + tq_loadBal;

tq_clutch = min(max(tq_clutchUnLim,-tq_clutchMax),tq_clutchMax);

% transmission losses are given as input shaft torque loss
tq_inTransmission = (tq_clutch - tq_losses);

tq_outTransmission = tq_inTransmission * gear_ratio;


tq_sumWheel = tq_outTransmission * final_gear_ratio - tq_loadWheelShaft;

% w_wheel is integrate outside
w_wheelDer = tq_sumWheel / ( m_vehicle * r_tire^2 );

v_vehicle = w_wheel * r_tire;

% slip estimation (r_slipFilt filtered outside this m-function)
r_slip = (tq_env + tq_sumWheel) / ( m_vehicle * 8 );

v_driveWheel = (r_slipFilt + 1) * v_vehicle;

w_out = v_driveWheel * final_gear_ratio / r_tire;

if gear_ratio == 0
    % when gear is in neutral the input shaft speed is integrated using the
    % torque coming from the clutch
    w_inShaftDer = tq_inTransmission / J_neutral; 
else
    % When a gear is engaged the transmission input shaft speed is calculated
    % from the output shaft speed scaled with gear ratio, (the result from
    % the inputshaft neutral integration is ignored)
    w_inShaft = w_out * gear_ratio;
    
    % when not in neutral, set the inputShaft derivative so that the
    % integrator follows the acutal speed aproximately
    w_inShaftDer = 0.5*(w_inShaft-w_inShaftNeutral)/ts;
end
