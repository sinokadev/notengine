#version 430 core

// Instanced velocity-aligned billboard used by knot::ParticleSystem.
//
// The streak is built in world space along the particle's own velocity and then
// projected, so it behaves like the line segment the particle actually sweeps in
// one exposure: looking along the fall direction foreshortens it to a dot, and
// looking across it keeps the full length. The quad's width axis is the
// cross product of the velocity and the view ray, which keeps the ribbon facing
// the camera at every camera angle.

layout(location = 0) in vec2 aCorner;      // quad corner in [-0.5, 0.5]
layout(location = 4) in vec3 iPosition;    // world-space particle center
layout(location = 5) in vec3 iVelocity;    // world-space particle velocity
layout(location = 6) in float iSize;       // world-space billboard width
layout(location = 7) in vec4 iColor;       // base color, alpha is opacity
layout(location = 8) in float iAge;        // normalized age, 0 at spawn, 1 at expiry

uniform mat4 view;
uniform mat4 projection;
uniform float stretch;    // seconds per velocity unit; 0 keeps the billboard square
uniform float maxStretch; // upper bound on the total length multiplier

out vec4 vColor;
out float vAge;
out vec2 vCorner;
out float vNearFade;

// Distance range over which a particle fades out as it reaches the lens.
const float kNearFadeStart = 0.35;
const float kNearFadeEnd = 1.5;

void main() {
    const float speed = length(iVelocity);

    // Direction of travel in world space. A particle that is not moving (or a
    // velocity that is numerically degenerate) still gets a sane axis instead of
    // a division by zero.
    vec3 axis = speed > 1.0e-4 ? iVelocity / speed : vec3(0.0, -1.0, 0.0);

    // The view matrix is a rigid transform, so this is the camera position in
    // world space without inverting the matrix on the GPU.
    const vec3 cameraPosition = -transpose(mat3(view)) * view[3].xyz;
    const vec3 toEye = cameraPosition - iPosition;

    // Width axis: perpendicular to both the streak and the view ray, so the
    // ribbon always presents its face to the camera.
    vec3 side = cross(axis, toEye);

    if (dot(side, side) < 1.0e-8) {
        // The streak points straight at the camera, so any perpendicular will do.
        const vec3 helper = abs(axis.y) < 0.9 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        side = cross(axis, helper);
    }

    side = normalize(side);

    // A still particle keeps the base size; a fast one grows into a streak up to
    // maxStretch times its size.
    const float lengthScale = clamp(1.0 + stretch * speed, 1.0, maxStretch);

    const vec3 worldPosition = iPosition + axis * (aCorner.y * iSize * lengthScale) + side * (aCorner.x * iSize);

    gl_Position = projection * view * vec4(worldPosition, 1.0);

    // A streak that passes next to the lens would cover a large part of the
    // screen and read as a smear, which is exactly what happens when the camera
    // sits inside the emitter box. Fade it out over the last metre instead.
    vNearFade = smoothstep(kNearFadeStart, kNearFadeEnd, length(toEye));

    vColor = iColor;
    vAge = iAge;
    vCorner = aCorner;
}
