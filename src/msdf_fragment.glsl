
#define IDX_CURR 0
#define IDX_SHAPE 1
#define IDX_INNER 2
#define IDX_OUTER 3
#define IDX_RED 0
#define IDX_GREEN 1
#define IDX_BLUE 2
#define IDX_NEGATIVE 0
#define IDX_POSITIVE 1
#define IDX_MAX_INNER 0
#define IDX_MAX_OUTER 1


precision mediump float;

precision mediump samplerBuffer;
precision mediump usamplerBuffer;
uniform usamplerBuffer metadata;
uniform samplerBuffer point_data;

#define meta_at(i) texelFetch(metadata, int(i)).r
#define point_at(i) vec2(texelFetch(point_data, 2 * int(i)).r, \
                         texelFetch(point_data, 2 * int(i) + 1).r)

uniform vec2 offset;

uniform vec2 translate;
uniform vec2 scale;
uniform float range;
uniform int meta_offset;
uniform int point_offset;
uniform float glyph_height;

out vec4 color;

const float PI = 3.1415926535897932384626433832795;
const float INFINITY = 3.402823466e+38;

const uint BLACK = 0u;
const uint RED = 1u;
const uint GREEN = 2u;
const uint BLUE = 4u;
const uint YELLOW = RED | GREEN;
const uint MAGENTA = BLUE | RED;
const uint CYAN = BLUE | GREEN;
const uint WHITE = RED | GREEN | BLUE;

struct segment {
    vec3 min_true;
    vec2 mins[2];
    int nearest_points;
    int nearest_npoints;
};

struct workspace {
    segment segments[4 * 3];

    vec3 maximums[2];
    vec3 min_absolute;
};

workspace ws;

vec3 signed_distance_linear(vec2 p0, vec2 p1, vec2 origin);
vec3 signed_distance_quad(vec2 p0, vec2 p1, vec2 p2, vec2 origin);
void add_segment_true_distance(int segment_index, int npoints, int points, vec3 d);
vec3 get_pixel_distance(vec2);

vec2 orthonormal(vec2 v) {float len = length(v); return vec2(v.y / len, -v.x / len);}
float cross_(vec2 a, vec2 b) { return a.x * b.y - a.y * b.x; }
float median(vec3 d) {return max(min(d.r, d.g), min(max(d.r, d.g), d.b));}
void add_segment_pseudo_distance(int segment_index, vec2 d);
vec2 distance_to_pseudo_distance(int npoints, int points, vec3 d, vec2 p);
bool point_facing_edge(int prev_npoints, int prev_points, int cur_npoints, int cur_points,
                       int next_npoints, int next_points, vec2 p, float param);
void add_segment(int prev_npoints, int prev_points, int cur_npoints, int cur_points,
                 int next_npoints, int next_points, uint color, vec2 point);
void set_contour_edge(int winding, vec2 point);
float compute_distance(int segment_index, vec2 point);


bool less(vec2 a, vec2 b) {
    return abs(a.x) < abs(b.x) || (abs(a.x) == abs(b.x) && a.y < b.y);
}


void main() {
    vec2 coords = gl_FragCoord.xy - offset;

    vec2 p = ((coords + 0.49) / scale) - vec2(translate.x, -translate.y);
    p.y  = (glyph_height / scale.y) - p.y;

    ws.maximums[0].r = -INFINITY;
    ws.maximums[1].r = -INFINITY;
    ws.maximums[0].g = -INFINITY;
    ws.maximums[1].g = -INFINITY;
    ws.maximums[0].b = -INFINITY;
    ws.maximums[1].b = -INFINITY;
    ws.min_absolute.r = -INFINITY;
    ws.min_absolute.g = -INFINITY;
    ws.min_absolute.b = -INFINITY;

    for (int i = 0; i < (4 * 3); ++i) {
        ws.segments[i].mins[0].x = -INFINITY;
        ws.segments[i].mins[1].x = -INFINITY;
        ws.segments[i].min_true.x = -INFINITY;
        ws.segments[i].nearest_points = -1;
    }
    int point_index = point_offset;
    int meta_index = meta_offset;


    uint ncontours = meta_at(meta_index++);

    for (uint _i = 0u; _i < ncontours; ++_i) {
        int winding = int(meta_at(meta_index++)) - 1;
        uint nsegments = meta_at(meta_index++);

        uint s_color = meta_at(meta_index);
        uint s_npoints = meta_at(meta_index + 1);

        /** TODO: Move the following checks to the preprocessor, no need to do
                  them for every fragment. */
        /* Ignore empty contours. */
        if (nsegments == 0u) {
            continue;
        }

        /* Ignore contours with just one linear segment, some fonts seem to have them. */
        if (nsegments == 1u && s_npoints == 2u) {
            point_index += 2;
            meta_index += 2;
            continue;
        }

        /* Ignore contours with just two linear segments, some fonts seem to have them. */
        if (nsegments == 2u && s_npoints == 2u && meta_at(meta_index + 3) == 2u) {
            point_index += 4;
            meta_index += 4;
            continue;
        }

        int cur_points = point_index;
        uint cur_color = meta_at(meta_index + 2 * (int(nsegments) - 1));
        uint cur_npoints = meta_at(meta_index + 2 * (int(nsegments) - 1) + 1);


        uint prev_npoints = nsegments >= 2u ?
            meta_at(meta_index + 2 * (int(nsegments) - 2) + 1) : s_npoints;
        int prev_points = point_index;

        for (uint _i = 0u; _i < nsegments - 1u; ++_i) {
            uint npoints = meta_at(meta_index + 2 * int(_i) + 1);
            cur_points += (int(npoints) - 1);
        }

        for (uint _i = 0u; (_i < (nsegments - 2u)) && nsegments >= 2u; ++_i) {
            uint npoints = meta_at(meta_index + 2 * int(_i) + 1);
            prev_points += (int(npoints) - 1);
        }

        for (uint _i = 0u; _i < nsegments; ++_i) {

            add_segment(int(prev_npoints), prev_points, int(cur_npoints), cur_points,
                        int(s_npoints), point_index, cur_color, p);

            prev_points = cur_points;
            prev_npoints = cur_npoints;

            cur_points = point_index;
            cur_npoints = s_npoints;
            cur_color = s_color;

            s_color = meta_at(meta_index++ + 2);
            point_index += (int(s_npoints) - 1);
            s_npoints = meta_at(meta_index++ + 2);
        }
        point_index += 1;

        set_contour_edge(winding, p);
    }

    vec3 d = get_pixel_distance(p);

    color = vec4(d / range + 0.5, 1.0);

    // For testing
    // color = median(color.rgb) > 0.5 ? vec4(1.0, 1.0, 1.0, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
}

void merge_segment(int s, int other) {
    if (less(ws.segments[other].min_true.xy, ws.segments[s].min_true.xy)) {
        ws.segments[s].min_true = ws.segments[other].min_true;

        ws.segments[s].nearest_npoints = ws.segments[other].nearest_npoints;
        ws.segments[s].nearest_points = ws.segments[other].nearest_points;
    }
    if (less(ws.segments[other].mins[IDX_NEGATIVE], ws.segments[s].mins[IDX_NEGATIVE]))
        ws.segments[s].mins[IDX_NEGATIVE] = ws.segments[other].mins[IDX_NEGATIVE];
    if (less(ws.segments[other].mins[IDX_POSITIVE], ws.segments[s].mins[IDX_POSITIVE])) {
        ws.segments[s].mins[IDX_POSITIVE] = ws.segments[other].mins[IDX_POSITIVE];
    }
}

void merge_multi_segment(int e, int other) {
    merge_segment(e * 3 + IDX_RED, other * 3 + IDX_RED);
    merge_segment(e * 3 + IDX_GREEN, other * 3 + IDX_GREEN);
    merge_segment(e * 3 + IDX_BLUE, other * 3 + IDX_BLUE);
}

void add_segment(int prev_npoints, int prev_points, int cur_npoints, int cur_points,
                 int next_npoints, int next_points, uint s_color, vec2 point) {

    vec3 d;
    if (cur_npoints == 2)
        d = signed_distance_linear(point_at(cur_points),
                                   point_at(cur_points + 1),
                                   point);
    else
        d = signed_distance_quad(point_at(cur_points),
                                 point_at(cur_points + 1),
                                 point_at(cur_points + 2),
                                 point);

    if ((s_color & RED) > 0u)
        add_segment_true_distance(IDX_CURR * 3 + IDX_RED, cur_npoints, cur_points, d);
    if ((s_color & GREEN) > 0u)
        add_segment_true_distance(IDX_CURR * 3 + IDX_GREEN, cur_npoints, cur_points, d);
    if ((s_color & BLUE) > 0u)
        add_segment_true_distance(IDX_CURR * 3 + IDX_BLUE, cur_npoints, cur_points, d);

    if (point_facing_edge(prev_npoints, prev_points, cur_npoints, cur_points,
                          next_npoints, next_points, point, d.z)) {

        vec2 pd = distance_to_pseudo_distance(cur_npoints, cur_points, d, point);
        if ((s_color & RED) > 0u)
            add_segment_pseudo_distance(IDX_CURR * 3 + IDX_RED, pd);
        if ((s_color & GREEN) > 0u)
            add_segment_pseudo_distance(IDX_CURR * 3 + IDX_GREEN, pd);
        if ((s_color & BLUE) > 0u)
            add_segment_pseudo_distance(IDX_CURR * 3 + IDX_BLUE, pd);
    }
}

vec3 get_distance(int segment_index, vec2 point) {
    vec3 d;
    d.r = compute_distance(segment_index * 3 + IDX_RED, point);
    d.g = compute_distance(segment_index * 3 + IDX_GREEN, point);
    d.b = compute_distance(segment_index * 3 + IDX_BLUE, point);
    return d;
}

void set_contour_edge(int winding, vec2 point) {

    vec3 d = get_distance(IDX_CURR, point);

    merge_multi_segment(IDX_SHAPE, IDX_CURR);
    if (winding > 0 && median(d) >= 0.0)
        merge_multi_segment(IDX_INNER, IDX_CURR);
    if (winding < 0 && median(d) <= 0.0)
        merge_multi_segment(IDX_OUTER, IDX_CURR);

    int i = winding < 0 ? IDX_MAX_INNER : IDX_MAX_OUTER;

    ws.maximums[i] = (median(d) > median(ws.maximums[i])) ? d : ws.maximums[i];
    ws.min_absolute = (abs(median(d)) < abs(median(ws.min_absolute))) ? d : ws.min_absolute;
}

vec2 segment_direction(int points, int npoints, float param) {
    return mix(point_at(points + 1) - point_at(points),
               point_at(points + npoints - 1) - point_at(points + npoints - 2),
               param);
}

vec2 segment_point(int points, int npoints, float param) {
    return mix(mix(point_at(points), point_at(points + 1), param),
               mix(point_at(points + npoints - 2), point_at(points + npoints - 1), param),
               param);
}


vec2 distance_to_pseudo_distance(int npoints, int points, vec3 d, vec2 p) {
    if (d.z >= 0.0 && d.z <= 1.0)
        return d.xy;

    vec2 dir = normalize(segment_direction(points, npoints, d.z < 0.0 ? 0.0 : 1.0));
    vec2 aq = p - segment_point(points, npoints, d.z < 0.0 ? 0.0 : 1.0);
    float ts = dot(aq, dir);
    if (d.z < 0.0 ? ts < 0.0 : ts > 0.0) {
        float pseudo_distance = cross_(aq, dir);
        if (abs(pseudo_distance) <= abs(d.x)) {
            d.x = pseudo_distance;
            d.y = 0.0;
        }
    }
    return d.xy;
}

void add_segment_true_distance(int segment_index, int npoints, int points, vec3 d) {
    bool is_less = less(d.xy, ws.segments[segment_index].min_true.xy);
    ws.segments[segment_index].min_true =
        is_less ? d : ws.segments[segment_index].min_true;

    ws.segments[segment_index].nearest_points =
        is_less ? points : ws.segments[segment_index].nearest_points;
    ws.segments[segment_index].nearest_npoints =
        is_less ? npoints : ws.segments[segment_index].nearest_npoints;
}


void add_segment_pseudo_distance(int segment_index, vec2 d) {
    int i = d.x < 0.0 ? IDX_NEGATIVE : IDX_POSITIVE;
    vec2 _d = ws.segments[segment_index].mins[i];
    ws.segments[segment_index].mins[i] = less(d, _d) ? d : _d;
}

bool point_facing_edge(int prev_npoints, int prev_points, int cur_npoints, int cur_points,
                       int next_npoints, int next_points, vec2 p, float param) {

    if (param >= 0.0 && param <= 1.0)
        return true;

    vec2 prev_edge_dir = -normalize(segment_direction(prev_points, prev_npoints, 1.0));
    vec2 edge_dir =
        normalize(segment_direction(cur_points, cur_npoints, param < 0.0 ? 0.0 : 1.0)) *
        (param < 0.0 ? 1.0 : -1.0);
    vec2 next_edge_dir = normalize(segment_direction(next_points, next_npoints, 0.0));
    vec2 point_dir = p - segment_point(cur_points, cur_npoints, param < 0.0 ? 0.0 : 1.0);
    return dot(point_dir, edge_dir) >=
           dot(point_dir, param < 0.0 ? prev_edge_dir : next_edge_dir);
}

float compute_distance(int segment_index, vec2 point) {

    int i = ws.segments[segment_index].min_true.xy.x < 0.0 ? IDX_NEGATIVE : IDX_POSITIVE;
    float min_distance = ws.segments[segment_index].mins[i].x;

    if (ws.segments[segment_index].nearest_points == -1) return min_distance;
    vec2 d = distance_to_pseudo_distance(ws.segments[segment_index].nearest_npoints,
                                         ws.segments[segment_index].nearest_points,
                                         ws.segments[segment_index].min_true, point);
    min_distance = abs(d.x) < abs(min_distance) ? d.x : min_distance;

    return min_distance;
}

vec3 signed_distance_linear(vec2 p0, vec2 p1, vec2 origin) {
    vec2 aq = origin - p0;
    vec2 ab = p1 - p0;
    float param = dot(aq, ab) / dot(ab, ab);
    vec2 eq = (param > .5 ? p1 : p0) - origin;
    float endpoint_distance = length(eq);
    if (param > 0.0 && param < 1.0) {
        float ortho_distance = dot(orthonormal(ab), aq);
        if (abs(ortho_distance) < endpoint_distance)
            return vec3(ortho_distance, 0, param);
    }
    return vec3(sign(cross_(aq, ab)) *endpoint_distance,
                abs(dot(normalize(ab), normalize(eq))),
                param);
}

vec3 signed_distance_quad(vec2 p0, vec2 p1, vec2 p2, vec2 origin) {
    vec2 qa = p0 - origin;
    vec2 ab = p1 - p0;
    vec2 br = p2 - p1 - ab;
    float a = dot(br, br);
    float b = 3.0 * dot(ab, br);
    float c = 2.0 * dot(ab, ab) + dot(qa, br);
    float d = dot(qa, ab);
    float coeffs[3];
    float _a = b / a;
    int solutions;

    float a2 = _a * _a;
    float q = (a2 - 3.0 * (c / a)) / 9.0;
    float r = (_a * (2.0 * a2 - 9.0 * (c / a)) + 27.0 * (d / a)) / 54.0;
    float r2 = r * r;
    float q3 = q * q * q;
    float A, B;
    _a /= 3.0;
    float t = r / sqrt(q3);
    t = t < -1.0 ? -1.0 : t;
    t = t > 1.0 ? 1.0 : t;
    t = acos(t);
    A = -pow(abs(r) + sqrt(r2 - q3), 1.0 / 3.0);
    A = r < 0.0 ? -A : A;
    B = A == 0.0 ? 0.0 : q / A;
    if (r2 < q3) {
        q = -2.0 * sqrt(q);
        coeffs[0] = q * cos(t / 3.0) - _a;
        coeffs[1] = q * cos((t + 2.0 * PI) / 3.0) - _a;
        coeffs[2] = q * cos((t - 2.0 * PI) / 3.0) - _a;
        solutions = 3;
    } else {
        coeffs[0] = (A + B) - _a;
        coeffs[1] = -0.5 * (A + B) - _a;
        coeffs[2] = 0.5 * sqrt(3.0) * (A - B);
        solutions = abs(coeffs[2]) < 1.0e-14 ? 2 : 1;
    }

    float min_distance = sign(cross_(ab, qa)) * length(qa); // distance from A
    float param = -dot(qa, ab) / dot(ab, ab);
    float distance = sign(cross_(p2 - p1, p2 - origin)) * length(p2 - origin); // distance from B
    if (abs(distance) < abs(min_distance)) {
        min_distance = distance;
        param = dot(origin - p1, p2 - p1) / dot(p2 - p1, p2 - p1);
    }
    for (int i = 0; i < solutions; ++i) {
        if (coeffs[i] > 0.0 && coeffs[i] < 1.0) {
            vec2 endpoint = p0 + ab * 2.0 * coeffs[i] + br * coeffs[i] * coeffs[i];
            float distance = sign(cross_(p2 - p0, endpoint - origin)) * length(endpoint - origin);
            if (abs(distance) <= abs(min_distance)) {
                min_distance = distance;
                param = coeffs[i];
            }
        }
    }
    vec2 v = vec2(min_distance, 0.0);
    v.y = param > 1.0 ? abs(dot(normalize(p2 - p1), normalize(p2 - origin))) : v.y;
    v.y = param < 0.0 ? abs(dot(normalize(ab), normalize(qa))) : v.y;

    return vec3(v, param);
}

vec3 get_pixel_distance(vec2 point) {
    vec3 shape_distance = get_distance(IDX_SHAPE, point);
    vec3 inner_distance = get_distance(IDX_INNER, point);
    vec3 outer_distance = get_distance(IDX_OUTER, point);
    float inner_d = median(inner_distance);
    float outer_d = median(outer_distance);

    bool inner = inner_d >= 0.0 && abs(inner_d) <= abs(outer_d);
    bool outer = outer_d <= 0.0 && abs(outer_d) < abs(inner_d);
    if (!inner && !outer)
        return shape_distance;

    vec3 d = inner ? inner_distance : outer_distance;
    vec3 contour_distance = ws.maximums[inner ? IDX_MAX_INNER : IDX_MAX_OUTER];

    float contour_d = median(contour_distance);
    d = (abs(contour_d) < abs(outer_d) && contour_d > median(d)) ? contour_distance : d;

    contour_distance = ws.min_absolute;
    contour_d = median(contour_distance);
    float d_d = median(d);

    d = abs(contour_d) < abs(d_d) ? contour_distance : d;
    d = median(d) == median(shape_distance) ? shape_distance : d;

    return d;
}
