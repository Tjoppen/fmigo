var config = require("../config.json"),
    fs = require('fs'),
    child_process = require("child_process");

var fmuDir = __dirname+"/../fmus";

function getModelDescriptionXML(fmuFile,callback){
    var command = config.fmuMasterCommand;
    var args = ["-qx",fmuDir+"/"+fmuFile];
    var commandLine = command+" "+args.join(" ");
    console.log(commandLine);
    child_process.exec(commandLine,function(err,stdout,stderr){
        if(err) return callback(err);
        if(stderr) return callback(new Error(stderr));
        callback(null,stdout);
    });
}
function getAvailableFMUs(callback){
    fs.readdir(fmuDir,function(err,files){
        callback(err,files);
    });
}

// GET /
exports.index = function(req,res){
    getAvailableFMUs(function(err,fmus){
        res.render("index",{
            fmus : fmus
        });
    });
};

// GET /fmus/:filename/modelDescription.xml
exports.modelDescription = function(req,res){

    // Todo should check if FMU exists

    getModelDescriptionXML(req.params.filename,function(err,xml){
        if(err){
            // Got an error.
            console.error(err);

            res.header("Content-Type","text/plain");
            res.send(500,err.message);
        } else {
            // OK
            res.header("Content-Type","application/xml");
            res.send(200,xml);
        }
    });
};

exports.simulate = function(req,res){
    var b = req.body;

    // Construct fmu paths
    var fmuToIndex = {};
    var fmuPaths = [];
    var idx = 0;
    for(var name in b.fmus){
        fmuPaths.push(fmuDir + "/" + b.fmus[name].fmu);
        fmuToIndex[name] = idx;
        idx++;
    }


    // Construct parameter triplets
    var parameters = [];
    for(var name in b.fmus){
        for(key in b.fmus[name].initialValues){
            var value = b.fmus[name].initialValues[key];
            var vr = parseInt(key);
            var fmuIdx = parseInt(fmuToIndex[name]);
            parameters.push([fmuIdx,vr,value].join(","));
        }
    }

    // Construct connection quads
    var connections = [];
    // TODO

    var args = ["-q","-o","stdout"];

    // Construct command line
    var commandLine = config.fmuMasterCommand + " -q -o stdout";
    if(parameters.length){
        commandLine += " -p " + parameters.join(":");
        args.push("-p",parameters.join(":"));
    }

    // Time step
    if(b.timestep){
        commandLine += " -d "+parseFloat(b.timestep);
        args.push("-d",parseFloat(b.timestep));
    }

    // Add fmu paths to the end of the command line
    commandLine += " "+fmuPaths.join(" ");
    args = args.concat(fmuPaths);
    
    console.log("running ",commandLine," or ", config.fmuMasterCommand+" "+args.join(" "));



    // TODO check number of running processes
    res.writeHead(200, { 'Content-Type': 'text/csv' });
    var master = child_process.spawn(config.fmuMasterCommand, args);
    master.stdout.on('data', function (data) {
        // Got a chunk of data
        res.write(data);
    });
    master.stderr.on('data', function (data) {
        // Got error
        console.error(data.toString());
        master.kill();
    });
    master.on('exit', function (code) {
        res.end();
    });

};