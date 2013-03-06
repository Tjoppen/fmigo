/**
 * Co-simulation things.
 * 
 * Functionality
 * - Load Components
 * - Instantiate Components into Instances
 * - Drag-drop Instances, connect inputs and outputs
 * - In short: use the FMI-JSON-RPC API and the central component loading API
 * 
 * Dependencies:
 * - BlockDiagram.js
 * 
 */
COSIM = {};
(function(COSIM){

    function copyArray(a){
	var result = [];
	for(var i=0; i<a.length; i++)
	    result.push(a[i]);
	return result;
    }
    
    /**
     * @class COSIM.Master
     * @param BLOCK.Diagram diagram
     * @event componentLoaded
     */
    COSIM.Master = function(diagram){
	BLOCK.EventTarget.call(this);
	var that = this;
	var uri2component = {}; // Map: uri => component
	var components = [],
	instances = [],
	componentDescriptions = {}; // Map: uri => json object
	var blockid2instance = {}; // Map: block id => Instance
	var name = "";
	var globalTime = 0.0, stepNumber = 0, timePoints=[];

	function blocks2instances(blocks){
	    var result = [];
	    for(var i=0; i<blocks.length; i++)
		result.push(blockid2instance[blocks[i].id]);
	    return result;
	}

	// Delegate selectionchange events
	diagram.addEventListener("selectionchange",function(e){
	    that.dispatchEvent(e);
	});

	this.getName = function(){
	    return name;
	};

	this.setName = function(n){
	    name = n;
	    that.dispatchEvent({type:"changename"});
	};

	this.getTimePoints = function(){
	    return copyArray(timePoints);
	};

	/**
	 * @method step
	 * @memberof COSIM.Master
	 * @brief Step the system
	 * @param float timeStep
	 */
	this.step = function(timeStep,callback){
	    // 1. Step all instances
	    that.dispatchEvent({type:"preStep"});
	    var numStepped = 0;
	    for(var i=0; i<instances.length; i++){
		instances[i].step(timeStep,function(err){
		    numStepped++;
		    // 2. Exchange all port data in correct direction
		    if(numStepped==instances.length){
			exchangePortValues(function(err){
			    // 3. Done
			    stepNumber++;
			    globalTime = timeStep*stepNumber;
			    timePoints.push(globalTime);
			    that.dispatchEvent({type:"step"});
			    callback(null);
			});
		    }
		});
	    }
	};

	function exchangePortValues(callback){
	    var connections = that.getConnections();
	    if(connections.length) {
		var numExchanged = 0;
		for(var i=0; i<connections.length; i++){

		    var vFrom = connections[i][0];
		    var vTo = connections[i][1];
		    var vFromDot = vFrom.getDerivativeVariable();
		    var vToDot = vTo.getDerivativeVariable();

		    (function(vFrom,vTo,vFromDot,vToDot){
			//console.log("-- Initiating value transfer from "+vFrom.getInstance().getName()+"."+vFrom.name+" to "+vTo.getInstance().getName()+"."+vTo.name);
			vFrom.getValue(function(err,val){
			    if(err) throw err;
			    //console.log("Got value from "+vFrom.getInstance().getName()+"."+vFrom.name);
			    vTo.setValue(val,function(err){
				if(err) throw err;
				//console.log("Transferred value from "+vFrom.getInstance().getName()+"."+vFrom.name+" to "+vTo.getInstance().getName()+"."+vTo.name);
				//if(numExchanged==connections.length){
				    if(vToDot && vFromDot){
					vFromDot.getValue(function(err,dot){
					    if(err) throw err;
					    vToDot.setValue(dot,function(err){
						if(err) throw err;
						numExchanged++;
						if(numExchanged==connections.length)
						    callback(null);
					    });
					});
				    } else {
					numExchanged++;
					if(numExchanged==connections.length)
					    callback(null);
				    }
				//}
			    });
			});
		    })(vFrom,vTo,vFromDot,vToDot);
		}
		
	    } else {
		callback(null);
	    }
	}
	
	// Ends the simulation, prepare for starting over
	this.end = function(callback){
	    callback= callback || function(){};
	    var numEnds = 0;
	    for(var i=0; i<instances.length; i++){
		instances[i].end(function(){
		    numEnds++;
		    if(numEnds==instances.length){
			that.dispatchEvent({type:"end"});
			globalTime = 0.0;
			stepNumber = 0;
			timePoints = [];
		    }			
		});
	    }
	};

	/**
	 * @method createUniqueInstanceName
	 * @memberof COSIM.Master
	 * @brief Create a new instance name
	 * @param COSIM.Component component
	 */
	var instanceNameIdCounter = {}; // Map: componentURI => int
	this.createUniqueInstanceName = function(component){

	    // First part is last part of component name
	    var firstPart = component.getName().match(/\.([^.]*)$/);
	    if(!firstPart)
		firstPart = component.getName();
	    else 
		firstPart = firstPart[0].replace(".","");

	    // Second part is a number
	    var number = 1;
	    if(component.uri in instanceNameIdCounter){
		number = ++instanceNameIdCounter[component.uri];
	    } else {
		instanceNameIdCounter[component.uri] = 1;
	    }

	    return firstPart + number;
	};

	/**
	 * @method addInstance
	 * @memberof COSIM.Master
	 * @param COSIM.Instance instance
	 */
	this.addInstance = function(instance){
	    instances.push(instance);
	    diagram.addBlock(instance.block);
	    blockid2instance[instance.block.id+""] = instance;
	};
	
	this.removeInstance = function(instance){
	    instances.splice(instances.indexOf(instance),1);
	    delete blockid2instance[instance.block.id+""];
	    diagram.removeBlock(instance.block);
	};

	/**
	 * @method getInstances
	 * @memberof COSIM.Master
	 * @return array
	 */
	this.getInstances = function(){
	    return copyArray(instances);
	};

	this.getVariable = function(id){
	    var instances = that.getInstances();
	    for(var i=0; i<instances.length; i++){
		var v = instances[i].getVariable(id);
		if(v) return v;
	    }
	    return false;
	};

	/**
	 * @method addComponent
	 * @memberof COSIM.Master
	 * @brief Add component to the internal component list
	 * @param COSIM.Component m
	 */
	this.addComponent = function(m){
	    uri2component[m.uri] = m;
	    components.push(m);
	};

	this.removeComponent = function(m){
	    components.splice(components.indexOf(m),1);
	    delete uri2component[m.uri];
	};

	/**
	 * @method getComponents
	 * @memberof COSIM.Master
	 * @return array
	 */
	this.getComponents = function(){
	    return copyArray(components);
	};

	this.getComponentByURI = function(uri){
	    return uri2component[uri];
	};

	/**
	 * @method getSelectedInstances
	 * @memberof COSIM.Master
	 * @return array
	 */
	this.getSelectedInstances = function() {
	    var blocks = diagram.getSelectedBlocks();
	    return blocks2instances(blocks);
	};

	// returns array of pairs of COSIM.Variable
	this.getConnections = function(){
	    var result = [];
	    var connections = diagram.getConnections();
	    for(var i=0; i<connections.length; i++){
		var pinFrom = connections[i].from, pinTo = connections[i].to;
		var vFrom = false, vTo = false;
		// Find variables
		for(var j=0; j<instances.length; j++){
		    var tempFrom = instances[j].pinToVariable(pinFrom);
		    var tempTo = instances[j].pinToVariable(pinTo);
		    if(tempFrom) vFrom = tempFrom;
		    if(tempTo) vTo = tempTo;
		}

		if(vFrom && vTo)
		    result.push([vFrom,vTo]);
	    }
	    return result;
	};

	/**
	 * @brief Gets the selected connections.
	 * @method getSelectedConnections
	 * @memberof COSIM.Master
	 * @return array Array of objects with fields "from" (COSIM.Variable) and "to" (COSIM.Variable)
	 * @todo
	 */
	this.getSelectedConnections = function(){
	    return [];
	};

	/**
	 * @method COSIM.browseComponents
	 * @param string query
	 * @param function callback
	 * @brief Connects to the central server and fetches a list of component URIs
	 */
	var componentURIs = [];
	var envs = [];
	var sockets = {}; // Map: envURI => socketURL
	this.browseComponents = function(query,callback){
	    callback(componentURIs);
	};

	this.getSocketURL = function(envURL){
	    return sockets[envURL];
	};

	function loadComponents(){
	    for(var i=0; i<envs.length; i++){
		(function(i){
		    $.ajax(envs[i]+"/components",{
			dataType:"text",
			success : function(data){
			    data = JSON.parse(data);
			    for(var j=0; j<data.components.length; j++){
				// got the uri!
				var uri = envs[i]+data.components[j];
				componentURIs.push(uri);
				new COSIM.Component(uri,function(err,comp){
				    that.addComponent(comp);
				    that.dispatchEvent({type:"componentLoaded",component:comp});
				});
			    }
			},
			error:function(err){
			    console.log(err);
			}
		    });

		    $.ajax(envs[i],{
			dataType:"text",
			success : function(data){
			    data = JSON.parse(data);
			    sockets[envs[i]] = data.socketURL;
			},
			error:function(err){
			    console.log(err);
			}
		    });

		})(i);
	    }
	}

	this.addEnvironmentURI = function(environmentURI){
	    envs.push(environmentURI);
	    loadComponents();
	};
    };

    /**
     * @class Component
     * @param string uri The ComponentDescription uri
     * @param function callback Called when loaded, like this: callback(error,component)
     */
    var componentCounter = 1;
    COSIM.Component = function(uri,callback){
	this.uri = uri;
	this.id = componentCounter++;
	var that = this;
	var componentDescription;
	var variables = [], socketURL;

	this.getSocketURL = function(){
	    return socketURL;
	};

	/**
	 * @return string or false if the component was not yet loaded
	 */
	this.getName = function(){
	    return componentDescription ? componentDescription.modelName : false;
	};

	this.getDescription = function(){
	    return componentDescription ? componentDescription.description : false;	    
	};

	this.getVariables = function(){
	    var result = [];
	    for(var i=0; i<variables.length; i++)
		result.push(variables[i]);
	    return result;
	};

	this.cloneVariables = function(){
	    var result = [];
	    for(var i=0; i<variables.length; i++)
		result.push(variables[i].clone());
	    return result;
	};

	this.getInputs = function(){
	    var result = [];
	    for(var i=0; i<variables.length; i++){
		var v = variables[i];
		if(v.causality=="input")
		    result.push(v);
	    }
	    return result;
	};

	this.getOutputs = function(){
	    var result = [];
	    for(var i=0; i<variables.length; i++){
		var v = variables[i];
		if(v.causality=="output")
		    result.push(v);
	    }
	    return result;
	};

	$.ajax(uri,{
	    dataType:"text",
	    success : function(data){
		componentDescription = JSON.parse(data);
		socketURL = componentDescription.socketURL;
		var vNames = getVariableNames(componentDescription);
		parseVars(vNames,componentDescription);
		callback(null,that);
	    },
	    error : function(err){
		// Todo: better error display
		callback(new Error("Error loading "+uri));
	    }
	});

	function getVariableNames(componentDesc){
	    var result = [];
	    for(var type in componentDesc.modelVariables){
		var vars = componentDesc.modelVariables[type];
		for(var i=0; i<vars.length; i++){
		    var v = vars[i];
		    result.push(v.name);
		}
	    }
	    return result;
	}

	function parseVars(varNames,componentDesc){
	    for(var i=0; i<varNames.length; i++){
		var v = new COSIM.Variable(varNames[i],componentDesc);
		variables.push(v);
	    }
	}
    };

    // Parses variable info from componentdescription, exposes needed things
    var varCounter = 0;
    COSIM.Variable = function(name,modelDesc){
	BLOCK.EventTarget.apply(this);
	var that = this;
	this.id = ++varCounter;
	var modelVariable, instance, derivative, structureVariable;

	// find modelvariable
	for(var type in modelDesc.modelVariables){
	    var vars = modelDesc.modelVariables[type];
	    for(var i=0; i<vars.length; i++){
		var v = vars[i];
		if(v.name == name)
		    modelVariable = v;
	    }
	}
	if(!modelVariable)
	    throw new Error(v.name + " could not be found in modelDescription");

	// Find structure variable
	for(var type in modelDesc.modelStructure){ // type=input,output,derivatives
	    var vars = modelDesc.modelStructure[type];
	    for(var i=0; i<vars.length; i++){
		var v = vars[i];
		if(v.name == name)
		    structureVariable = v;
	    }
	}
	
	this.name = modelVariable.name;
	this.causality = modelVariable.causality;
	if(modelVariable.start)
	    this.start = modelVariable.start;
	else if(modelVariable.causality=="derivative"){
	    this.state = structureVariable.state;
	}
	this.valueRef = modelVariable.valueReference;
	this.history = [];
	this.clone = function(){
	    var v = new COSIM.Variable(name,modelDesc);
	    if(derivative)
		v.setDerivativeVariable(derivative);
	    if(instance)
		v.setInstance(instance);
	    return v;
	};
	this.setStartValue = function(val){
	    that.start = val;
	    that.dispatchEvent({type:"startValueChange"});
	};
	this.setValue = function(val,callback){
	    if(!instance)
		throw new Error("Set instance before setting value of "+that.name+"!");
	    instance.setVariableValue(that,val,callback);
	};
	this.getValue = function(callback){
	    if(!instance)
		throw new Error("Set instance before getting value of "+that.name+" (id="+that.id+")!");
	    that.getInstance().getVariableValue(that,callback);
	};
	this.setInstance = function(ins){ instance = ins; };
	this.getInstance = function(){ return instance; };
	this.setDerivativeVariable = function(v){
	    if(v.id==that.id)
		throw new Error("Trying to set variable as derivative of itself");
	    derivative = v;
	};
	this.getDerivativeVariable = function(){ return derivative; };
    };

    /**
     * @class Instance
     * @param COSIM.Component component The Component to instantiate
     * @param function callback Called when loaded, like this: callback(error)
     */
    var instanceCounter = 1;
    COSIM.Instance = function(component,callback){
	BLOCK.EventTarget.apply(this);
	this.id = instanceCounter++;
	var that = this,
	name,
	conn,
	rpc,
	instanceURI,
	pinIdToVariable = {};

	// Create block for the instance
	var block = this.block = new BLOCK.Block();
	block.setPosition(100,30);
	
	// Create pins for all in/outputs
	var variables = component.cloneVariables();
	for(var i=0; i<variables.length; i++){
	    var v = variables[i];
	    v.setInstance(that);
	    if(v.causality=="input" || v.causality=="output"){
		var pin = new BLOCK.Pin(v.causality=="input");
		pinIdToVariable[pin.id+""] = v;
		block.addPin(pin);
		pin.setName(v.name);
	    }
	}

	// assemble derivatives
	for(var i=0; i<variables.length; i++){
	    var v = variables[i];
	    // find the derivative of v
	    for(var j=0; j<variables.length; j++){
		var u = variables[j];
		if(u.causality=="derivative" && u.state==v.name)
		    v.setDerivativeVariable(u);
	    }
	}

	this.pinToVariable = function(pin){
	    var v = pinIdToVariable[pin.id];
	    return v ? v : false;
	};

	function connect(callback){
	    if(!conn){
		conn = new WebSocket(component.getSocketURL()); // todo get from component
		conn.onopen = function(){
		    that.dispatchEvent({type:"connected"});
		    block.el.addClass("connected");
		    var relURI = that.getComponent().uri.match(/(\/components\/.*$)/);
		    if(relURI && relURI.length>0){
			relURI = relURI[0];
			rpc.call("instantiateSlave",[that.getName(),relURI,true,false],function(err,result){
			    if(err) throw new Error(err);
			    else {
				
				instanceURI = result.instanceURI;
				that.dispatchEvent({type:"instantiated"});
				block.el.addClass("instantiated");

				rpc.call("initializeSlave",[instanceURI,0.0001,0.0,10.0],function(err,result){
				    // set parameter values
				    var n = 0, finished=0;
				    for(var i=0; i<variables.length; i++)
					if(typeof(variables[i].start)!="undefined")
					    n++;
				    if(n>0){
					for(var i=0; i<variables.length; i++){
					    if(typeof(variables[i].start)!="undefined"){
						//console.log("setting start value of ",variables[i].name);
						rpc.call("setReal",[instanceURI,[variables[i].valueRef],[variables[i].start]],function(err,result){
						    finished++;
						    if(finished==n)
							callback();
						});
					    }
					}
				    } else {
					callback();
				    }
				});
			    }
			});
		    }
		};
		conn.onclose = function(){
		    block.el.removeClass("connected");
		    block.el.removeClass("instantiated");
		    conn = null;
		    rpc = null;
		};
		rpc = new COSIM.JSONRPC.Client(conn);
	    } else 
		callback(null);
	}

	this.setVariableValue = function(variable,value,callback){
	    if(rpc){
		rpc.call("setReal",[instanceURI,[variable.valueRef],[value]],function(err,result){
		    callback(err);
		});
	    } else {
		throw new Error("Cannot set value of variable when not connected.");
	    }
	};

	this.getVariableValue = function(variable,callback){
	    rpc.call("getReal",[instanceURI,[variable.valueRef],"json"],function(err,result){
		if(err){
		    callback(err);
		} else {
		    callback(null,result.values[0]);
		}
	    });
	};

	this.step = function(timeStep,callback){
	    // Make sure we are connected
	    connect(function(err){
		if(!err){
		    rpc.call("doStep",[instanceURI,0.0,timeStep,false],function(err,result){
			// get all variable values, add to their history
			var numFetched = 0;
			var errors = [];
			for(var i=0; i<variables.length; i++){
			    (function(v){
				rpc.call("getReal",[instanceURI,[v.valueRef],"json"],function(err,result){
				    if(err) errors.push(err);
				    // Save value
				    v.history.push(result.values[0]);
				    numFetched++;
				    if(numFetched == variables.length){
					callback(errors.length ? errors.join(" , ") : null);
				    }
				});
			    })(variables[i]);
			}
		    });
		}
	    });
	};

	// Delete the instance
	this.destruct = function(callback){
	    if(rpc){
		rpc.call("terminateSlave",[instanceURI],function(err,result){
		    if(conn) conn.close();
		    rpc.close();
		    rpc = null;
		    callback(err);
		});
	    }
	};

	// run on simulation end
	this.end = function(callback){
	    that.destruct(callback);
	};

	this.setName = function(n){
	    block.setTopLabel(n);
	    name = n;
	};

	this.getVariables = function(){
	    return copyArray(variables);
	};

	this.getVariable = function(id){
	    var vars = that.getVariables();
	    for(var i=0; i<variables.length; i++){
		if(variables[i].id == id)
		    return variables[i];
	    }
	    return false;
	};

	this.getName = function(){
	    return name;
	};

	this.getComponent = function(){
	    return component;
	};

	this.setName(component.getName());

	callback(null,that);
    };

    COSIM.JSONRPC = {
	Client : function(websocket){
	    var messageCounter = 0;
	    var callbacks = {}; // id => function
	    websocket.onmessage = function(e){
		var json = JSON.parse(e.data);
		if((json.id+"") in callbacks){
		    callbacks[json.id+""].call(null,json.error,json.result);
		}
	    };
	    this.call = function(method,params,callback){
		var id = ++messageCounter;
		var json = {
		    id:id,
		    method:method,
		    params:params
		};
		//console.log("-->",JSON.stringify(json));
		callbacks[id+""] = callback;
		websocket.send(JSON.stringify(json));
	    };
	    this.close = function(){
		callbacks = {};
		messageCounter = 0;
	    }
	}
    };

    /**
     * Analysis plotting window.
     * @param string container
     * @param COSIM.Master master
     */
    COSIM.AnalysisWindow = function(container,master){
	var el = this.el = $(container);
	function update(){
	    // Gather all values
	    // Update plot
	    var lines = [];
	    var instances = master.getInstances();
	    var timePoints = master.getTimePoints();
	    for(var i=0; i<instances.length; i++){
		var vars = instances[i].getVariables();
		for(var j=0; j<vars.length; j++){
		    var v = vars[j];
		    if(v.causality!="derivative") {
			var line = {data:[],label:instances[i].getName()+"."+v.name};
			for(var k=0; k<v.history.length; k++)
			    line.data.push([timePoints[k],v.history[k]]);
			lines.push(line);
		    }
		}
	    }
	    $.plot(el,lines);
	}
	master.addEventListener("end",update);
	$.plot(el,[[],[]]);
    };
    
})(COSIM);