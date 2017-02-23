#define SQ(x)  ( x ) * ( x ) 
#define min(x,y)  ( ( x ) < ( y ) )?  x : y
#define max(x,y)  ( ( x ) > ( y ) )?  x : y


typedef struct ins {
  double w_inShaftNeutral;
  double w_wheel; 
  double w_inShaftOld;
  double tq_retarder; 
  double tq_fricLoss; 
  double tq_env; 
  double gear_ratio; 
  double tq_clutchMax; 
  double tq_losses; 
  double r_tire; 
  double m_vehicle; 
  double final_gear_ratio; 
  double w_eng; 
  double tq_eng; 
  double J_eng; 
  double J_neutral; 
  double tq_brake; 
  double ts; 
  double r_slipFilt;
} ins;

typedef struct outs {
  double  w_inShaftDer;
  double  w_wheelDer;
  double  tq_clutch;
  double  v_vehicle;
  double  w_out;
  double  w_inShaft;
  double  tq_outTransmission;
  double  v_driveWheel;
  double  r_slip;
} outs;

typedef struct everything{
  ins inputs;
  outs outputs;
} everything;

int  fcn( double t, const double * x, double *dxdt, void * params){
  
  /// copy the struct: we don't write to this
  ins   inputs  =   ( ( everything * ) params)->inputs;
  /// pointer access needed for outputs
  outs *outputs = & ( ( everything * ) params)->outputs;

  /// real state is in x
  /// This makes only temporary changes.
  /// TODO: should we remove these variables from the input struct? 
  inputs.w_inShaftNeutral = x[0];
  inputs.w_wheel          = x[1]; 
  
  outputs->w_inShaft = inputs.w_inShaftOld;

  double tq_retWheel = inputs.tq_retarder * inputs.final_gear_ratio;

// This is the sum of external forces acting on the vehicle:
// wind + roll + m*g*sin(slope) + brakes, translated to torque at wheel shaft 
// and added friction loss in final gear
// in other words the torque required at prop shaft to maintain current vehicle speed

  double tq_loadWheelShaft = inputs.tq_brake + inputs.tq_env + tq_retWheel + inputs.tq_fricLoss;

// external load translated to prop shaft torque
  double tq_loadPropShaft = tq_loadWheelShaft / inputs.final_gear_ratio;

// the external load is translated to a torque at the input shaft and
// the mass of the vehicle is translated to an equivalent rotational inertia 
// at transmission input shaft

  double J_atInShaft;
  double tq_loadAtInShaft;

  if ( inputs.gear_ratio != 0 ){
    tq_loadAtInShaft = tq_loadPropShaft / inputs.gear_ratio;

    J_atInShaft = inputs.m_vehicle * SQ ( ( inputs.r_tire / (inputs.final_gear_ratio*inputs.gear_ratio) ) ); 
  } else {
    // when in neutral the transmission input shaft is disconnected and the
    // speed is then integrated and the shaft inertia is set to J_neutral
    // w_inShaftNeutral is the integration result during  outside
    outputs->w_inShaft = inputs.w_inShaftNeutral;
    tq_loadAtInShaft = 0;
    J_atInShaft = inputs.J_neutral;
  }

// Clutch balance speed
// if simplifying the engine and the vehicle as two spinning flywheels attached
// to each plate of the clutch and then closing the clutch, the resulting 
// rotational speed of the clutch w_bal would be the weighted average 
  double w_bal = (inputs.J_eng*inputs.w_eng + J_atInShaft*outputs->w_inShaft )/(inputs.J_eng+J_atInShaft);

// calculate the torque required to accelerate the engine to w_bal in two
// timesteps


  double tq_loadBal = (inputs.tq_eng * J_atInShaft + tq_loadAtInShaft * inputs.J_eng) / (inputs.J_eng + J_atInShaft);

  double tq_bal =  (inputs.w_eng-w_bal) * inputs.J_eng / (2*inputs.ts);
  double tq_clutchUnLim = tq_bal + tq_loadBal;

  outputs->tq_clutch = min(max(tq_clutchUnLim,-inputs.tq_clutchMax),inputs.tq_clutchMax);

// transmission losses are given as input shaft torque loss
  double tq_inTransmission = (outputs->tq_clutch - inputs.tq_losses);

  outputs->tq_outTransmission = tq_inTransmission * inputs.gear_ratio;

  double tq_sumWheel = outputs->tq_outTransmission * inputs.final_gear_ratio - tq_loadWheelShaft;

// w_wheel is integrate outside
  outputs->w_wheelDer = tq_sumWheel / ( inputs.m_vehicle * SQ( inputs.r_tire ) );

  outputs->v_vehicle = inputs.w_wheel * inputs.r_tire;

// slip estimation (r_slipFilt filtered outside this m-function)
  outputs->r_slip = (inputs.tq_env + tq_sumWheel) / ( inputs.m_vehicle * 8 );

  outputs->v_driveWheel = (inputs.r_slipFilt + 1) * outputs->v_vehicle;

  outputs->w_out = outputs->v_driveWheel * inputs.final_gear_ratio / inputs.r_tire;

  if (inputs.gear_ratio == 0){
    // when gear is in neutral the input shaft speed is integrated using the
    // torque coming from the clutch
    outputs->w_inShaftDer = tq_inTransmission / inputs.J_neutral; 
  }
  else{
    // When a gear is engaged the transmission input shaft speed is calculated
    // from the output shaft speed scaled with gear ratio, (the result from
    // the inputshaft neutral integration is ignored)
    outputs->w_inShaft = outputs->w_out * inputs.gear_ratio;
  }
    
// when not in neutral, set the inputShaft derivative so that the
// integrator follows the acutal speed aproximately
  outputs->w_inShaftDer = 0.5*(outputs->w_inShaft-inputs.w_inShaftNeutral)/inputs.ts;

  dxdt[ 0 ]  = ouputs->w_inShaftDer;
  dxdt[ 1 ]  = ouputs->w_wheelDer;
  
  return 0;
}
