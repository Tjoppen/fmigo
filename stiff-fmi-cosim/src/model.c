#include <math.h>
#include "model.h"

void step(SlaveInstance * s, fmiReal h, fmiBoolean rhs_enabled){
    fmiReal f = s->f;
    fmiReal v = s->v;

    if(rhs_enabled == fmiTrue){
        f += s->amplitude * sin(s->time);
    } else {
        v = 0;
    }

    s->v = v + s->invMass * f * h;
    s->x += s->v * h;
}
