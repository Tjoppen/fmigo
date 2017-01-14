
MAKEDIR=~/work/umit
DATADIR=$(HOMEDIR)/data
DATAFILES= vanDerPol bouncingBall

plot:$(DATAFILES)

$(DATAFILES): FORCE
	(cd $(HOMEDIR) && bin/fmusim_me fmu/me/$@.fmu 20)
	octave $(PLOTDIR)/plot-$@.m

FORCE:
