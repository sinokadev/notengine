#version 430 core

// Soft instanced billboard fragment shader. Color comes straight from the
// particle, opacity is the particle's own alpha shaped by its remaining
// lifetime and by a soft elliptical falloff across the quad.

in vec4 vColor;
in float vAge;
in vec2 vCorner;
in float vNearFade;

out vec4 FragColor;

void main() {
    // vCorner is in [-0.5, 0.5]; the doubled length is ~1 at every edge midpoint
    // and ~1.41 at a corner, which gives a soft ellipse instead of a hard quad edge.
    float radial = length(vCorner * 2.0);
    float edge = 1.0 - smoothstep(0.55, 1.0, radial);

    // Blend in quickly, then fade out over the last part of the lifetime.
    float fadeIn = smoothstep(0.0, 0.08, vAge);
    float fadeOut = 1.0 - smoothstep(0.6, 1.0, vAge);

    float alpha = vColor.a * edge * fadeIn * fadeOut * vNearFade;

    if (alpha <= 0.001) {
        discard;
    }

    FragColor = vec4(vColor.rgb, alpha);
}
