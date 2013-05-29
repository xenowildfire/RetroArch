// GLSL shader autogenerated by cg2glsl.py.
#if defined(VERTEX)
varying     float _frame_rotation;
struct input_dummy {
    vec2 _video_size;
    vec2 _texture_size;
    vec2 _output_dummy_size;
    float _frame_count;
    float _frame_direction;
    float _frame_rotation;
};
vec4 _oPosition1;
varying vec2 _border1;
uniform mat4 MVPMatrix;
vec4 _r0005;
attribute vec4 VertexCoord;
attribute vec4 COLOR;
varying vec4 COL0;
attribute vec4 LUTTexCoord;
varying vec4 TEX1;
 

         mat4 transpose_(mat4 matrix)
         {
            mat4 ret;
            for (int i = 0; i < 4; i++)
               for (int j = 0; j < 4; j++)
                  ret[i][j] = matrix[j][i];

            return ret;
         }
         
uniform int FrameDirection;
uniform int FrameCount;
#ifdef GL_ES
uniform mediump vec2 OutputSize;
uniform mediump vec2 TextureSize;
uniform mediump vec2 InputSize;
#else
uniform vec2 OutputSize;
uniform vec2 TextureSize;
uniform vec2 InputSize;
#endif
void main()
{
    mat4 MVPMatrix_ = transpose_(MVPMatrix);
    vec4 _oColor;
    vec2 _otex_border;
    _r0005.x = dot(MVPMatrix_[0], VertexCoord);
    _r0005.y = dot(MVPMatrix_[1], VertexCoord);
    _r0005.z = dot(MVPMatrix_[2], VertexCoord);
    _r0005.w = dot(MVPMatrix_[3], VertexCoord);
    _oPosition1 = _r0005;
    _oColor = COLOR;
    _border1 = vec2( -2.50000000E-01, 1.25000000E+00);
    _otex_border = LUTTexCoord.xy;
    gl_Position = _r0005;
    COL0 = COLOR;
    TEX1.xy = LUTTexCoord.xy;
} 
#elif defined(FRAGMENT)
#ifdef GL_ES
precision mediump float;
#endif
varying     float _frame_rotation;
struct input_dummy {
    vec2 _video_size;
    vec2 _texture_size;
    vec2 _output_dummy_size;
    float _frame_count;
    float _frame_direction;
    float _frame_rotation;
};
vec4 _ret_0;
uniform sampler2D Texture;
uniform sampler2D bg;
varying vec2 _border1;
varying vec4 TEX0;
varying vec4 TEX1;
 
uniform int FrameDirection;
uniform int FrameCount;
#ifdef GL_ES
uniform mediump vec2 OutputSize;
uniform mediump vec2 TextureSize;
uniform mediump vec2 InputSize;
#else
uniform vec2 OutputSize;
uniform vec2 TextureSize;
uniform vec2 InputSize;
#endif
void main()
{
    vec4 _background;
    vec4 _source_image;
    float _sel;
    _background = texture2D(bg, TEX1.xy);
    _source_image = texture2D(Texture, TEX0.xy);
    if (TEX0.x < _border1.x || TEX0.x > _border1.y) { 
        _sel = 0.00000000E+00;
    } else {
        _sel = 1.00000000E+00;
    } 
    _ret_0 = _background + _sel*(_source_image - _background);
    gl_FragColor = _ret_0;
    return;
} 
#endif