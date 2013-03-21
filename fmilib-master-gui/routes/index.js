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

    // Construct command line
    var commandLine = config.fmuMasterCommand + " -q -o stdout -p " + parameters.join(":") + " " + fmuPaths.join(" ");
    console.log("running ",commandLine);

    // TODO check number of running processes
    child_process.exec(commandLine,function(err,stdout,stderr){

        // error?
        if(err){
            console.error(err);
            return res.send(500);
        }
        res.header("text/csv");
        res.send(200,stdout);
    });

};