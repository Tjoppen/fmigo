/**
 * Block diagram editor for HTML5.
 * 
 * Functionality
 * - drag/drop blocks
 * - available block types list
 * - drag lines in between block pins
 * - set block title
 * - add user data
 * - events
 *
 * Dependencies:
 * - jQuery 1.8.1
 */
BLOCK = {};

(function(BLOCK){

    // Extend object a with the properties in b
    function extend(a,b){
	for(key in b){
	    if(key in b){
		a[key] = b[key];
	    }
	}
    }
    
    /**
     * @class BLOCK.Diagram
     * @event connect The event object contains fields "from" and "to", which are Pins
     * @event selectionchange When the selection of blocks and lines changes
     * @param container
     * @param options { formatRaphaelPath:function(Rpath) , formatSelectedRaphaelPath:function(Rpath) }
     * @extends BLOCK.EventTarget
     */
    BLOCK.Diagram = function(container,options){
	BLOCK.EventTarget.apply(this);
	options = options||{};
	extend(options,{
	    formatRaphaelPath:function(path){
		path.attr("stroke-width",2);
		path.attr("stroke","black");
	    },
	    formatSelectedRaphaelPath:function(path){
		path.attr("stroke-width",3);
		path.attr("stroke","#ddd");
	    },
	});
	$(container).disableSelection();
	var blocks = [];
	var that = this;
	var el = this.el = $(container);
	var paper = this.paper = Raphael(container.replace("#",""),
					 window.innerWidth,
					 window.innerHeight);

	// To keep track of all connections
	var connections = [];
	var selectedConnections = []; // array of strings ["pinId1 pinId2",...] where pinId1<pinId2

	/**
	 * @method addSelectedConnection
	 * @param BLOCK.Pin p1
	 * @param BLOCK.Pin p2
	 * @brief Mark the connection between p1 and p2 as selected
	 */
	this.addSelectedConnection = function(p1,p2){
	    var s = p1.id<p2.id ? (p1.id+" "+p2.id) : (p2.id+" "+p1.id);
	    if(selectedConnections.indexOf(s)==-1){
		selectedConnections.push(s);
		that.dispatchEvent({type:"selectionchange"});
	    }
	}

	/**
	 * @method removeSelectedConnection
	 * @param BLOCK.Pin p1
	 * @param BLOCK.Pin p2
	 * @brief Removes the connection selection
	 */
	this.removeSelectedConnection = function(p1,p2){
	    var s = p1.id<p2.id ? (p1.id+" "+p2.id) : (p2.id+" "+p1.id);
	    var idx = selectedConnections.indexOf(s);
	    if(idx!=-1){
		selectedConnections.splice(idx,1);
		that.dispatchEvent({type:"selectionchange"});
	    }
	}

	/**
	 * @method isSelectedConnection
	 * @param BLOCK.Pin p1
	 * @param BLOCK.Pin p2
	 * @return bool
	 */
	this.isSelectedConnection = function(p1,p2){
	    var s = p1.id<p2.id ? (p1.id+" "+p2.id) : (p2.id+" "+p1.id);
	    var idx = selectedConnections.indexOf(s);
	    return (idx!=-1);
	}

	/**
	 * @method getSelectedConnections
	 * @memberof BLOCK.Diagram
	 * @return array
	 */
	this.getSelectedConnections = function(){
	    var result = [];
	    for(var i=0; i<selectedConnections.length; i++){
		var ids = selectedConnections[i].match(/\d+/g);
		var id1 = parseInt(ids[0]);
		var id2 = parseInt(ids[1]);
		var pin1 = that.getPin(id1);
		var pin2 = that.getPin(id2);
		result.push(newConn(pin1,pin2));
	    }
	    return result;
	};

	function newConn(from,to){
	    return { from : from,
		     to : to };
	}

	/**
	 * @method getConnections
	 * @memberof BLOCK.Diagram
	 * @return array
	 */
	this.getConnections = function(){
	    return connections;
	};

	/**
	 * @method getSelectedBlocks
	 * @memberof BLOCK.Diagram
	 * @return array
	 */
	this.getSelectedBlocks = function(){
	    var result = [];
	    for(var i=0; i<blocks.length; i++){
		if(blocks[i].isSelected()){
		    result.push(blocks[i]);
		}
	    }
	    return result;
	};

	/**
	 * @method addBlock
	 * @memberof BLOCK.Diagram
	 * @param BLOCK.Block block
	 */
	this.addBlock = function(block){
	    blocks.push(block);
	    el.prepend(block.el);
	    block.setDiagram(that);
	    block.addEventListener("selectchange",function(){
		that.dispatchEvent({type:"selectionchange"});
	    });
	};

	/**
	 * @method removeBlock
	 * @memberof BLOCK.Diagram
	 * @param BLOCK.Block block
	 */
	this.removeBlock = function(block){
	    for(var i=0; i<blocks.length; i++){
		if(blocks[i].id == block.id){
		    var wasSelected = block.isSelected();

		    // Disconnect all pins
		    var pins = blocks[i].getPins();
		    for(var j=0; j<pins.length; j++){
			that.disconnect(pins[j]);
		    }

		    // Remove from blocks array
		    block.el.remove();
		    blocks.splice(i,1);
		    block.unsetDiagram();

		    if(wasSelected){
			block.dispatchEvent({type:"selectchange"});
		    }
		    block.removeEventListeners("selectchange");

		    break;
		}
	    }
	};
	
	var draggingPin = null;

	/**
	 * @method getBlock
	 * @memberof BLOCK.Diagram
	 * @param int id
	 * @return BLOCK.Block or FALSE
	 */
	this.getBlock = function(id){
	    for(var i=0; i<blocks.length; i++){
		var block = blocks[i];
		if(block.id == id) return block;
	    }
	    return false;
	};

	/**
	 * @method getPin
	 * @memberof BLOCK.Diagram
	 * @param int id
	 * @return BLOCK.Pin or FALSE
	 */
	this.getPin = function(id){
	    for(var i=0; i<blocks.length; i++){
		var pin = blocks[i].getPin(id);
		if(pin)
		    return pin;
	    }
	    return false;
	}

	// Generate SVG path string. first coord is from OUTPUT
	function horizontalCurvePathString(x1,y1,x2,y2){
	    // Normal: output is on left side.
	    if(x1 < x2)
		return "M"+(x1+","+y1)+"C"+(((x1+x2)*0.5)+","+y1)+" "+(((x1+x2)*0.5)+","+y2)+" "+(x2+","+y2);
	    // Output is on right side... Make the path point out from the block a bit
	    else {
		var diff = Math.abs(x1-x2);
		return "M"+(x1+","+y1)+"C"+(x1+diff+","+y1)+" "+((x2-diff)+","+y2)+" "+(x2+","+y2);
	    }
	}
	
	/**
	 * @method connect
	 * @memberof BLOCK.Diagram
	 * @param BLOCK.Pin pin1
	 * @param BLOCK.Pin pin2
	 */
	this.connect = function(pin1,pin2){
	    if(!pin1.connectedTo && !pin2.connectedTo){
		if(pin2.isOutput() && pin1.isInput()){
		    
		    // Draw bezier
		    var pos1 = pin1.getLeftPosition();
		    var pos2 = pin2.getRightPosition();

		    var path = horizontalCurvePathString(pos2.left,pos2.top,
							 pos1.left,pos1.top);
		    var rpath = that.paper.path(path);
		    options.formatRaphaelPath(rpath);
		    rpath.click(function(){
			if(!that.isSelectedConnection(pin1,pin2)){
			    // Select
			    options.formatSelectedRaphaelPath(rpath);
			    that.addSelectedConnection(pin1,pin2);
			    that.dispatchEvent({type:"selectionchange"});
			} else {
			    // Unselect
			    options.formatRaphaelPath(rpath);
			    that.removeSelectedConnection(pin1,pin2);
			    that.dispatchEvent({type:"selectionchange"});
			}
		    });
		    pin1.path = rpath;
		    pin2.path = rpath;
		    pin1.connectedTo = pin2;
		    pin2.connectedTo = pin1;

		    connections.push(newConn(pin2,pin1));
		    that.dispatchEvent({type:"connect",from:pin2,to:pin1});

		} else if(pin1.isOutput() && pin2.isInput()) {
		    that.connect(pin2,pin1);
		}
	    }
	};

	/**
	 * @method disconnect
	 * @memberof BLOCK.Diagram
	 * @param BLOCK.Pin pin
	 */
	this.disconnect = function(pin){
	    if(pin.connectedTo){
		// Remove from connections list
		for(var i=0; i<connections.length; i++){
		    if((connections[i].from.id == pin.id && 
			connections[i].to.id == pin.connectedTo.id) || 
		       (connections[i].to.id == pin.id && 
			connections[i].from.id == pin.connectedTo.id)){
			connections.splice(i,1);
			that.removeSelectedConnection(pin,pin.connectedTo);
			break;
		    }
		}

		var pin1 = pin;
		var pin2 = pin1.connectedTo;
		var p1 = pin1.path;
		var p2 = pin2.path;
		pin1.path = null;
		pin2.path = null;
		pin1.connectedTo = null;
		pin2.connectedTo = null;
		p1.remove();
		p2.remove();
	    }
	};

	/**
	 * @method updatePathForPin
	 * @brief Updates the path connected to a pin
	 * @memberof BLOCK.Diagram
	 * @param BLOCK.Pin pin
	 */
	this.updatePathForPin = function(pin){
	    if(pin.connectedTo){
		var pos1, pos2,p;
		if(pin.isOutput()){
		    pos1 = pin.getRightPosition();
		    pos2 = pin.connectedTo.getLeftPosition();
		    p = horizontalCurvePathString(pos1.left,pos1.top,
						  pos2.left,pos2.top);
		} else {
		    pos1 = pin.getLeftPosition();
		    pos2 = pin.connectedTo.getRightPosition();
		    p = horizontalCurvePathString(pos2.left,pos2.top,
						  pos1.left,pos1.top);
		}
		pin.path.attr({path:p});
	    }
	};


	function element2pin(el){
	    var parPin = $(el).parents(".pin");
	    if(parPin){
		// Get target pin we dropped on
		var id = $(parPin).attr("id");
		if(id){
		    var n = id.match(/pin\-(\d+)$/);
		    if(n && n.length==2){
			var pinId = parseInt(n[1]);
			// Now get the Block id
			var bid = $(parPin).parents(".block").attr("id");
			if(bid){
			    var bn = bid.match(/block\-(\d+)$/);
			    if(bn && bn.length==2){
				var blockId = parseInt(bn[1]);
				var block = that.getBlock(blockId);
				var pin = block.getPin(pinId);
				return pin;
			    }
			}
		    }
		}
	    }
	    return null;
	}
	
	// TODO: via mouse events instead. Better handling of positions
	var line, lineStartX, lineStartY;
	function page2canvasCoords(event){
	    var offset = el.offset();
	    return {left : event.pageX - offset.left,
		    top :  event.pageY - offset.top};
	}
	
	this.el.mousedown(function(e){
	    if(e.which==1){
		var pin = element2pin(e.target);
		if(pin && !pin.connectedTo){ // Only if not yet connected
		    // Start dragging
		    draggingPin = pin;
		    $(".block").trigger("mouseup",["dummy"]);

		    // Create raphael line from pin to mouse position
		    var p = pin.isInput() ? pin.getLeftPosition() : pin.getRightPosition();
		    var coords = page2canvasCoords(e);
		    lineStartX = coords.left;
		    lineStartY = coords.top;

		    var path = "M"+p.left+","+p.top+"L"+(coords.left)+","+(coords.top);
		    line = that.paper.path(path);
		}
	    }
	});
    
	this.el.mousemove(function(e){
	    if(e.which==1 && draggingPin && line){
		var coords = page2canvasCoords(e);
		line.attr({path:"M"+lineStartX+","+lineStartY+"L"+(coords.left)+","+(coords.top)});
	    }
	});

	this.el.mouseup(function(e){
	    if(e.which==1 && draggingPin){
		// Get target pin we dropped on
		var pin = element2pin(e.target);
		if(pin){
		    that.connect(draggingPin,pin);
		}
		line.remove();
		line = null;
		draggingPin = null;
	    }
	});
    };

    var blockCounter = 1;

    /**
     * @class BLOCK.Block
     * @brief Block in the block diagram
     * @event dragstart
     * @event drag
     * @event dragstop
     * @event select
     * @event unselect
     * @event selectchange
     * @extends BLOCK.EventTarget
     */
    BLOCK.Block = function(){
	BLOCK.EventTarget.apply(this);

	/**
	 * @property int id
	 * @memberof BLOCK.Block
	 */
	this.id = blockCounter++;
	var diagram, that=this;

	/**
	 * @property Element el
	 * @memberof BLOCK.Block
	 */
	var el = this.el = $("<div></div>").addClass("block").attr({id:"block-"+this.id});
	var $head = $("<div>BlockName</div>").addClass("head");
	var $inputs = $("<div></div>").addClass("inputs");
	var $outputs = $("<div></div>").addClass("outputs");
	var inputs = [], outputs = [], pins = [];

	// Make sure clicked but not dragged to select
	var selected = false;
	var failed = null;
	el.mousedown(function(e){
	    failed = false;
	}).mousemove(function(){
	    failed = true;
	}).mouseup(function(e,isDummy){
	    if(!isDummy && !e.target.className.match("ui-draggable-dragging") && failed===false){
		el.toggleClass("selected");
		selected = !selected;
		that.dispatchEvent({type:selected ? "select" : "unselect"});
		that.dispatchEvent({type:"selectchange"});
		failed = null;
	    }
	});

	el
	    .append($head)
	    .append($inputs)
	    .append($outputs);
	el.draggable({
	    start:function(e,ui){
		that.dispatchEvent({type:"dragstart"});
	    },
	    drag:function(e,ui){
		// Update pin paths
		for(var i=0; i<pins.length; i++)
		    diagram.updatePathForPin(pins[i]);
		that.dispatchEvent({type:"drag"});
	    },
	    stop:function(e,ui){
		that.dispatchEvent({type:"dragstop"});
	    },
	});

	/**
	 * @method addPin
	 * @memberof BLOCK.Block
	 * @param BLOCK.Pin pin
	 */
	this.addPin = function(pin){
	    if(pin.isInput()){
		inputs.push(pin);
		$inputs.append(pin.el);
	    } else {
		$outputs.append(pin.el);
		outputs.push(pin);
	    }
	    pins.push(pin);
	    pin.setBlock(that);
	};

	/**
	 * @method setPosition
	 * @brief Set position relative to the container
	 * @memberof BLOCK.Block
	 * @param int x
	 * @param int y
	 */
	this.setPosition = function(x,y){
	    el.css({ left:x+"px", top:y+"px" });
	};

	/**
	 * @method setDiagram
	 * @memberof BLOCK.Block
	 * @param BLOCK.Diagram d
	 */
	this.setDiagram = function(d){
	    diagram = d;
	};

	/**
	 * @method unsetDiagram
	 * @memberof BLOCK.Block
	 */
	this.unsetDiagram = function(){
	    diagram = null;
	};

	/**
	 * @method getDiagram
	 * @memberof BLOCK.Block
	 * @return BLOCK.Diagram
	 */
	this.getDiagram = function(){
	    return diagram;
	};

	/**
	 * @method setTopLabel
	 * @memberof BLOCK.Block
	 * @param string str
	 */
	this.setTopLabel = function(str){
	    $head.html(str);
	};

	/**
	 * @method isSelected
	 * @memberof BLOCK.Block
	 * @return bool
	 */
	this.isSelected = function(){
	    return selected;
	};

	/**
	 * @method getPin
	 * @memberof BLOCK.Block
	 * @param int id
	 * @return BLOCK.Pin or FALSE
	 */
	this.getPin = function(id){
	    for(var i=0; i<pins.length; i++){
		var pin = pins[i];
		if(pin.id == id) return pin;
	    }
	    return false;
	}

	this.getPins = function() {
	    return pins;
	};
    };

    var pinCounter = 1;
    /**
     * @class BLOCK.Pin
     * @param bool isInput
     */
    BLOCK.Pin = function(isInput){
	var block, diagram;
	/**
	 * @property int id
	 * @memberof BLOCK.Pin
	 */
	this.id = pinCounter++;
	/**
	 * @property Element el
	 * @memberof BLOCK.Pin
	 */
	var el = this.el = $("<div></div>").addClass("pin "+(isInput ? "output" : "input")).attr({id:"pin-"+(this.id)});
	var inner = $("<div class=\"name\"></div>");
	el.append(inner);
	var that = this;

	var line, lineStartX, lineStartY;

	/**
	 * @method getPosition
	 * @memberof BLOCK.Pin
	 * @return Object An object with properties "top" and "left"
	 */
	this.getPosition = function(){
	    var p_inner = inner.offset();
	    var p_outer = that.getDiagram().el.offset();
	    return { left: p_inner.left - p_outer.left,
		     top:  p_inner.top - p_outer.top};
	};

	/**
	 * @method getPosition
	 * @memberof BLOCK.Pin
	 * @return Object An object with properties "top" and "left"
	 */
	this.getCenterPosition = function(){
	    var pos = that.getPosition();
	    return { left:pos.left+that.el.width()*0.5, top:pos.top+that.el.height()*0.5 };
	};

	/**
	 * @method getLeftPosition
	 * @memberof BLOCK.Pin
	 */
	this.getLeftPosition = function(){
	    var pos = that.getPosition();
	    return { left:pos.left, top:pos.top+that.el.height()*0.5 };
	};

	/**
	 * @method getRightPosition
	 * @memberof BLOCK.Pin
	 */
	this.getRightPosition = function(){
	    var pos = that.getPosition();
	    return { left:pos.left+that.el.width(), top:pos.top+that.el.height()*0.5 };
	};

	/**
	 * @method setName
	 * @memberof BLOCK.Pin
	 * @param string name
	 */
	this.setName = function(name){
	    inner.html(name);
	};

	/**
	 * @method isInput
	 * @memberof BLOCK.Pin
	 * @return bool
	 */
	this.isInput = function(){
	    return isInput;
	};

	/**
	 * @method isOutput
	 * @memberof BLOCK.Pin
	 * @return bool
	 */
	this.isOutput = function(){
	    return !that.isInput();
	};

	/**
	 * @method setBlock
	 * @memberof BLOCK.Pin
	 * @param BLOCK.Block b
	 */
	this.setBlock = function(b){
	    block = b;
	};

	/**
	 * @method getBlock
	 * @memberof BLOCK.Pin
	 * @return BLOCK.Block
	 */
	this.getBlock = function(){
	    return block;
	};

	this.getDiagram = function(){
	    return that.getBlock().getDiagram();
	};
    };

    /**
     * @class BLOCK.EventTarget
     * @author mr.doob / http://mrdoob.com/
     */   
    BLOCK.EventTarget = function(){
	var listeners = {};
	this.addEventListener = function ( type, listener ) {
	    if ( listeners[ type ] == undefined )
		listeners[ type ] = [];
	    if ( listeners[ type ].indexOf( listener ) === - 1 )
		listeners[ type ].push( listener );
	};
	this.dispatchEvent = function ( event ) {
	    for ( var listener in listeners[ event.type ] )
		listeners[ event.type ][ listener ]( event );
	};
	this.removeEventListener = function ( type, listener ) {
	    var index = listeners[ type ].indexOf( listener );
	    if ( index !== - 1 )
		listeners[ type ].splice( index, 1 );
	};
	this.removeEventListeners = function(type){
	    listeners[ type ] = [];
	};
    };

})(BLOCK);