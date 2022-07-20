#ifndef __GRID_FRAG_H__
#define __GRID_FRAG_H__

namespace raco::ramses_widgets {

static const char* grid_frag = R"SHADER(

#version 320 es
precision highp float;

in vec3 local_pos;

out vec4 FragColor;

uniform vec3 planeAxes;
uniform float gridDistance;
uniform vec3 gridSize;
uniform float lineKernel;

uniform int gridFlag;
uniform float zoomFactor;

uniform highp mat4 ProjectionMatrix;
uniform highp mat4 ViewMatrix;

#define AXIS_X (1 << 0)
#define AXIS_Y (1 << 1)
#define AXIS_Z (1 << 2)
#define GRID (1 << 3)
#define PLANE_XY (1 << 4)
#define PLANE_XZ (1 << 5)
#define PLANE_YZ (1 << 6)
#define GRID_BACK (1 << 9)    /* grid is behind objects */
#define GRID_CAMERA (1 << 10) /* In camera view */
#define PLANE_IMAGE (1 << 11) /* UV/Image Image editor */
#define CUSTOM_GRID (1 << 12) /* UV Editor only */

#define M_1_SQRTPI 0.5641895835477563 /* 1/sqrt(pi) */

/**
 * We want to know how much a pixel is covered by a line.
 * We replace the square pixel with acircle of the same area and try to find the intersection area.
 * The area we search is the circular segment. https://en.wikipedia.org/wiki/Circular_segment
 * The formula for the area uses inverse trig function and is quite complexe. Instead,
 * we approximate it by using the smoothstep function and a 1.05 factor to the disc radius.
 */
#define DISC_RADIUS (M_1_SQRTPI * 1.05)
#define GRID_LINE_SMOOTH_START (0.5 - DISC_RADIUS)
#define GRID_LINE_SMOOTH_END (0.5 + DISC_RADIUS)

float get_grid(vec2 co, vec2 fwidthCos, float grid_size)
{
  float half_size = grid_size / 2.0;
  vec2 grid_domain = abs(mod(co + half_size, grid_size) - half_size);
  grid_domain /= fwidthCos;

  float line_dist = min(grid_domain.x, grid_domain.y);

  return (1.0 - smoothstep(GRID_LINE_SMOOTH_START, GRID_LINE_SMOOTH_END, line_dist - lineKernel));
}

vec3 get_axes(vec3 co, vec3 fwidthCos, float line_size)
{
  vec3 axes_domain = abs(co);
  axes_domain /= fwidthCos;

  return (1.0 - smoothstep(GRID_LINE_SMOOTH_START,
                          GRID_LINE_SMOOTH_END,
                          axes_domain - (line_size + lineKernel)));
}

float linearstep(float p0, float p1, float v) {
  return (clamp(((v) - (p0)) / abs((p1) - (p0)), 0.0, 1.0));
}

void main()
{

  mat4 ViewMatrixInverse = inverse(ViewMatrix);
  vec3 cameraPos = vec3(ViewMatrixInverse[3].xyz);
  vec3 wPos = local_pos * gridSize;
  vec3 dFdxPos = dFdx(wPos);
  vec3 dFdyPos = dFdy(wPos);
  vec3 fwidthPos = abs(dFdxPos) + abs(dFdyPos);
  wPos += cameraPos * planeAxes;
  float gridSteps_0 = 0.001;
  float gridSteps_1 = 0.01;
  float gridSteps_2 = 0.1;
  float gridSteps_3 = 1.0;
  float gridSteps_4 = 10.0;
  float gridSteps_5 = 100.0;
  float gridSteps_6 = 1000.0;
  float gridSteps_7 = 10000.0;

  vec4 colorGrid = vec4(0.4, 0.4, 0.4, 0.5);
  vec4 colorGridEmphasis = vec4(0.41, 0.41, 0.41, 0.5);
  vec4 colorGridAxisX = vec4(0.346704096, 0.0409152061, 0.0666259751, 0.749019623);
  vec4 colorGridAxisY = vec4(0.130136520, 0.270497888, 0.0144438418, 0.749019623);
  vec4 colorGridAxisZ = vec4(0.0343398042, 0.138431653, 0.346704096, 0.749019623);

  float dist, fade;
  /* if persp */
  if (ProjectionMatrix[3][3] == 0.0) {
    vec3 viewvec = cameraPos - wPos;
    dist = length(viewvec);
    viewvec /= dist;

    float angle;
    if ((gridFlag & PLANE_XZ) != 0) {
      angle = viewvec.y;
    }
    else if ((gridFlag & PLANE_YZ) != 0) {
      angle = viewvec.x;
    }
    else {
      angle = viewvec.z;
    }

    angle = 1.0 - abs(angle);
    angle *= angle;
    fade = 1.0 - angle * angle;
    fade *= 1.0 - smoothstep(0.0, gridDistance, dist - gridDistance);
  }
  else {
    if ((gridFlag & PLANE_XY) != 0) {
      dist = gl_FragCoord.z * 2.0 - 1.0;
    } else if ((gridFlag & PLANE_XZ) != 0) {
      dist = gl_FragCoord.y * 2.0 - 1.0;
    }
    /* Avoid fading in +Z direction in camera view (see T70193). */
    dist = ((gridFlag & GRID_CAMERA) != 0) ? clamp(dist, 0.0, 1.0) : abs(dist);
    fade = 1.0 - smoothstep(0.0, 0.5, dist - 0.5);
    dist = 1.0; /* avoid branch after */

    if ((gridFlag & PLANE_XY) != 0) {
      float angle = 1.0 - abs(ViewMatrixInverse[2].z);
      dist = 1.0 + angle * 2.0;
      angle *= angle;
      fade *= 1.0 - angle * angle;
    } else if ((gridFlag & PLANE_XZ) != 0) {
      float angle = 1.0 - abs(ViewMatrixInverse[2].y);
      dist = 1.0 + angle * 2.0;
      angle *= angle;
      fade *= 1.0 - angle * angle;
    }
  }

  if ((gridFlag & GRID) != 0) {
    /* Using `max(dot(dFdxPos, screenVecs[0]), dot(dFdyPos, screenVecs[1]))`
     * would be more accurate, but not really necessary. */
    // float grid_res = dot(dFdxPos, screenVecs[0].xyz);
    float grid_res = dot(dFdxPos, normalize(ViewMatrixInverse[0].xyz));

    /* The grid begins to appear when it comprises 4 pixels */
    // grid_res *= 12.0;

    /* For UV/Image editor use zoomFactor */
    if ((gridFlag & PLANE_IMAGE) != 0 &&
        /* Grid begins to appear when the length of one grid unit is at least
         * (256/grid_size) pixels Value of grid_size defined in `overlay_grid.c`. */
        (gridFlag & CUSTOM_GRID) == 0) {
      grid_res = zoomFactor;
    }
    grid_res *= zoomFactor;
    /* from biggest to smallest */
    vec4 scale;

    /* For more efficiency, unroll the loop above. */
    if (gridSteps_0 > grid_res) {
      scale = vec4(0.0, gridSteps_0, gridSteps_1, gridSteps_2);
    }
    else if (gridSteps_1 > grid_res) {
      scale = vec4(gridSteps_0, gridSteps_1, gridSteps_2, gridSteps_3);
    }
    else if (gridSteps_2 > grid_res) {
      scale = vec4(gridSteps_1, gridSteps_2, gridSteps_3, gridSteps_4);
    }
    else if (gridSteps_3 > grid_res) {
      scale = vec4(gridSteps_2, gridSteps_3, gridSteps_4, gridSteps_5);
    }
    else if (gridSteps_4 > grid_res) {
      scale = vec4(gridSteps_3, gridSteps_4, gridSteps_5, gridSteps_6);
    }
    else if (gridSteps_5 > grid_res) {
      scale = vec4(gridSteps_4, gridSteps_5, gridSteps_6, gridSteps_7);
    }
    else if (gridSteps_6 > grid_res) {
      scale = vec4(gridSteps_5, gridSteps_6, gridSteps_7, gridSteps_7);
    }
    else {
      scale = vec4(gridSteps_6, gridSteps_7, gridSteps_7, gridSteps_7);
    }

    float blend = 1.0 - linearstep(scale[0], scale[1], grid_res);
    blend = blend * blend * blend;

    vec2 grid_pos, grid_fwidth;
    if ((gridFlag & PLANE_XZ) != 0) {
      grid_pos = wPos.xz;
      grid_fwidth = fwidthPos.xz;
    }
    else if ((gridFlag & PLANE_YZ) != 0) {
      grid_pos = wPos.yz;
      grid_fwidth = fwidthPos.yz;
    }
    else {
      grid_pos = wPos.xy;
      grid_fwidth = fwidthPos.xy;
    }

    float gridA = get_grid(grid_pos, grid_fwidth, scale[1]);
    float gridB = get_grid(grid_pos, grid_fwidth, scale[2]);
    float gridC = get_grid(grid_pos, grid_fwidth, scale[3]);

    FragColor = colorGrid;
    FragColor.a *= gridA * blend;
    FragColor = mix(FragColor, mix(colorGrid, colorGridEmphasis, blend), gridB);
    FragColor = mix(FragColor, colorGridEmphasis, gridC);
  }
  else {
    FragColor = vec4(colorGrid.rgb, 0.0);
  }

  if ((gridFlag & (AXIS_X | AXIS_Y | AXIS_Z)) != 0) {
    /* Setup axes 'domains' */
    vec3 axes_dist, axes_fwidth;

    if ((gridFlag & AXIS_X) != 0) {
      axes_dist.x = dot(wPos.yz, planeAxes.yz);
      axes_fwidth.x = dot(fwidthPos.yz, planeAxes.yz);
    }
    if ((gridFlag & AXIS_Y) != 0) {
      axes_dist.y = dot(wPos.xz, planeAxes.xz);
      axes_fwidth.y = dot(fwidthPos.xz, planeAxes.xz);
    }
    if ((gridFlag & AXIS_Z) != 0) {
      axes_dist.z = dot(wPos.xy, planeAxes.xy);
      axes_fwidth.z = dot(fwidthPos.xy, planeAxes.xy);
    }

    /* Computing all axes at once using vec3 */
    vec3 axes = get_axes(axes_dist, axes_fwidth, 0.1);

    if ((gridFlag & AXIS_X) != 0) {
      FragColor.a = max(FragColor.a, axes.x);
      FragColor.rgb = (axes.x < 1e-8) ? FragColor.rgb : colorGridAxisX.rgb;
    }
    if ((gridFlag & AXIS_Y) != 0) {
      FragColor.a = max(FragColor.a, axes.y);
      FragColor.rgb = (axes.y < 1e-8) ? FragColor.rgb : colorGridAxisY.rgb;
    }
    if ((gridFlag & AXIS_Z) != 0) {
      FragColor.a = max(FragColor.a, axes.z);
      FragColor.rgb = (axes.z < 1e-8) ? FragColor.rgb : colorGridAxisZ.rgb;
    }
  }

  float scene_depth = 1.0;
  if ((gridFlag & GRID_BACK) != 0) {
    fade *= (scene_depth == 1.0) ? 1.0 : 0.0;
  }
  else {
    /* Add a small bias so the grid will always be below of a mesh with the same depth. */
    float grid_depth = gl_FragCoord.z + 4.8e-7;
      /* Manual, non hard, depth test:
      * Progressively fade the grid below occluders
      * (avoids popping visuals due to depth buffer precision) */
      /* Harder settings tend to flicker more,
      * but have less "see through" appearance. */
      float bias = max(fwidth(gl_FragCoord.z), 2.4e-7);
    fade *= linearstep(grid_depth, grid_depth + bias, scene_depth);
  }

   FragColor.a *= fade;
}


)SHADER";

}
#endif  // __GRID_FRAG_H__