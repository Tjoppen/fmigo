
MAKEDIR=~/work/umit
DATADIR=$(HOMEDIR)/data
DATAFILES= vanDerPol # bouncingBall

plot:$(DATAFILES)

$(DATAFILES): FORCE
	octave plot-$@.m

FORCE:
