#version 430 core

// Instanced billboard used by knot::ParticleSystem.
//
// The quad is expanded in view space so it always faces the camera. The
// particle velocity, also transformed into view space, gives the billboard
// its direction: a falling raindrop becomes a vertical streak, a drifting
// smoke puff stays nearly round.

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

void main() {
    vec4 viewPos = view * vec4(iPosition, 1.0);
    vec3 viewVelocity = mat3(view) * iVelocity;

    // Align the long axis with the on-screen direction of motion.
    vec2 axis = vec2(0.0, -1.0);
    if (length(viewVelocity.xy) > 1.0e-4) {
        axis = normalize(viewVelocity.xy);
    }

    float lengthScale = clamp(1.0 + stretch * length(iVelocity), 1.0, maxStretch);
    vec2 perpendicular = vec2(-axis.y, axis.x);

    viewPos.xy += axis * (aCorner.y * iSize * lengthScale) + perpendicular * (aCorner.x * iSize);

    gl_Position = projection * viewPos;

    vColor = iColor;
    vAge = iAge;
    vCorner = aCorner;
}
