cmake_minimum_required(VERSION 2.8)
include(FmuBase.cmake)



# GSL Co-Simulation FMUs
add_fmu(gsl2/clutch2                    clutch2                         "cgsl")
add_fmu(gsl2/clutch3                    clutch3                         "cgsl")
add_fmu(gsl2/chained_sho                chained_sho                     "cgsl")
add_fmu(gsl2/clutch_ef                  clutch_ef                       "cgsl")
add_fmu(gsl2/clutch                     clutch                          "cgsl")
add_fmu(gsl2/coupled_sho                coupled_sho                     "cgsl")
add_fmu(gsl2/mass_force                 mass_force                      "cgsl")
add_fmu(gsl2/mass_force_fe              mass_force_fe                   "cgsl")
add_fmu(gsl2/trailer                    trailer                         "cgsl")
add_fmu(gsl2/engine2                    engine2                         "cgsl")
add_fmu(gsl2/two_masses                 two_masses                      "cgsl")

# Non-GSL Co-Simulation FMUs (internal verlet integrator typically)
add_fmu(impulse                         impulse                         "")
add_fmu(kinematictruck/body             body                            "")
add_fmu(kinematictruck/engine           engine                          "")
add_fmu(kinematictruck/gearbox2         gearbox2                        "")
add_fmu(forcevelocitytruck/fvbody       fvbody                          "")
add_fmu(forcevelocitytruck/gearbox      gearbox                         "")
add_fmu(tests/typeconvtest              typeconvtest                    "")
add_fmu(tests/loopsolvetest/add         add                             "")
add_fmu(tests/loopsolvetest/sub         sub                             "")
add_fmu(tests/loopsolvetest/mul         mul                             "")
add_fmu(tests/stringtest                stringtest                      "")
add_fmu(tests/strange_variable_names    strange_variable_names          "")

# Model-Exchange FMUs
add_fmu(me/springs                      springs                         "")
add_fmu(me/springs2                     springs2                        "")
add_fmu(me/springs3                     springs3                        "")
add_fmu(me/springs4                     springs4                        "")
add_fmu(me/bouncingBall                 bouncingBall                    "")
add_fmu(me/bouncingBallWithSpring       bouncingBallWithSprings         "")
add_fmu(me/fixedPoint                   fixedPoint                      "")

# Wrap Model-Exchange FMUs into Co-Simulation FMUs using GSL for integrating
# The ME FMUs are taken as-is and put into resources/ in the outer FMU
# The modelDescription file is found in one of two ways:
#
# - on disc, uncompressed (plain XML file)
# - from inside the source FMU

# These generate the modelDescription from XML
add_wrapped(meWrapper/bouncingBall              bouncingBall)
add_wrapped(meWrapper/springs                   springs)
add_wrapped(meWrapper/springs2                  springs2)
add_wrapped(meWrapper/springs3                  springs3)
add_wrapped(meWrapper/springs4                  springs4)
add_wrapped_filter(meWrapperFiltered/springs2                  springs2)


# These generate the modelDescription from the source FMU
add_wrapped_fmu(meWrapperFmu/bouncingBall       bouncingBall    "${bouncingBall_fmu}")
add_wrapped_fmu(meWrapperFmu/springs2           springs2        "${springs2_fmu}")

