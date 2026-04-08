vec3 rotate_vector(vec3 v, vec4 q) {
    vec4 q_normalized = normalize(q);
    vec3 t = 2.0 * cross(q_normalized.xyz, v);
    return v + q_normalized.w * t + cross(q_normalized.xyz, t);
}