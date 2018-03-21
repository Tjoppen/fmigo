This contains wrappers and support classes for simulations written using the IVP toolbox of GSL. 

Under gsl is an interface which simplifies the use of GSL itself so that a model can be defined with minimal amount of codde.

Also provided is an abstraction which allows to add filters to the outputs of a model, such as Edo's EPCE, which integrates and averages outputs.  This is nearly automated, requiring very little user code, most of which is boiler plate. 

This is all written in plain C99 which means that code is slightly more verbose than desirable and memory allocation has to be considered with care.  Nevertheless, this should suffice for implementing simple though nontrivial models. 

There is then a template generator to produce FMUs from these models.  This works using a modelDescription.xml file which has to be tailored to the given model.  This file is parsed to generate C struct's which are used to copy data to and from the model from the FMI API.   Here also, the amount of hand-coding is minimal, and contained in one C file.  

Makefiles and driver scripts in the umit/fmu-bulider module.  