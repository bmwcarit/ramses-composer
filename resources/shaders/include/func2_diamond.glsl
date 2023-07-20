// When included together with func2.glsl into main file, result in diamond include:
//      main
//    /      \
// func2    func2_diamond
//    \      /
//      func1

#include "func1.glsl"
vec3 function2() {
    return vec3(0.0, 0.0, 1.0);
}
