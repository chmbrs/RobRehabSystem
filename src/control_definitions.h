#ifndef CONTROL_DEFINITIONS_H
#define CONTROL_DEFINITIONS_H

#define CONTROLLER_INVALID_HANDLE NULL

typedef void* Controller;

// Control used values enumeration
enum ControlVariables { CONTROL_POSITION, CONTROL_VELOCITY, CONTROL_FORCE, CONTROL_ACCELERATION, CONTROL_MODES_NUMBER,
                        CONTROL_STIFFNESS = CONTROL_MODES_NUMBER, CONTROL_DAMPING, CONTROL_VARS_NUMBER };

#endif // CONTROL_DEFINITIONS_H
