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