/**
 * Starts the app
 */
$(function(){

    var untitledName = "Untitled";
    var diagram = new BLOCK.Diagram("#container");
    var sim = new COSIM.Master(diagram);
    var analysis = new COSIM.AnalysisWindow("#plot",sim);
    sim.setName(untitledName);
    function updateTitle(){
	$("#title").html(sim.getName());
    }
    sim.addEventListener("changename",updateTitle);
    updateTitle();
    $("#simulate").click(function(){
	var nSteps = 100, step = 0, timeStep = 1/60;
	function doStep(){
	    if(step<nSteps){
		step++;
		sim.step(timeStep,doStep);
	    } else {
		sim.end();
	    }
	}
	doStep();
    });

    sim.addEventListener("componentLoaded",function(e){
	updateComponentList();
    });
    sim.addEnvironmentURI("http://granular.cs.umu.se:3017");
    
    // Keyboard callbacks
    $(window).keydown(function(e){
	switch(e.keyCode){
	case 46:
	    // DEL = delete selection
	    deleteSelection();
	    break;
	}
    });

    // Delete all selected things
    function deleteSelection(){
	// Remove connections
	var conns = sim.getSelectedConnections();
	for(var i=0; i<conns.length; i++){
	    var conn = conns[i];
	    sim.disconnect(conn.from,conn.to);
	}

	// Remove instances
	var instances = sim.getSelectedInstances();
	for(var i=0; i<instances.length; i++){
	    instances[i].destruct();
	    sim.removeInstance(instances[i]);
	}
    }

    // Context menu
    $context = $("#contextmenu");
    var emptyHTML = "<h1>Context menu</h1><p>Please select something.</p>";
    $context.html(emptyHTML);
    sim.addEventListener("selectionchange",updateContextMenu);
    function updateContextMenu(){
	var instances = sim.getSelectedInstances();
	var connections = sim.getSelectedConnections();
	if(instances.length==1 && connections.length==0){
	    // Info about instance
	    var instance = instances[0];
	    var variables = instance.getVariables();
	    var inputs=[], outputs=[], parameters=[];
	    for(var i=0; i<variables.length; i++){
		var v = variables[i];
		switch(v.causality){
		case "parameter":
		    parameters.push("<button class=\"variable\" data-variableId=\""+v.id+"\" data-variableName=\""+v.name+"\">"+v.name+"="+v.start+"</button>");
		    break;
		case "input":
		    inputs.push("<button class=\"variable\" data-variableId=\""+v.id+"\" data-variableName=\""+v.name+"\">"+v.name+"="+(v.start?v.start:"?")+"</button>");
		    break;
		case "output":
		    outputs.push("<button class=\"variable\" data-variableId=\""+v.id+"\" data-variableName=\""+v.name+"\">"+v.name+"="+(v.start?v.start:"?")+"</button>");
		    break;
		}
	    }
	    $context.html("<h1>"+instance.getName()+"</h1>"+
			  "<p>"+instance.getComponent().getDescription()+"</p>"+
			  "<h2>Parameters</h2>"+
			  "<p>"+parameters.join(" ")+"</p>"+
			  "<h2>Inputs</h2>"+
			  "<p>"+inputs.join(" , ")+"</p>"+
			  "<h2>Outputs</h2>"+
			  "<p>"+outputs.join(" , ")+"</p>");
	    $("button.variable").click(function(e){
		var name = $(e.target).attr("data-variableName");
		var varid = $(e.target).attr("data-variableId");
		var newval = prompt("New (start) value for variable "+name+":");
		if(newval!==null){
		    var variable = sim.getVariable(varid);
		    variable.setStartValue(newval);
		    updateContextMenu();
		}
	    });
	} else if(instances.length>0 || connections.length>0){
	    // Selection menu
	    $context.html("<h1>Selection</h1><p>"+instances.length+" blocks and "+connections.length+" connections selected. Delete them by pressing DEL.");
	} else {
	    $context.html(emptyHTML);
	}
    }

    function updateComponentList(){
	// Get components
	var components = sim.getComponents();

	// Sort by name
	components.sort(function(m1,m2){
	    if(m1.name > m2.name) return 1;
	    else if(m1.name == m2.name) return 0;
	    else return -1;
	});
	
	// Construct component list
	var clist = $("#component-list").html("");
	for(var i=0; i<components.length; i++){
	    var m = components[i];
	    (function(m){
		var $item = $("<li><a>"+m.getName()+"</a></li>").click(function(){
		    var instance = new COSIM.Instance(m,function(err,ins){
			sim.addInstance(ins);
			ins.setName(sim.createUniqueInstanceName(ins.getComponent()));
		    });
		});
		clist.append($item);
	    })(m);
	}
    }

    // Autocomplete
    var $s = $(".leftmenu input[type=text]");
    var selected = false;
    $s.autocomplete({
	source:function(request,callback){
	    sim.browseComponents(request.term,function(uris){
		callback(uris);
	    });
	},
	select:function(e,ui){
	    var uri = ui.item.value;
	    if(!sim.getComponentByURI(uri)){
		var component = new COSIM.Component(uri,function(err,m){
		    if(err)
			throw err;
		    else {
			sim.addComponent(m);
			updateComponentList();
		    }
		});
	    }
	    selected = true;
	},
	close: function(){
	    if(selected){
		$s.val(""); // Empty text area
		selected = false;
	    }
	}
    });
    // Hide annoying jQuery helper
    $(".leftmenu span[role=status]").hide();


    // Rename simulation
    var $renameDialog = $("#rename-simulation-form");
    $("#title").click(function(){
	$renameDialog.dialog("open");
    });
    $renameDialog.dialog({
	autoOpen: false,
	height: 150,
	width: 300,
	modal: true,
	open : function(){
	    $("#rename-simulation-name").val(sim.getName());
	},
	buttons: {
	    "Save": function() {
		var newName = $("#rename-simulation-name").val();
		if(!newName)
		    newName = untitledName;
		sim.setName(newName);
		$( this ).dialog( "close" );		
	    },
	    "Cancel": function() {
		$( this ).dialog( "close" );
	    }
	},
	close: function() {
	    
	}
    });

    
    // Analysis dialog
    var $analysisDialog = $("#analysis");
    $("#analyse").click(function() {
	$analysisDialog.dialog("open");
    });
    $analysisDialog.dialog({
	autoOpen: false,
	height: 480,
	width: 620,
	modal: true,
	open : function(){
	    
	},
	buttons: {
	    "Close": function() {
		$( this ).dialog( "close" );
	    }
	},
	close: function() {
	    
	}
    });
    $analysisDialog.parent().css({"background-color":"white"});
});